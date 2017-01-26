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
Project::Project(const std::string& pFilename,size_t pNumThreads,bool pVerboseOutput,bool pRebuild):
		mNumThreads(pNumThreads>0?pNumThreads:1),
		mVerboseOutput(pVerboseOutput),
		mRebuild(pRebuild),
		mDependencies(pFilename),
		mOk(true)
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

	// Build pathed filename for the project file.
	mPathedProjectFilename = pFilename;
	Debug("Reading project file %s",mPathedProjectFilename.c_str());

	// Now build our project object.
	std::ifstream ProjectFile(mPathedProjectFilename);
	Json ProjectJson(ProjectFile);
    if( ProjectJson )
    {
		// Read the configs.
		const JSONValue* configs = ProjectJson.Find("configurations");
		if( configs )
			ReadConfigurations(configs);

		if( mBuildConfigurations.size() == 0 )
		{
			std::cout << "No configurations found in the project file \'" << mPathedProjectFilename << "\' using default exec configuration." << std::endl;
			const Configuration* config = new Configuration(mProjectDir,GetFileName(mPathedProjectFilename,true));
			mBuildConfigurations[config->GetName()] = config;
		}

		assert( mBuildConfigurations.size() > 0 );

		// Add the files, building the make commands.
		// We do the files last as we now have all the information. They can be anywhere in the file, that's ok.
		const JSONValue* groups = ProjectJson.Find("source_files");
		if( groups )
		{// Instead of looking for specific items here we'll enumerate them.
			for( auto group : groups->GetObject()->GetChildren() )
			{// Can be any number of json elements with the same name at the same level. So we have a vector.
				for(auto x : group.second)
					ReadSourceFiles(group.first,x);
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

bool Project::Build(const std::string& pActiveConfig)
{
	const Configuration* config = mBuildConfigurations.find(pActiveConfig)->second;
	if( !config )
	{
		// If there is only one config, then just used that.
		if( mBuildConfigurations.size() == 1 )
			config = mBuildConfigurations.begin()->second;

		if( !config )
		{
			if( pActiveConfig.size() > 0 )
				std::cout << "The configuration \'" << pActiveConfig << "\' to build was not found in the project file \'" << mPathedProjectFilename << "\'" << std::endl;
			else if( mBuildConfigurations.size() < 1 )
			{//Should not get here, but just incase, show an error.
				std::cout << "No configurations found in the project file \'" << mPathedProjectFilename << "\' can not continue." << std::endl;
			}
			else
			{
				std::cout << "No configuration was specified to build, your choices are:-" << std::endl;
				for(auto conf : mBuildConfigurations )
				{
					std::cout << conf.first << std::endl;
				}
				std::cout << "Use -c [config name] to specify which to build." << std::endl;
			}
			return false;
		}
	}

	// We know what config to build with, lets go.
	std::cout << "Compiling configuration \'" << pActiveConfig << "\'" << std::endl;
	StringVec OutputFiles;
	if( CompileSource(config,OutputFiles) )
	{
		return LinkTarget(config,OutputFiles);
	}
	return false;
}

bool Project::RunOutputFile(bool pAsSudo)
{
	std::string PathedTargetName;

	BuildConfigurations::const_iterator found = mBuildConfigurations.find(mCurrentConfigName);
	if( found == mBuildConfigurations.end() )
		return false;// Config not found.

	const Configuration* Config = found->second;

	if( Config )
	{
		if( Config->GetTargetType() == TARGET_EXEC )
		{
			char command[256];
			command[0] = 0;
			if( pAsSudo )
				strcat(command,"sudo ");

			strcat(command,Config->GetPathedTargetName().c_str());

			char* TheArgs[]={command,NULL};
			return execvp(TheArgs[0],TheArgs) == 0;
		}
	}
	return false;
}

void Project::ReadSourceFiles(const char* pGroupName,const JSONValue* pFiles)
{
	if(pFiles && pGroupName)// Can be null.
	{
		if( pFiles->GetType() != JSONValue::ARRAY )
		{
			std::cout << "The \'files\' object in the \'group\' object of this project file \'" << mPathedProjectFilename << "\' is not an array" << std::endl;
			mOk = false;
		}
		else
		{
			for( int n = 0 ; n < pFiles->GetArraySize() ; n++ )
			{
				const char* filename = pFiles->GetString(n);
				if(filename)
				{
					const std::string InputFilename = mProjectDir + filename;
					// If the source file exists then we'll continue, else show an error.
					if( FileExists(InputFilename) )
					{
						mSourceFiles[pGroupName].push_back(filename);
					}
					else
					{
						std::cout << "Input filename \'" << filename << "\' in group \'" << pGroupName << "\' not found at \'" << InputFilename << "\'" << std::endl;
					}
				}
			}
		}
	}
}

void Project::ReadConfigurations(const JSONValue* pSettings)
{
	assert(pSettings);

	for(auto& configs : pSettings->GetObject()->GetChildren() )
	{
		const char* name = configs.first;
		if( name )
		{
			if( configs.second.size() > 1 )
			{
				std::cout << "Configuration \'"<< name << "\' not unique, names must be unique, error in project " << mPathedProjectFilename << std::endl;
			}
			else if( configs.second.size() < 1 )
			{
				std::cout << "Configuration \'"<< name << "\' has no body to the object, check syntax. Error in project " << mPathedProjectFilename << std::endl;
			}
			else
			{
				const JSONValue* Obj = configs.second[0];
				if(Obj)
				{
					mBuildConfigurations[name] = new Configuration(name,Obj,mPathedProjectFilename,mProjectDir);
				}
			}
		}
		else
		{
			std::cout << "Malformed configuration in project without a name in project" << mPathedProjectFilename << std::endl;
		}
	}
}



bool Project::CompileSource(const Configuration* pConfig,StringVec& rOutputFiles)
{
	assert(pConfig);
	if( !pConfig )
		return false;

	BuildTaskStack BuildTasks;
	if( !pConfig->GetBuildTasks(mSourceFiles,mRebuild,BuildTasks,mDependencies,rOutputFiles) )
	{
		std::cout << "Unable to create build tasks for the configuration \'" << pConfig->GetName() << "\' in project \'" << mPathedProjectFilename << "\'" << std::endl;
		return false;
	}

	std::cout << "Building:"<< BuildTasks.size() <<" files with " << mNumThreads << " threads." << std::endl;
	bool CompileOk = true;

	// I always delete the target if something needs to be build so there is no exec to run if the source has failed to build.
	remove(pConfig->GetPathedTargetName().c_str());

	RunningBuildTasks RunningTasks;
	while( BuildTasks.size() > 0 || RunningTasks.size() > 0 )
	{
		// Make sure at least N tasks are running.
		if( BuildTasks.size() > 0 && RunningTasks.size() < mNumThreads )
		{
			BuildTask* task = BuildTasks.top();
			BuildTasks.pop();
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
				// Print the results.
				std::string res = (*task)->GetResults();
				if( res.size() > 1 )
					std::cout << res << std::endl;

				// See if it worked ok.
				if( (*task)->GetOk() == false )
				{
					CompileOk = false;
					// If the compile failed, clean up the tasks that are waiting to start and then just wait for the ones in progress to finish.
					while( !BuildTasks.empty() )
					{
						delete BuildTasks.top();
						BuildTasks.pop();
					}
				}

				delete (*task);
				task = RunningTasks.erase(task);// This removes the one just deleted and advances our linked list pointer.

			}
			else
			{// Go to the next running task.
				++task;
			}
		};
	};

	std::cout << "Build finished" << std::endl;
	return CompileOk;
}

bool Project::LinkTarget(const Configuration* pConfig,const StringVec& pOutputFiles)
{
	assert(pConfig);
	if( !pConfig )
		return false;

	ArgList LinkerArguments(pConfig->GetLibrarySearchPaths());

	// Add the object files.
	LinkerArguments.AddArg(pOutputFiles);

	// Add the libs, must come after the object files.
	LinkerArguments.AddArg(pConfig->GetLibraryFiles());

	// And add the output.
	LinkerArguments.AddArg("-o");
	LinkerArguments.AddArg(pConfig->GetPathedTargetName());

	std::cout << "Linking: " << pConfig->GetPathedTargetName() << std::endl;

/*	const StringVec& check = LinkerArguments;
	for(auto arg : check )
		std::cout << "Args: \'" << arg <<  "\'" << std::endl;*/

	std::string LinkerResults;
	bool ok = ExecuteShellCommand(pConfig->GetLinker(),LinkerArguments,LinkerResults);
	if( LinkerResults.size() < 1 )
		LinkerResults = "Linked ok";
	std::cout << LinkerResults << std::endl;
	return ok;
}



//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
