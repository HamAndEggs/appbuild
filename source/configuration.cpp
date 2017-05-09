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

#include "misc.h"
#include "configuration.h"
#include "build_task.h"
#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

Configuration::Configuration(const std::string& pProjectDir,const std::string& pProjectName):// Creates a default configuration suitable for simple c++11 projects.
	mConfigName("default"),
	mProjectDir(pProjectDir),
	mOk(true),
	mTargetType(TARGET_EXEC),
	mComplier("gcc"),
	mLinker("gcc"),
	mArchiver("ar"),
	mOutputPath("./bin/")
{
	AddIncludeSearchPath("/usr/include");
	AddLibrary("stdc++");
	AddLibrary("pthread");
	AddDefine("APP_BUILD_DATE_TIME=\"" + GetTimeString() + "\"");
	AddDefine("APP_BUILD_DATE=\"" + GetTimeString("%d-%m-%Y") + "\"");
	AddDefine("APP_BUILD_TIME=\"" + GetTimeString("%X") + "\"");

	mCompileArguments.AddArg("-o0");
	mCompileArguments.AddArg("-std=c++11");
	
	mPathedTargetName = CleanPath(mProjectDir + mOutputPath + pProjectName);
}

Configuration::Configuration(const std::string& pConfigName,const JSONValue* pConfig,const std::string& pPathedProjectFilename,const std::string& pProjectDir,bool pVerboseOutput):
		mConfigName(pConfigName),
		mProjectDir(pProjectDir),
		mOk(false),
		mTargetType(TARGET_NOT_SET),
		mComplier("gcc"),
		mLinker("gcc"),
		mArchiver("ar"),
		mOutputPath(pProjectDir + "bin/" + pConfigName + "/")
{
	AddDefine("APP_BUILD_DATE_TIME=\"" + GetTimeString() + "\"");
	AddDefine("APP_BUILD_DATE=\"" + GetTimeString("%d-%m-%Y") + "\"");
	AddDefine("APP_BUILD_TIME=\"" + GetTimeString("%X") + "\"");

	const JSONValue* includes = pConfig->Find("include");
	if( includes && AddIncludeSearchPaths(includes) == false )
	{
		std::cout << "The \'include\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
		return; // We're done, no need to continue.
	}

	// These can be added to the args now as they need to come before libs.
	const JSONValue* libpaths = pConfig->Find("libpaths");
	if( libpaths && AddLibrarySearchPaths(libpaths) == false )
	{
		std::cout << "The \'libpaths\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
		return; // We're done, no need to continue.
	}

	// These go into a different place for now as they have to be adde to the args after the object files have been.
	// This is because of the way linkers work.
	const JSONValue* libraries = pConfig->Find("libs");
	if( libraries )
	{
		if( AddLibraries(libraries) == false  )
		{
			std::cout << "The \'libraries\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
			return; // We're done, no need to continue.
		}
	}
	else
	{
		if( pVerboseOutput )
			std::cout << "The \'libraries\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is missing, adding stdc++ for a default." << std::endl;
		
		AddLibrary("stdc++");
	}

	const JSONValue* defines = pConfig->Find("define");
	if( defines )
	{
		if( AddDefines(defines) == false  )
		{
			std::cout << "The \'defines\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
			return; // We're done, no need to continue.
		}
	}	
	else
	{
		if( pVerboseOutput )
			std::cout << "The \'define\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is missing, adding NDEBUG for a default." << std::endl;
		AddDefine("NDEBUG");	
	}

	// Look for the output folder name. If not found default to bin.
	const JSONValue* outputpath = pConfig->Find("output_path");
	if( outputpath )
	{
		mOutputPath = outputpath->GetString();
		if(mOutputPath.back() != '/')
			mOutputPath += "/";
	}
	else if(pVerboseOutput)
	{
		std::cout << "The \'output_path\' object in the configuration \'" << mConfigName << "\' object of the project file \'" << pPathedProjectFilename << "\' was not set, defaulting too \'" << mOutputPath << "\'" << std::endl;
	}

	// Find out what they wish to build.
	const JSONValue* target = pConfig->Find("target");
	if( target )
	{
		const char* TargetName = target->GetString();
		if( CompareNoCase("executable",TargetName) )
			mTargetType = TARGET_EXEC;
		else if( CompareNoCase("library",TargetName) )
			mTargetType = TARGET_LIBRARY;
		else if( CompareNoCase("sharedlibrary",TargetName) )
			mTargetType = TARGET_SHARED_LIBRARY;

		// Was it set?
		if( mTargetType == TARGET_NOT_SET )
		{
			std::cout << "The \'target\' object \"" << TargetName << "\" in the configuration:" << mConfigName << " object of this project file \'" << pPathedProjectFilename << "\' is not a valid type." << std::endl;
			return; // We're done, no need to continue.
		}
	}

	const JSONValue* output_name = pConfig->Find("output_name");
	if( output_name && output_name->GetType() == JSONValue::STRING )
	{
		std::string filename = output_name->GetString();
		mOutputName = filename;
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

		mPathedTargetName = mProjectDir + mOutputPath + filename;
	}
	else
	{
		std::cout << "The \'output_name\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is missing, how could I know the destination filename?" << std::endl;
		return; // We're done, no need to continue.
	}

	// Read optimisation / optimisation settings
	const JSONValue* optimisation = pConfig->Find("optimisation");
	if( !optimisation )
		optimisation = pConfig->Find("optimization");
	if( optimisation )
	{
		std::string level="0";
		if(optimisation->GetType() == JSONValue::INT32)
		{
			level = std::to_string(optimisation->GetInt32());
		}
		else if(optimisation->GetType() == JSONValue::STRING)
		{
			level = optimisation->GetString();
		}
		mCompileArguments.AddArg("-o" + level);
	}

	// Check for the c standard settings.
	// Read optimisation / optimization settings
	const JSONValue* standard = pConfig->Find("standard");
	if( standard && standard->GetType() == JSONValue::STRING )
	{
		std::string level = standard->GetString();
		mCompileArguments.AddArg("-std=" + level);
	}

	// See if there are any projects we are not dependent on.
	const JSONValue* dependencies = pConfig->Find("dependencies");
	if( dependencies && AddDependantProjects(dependencies) == false )
	{
		std::cout << "The \'dependencies\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
		return; // We're done, no need to continue.
	}

	// Add configuration specific source files
	mSourceFiles.Read(pConfig->Find("source_files"),pPathedProjectFilename);

	// If we get here, then all is ok.
	mOk = true;
}

