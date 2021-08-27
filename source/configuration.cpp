/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
   
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <iostream>

#include "json.h"
#include "configuration.h"
#include "build_task_compile.h"
#include "dependencies.h"
#include "source_files.h"
#include "logging.h"
#include "shell.h"
#include "project.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
Configuration::Configuration(const std::string& pConfigName,const Project* pParentProject,int pLoggingMode,const rapidjson::Value& pConfig):
		mConfigName(pConfigName),
		mProjectDir(pParentProject->GetProjectDir()),
		mLoggingMode(pLoggingMode),
		mIsDefaultConfig(false),
		mOk(false),
		mTargetType(TARGET_NOT_SET),
		mWarningsAsErrors(false),
		mEnableAllWarnings(false),
		mFatalErrors(false),
		mSourceFiles(pParentProject->GetProjectDir(),pLoggingMode)		
{
	AddIncludeSearchPath(mProjectDir);

	mIsDefaultConfig = GetBool(pConfig,"default",false);
	
	if( pConfig.HasMember("include") )
	{
        if( AddIncludeSearchPaths(pConfig["include"]) == false )
        {
            std::cerr << "The \'include\' object in the \'settings\' " << mConfigName << " is not an array\n";
            return; // We're done, no need to continue.
        }
    }
    else
    {
   		std::cerr << "Internal error: The \'include\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?\n";
        return; // We're done, no need to continue.
    }

	// These can be added to the args now as they need to come before libs.
	if( pConfig.HasMember("libpaths") )
    {
        if( AddLibrarySearchPaths(pConfig["libpaths"]) == false )
        {
            std::cerr << "The \'libpaths\' object in the \'settings\' " << mConfigName << " is not an array\n";
            return; // We're done, no need to continue.
        }
    }
    else
    {
        if( mLoggingMode >= LOG_VERBOSE )
        {
    		std::clog << "The \'libpaths\' object in the \'settings\' " << mConfigName << " not set.\n";
        }
    }

	// These go into a different place for now as they have to be added to the args after the object files have been.
	// This is because of the way linkers work.
	if( pConfig.HasMember("libs") )
	{
		if( AddLibraries(pConfig["libs"]) == false  )
		{
			std::cerr << "The \'libraries\' object in the \'settings\' " << mConfigName << " is not an array\n";
			return; // We're done, no need to continue.
		}
	}
    else
    {
   		std::cerr << "Internal error: The \'libs\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?\n";
        return; // We're done, no need to continue.
    }

	if( pConfig.HasMember("define") )
	{
		if( AddDefines(pConfig["define"]) == false  )
		{
			std::cerr << "The \'defines\' object in the \'settings\' " << mConfigName << " is not an array\n";
			return; // We're done, no need to continue.
		}
	}	
	else
	{
   		std::cerr << "Internal error: The \'define\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?\n";
        return; // We're done, no need to continue.
	}
	
	if( pConfig.HasMember("extra_compiler_args") )
	{
		const rapidjson::Value &extraArgs = pConfig["extra_compiler_args"];
		if( extraArgs.IsArray() )
		{
			for( const auto& val : extraArgs.GetArray() )
			{
				mExtraCompilerArgs.push_back(val.GetString());
			}
		}
		else
		{
			std::cerr << "The \'extra_compiler_args\' object in the \'settings\' " << mConfigName << " is not an array!";
			return; // We're done, no need to continue.
		}
	}

	// Look for the output folder name. If not found default to bin.
	// TODO: Add some kind of environment variable system so this can be added to the default project.
	if( pConfig.HasMember("output_path") )
	{
		mOutputPath = pConfig["output_path"].GetString();
		if(mOutputPath.back() != '/')
			mOutputPath += "/";

		// Add the projects dir to the start so when build from a sub project will work.
		mOutputPath = CleanPath(mProjectDir + mOutputPath);

		// If path is absolute make it relative.
		// In this app all paths except includes are relative to the project files location.
		if( GetIsPathAbsolute(mOutputPath) )
		{
			mOutputPath = GetRelativePath(GetCurrentWorkingDirectory(),mOutputPath);

			if( mLoggingMode >= LOG_VERBOSE )
			{
				std::clog << "The \'output_path\' object in the \'settings\' " << mConfigName << " is an absolute path, changing to " << mOutputPath << '\n';
			}
		}
	}
	else
	{
		mOutputPath = CleanPath(mProjectDir + "bin/" + pConfigName + "/");
		if(mLoggingMode >= LOG_VERBOSE)
		{
			std::clog << "The \'output_path\' object in the configuration \'" << mConfigName << "\' was not set, defaulting too \'" << mOutputPath << "\'\n";
		}
	}

	// Find out what they wish to build.
	if( pConfig.HasMember("target") )
	{
		const std::string TargetName = pConfig["target"].GetString();
		if( CompareNoCase("executable",TargetName.c_str()) )
			mTargetType = TARGET_EXEC;
		else if( CompareNoCase("library",TargetName.c_str()) )
			mTargetType = TARGET_LIBRARY;
		else if( CompareNoCase("sharedobject",TargetName.c_str()) )
			mTargetType = TARGET_SHARED_OBJECT;

		// Was it set?
		if( mTargetType == TARGET_NOT_SET )
		{
			std::cerr << "The \'target\' object \"" << TargetName << "\" in the configuration:" << mConfigName << " is not a valid type.\n";
			return; // We're done, no need to continue.
		}
	}
	else
	{
   		std::cerr << "Internal error: The \'target\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?\n";
        return; // We're done, no need to continue.
	}

	if( pConfig.HasMember("output_name") && pConfig["output_name"].IsString() )
	{
		std::string filename = pConfig["output_name"].GetString();		
		// Depending on the target type, we need to make sure the format is right.
		if( mTargetType == TARGET_LIBRARY )
		{
			// Archive libs, by convention, should be libSOMETHING.a format.
			// I think I should write a class that adds handy functions to std::string. Been trying to avoid doing that.
			if( CompareNoCase(filename.c_str(),"lib",3) == false )
			{
				filename = std::string("lib") + filename;
			}

			// This could be a function called filename.ends_with(".a") .......
			if( CompareNoCase(filename.c_str() + filename.size() - 2,".a") == false )
			{
				filename += ".a";
			}
		}
		else if( mTargetType == TARGET_SHARED_OBJECT )
		{
			// Shared object files, by convention, should be libSOMETHING.so format.
			if( CompareNoCase(filename.c_str(),"lib",3) == false )
			{
				filename = std::string("lib") + filename;
			}
			
			// This could be a function called filename.ends_with(".so") .......
			if( CompareNoCase(filename.c_str() + filename.size() - 3,".so") == false )
			{
				filename += ".so";
			}
		}

		mOutputName = filename;
	}
	else
	{
		// Lets make up a reasonble name for the exe.
		// Use the passed in project name, but we do need to ensure that there is not characters to do with paths as they will cause problems.
		// The passed in project name could be anything, so may include path info. It's used to uniquely identify the project in a build. So could be verbose.
		mOutputName = GuessOutputName(pParentProject->GetProjectName());

		// If the target is an executable then we're safe to use a default name. For other output types, user has to supply an output.
		if( mTargetType == TARGET_LIBRARY )
		{
			mOutputName = std::string("lib") + mOutputName + ".a";
		}
		else if( mTargetType == TARGET_SHARED_OBJECT  )
		{
			mOutputName = std::string("lib") + mOutputName + ".so";
		}

        if( mLoggingMode >= LOG_VERBOSE )
        {
    		std::clog << "The \'output_name\' object in the \'settings\' is missing, defaulting too \'" << mOutputName << "\'\n";
        }
	}

	mOptimisation = GetStringLog(pConfig,"optimisation",mOptimisation,mLoggingMode >= LOG_VERBOSE);
	mDebugLevel = GetStringLog(pConfig,"debug_level",mDebugLevel,mLoggingMode >= LOG_VERBOSE);
	mGTKVersion = GetStringLog(pConfig,"gtk_version",mGTKVersion,mLoggingMode >= LOG_VERBOSE);
	mCppStandard = GetStringLog(pConfig,"standard",mCppStandard,mLoggingMode >= LOG_VERBOSE);

	mComplier = GetStringLog(pConfig,"compiler",mComplier,mLoggingMode >= LOG_VERBOSE);
	mLinker = GetStringLog(pConfig,"linker",mLinker,mLoggingMode >= LOG_VERBOSE);
	mArchiver = GetStringLog(pConfig,"archiver",mArchiver,mLoggingMode >= LOG_VERBOSE);

	mWarningsAsErrors = GetBoolLog(pConfig,"warnings_as_errors",mWarningsAsErrors,mLoggingMode >= LOG_VERBOSE);
	mEnableAllWarnings = GetBoolLog(pConfig,"enable_all_warnings",mEnableAllWarnings,mLoggingMode >= LOG_VERBOSE);
	mFatalErrors = GetBoolLog(pConfig,"fatal_errors",mFatalErrors,mLoggingMode >= LOG_VERBOSE);

	// See if there are any projects we are not dependent on.
	if( pConfig.HasMember("dependencies") && AddDependantProjects(pConfig["dependencies"]) == false )
	{
		std::cout << "The \'dependencies\' object in the \'settings\' is not an array" << std::endl;
		return; // We're done, no need to continue.
	}

	// Add configuration specific source files
	if( pConfig.HasMember("source_files") )
	{
		mSourceFiles.Read(pConfig["source_files"]);
	}

	if( pLoggingMode  >= LOG_VERBOSE )
	{
		std::clog << "Configuration " << pConfigName << '\n';
		std::clog << "    mProjectDir " << mProjectDir << '\n';
		std::clog << "    mOutputPath " << mOutputPath << '\n';
		std::clog << "    mOutputName " << mOutputName << '\n';
	}

	// If we get here, then all is ok.
	mOk = true;
}

