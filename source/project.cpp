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
#include <fstream>
#include <sstream>
#include <iostream>
#include <libgen.h>
#include <unistd.h>

#include "project.h"
#include "json.h"
#include "misc.h"
#include "arg_list.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
Project::Project(const std::string& pFilename,size_t pNumThreads,bool pVerboseOutput,bool pRebuild):mNumThreads(pNumThreads>0?pNumThreads:1),mVerboseOutput(pVerboseOutput),mRebuild(pRebuild),mOutputPath("./bin/"),mComplier("gcc"),mLinker("gcc"),mArchiver("ar"),mDependencies(pFilename),mOk(true),mTargetType(TARGET_NOT_SET)
{
	assert( pNumThreads > 0 );

	// Get the absolute path to the project file, change to that directory and build.
	// This is because the project file is written expecting that all paths are relative to it's location.
	char *projectfile = CopyString(pFilename);
	mProjectDir = dirname(projectfile);
	delete[]projectfile;
	projectfile = NULL;

	if(mProjectDir.back() != '/')
		mProjectDir += "/";

	AddIncludeSearchPath("/usr/include");
	mCompileArguments.AddArg("-g");

	// Build pathed filename for the project file.
	mPathedProjectFilename = pFilename;
	Debug("Reading project file %s",mPathedProjectFilename.c_str());

	// Now build our project object.
	std::ifstream ProjectFile(mPathedProjectFilename);
	Json ProjectJson(ProjectFile);
    if( ProjectJson )
    {
		// Read the settings.
		const JSONValue* settings = ProjectJson.Find("settings");
		if( settings )
			ReadSettings(settings);

		// Add the files, building the make commands.
		// We do the files last as we now have all the information. They can be anywhere in the file, that's ok.
		const JSONValue* groups = ProjectJson.Find("groups");
		if( groups )
		{// Instead of looking for specific items here we'll enumerate them.
			for( auto group : groups->GetObject()->GetChildren() )
			{// Can be any number of json elements with the same name at the same level. So we have a vector.
				for(auto x : group.second)
					ReadGroupFiles(group.first,x);
			}
		}
		else
		{
			std::cout << "The \'groups\' object in this project file \'" << mPathedProjectFilename << "\' is not found" << std::endl;
		}

    }

}

Project::~Project()
{

}

bool Project::Build()
{
	if( CompileSource() )
	{
		return LinkTarget();
	}
    return false;
}

bool Project::RunOutputFile(bool pAsSudo)
{
	if( mTargetType == TARGET_EXEC )
	{
		char command[256];
		command[0] = 0;
		if( pAsSudo )
			strcat(command,"sudo ");

		strcat(command,mPathedTargetName.c_str());

		char* TheArgs[]={command,NULL};
		return execvp(TheArgs[0],TheArgs) == 0;
	}
	return false;
}

void Project::ReadGroupFiles(const char* pGroupName,const JSONValue* pFiles)
{
	if(pFiles)// Can be null.
	{
		if( pFiles->GetType() != JSONValue::ARRAY )
		{
			std::cout << "The \'files\' object in the \'group\' object of this project file \'" << mPathedProjectFilename << "\' is not an array" << std::endl;
			mOk = false;
		}
		else
		{
			// Make sure output path is there.
			MakeDir(mOutputPath + pGroupName);

			for( int n = 0 ; n < pFiles->GetArraySize() ; n++ )
			{
				const char* filename = pFiles->GetString(n);
				if(filename)
				{// Lets make a compile command.
					const std::string InputFilename = mProjectDir + filename;
					// If the source file exists then we'll continue, else show an error.
					if( FileExists(InputFilename) )
					{
						const std::string OutputFilename = MakeOutputFilename(filename, pGroupName);

						// Record the object filenames created.
						mObjectFiles.push_back(OutputFilename);

						if( mRebuild || mDependencies.RequiresRebuild(InputFilename,OutputFilename,mCompileArguments.GetIncludePaths()) )
						{
							// Going to build the file, so delete the obj that is there.
							// If we do not do this then it can effect the dependency system.
							std::remove(OutputFilename.c_str());

							ArgList args(mCompileArguments);
							args.AddArg("-o");
							args.AddArg(OutputFilename);
							args.AddArg("-c");
							args.AddArg(InputFilename);

							mBuildTasks.push(new BuildTask(GetFileName(filename), OutputFilename, mComplier,args));
						}
					}
					else
					{
						std::cout << "Input filename not found " << InputFilename << std::endl;
					}
				}
			}
		}

	}
}

