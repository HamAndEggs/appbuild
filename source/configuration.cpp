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
	if( mLoggingMode >= LOG_VERBOSE )
		std::cout << "Project Directory: " << mProjectDir << std::endl;
	
	AddIncludeSearchPath(mProjectDir);

	mIsDefaultConfig = GetBool(pConfig,"default",false);
	
	if( pConfig.HasMember("include") )
	{
        if( AddIncludeSearchPaths(pConfig["include"]) == false )
        {
            std::cout << "The \'include\' object in the \'settings\' " << mConfigName << " is not an array" << std::endl;
            return; // We're done, no need to continue.
        }
    }
    else
    {
   		std::cout << "Internal error: The \'include\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?" << std::endl;
        return; // We're done, no need to continue.
    }

	// These can be added to the args now as they need to come before libs.
	if( pConfig.HasMember("libpaths") )
    {
        if( AddLibrarySearchPaths(pConfig["libpaths"]) == false )
        {
            std::cout << "The \'libpaths\' object in the \'settings\' " << mConfigName << " is not an array" << std::endl;
            return; // We're done, no need to continue.
        }
    }
    else
    {
        if( mLoggingMode >= LOG_VERBOSE )
        {
    		std::cout << "The \'libpaths\' object in the \'settings\' " << mConfigName << " not set." << std::endl;
        }
    }

	// These go into a different place for now as they have to be added to the args after the object files have been.
	// This is because of the way linkers work.
	if( pConfig.HasMember("libs") )
	{
		if( AddLibraries(pConfig["libs"]) == false  )
		{
			std::cout << "The \'libraries\' object in the \'settings\' " << mConfigName << " is not an array" << std::endl;
			return; // We're done, no need to continue.
		}
	}
    else
    {
   		std::cout << "Internal error: The \'libs\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?" << std::endl;
        return; // We're done, no need to continue.
    }

	if( pConfig.HasMember("define") )
	{
		if( AddDefines(pConfig["define"]) == false  )
		{
			std::cout << "The \'defines\' object in the \'settings\' " << mConfigName << " is not an array" << std::endl;
			return; // We're done, no need to continue.
		}
	}	
	else
	{
   		std::cout << "Internal error: The \'define\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?" << std::endl;
        return; // We're done, no need to continue.
	}

	// Look for the output folder name. If not found default to bin.
	// TODO: Add some kind of environment variable system so this can be added to the default project.
	if( pConfig.HasMember("output_path") )
	{
		mOutputPath = pConfig["output_path"].GetString();
		if(mOutputPath.back() != '/')
			mOutputPath += "/";

		// If path is absolute make it relative.
		// In this app all paths except includes are relative to the project files location.
		if( GetIsPathAbsolute(mOutputPath) )
		{
			mOutputPath = GetRelativePath(GetCurrentWorkingDirectory(),mOutputPath);

			if( mLoggingMode >= LOG_VERBOSE )
			{
				std::cout << "The \'output_path\' object in the \'settings\' " << mConfigName << " is an absolute path, changing to " << mOutputPath << std::endl;
			}
		}
	}
	else if(mLoggingMode >= LOG_VERBOSE)
	{
		mOutputPath = CleanPath(mProjectDir + "bin/" + pConfigName + "/");
		std::cout << "The \'output_path\' object in the configuration \'" << mConfigName << "\' was not set, defaulting too \'" << mOutputPath << "\'" << std::endl;
	}

	// Find out what they wish to build.
	if( pConfig.HasMember("target") )
	{
		const std::string TargetName = pConfig["target"].GetString();
		if( CompareNoCase("executable",TargetName.c_str()) )
			mTargetType = TARGET_EXEC;
		else if( CompareNoCase("library",TargetName.c_str()) )
			mTargetType = TARGET_LIBRARY;
		else if( CompareNoCase("sharedlibrary",TargetName.c_str()) )
			mTargetType = TARGET_SHARED_LIBRARY;

		// Was it set?
		if( mTargetType == TARGET_NOT_SET )
		{
			std::cout << "The \'target\' object \"" << TargetName << "\" in the configuration:" << mConfigName << " is not a valid type." << std::endl;
			return; // We're done, no need to continue.
		}
	}
	else
	{
   		std::cout << "Internal error: The \'target\' object in the \'settings\' " << mConfigName << " is missing! What happened to our defaults?" << std::endl;
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
			if( CompareNoCase(mPathedTargetName.c_str(),"lib",3) == false )
			{
				filename = std::string("lib") + filename;
			}
			// This could be a function called mPathedTargetName.ends_with(".a") .......
			if( CompareNoCase(mPathedTargetName.c_str() + filename.size() - 2,".a") == false )
			{
				filename += ".a";
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
		if( mTargetType == TARGET_LIBRARY || mTargetType == TARGET_SHARED_LIBRARY )
		{
			mOutputName = std::string("lib") + mOutputName + ".a";
		}

        if( mLoggingMode >= LOG_VERBOSE )
        {
    		std::cout << "The \'output_name\' object in the \'settings\' is missing, defaulting too \'" << mOutputName << "\'" << std::endl;		
        }
	}

	mPathedTargetName = CleanPath(mProjectDir + mOutputPath + mOutputName);
	
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
		std::cout << "Configuration " << pConfigName << std::endl;
		std::cout << "    mProjectDir " << mProjectDir << std::endl;
		std::cout << "    mOutputPath " << mProjectDir << std::endl;
		std::cout << "    mOutputName " << mOutputName << std::endl;
		std::cout << "    mPathedTargetName " << mPathedTargetName << std::endl;
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

bool Configuration::GetBuildTasks(const SourceFiles& pProjectSourceFiles,const SourceFiles& pGeneratedResourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const
{
	StringSet InputFilesSeen;	// Used to make sure a source file is not included twice. At the moment I show an error.

	// add the generated resource files.
	if( !GetBuildTasks(pGeneratedResourceFiles,pRebuildAll,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	// Add the project files.
	if( !GetBuildTasks(pProjectSourceFiles,pRebuildAll,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	// Add the configuration files.
	if( !GetBuildTasks(mSourceFiles,pRebuildAll,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
		return false;

	return true;
}

bool Configuration::GetBuildTasks(const SourceFiles& pSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles,StringSet& rInputFilesSeen)const
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
		// Later on in the code we deal with duplicate file names with a nice little sneeky trick.
		// Only posible because we use one process many threads for the whole build process and not many proccess, like make does.
		
		// Make sure output path is there.
		MakeDir(mOutputPath); 
		
		// Lets make a compile command.
		const std::string InputFilename = CleanPath(mProjectDir + filename);

		// See if the file has already been seen, if not continue.
		if( rInputFilesSeen.find(InputFilename) != rInputFilesSeen.end() )
		{
			std::cout << "Source file \'" << InputFilename << "\' is in the project twice." << std::endl;
			return false;
		}
		else if( FileExists(InputFilename) )			// If the source file exists then we'll continue, else show an error.
		{
			rInputFilesSeen.insert(InputFilename);

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
				bool isCfile = GetExtension(InputFilename) == "c";

				// Going to build the file, so delete the obj that is there.
				// If we do not do this then it can effect the dependency system.
				std::remove(OutputFilename.c_str());

				ArgList args;
				args.AddArg("-o" + mOptimisation);
				args.AddArg("-g" + mDebugLevel);

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

				args.AddArg("-o");
				args.AddArg(OutputFilename);
				args.AddArg("-c");
				args.AddArg(InputFilename);

				rBuildTasks.push(new BuildTaskCompile(GetFileName(filename), OutputFilename, mComplier,args,mLoggingMode));
			}
		}
		else
		{
			std::cout << "Input filename not found " << InputFilename << std::endl;
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
				std::cout << "Dependency list has a none string entry, please correct. Item being ignored." << std::endl;
			}
		}
		return true;
	}
	else
	{
		std::cout << "Dependency list for projects is not an array type, please correct." << std::endl;
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
		std::cout << "Calling \"pkg-config --cflags " << pVersion << "\" to add folders to include search " << std::endl;

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
						std::cout << "pkg-config adding folder to include search " << theFolder << std::endl;
				}
				else if(mLoggingMode >= LOG_VERBOSE)
				{
					std::cout << "pkg-config proposed folder not found, NOT added to include search " << theFolder << std::endl;
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
			std::cout << results << std::endl;
		}
	}

	return false;
}

bool Configuration::AddLibrariesFromPKGConfig(StringVec& pLibraryFiles,const std::string& pVersion)const
{
	if(mLoggingMode >= LOG_VERBOSE)
		std::cout << "Calling \"pkg-config --libs " << pVersion << "\" to add library dependancies to the linker " << std::endl;

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
					std::cout << "pkg-config adding library " << theFolder << std::endl;

			}
		}

		// Did it work?
		if( FoundLibraries )
		{
			return true;
		}
		else
		{// Something went wrong, so return false for an error and show output from pkg-config
			std::cout << results << std::endl;
		}
	}

	return false;
}

const std::string Configuration::TargetTypeToString(eTargetType pTarget)const
{
	assert( pTarget == TARGET_EXEC || pTarget == TARGET_LIBRARY || pTarget == TARGET_SHARED_LIBRARY );
	switch(pTarget)
	{
	case TARGET_NOT_SET:
		break;
		
	case TARGET_EXEC:
		return "executable";

	case TARGET_LIBRARY:
		return "library";

	case TARGET_SHARED_LIBRARY:
		return "sharedlibrary";
	}
	return "TARGET_NOT_SET ERROR REPORT THIS BUG!";
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
