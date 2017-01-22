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

Configuration::Configuration(const std::string& pConfigName,const JSONValue* pConfig,const std::string& pPathedProjectFilename,const std::string& pProjectDir):
		mConfigName(pConfigName),
		mProjectDir(pProjectDir),
		mOk(false),
		mTargetType(TARGET_NOT_SET),
		mComplier("gcc"),
		mLinker("gcc"),
		mArchiver("ar"),
		mOutputPath("./bin/" + pConfigName + "/")
{
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
	if( libraries && AddLibraries(libraries) == false )
	{
		std::cout << "The \'libraries\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
		return; // We're done, no need to continue.
	}

	// Look for the output folder name. If not found default to bin.
	const JSONValue* outputpath = pConfig->Find("output_path");
	if( outputpath )
	{
		mOutputPath = outputpath->GetString();
	}
	else
	{
		std::cout << "The \'output_path\' object in the \'configuration\' object of this project file \'" << pPathedProjectFilename << "\' was not set, defaulting too \'" << mOutputPath << "\'" << std::endl;
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
		mPathedTargetName = mProjectDir + mOutputPath + output_name->GetString();
	}
	else
	{
		std::cout << "The \'output_name\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is missing, how could I know the destination filename?" << std::endl;
		return; // We're done, no need to continue.
	}

	// Read optimisation / optimization settings
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
		else
		{
			std::cout << "The \'output_name\' object in the \'settings\' object of this project file \'" << pPathedProjectFilename << "\' is missing, how could I know the destination filename?" << std::endl;
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

	// If we get here, then all is ok.
	mOk = true;
}

Configuration::~Configuration()
{
	mOk = false;
}

bool Configuration::GetBuildTasks(const StringVecMap& pSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const
{
	for( const auto& group : pSourceFiles )
	{
		// Make sure output path is there.
		MakeDir(mOutputPath + group.first);

		for( const auto& filename : group.second )
		{
			// Lets make a compile command.
			const std::string InputFilename = mProjectDir + filename;
			// If the source file exists then we'll continue, else show an error.
			if( FileExists(InputFilename) )
			{
				// Makes an output file name that is in the bin folder using the passed in folder and filename. Deals with the filename having '../..' stuff in the path. Just stripped it.
				// pFolder can be null. This is normally the group name.
				std::string OutputFilename = mProjectDir + mOutputPath + group.first;
				std::string fname = GetFileName(filename);

				if( OutputFilename.back() != '/' )
					OutputFilename += '/';
				OutputFilename += fname;
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

	return true;
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

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