void Project::ReadSettings(const JSONValue* pSettings)
{
	assert(pSettings);

	const JSONValue* includes = pSettings->Find("include");
	if( includes && AddIncludeSearchPaths(includes) == false )
	{
		std::cout << "The \'include\' object in the \'settings\' object of this project file \'" << mPathedProjectFilename << "\' is not an array" << std::endl;
		mOk = false;
	}

	// These can be added to the args now as they need to come before libs.
	const JSONValue* libpaths = pSettings->Find("libpaths");
	if( libpaths && AddLibrarySearchPaths(libpaths) == false )
	{
		std::cout << "The \'libpaths\' object in the \'settings\' object of this project file \'" << mPathedProjectFilename << "\' is not an array" << std::endl;
		mOk = false;
	}

	// These go into a different place for now as they have to be adde to the args after the object files have been.
	// This is because of the way linkers work.
	const JSONValue* libraries = pSettings->Find("libs");
	if( libraries &&AddLibraries(libraries) == false )
	{
		std::cout << "The \'libraries\' object in the \'settings\' object of this project file \'" << mPathedProjectFilename << "\' is not an array" << std::endl;
		mOk = false;
	}

	// Find out what they wish to build.
	const JSONValue* target = pSettings->Find("target");
	if( target )
	{
		if( CompareNoCase("executable",target->GetString()) )
			mTargetType = TARGET_EXEC;
		else if( CompareNoCase("library",target->GetString()) )
			mTargetType = TARGET_LIBRARY;
		else if( CompareNoCase("sharedlibrary",target->GetString()) )
			mTargetType = TARGET_SHARED_LIBRARY;

		if( mTargetType == TARGET_NOT_SET )
			mOk = false;
	}

	const JSONValue* output_name = pSettings->Find("output_name");
	if( output_name && output_name->GetType() == JSONValue::STRING )
	{
		mPathedTargetName = mProjectDir + mOutputPath + output_name->GetString();
	}
	else
	{
		std::cout << "The \'output_name\' object in the \'settings\' object of this project file \'" << mPathedProjectFilename << "\' is missing, how could I know the destination filename?" << std::endl;
		mOk = false;
	}

	// Read optimisation / optimization settings
	const JSONValue* optimisation = pSettings->Find("optimisation");
	if( !optimisation )
		optimisation = pSettings->Find("optimization");
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
			std::cout << "The \'output_name\' object in the \'settings\' object of this project file \'" << mPathedProjectFilename << "\' is missing, how could I know the destination filename?" << std::endl;
		}
		mCompileArguments.AddArg("-o" + level);
	}

	// Check for the c standard settings.
	// Read optimisation / optimization settings
	const JSONValue* standard = pSettings->Find("standard");
	if( standard && standard->GetType() == JSONValue::STRING )
	{
		std::string level = standard->GetString();
		mCompileArguments.AddArg("-std=" + level);
	}


}

const std::string Project::MakeOutputFilename(const std::string& pFilename, const std::string& pFolder)const
{
	assert(pFilename.size() > 0);

// Makes an output file name that is in the bin folder using the passed in folder and filename. Deals with the filename having '../..' stuff in the path. Just stripped it.
 // pFolder can be null. This is normally the group name.
	std::string path = mProjectDir + mOutputPath + pFolder;
	std::string fname = GetFileName(pFilename);

	if( path.back() != '/' )
		path += '/';
	path += fname;
	path += ".obj";

	return path;
}

bool Project::CompileSource()
{
	std::cout << "Building: Num Threads " << mNumThreads << std::endl;

	RunningBuildTasks RunningTasks;
	bool CompileOk = true;

	// I always delete the target if something needs to be build so there is no exec to run if the source has failed to build.
	remove(mPathedTargetName.c_str());

	while( mBuildTasks.size() > 0 || RunningTasks.size() > 0 )
	{
		// Make sure at least N tasks are running.
		if( mBuildTasks.size() > 0 && RunningTasks.size() < mNumThreads )
		{
			BuildTask* task = mBuildTasks.top();
			mBuildTasks.pop();
			RunningTasks.push_back(task);
			task->Execute();
		}
		else
		{
			// For when GetNumThreads are running or waiting for the last tasks to complete.
			// Back the main thread off a little so we don't thrash the cpu just waiting.
			std::this_thread::yield();
		}

		// See if any running task has finished.
		RunningBuildTasks::iterator task = RunningTasks.begin();
		while( task != RunningTasks.end() )
		{
			if( (*task)->GetIsCompleted() )
			{
				CompileOk = CompileOk && (*task)->GetOk();
				std::string res = (*task)->GetResults();
				if( res.size() > 1 )
					std::cout << res << std::endl;
				delete (*task);
				task = RunningTasks.erase(task);

				// If the compile failed, clean up the tasks that are waiting to start and then just wait for the ones in progress to finish.
				if( !CompileOk )
				{
					while( !mBuildTasks.empty() )
					{
						delete mBuildTasks.top();
						mBuildTasks.pop();
					}
				}
			}
			else
			{
				++task;
			}
		};
	};

	std::cout << "Build finished" << std::endl;
	return CompileOk;
}

bool Project::LinkTarget()
{
	// Add the object files.
	mLinkerArguments.AddArg(mObjectFiles);

	// Add the libs, must come after the object files.
	mLinkerArguments.AddArg(mLibraryFiles);

	// And add the output.
	mLinkerArguments.AddArg("-o");
	mLinkerArguments.AddArg(mPathedTargetName);

	std::cout << "Linking: " << mPathedTargetName << std::endl;

	std::string LinkerResults;
	bool ok = ExecuteShellCommand(mLinker,mLinkerArguments,LinkerResults);
	if( LinkerResults.size() < 1 )
		LinkerResults = "Linked ok";
	std::cout << LinkerResults << std::endl;
	return ok;
}

bool Project::AddIncludeSearchPaths(const JSONValue* pPaths)
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

void Project::AddIncludeSearchPath(const std::string& pPath)
{
	mCompileArguments.AddIncludeSearchPath(pPath,mProjectDir);
}

bool Project::AddLibrarySearchPaths(const JSONValue* pPaths)
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

void Project::AddLibrarySearchPath(const std::string& pPath)
{
	mLinkerArguments.AddLibrarySearchPath(pPath,mProjectDir);
}

bool Project::AddLibraries(const JSONValue* pLibs)
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

void Project::AddLibrary(const std::string& pLib)
{
	mLibraryFiles.push_back("-l" + pLib);
}


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