Configuration::~Configuration()
{
	mOk = false;
}

bool Configuration::GetBuildTasks(const StringSetMap& pProjectSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const
{
	StringSet InputFilesSeen;	// Used to make sure a source file is not included twice. At the moment I show an error.

	// Add the project files.
	for( const auto& group : pProjectSourceFiles )
	{
		if( !GetBuildTasks(group,pRebuildAll,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
			return false;
	}

	// Add the configuration files.
	for( const auto& group : mSourceFiles )
	{
		if( !GetBuildTasks(group,pRebuildAll,rBuildTasks,rDependencies,rOutputFiles,InputFilesSeen) )
			return false;
	}
	return true;
}

bool Configuration::GetBuildTasks(const StringSetMap::value_type& pGroupSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles,StringSet& rInputFilesSeen)const
{
	// This is used to uniquify source files that could create the same output file. (same file name in different folders)
	// It is important that this is updated even when files don't need to be compiled.
	// This is done in a predictable way that will always generate the same result.
	// The output file name is generated at this point and so the dependency checking and linking will work. I could name the obj file anything if I wanted.
	StringIntMap FileUseCount;

	for( const auto& filename : pGroupSourceFiles.second )
	{
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
			std::string OutputFilename = CleanPath(mProjectDir + mOutputPath + pGroupSourceFiles.first);
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

			if( pRebuildAll || rDependencies.RequiresRebuild(InputFilename,OutputFilename,mCompileArguments.GetIncludePaths()) )
			{
				// Going to build the file, so delete the obj that is there.
				// If we do not do this then it can effect the dependency system.
				std::remove(OutputFilename.c_str());

				ArgList args(mCompileArguments);
				args.AddArg("-o");
				args.AddArg(OutputFilename);
				args.AddArg("-c");
				args.AddArg(InputFilename);

				rBuildTasks.push(new BuildTask(GetFileName(filename), OutputFilename, mComplier,args));
			}
		}
		else
		{
			std::cout << "Input filename not found " << InputFilename << std::endl;
		}
	}
}

bool Configuration::AddIncludeSearchPaths(const JSONValue* pPaths)
{
	if( pPaths->GetType() == JSONValue::ARRAY )
	{
		for( int n = 0 ; n < pPaths->GetArraySize() ; n++ )
		{
			AddIncludeSearchPath(pPaths->GetString(n));
		}
		return true;
	}
	return false;
}

void Configuration::AddIncludeSearchPath(const std::string& pPath)
{
	mCompileArguments.AddIncludeSearchPath(pPath,mProjectDir);
}

bool Configuration::AddLibrarySearchPaths(const JSONValue* pPaths)
{
	if( pPaths->GetType() == JSONValue::ARRAY )
	{
		for( int n = 0 ; n < pPaths->GetArraySize() ; n++ )
		{
			AddLibrarySearchPath(pPaths->GetString(n));
		}
		return true;
	}
	return false;
}

void Configuration::AddLibrarySearchPath(const std::string& pPath)
{
	mLibrarySearchPaths.AddLibrarySearchPath(pPath,mProjectDir);
}

bool Configuration::AddLibraries(const JSONValue* pLibs)
{
	if( pLibs->GetType() == JSONValue::ARRAY )
	{
		for( int n = 0 ; n < pLibs->GetArraySize() ; n++ )
		{
			AddLibrary(pLibs->GetString(n));
		}
		return true;
	}
	return false;
}

void Configuration::AddLibrary(const std::string& pLib)
{
	mLibraryFiles.push_back("-l" + pLib);
}

bool Configuration::AddDefines(const JSONValue* pDefines)
{
	if( pDefines->GetType() == JSONValue::ARRAY )
	{
		for( int n = 0 ; n < pDefines->GetArraySize() ; n++ )
		{
			AddDefine(pDefines->GetString(n));
		}
		return true;
	}
	return false;
}

void Configuration::AddDefine(const std::string& pDefine)
{
	mCompileArguments.AddArg("-D" + pDefine);
}

bool Configuration::AddDependantProjects(const JSONValue* pLibs)
{
	if( pLibs->GetType() == JSONValue::ARRAY )
	{
		for( int n = 0 ; n < pLibs->GetArraySize() ; n++ )
		{
			if( pLibs->GetType(n) == JSONValue::STRING )
			{
				mDependantProjects[pLibs->GetString(n)] = "";// config to use will be resolved when the build starts
			}
			else if( pLibs->GetType(n) == JSONValue::OBJECT )
			{
				const JSONObject* projects = pLibs->GetObject(n);
				if(projects)
				{
					const ValueMap& children = projects->GetChildren();
					for( auto& proj : children )
					{
						for( auto& obj : proj.second )
							mDependantProjects[proj.first] = obj->GetString();
					}
				}
			}
		}
		return true;
	}
	return false;	
}


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