Configuration::~Configuration()
{
	mOk = false;
}

rapidjson::Value Configuration::Write(rapidjson::Document::AllocatorType& pAllocator)const
{
	rapidjson::Value jsonConfig = rapidjson::Value(rapidjson::kObjectType);

	jsonConfig.AddMember("default",mIsDefaultConfig,pAllocator);
	jsonConfig.AddMember("target",std::string(TargetTypeToString(mTargetType)),pAllocator);
	jsonConfig.AddMember("compiler",mComplier,pAllocator);
	jsonConfig.AddMember("linker",mLinker,pAllocator);
	jsonConfig.AddMember("archiver",mArchiver,pAllocator);
	jsonConfig.AddMember("output_path",mOutputPath,pAllocator);
	jsonConfig.AddMember("output_name",mOutputName,pAllocator);
	jsonConfig.AddMember("standard",mCppStandard,pAllocator);
	jsonConfig.AddMember("optimisation",mOptimisation,pAllocator);
	jsonConfig.AddMember("debug_level",mDebugLevel,pAllocator);
	jsonConfig.AddMember("warnings_as_errors",mWarningsAsErrors,pAllocator);
	jsonConfig.AddMember("enable_all_warnings",mEnableAllWarnings,pAllocator);
	jsonConfig.AddMember("fatal_errors",mFatalErrors,pAllocator);
	
	

	if( mGTKVersion.size() > 0 )
	{
		jsonConfig.AddMember("gtk_version",mGTKVersion,pAllocator);
	}
	
	if( mIncludeSearchPaths.size() > 0 )
	{	
		jsonConfig.AddMember("include",BuildStringArray(mIncludeSearchPaths,pAllocator),pAllocator);
	}

	if( mLibrarySearchPaths.size() > 0 )
	{
		jsonConfig.AddMember("libpaths",BuildStringArray(mLibrarySearchPaths,pAllocator),pAllocator);
	}

	if( mLibraryFiles.size() > 0 )
	{
		jsonConfig.AddMember("libs",BuildStringArray(mLibraryFiles,pAllocator),pAllocator);
	}

	if( mDefines.size() > 0 )
	{
		jsonConfig.AddMember("define",BuildStringArray(mDefines,pAllocator),pAllocator);
	}

	if( mExtraCompilerArgs.size() > 0 )
	{
		jsonConfig.AddMember("extra_compiler_args",BuildStringArray(mExtraCompilerArgs,pAllocator),pAllocator);
	}

	if( mDependantProjects.size() > 0 )
	{
		jsonConfig.AddMember("dependencies",BuildStringArray(GetKeys(mDependantProjects),pAllocator),pAllocator);
	}

	if( mSourceFiles.size() > 0 )
	{
		jsonConfig.AddMember("source_files",mSourceFiles.Write(pAllocator),pAllocator);
	}

	return jsonConfig;
}

