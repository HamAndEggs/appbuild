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
#include "json_writer.h"
#include "build_task_resource_files.h"

namespace appbuild{
StringSet Project::sLoadedProjects;

//////////////////////////////////////////////////////////////////////////
Project::Project(const std::string& pFilename,size_t pNumThreads,bool pVerboseOutput,bool pRebuild,size_t pTruncateOutput):
		mNumThreads(pNumThreads>0?pNumThreads:1),
		mVerboseOutput(pVerboseOutput),
		mRebuild(pRebuild),
		mTruncateOutput(pTruncateOutput),
		mPathedProjectFilename(pFilename),
		mProjectDir(GetPath(pFilename)),
		mDependencies(pFilename),
		mSourceFiles(GetPath(pFilename)),
		mOk(false)
{
	assert( pNumThreads > 0 );

	if( pVerboseOutput )
		std::cout << "Reading project file " << mPathedProjectFilename << std::endl;

	// Now build our project object.
	std::ifstream ProjectFile(mPathedProjectFilename);
	Json ProjectJson(ProjectFile);
    if( ProjectJson )
    {
		// Is the project file already loaded, if so then there is a dependancy loop, we have to fail else will compile for ever.
		if( sLoadedProjects.find(mPathedProjectFilename) != sLoadedProjects.end() )
		{
			std::cout << "Dependancy loop detected, the project file \'" << mPathedProjectFilename << "\' has already been referenced as a dependancy." << std::endl;
			return;
		}

		// Record this file that we are loaded so we know it is loaded.
		sLoadedProjects.insert(mPathedProjectFilename);

		// Read the configs.
		const JSONValue* configs = ProjectJson.Find("configurations");
		if( configs )
		{
			if( !ReadConfigurations(configs) )
				return;
		}

		if( mBuildConfigurations.size() == 0 )
		{
			if( mVerboseOutput )
				std::cout << "No configurations found in the project file \'" << mPathedProjectFilename << "\' using default exec configuration." << std::endl;

			const Configuration* config = new Configuration(mProjectDir,GetFileName(mPathedProjectFilename,true),mVerboseOutput);
			mBuildConfigurations[config->GetName()] = config;
		}

		assert( mBuildConfigurations.size() > 0 );

		// Add the source files
		mSourceFiles.Read(ProjectJson.Find("source_files"));

		// Add the source files
		mResourceFiles.Read(ProjectJson.Find("resource_files"),mPathedProjectFilename);
    }
	else
	{
		std::cout << "The project file \'" << mPathedProjectFilename << "\' could not be loaded" << std::endl;
		return;
	}

	// Get here all is ok.
	mOk = true;
}

Project::~Project()
{
	// Done with this, can forget it now.
	sLoadedProjects.erase(mPathedProjectFilename);
}

bool Project::Build(const Configuration* pActiveConfig)
{
	assert(pActiveConfig);
	if(!pActiveConfig)return false;

	// See if there are any dependant projects that need to be built first.
	const StringMap& DependantProjects = pActiveConfig->GetDependantProjects();
	for( auto& proj : DependantProjects )
	{
		const std::string ProjectFile = proj.first;
		std::cout << "Checking dependency \'" << ProjectFile << "\'" << std::endl;

		if( !FileExists(ProjectFile) )
		{
			std::cout << "Error: Project file " << ProjectFile << " is not found." << std::endl << "Current working dir " << GetCurrentWorkingDirectory() << std::endl;
			return false;
		}

		Project TheProject(ProjectFile,mNumThreads,mVerboseOutput,mRebuild,mTruncateOutput);
		if( TheProject )
		{
			std::string configname = proj.second;
			if( configname.empty() )
				configname = pActiveConfig->GetName();

			const appbuild::Configuration* DepConfig = TheProject.GetActiveConfiguration(configname);
			if( !DepConfig || TheProject.Build(DepConfig) == false )
			{
				return false;
			}

			// That worked, lets add it's output file to our input libs.
			if( DepConfig->GetTargetType() == TARGET_LIBRARY )
			{
				const std::string RelitiveOutputpath = GetPath(DepConfig->GetPathedTargetName());
				mDependencyLibraryFiles.push_back(DepConfig->GetOutputName());
				mDependencyLibrarySearchPaths.push_back(RelitiveOutputpath);

				if( mVerboseOutput )
					std::cout << "Adding dependancy for lib \'" << DepConfig->GetOutputName() << "\' located at \'" << RelitiveOutputpath << "\'" << std::endl;
			}
		}
		else
		{
			std::cout << "There was an error in the project file \'" << ProjectFile << "\'" << std::endl;
			return false;
		}		
	}

	// We know what config to build with, lets go.
	std::cout << "Compiling configuration \'" << pActiveConfig->GetName() << "\'" << std::endl;

	BuildTaskStack BuildTasks;

	// See if we need to build the resoure files first.
	SourceFiles GeneratedResourceFiles(mProjectDir);
	if( mResourceFiles.GetNeedsRebuild() )
	{
		// We runthis build task now as it's a prebuild step and will need to make new tasks of it's own.
		BuildTaskResourceFiles* ResourceTask = new BuildTaskResourceFiles("Resource Files",mResourceFiles,pActiveConfig->GetOutputPath(),mVerboseOutput);

		ResourceTask->Execute();
		while( ResourceTask->GetIsCompleted() == false )
		{
			std::this_thread::yield();
		}

		if( ResourceTask->GetOk() )
		{
			ResourceTask->GetGeneratedResourceFiles(GeneratedResourceFiles);
		}

	}

	StringVec OutputFiles;
	if( pActiveConfig->GetBuildTasks(mSourceFiles,GeneratedResourceFiles,mRebuild,BuildTasks,mDependencies,OutputFiles) )
	{
		if( BuildTasks.size() > 0 )
		{
			if( !CompileSource(pActiveConfig,BuildTasks) )
			{
				return false;
			}
		}

		switch(pActiveConfig->GetTargetType())
		{
		case TARGET_EXEC:
			// Link. May just do a link if none of the source files needed to be build.
			return LinkTarget(pActiveConfig,OutputFiles);

		case TARGET_LIBRARY:
			return ArchiveLibrary(pActiveConfig,OutputFiles);

		case TARGET_SHARED_LIBRARY:
			return LinkSharedLibrary(pActiveConfig,OutputFiles);

		case TARGET_NOT_SET:
			std::cout << "Target type not set, unable to compile configuration \'" << pActiveConfig->GetName() << "\' in project \'" << mPathedProjectFilename << "\'" << std::endl;
			break;
		}
	}
	else
	{
		std::cout << "Unable to create build tasks for the configuration \'" << pActiveConfig->GetName() << "\' in project \'" << mPathedProjectFilename << "\'" << std::endl;
	}

	return false;
}

bool Project::RunOutputFile(const Configuration* pActiveConfig)
{
	assert(pActiveConfig);
	if(!pActiveConfig)return false;

	std::string PathedTargetName;
	if( pActiveConfig->GetTargetType() == TARGET_EXEC )
	{
		char command[256];
		command[0] = 0;

		strcat(command,pActiveConfig->GetPathedTargetName().c_str());

		char* TheArgs[]={command,NULL};
		return execvp(TheArgs[0],TheArgs) == 0;
	}

	return false;
}

bool Project::Write(JsonWriter& rJsonOutput)const
{
	rJsonOutput.StartObject();
	rJsonOutput.StartObject("configurations");
	for( const auto& conf : mBuildConfigurations )
	{
		conf.second->Write(rJsonOutput);
	}
	rJsonOutput.EndObject();
	mSourceFiles.Write(rJsonOutput);
	rJsonOutput.EndObject();

	return true;
}

const Configuration* Project::GetActiveConfiguration(const std::string& pConfigName)const
{
		// See if there is a default config, but only if the name was not specified.
	if( pConfigName.size() == 0 )
		for( auto conf : mBuildConfigurations )
			if( conf.second->GetIsDefaultConfig() )
				return conf.second;
	
	auto FoundConfig = mBuildConfigurations.find(pConfigName);
	if( FoundConfig != mBuildConfigurations.end() )
	{
		return FoundConfig->second;
	}
	else if( mBuildConfigurations.size() == 1 )// If there is only one config, then just used that.
		return mBuildConfigurations.begin()->second;

	if( pConfigName.size() > 0 )
		std::cout << "The configuration \'" << pConfigName << "\' to build was not found in the project file \'" << mPathedProjectFilename << "\'" << std::endl;
	else if( mBuildConfigurations.size() < 1 )
	{//Should not get here, but just in case, show an error.
		std::cout << "No configurations found in the project file \'" << mPathedProjectFilename << "\' can not continue." << std::endl;
	}
	else
	{
		std::cout << "No configuration was specified to build, your choices are:-" << std::endl;
		for(auto conf : mBuildConfigurations )
		{
			std::cout << conf.first << std::endl;
		}
		std::cout << "Use -c [config name] to specify which to build. Consider using \"default\":true in the configuration that you build the most to make life easier." << std::endl;
	}

	return NULL;
}


bool Project::ReadConfigurations(const JSONValue* pSettings)
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
				return false;
			}
			else if( configs.second.size() < 1 )
			{
				std::cout << "Configuration \'"<< name << "\' has no body to the object, check syntax. Error in project " << mPathedProjectFilename << std::endl;
				return false;
			}
			else
			{
				const JSONValue* Obj = configs.second[0];
				if(Obj)
				{
					mBuildConfigurations[name] = new Configuration(name,Obj,mPathedProjectFilename,mProjectDir,mVerboseOutput);
				}
			}
		}
		else
		{
			std::cout << "Malformed configuration in project without a name in project" << mPathedProjectFilename << std::endl;
			return false;
		}
	}
	
	return true;
}



bool Project::CompileSource(const Configuration* pConfig,BuildTaskStack& pBuildTasks)
{
	assert(pConfig);
	if( !pConfig )
		return false;

	assert( pBuildTasks.size() > 0 );

	// I always delete the target if something needs to be build so there is no exec to run if the source has failed to build.
	remove(pConfig->GetPathedTargetName().c_str());

	std::cout << "Building: " << pBuildTasks.size() << " file" << (pBuildTasks.size()>1?"s.":".");
	const size_t ThreadCount = std::max((size_t)1,std::min(mNumThreads,pBuildTasks.size()));
	if( ThreadCount > 1 )
		std::cout << " Num threads " << ThreadCount;

	std::cout << " Output path " << pConfig->GetOutputPath();
	std::cout << std::endl;
	bool CompileOk = true;


	RunningBuildTasks RunningTasks;
	while( pBuildTasks.size() > 0 || RunningTasks.size() > 0 )
	{
		// Make sure at least N tasks are running.
		if( pBuildTasks.size() > 0 && RunningTasks.size() < ThreadCount )
		{
			BuildTask* task = pBuildTasks.top();
			pBuildTasks.pop();
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
				const std::string& res = (*task)->GetResults();
				if( res.size() > 1 )
				{
					std::cout << std::endl << "Unexpected output from task: " << (*task)->GetTaskName() << std::endl;
					if( mTruncateOutput > 0 )
					{
						StringVec chopped = SplitString(res,"\n");
						for( size_t n = 0 ; n < chopped.size() && n < mTruncateOutput ; n++ )
							std::cout << chopped[n] << std::endl;
					}
					else
					{
						std::cout << res << std::endl;
					}
					// Leave a gap.
					std::cout << std::endl;
				}

				// See if it worked ok.
				if( (*task)->GetOk() == false )
				{
					CompileOk = false;
					// If the compile failed, clean up the tasks that are waiting to start and then just wait for the ones in progress to finish.
					while( !pBuildTasks.empty() )
					{
						delete pBuildTasks.top();
						pBuildTasks.pop();
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

	ArgList Arguments;
	Arguments.AddLibrarySearchPaths(pConfig->GetLibrarySearchPaths());
	Arguments.AddLibrarySearchPaths(mDependencyLibrarySearchPaths);

	// Add the object files.
	Arguments.AddArg(pOutputFiles);

	// Add the libs, must come after the object files.
	Arguments.AddLibraryFiles(mDependencyLibraryFiles);
	Arguments.AddLibraryFiles(pConfig->GetLibraryFiles());

	// And add the output.
	Arguments.AddArg("-o");
	Arguments.AddArg(pConfig->GetPathedTargetName());

	std::cout << "Linking: " << pConfig->GetPathedTargetName() << std::endl;

	if( mVerboseOutput )
	{
		const StringVec& args = Arguments;
		std::cout << pConfig->GetLinker() << " ";
		for( const auto& arg : args )
			std::cout << arg << " ";

		std::cout << std::endl;
	}

	std::string Results;
	bool ok = ExecuteShellCommand(pConfig->GetLinker(),Arguments,Results);
	if( Results.size() < 1 )
		Results = "Linked ok";
	std::cout << Results << std::endl;
	return ok;
}


bool Project::ArchiveLibrary(const Configuration* pConfig,const StringVec& pOutputFiles)
{
	ArgList Arguments;

	// Add standard params. Maybe I should put this in the project file and have a default.
	// As I always remove the target before the link stage I 'think' the r opt is not needed.
	// r[ab][f][u]  - replace existing or insert new file(s) into the archive
	// [c]          - do not warn if the library had to be created
	// s            - act as ranlib
	Arguments.AddArg("rcs");

	// And add the output.
	Arguments.AddArg(pConfig->GetPathedTargetName());

	// Add the object files.
	Arguments.AddArg(pOutputFiles);

	std::cout << "Archiving: " << pConfig->GetPathedTargetName() << std::endl;
	
	if( mVerboseOutput )
	{
		const StringVec& args = Arguments;
		std::cout << pConfig->GetArchiver() << " ";
		for( const auto& arg : args )
			std::cout << arg << " ";

		std::cout << std::endl;
	}

	std::string Results;
	bool ok = ExecuteShellCommand(pConfig->GetArchiver(),Arguments,Results);
	if( Results.size() < 1 )
		Results = "Linked ok";
	std::cout << Results << std::endl;
	return ok;
}

bool Project::LinkSharedLibrary(const Configuration* pConfig,const StringVec& pOutputFiles)
{/*
	ArgList Arguments("-shared");

	// And add the output.
	Arguments.AddArg(pConfig->GetPathedTargetName());

	// Add the object files.
	Arguments.AddArg(pOutputFiles);

	std::string Results;
	bool ok = ExecuteShellCommand(pConfig->GetArchiver(),Arguments,Results);
	if( Results.size() < 1 )
		Results = "Linked ok";
	std::cout << Results << std::endl;
	return ok;*/
	return false;
}


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