const StringVec Configuration::GetLibraryFiles()const
{
	StringVec allLibraryFiles = mLibraryFiles;
	if( mGTKVersion.size() > 0 )
	{
		AddLibrariesFromPKGConfig(allLibraryFiles,mGTKVersion);
	}
	return allLibraryFiles;
}

bool Configuration::GetBuildTasks(const SourceFiles& pProjectSourceFiles,const SourceFiles& pGeneratedResourceFiles,bool pRebuildAll,const ArgList& pAdditionalArgs,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const
{
	StringSet InputFilesSeen;	// Used to make sure a source file is not included twice. At the moment I show an error.

	// add the generated resource files.
	if( !AddCompileTasks(pGeneratedResourceFiles,pRebuildAll,pAdditionalArgs,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	// Add the project files.
	if( !AddCompileTasks(pProjectSourceFiles,pRebuildAll,pAdditionalArgs,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	// Add the configuration files.
	if( !AddCompileTasks(mSourceFiles,pRebuildAll,pAdditionalArgs,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	return true;
}

bool Configuration::RunOutputFile(const std::string& pSharedObjectPaths)const
{
	if( mTargetType == TARGET_EXEC )
	{
		const std::string pathedTargetName = GetPathedTargetName();
		if( mLoggingMode >= LOG_INFO )
		{
			std::clog << "Running command " << pathedTargetName << " ";
			if( pSharedObjectPaths.size() > 0 )
			{
				std::clog << "LD_LIBRARY_PATH = " << pSharedObjectPaths;
			}
			std::clog << '\n';
		}

		StringMap Env;
		if( pSharedObjectPaths.size() > 0 )
		{
			Env["LD_LIBRARY_PATH"] = pSharedObjectPaths;
		}

		ExecuteCommand(pathedTargetName,mExecuteParams,Env);
	}

	return false;
}

bool Configuration::AddCompileTasks(const SourceFiles& pSourceFiles,bool pRebuildAll,const ArgList& pAdditionalArgs,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles,StringSet& rInputFilesSeen)const
{
	// A little earlyout for small projects as this function can be called multiple times!
	if( pSourceFiles.IsEmpty() )
		return true;

	// This is used to uniquify source files that could create the same output file. (same file name in different folders)
	// It is important that this is updated even when files don't need to be compiled.
	// This is done in a predictable way that will always generate the same result.
	// The output file name is generated at this point and so the dependency checking and linking will work. I could name the obj file anything if I wanted.
	StringIntMap FileUseCount;

	// Some defines I add.
	const std::string DEF_APP_BUILD_DATE_TIME = "-DAPP_BUILD_DATE_TIME=\"" + GetTimeString() + "\"";
	const std::string DEF_APP_BUILD_DATE = "-DAPP_BUILD_DATE=\"" + GetTimeString("%d-%m-%Y") + "\"";
	const std::string DEF_APP_BUILD_TIME = "-DAPP_BUILD_TIME=\"" + GetTimeString("%X") + "\"";
	const std::string DEF_BUILT_BY_APPBUILD = "-DBUILT_BY_APPBUILD";

	// We need to taek the include paths that the user added and append some of ours based on some other options the user has requested.
	StringVec includeSearchPaths = mIncludeSearchPaths;
	if( mGTKVersion.size() > 0 )
	{
		AddIncludesFromPKGConfig(includeSearchPaths,mGTKVersion);
	}

	for( const auto& filename : pSourceFiles )
	{
		// Later on in the code we deal with duplicate file names with a nice little sneaky trick.
		// Only possible because we use one process many threads for the whole build process and not many proccess, like make does.
		
		// Make sure output path is there.
		MakeDir(mOutputPath); 
		
		// Lets make a compile command.
		const std::string InputFilename = CleanPath(mProjectDir + filename);

		// See if the file has already been seen, if not continue.
		if( rInputFilesSeen.find(InputFilename) != rInputFilesSeen.end() )
		{
			std::cerr << "Source file \'" << InputFilename << "\' is in the project twice.\n";
			return false;
		}
		else if( FileExists(InputFilename) )			// If the source file exists then we'll continue, else show an error.
		{
			rInputFilesSeen.insert(InputFilename);

			if(mLoggingMode >= LOG_VERBOSE)
			{
				std::clog << mOutputPath << "\n";
			}

			// Makes an output file name that is in the bin folder using the passed in folder and filename. Deals with the filename having '../..' stuff in the path. Just stripped it.
			// pFolder can be null. This is normally the group name.
			std::string OutputFilename = mOutputPath;
			std::string fname = GetFileName(filename);
			const int UseIndex = FileUseCount[fname]++;	// The first time this is found, zero is returned and so no 'numbered' extension will be added. Ensures unique output file names when needed.

			if( OutputFilename.back() != '/' )
				OutputFilename += '/';
			OutputFilename += fname;
			if( UseIndex > 0 )
			{
				OutputFilename += ".";
				OutputFilename += UseIndex;
				OutputFilename += ".";
			}
			OutputFilename += ".obj";

			rOutputFiles.push_back(OutputFilename);// Need to record all the output files even if not built as we need that for the linker.

			if( pRebuildAll || rDependencies.RequiresRebuild(InputFilename,OutputFilename,includeSearchPaths) )
			{
				if(mLoggingMode >= LOG_VERBOSE)
				{
					std::clog << "Creating " << OutputFilename << " from file " << InputFilename << "\n";
				}


				bool isCfile = GetExtension(InputFilename) == "c";

				// Going to build the file, so delete the obj that is there.
				// If we do not do this then it can effect the dependency system.
				std::remove(OutputFilename.c_str());

				ArgList args(pAdditionalArgs);

				// This string has to have a value, else param will be -o that sets output. Would case big issues is this string was empty
				if( mOptimisation.size() > 0 )
				{
					args.AddArg("-o" + mOptimisation);
				}

				if( mDebugLevel.size() > 0 )
				{// Again, only build if string is not empty.
					args.AddArg("-g" + mDebugLevel);
				}

				args.AddArg(DEF_APP_BUILD_DATE_TIME);
				args.AddArg(DEF_APP_BUILD_DATE);
				args.AddArg(DEF_APP_BUILD_TIME);
				args.AddArg(DEF_BUILT_BY_APPBUILD);
				
				args.AddDefines(mDefines);
				
				args.AddIncludeSearchPath(includeSearchPaths);
				if( !isCfile )
					args.AddArg("-std=" + mCppStandard);

				if( mWarningsAsErrors )
					args.AddArg("-Werror");

				if( mEnableAllWarnings )
					args.AddArg("-Wall");

				if( mFatalErrors )
					args.AddArg("-Wfatal-errors");

				args.AddArg(mExtraCompilerArgs);

				args.AddArg("-o");
				args.AddArg(OutputFilename);
				args.AddArg("-c");
				args.AddArg(InputFilename);

				rBuildTasks.push(new BuildTaskCompile(GetFileName(filename), OutputFilename, mComplier,args,mLoggingMode));
			}
		}
		else
		{
			std::cerr << "Input filename not found " << InputFilename << '\n';
		}
	}
	return true;
}

bool Configuration::AddIncludeSearchPaths(const rapidjson::Value& pPaths)
{
	if( pPaths.IsArray() )
	{
		for( const auto& val : pPaths.GetArray() )
		{
			AddIncludeSearchPath(val.GetString());
		}
		return true;
	}
	return false;
}

void Configuration::AddIncludeSearchPath(const std::string& pPath)
{
	const std::string path = PreparePath(pPath);
	if( path.size() > 0 )
	{
		mIncludeSearchPaths.push_back(path);
	}
}

bool Configuration::AddLibrarySearchPaths(const rapidjson::Value& pPaths)
{
	if( pPaths.IsArray() )
	{
		for( const auto& val : pPaths.GetArray() )
		{
			AddLibrarySearchPath(val.GetString());
		}
		return true;
	}
	return false;
}

void Configuration::AddLibrarySearchPath(const std::string& pPath)
{
	const std::string path = PreparePath(pPath);
	mLibrarySearchPaths.push_back(path);
}

bool Configuration::AddLibraries(const rapidjson::Value& pLibs)
{
	if( pLibs.IsArray() )
	{
		for( const auto& val : pLibs.GetArray() )
		{
			AddLibrary(val.GetString());
		}
		return true;
	}
	return false;
}

void Configuration::AddLibrary(const std::string& pLib)
{
	mLibraryFiles.push_back(pLib);
}

bool Configuration::AddDefines(const rapidjson::Value& pDefines)
{
	if( pDefines.IsArray() )
	{
		for( const auto& val : pDefines.GetArray() )
		{
			AddDefine(val.GetString());
		}
		return true;
	}
	return false;
}

void Configuration::AddDefine(const std::string& pDefine)
{
	mDefines.push_back(pDefine);
}

bool Configuration::AddDependantProjects(const rapidjson::Value& pLibs)
{
	if( pLibs.IsArray() )
	{
		for( const auto& val : pLibs.GetArray() )
		{
			if( val.IsString() )
			{
				// Only add an empty entry if there is not one.
				// To prevent overwriting one.
				if( mDependantProjects.find(val.GetString()) == mDependantProjects.end() )
					mDependantProjects[val.GetString()] = "";// config to use will be resolved when the build starts
			}
			else
			{
				std::cerr << "Dependency list has a none string entry, please correct.\n";
				return false;
			}
		}
		return true;
	}
	else
	{
		std::cerr << "Dependency list for projects is not an array type, please correct.\n";
	}
	return false;	
}

const std::string Configuration::PreparePath(const std::string& pPath)
{
	assert(pPath.size() > 0);
	if( pPath.size() > 0 )
	{
		// Before we add it, see if it's an absolute path, if not add it to the project dir, then check if it exists.
		std::string ArgPath;
		if( pPath.front() != '/' )
			ArgPath = mProjectDir;

		ArgPath += pPath;
		if( DirectoryExists(ArgPath) )
		{
			if( ArgPath.back() != '/' )
				ArgPath += "/";
			return CleanPath(ArgPath);
		}
	}
	return std::string();
}

bool Configuration::AddIncludesFromPKGConfig(StringVec& pIncludeSearchPaths,const std::string& pVersion)const
{
	if(mLoggingMode >= LOG_VERBOSE)
		std::clog << "Calling \"pkg-config --cflags " << pVersion << "\" to add folders to include search \n";

	StringVec args;
	args.push_back("--cflags");
	args.push_back(pVersion);

	std::string results;
	if( ExecuteShellCommand("pkg-config",args,results) )
	{
		bool FoundIncludes = false;// This is used to see if we found any, if we did not then assume pkg-config failed and show an error
		const StringVec includes = SplitString(results," ");
		for( auto inc : includes )
		{
			if( inc.size() > 2 && CompareNoCase(inc.c_str(),"-I",2) )
			{
				const std::string theFolder = TrimWhiteSpace(inc.substr(2));

				if( DirectoryExists(theFolder) )
				{
					pIncludeSearchPaths.push_back(theFolder);
					FoundIncludes = true;
					if(mLoggingMode >= LOG_VERBOSE)
					{
						std::clog << "pkg-config adding folder to include search " << theFolder << '\n';
					}
				}
				else if(mLoggingMode >= LOG_VERBOSE)
				{
					std::clog << "pkg-config proposed folder not found, NOT added to include search " << theFolder << '\n';
				}
			}
		}
		// Did it work?
		if( FoundIncludes )
		{
			return true;
		}
		else
		{// Something went wrong, so return false for an error and show output from pkg-config
			std::cerr << results << '\n';
		}
	}

	return false;
}

bool Configuration::AddLibrariesFromPKGConfig(StringVec& pLibraryFiles,const std::string& pVersion)const
{
	if(mLoggingMode >= LOG_VERBOSE)
		std::clog << "Calling \"pkg-config --libs " << pVersion << "\" to add library dependencies to the linker \n";

	StringVec args;
	args.push_back("--libs");
	args.push_back(pVersion);

	std::string results;
	if( ExecuteShellCommand("pkg-config",args,results) )
	{
		bool FoundLibraries = false;// This is used to see if we found any, if we did not then assume pkg-config failed and show an error
		const StringVec includes = SplitString(results," ");
		for( auto inc : includes )
		{
			if( inc.size() > 2 && CompareNoCase(inc.c_str(),"-l",2) )
			{
				const std::string theFolder = TrimWhiteSpace(inc.substr(2));
				pLibraryFiles.push_back(theFolder);
				FoundLibraries = true;
				if(mLoggingMode >= LOG_VERBOSE)
					std::clog << "pkg-config adding library " << theFolder << '\n';

			}
		}

		// Did it work?
		if( FoundLibraries )
		{
			return true;
		}
		else
		{// Something went wrong, so return false for an error and show output from pkg-config
			std::cerr << results << '\n';
		}
	}

	return false;
}

const std::string Configuration::TargetTypeToString(eTargetType pTarget)const
{
	assert( pTarget == TARGET_EXEC || pTarget == TARGET_LIBRARY || pTarget == TARGET_SHARED_OBJECT );
	switch(pTarget)
	{
	case TARGET_NOT_SET:
		break;
		
	case TARGET_EXEC:
		return "executable";

	case TARGET_LIBRARY:
		return "library";

	case TARGET_SHARED_OBJECT:
		return "sharedobject";
	}
	return "TARGET_NOT_SET ERROR REPORT THIS BUG!";
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
