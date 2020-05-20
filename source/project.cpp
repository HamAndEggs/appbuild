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
#include "misc.h"
#include "shell.h"
#include "arg_list.h"
#include "build_task_resource_files.h"
#include "logging.h"

namespace appbuild{

StringSet Project::sLoadedProjects;

//////////////////////////////////////////////////////////////////////////
Project::Project(const std::string& pFilename,const SourceFiles& pSourceFiles,ConfigurationsVec& pConfigurations,int pLoggingMode):
	mNumThreads(1),
	mLoggingMode(pLoggingMode),
	mRebuild(false),
	mTruncateOutput(0),
	mPathedProjectFilename(pFilename),
	mProjectDir(GetPath(pFilename)),
	mDependencies(pFilename),
	mSourceFiles(pSourceFiles),
	mResourceFiles(GetPath(pFilename),pLoggingMode),
	mOk(false)
{
	for( auto& c : pConfigurations )
	{
		mBuildConfigurations[c->GetName()] = c;
	}
	
	if( pLoggingMode  >= LOG_VERBOSE )
	{
		std::cout << "Project file name and path is " << pFilename << std::endl;
		std::cout << "Project path is " << mProjectDir << std::endl;
	}

	mOk = mSourceFiles.size() > 0;
}

Project::Project(const std::string& pFilename,size_t pNumThreads,int pLoggingMode,bool pRebuild,size_t pTruncateOutput):
		mNumThreads(pNumThreads>0?pNumThreads:1),
		mLoggingMode(pLoggingMode),
		mRebuild(pRebuild),
		mTruncateOutput(pTruncateOutput),
		mPathedProjectFilename(pFilename),
		mProjectDir(GetPath(pFilename)),
		mDependencies(pFilename),
		mSourceFiles(GetPath(pFilename),pLoggingMode),
		mResourceFiles(GetPath(pFilename),pLoggingMode),
		mOk(false)
{
	assert( pNumThreads > 0 );

	if( pLoggingMode >= LOG_VERBOSE )
		std::cout << "Reading project file " << mPathedProjectFilename << std::endl;

	// Now build our project object.
	rapidjson::Document ProjectJson;

	if( ReadJson(mPathedProjectFilename,ProjectJson) )
    {
		// Is the project file already loaded, if so then there is a dependency loop, we have to fail else will compile for ever.
		if( sLoadedProjects.find(mPathedProjectFilename) != sLoadedProjects.end() )
		{
			std::cout << "Dependency loop detected, the project file \'" << mPathedProjectFilename << "\' has already been referenced as a dependency." << std::endl;
			return;
		}

		// Record this file that we are loaded so we know it is loaded.
		sLoadedProjects.insert(mPathedProjectFilename);

		// Read the configs.
		if( ProjectJson.HasMember("configurations") )
		{
			if( !ReadConfigurations(ProjectJson["configurations"]) )
				return;
		}

		if( mBuildConfigurations.size() == 0 )
		{
			if( mLoggingMode >= LOG_VERBOSE )
				std::cout << "No configurations found in the project file \'" << mPathedProjectFilename << "\' using default exec configuration." << std::endl;

			ConfigurationPtr config = std::make_shared<Configuration>("release",GetFileName(mPathedProjectFilename,true),mProjectDir,mLoggingMode,true,"2","0");
			mBuildConfigurations[config->GetName()] = config;
		}

		assert( mBuildConfigurations.size() > 0 );

		// Add the source files
		if( ProjectJson.HasMember("source_files") )
		{
			mSourceFiles.Read(ProjectJson["source_files"]);
		}
		else
		{
			std::cout << "No source files, what the should do I compile? Project file \'" << mPathedProjectFilename << std::endl;
			return;
		}

		// Add the source files
		if( ProjectJson.HasMember("resource_files") )
		{
			mResourceFiles.Read(ProjectJson["resource_files"]);
		}
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

bool Project::Build(const std::string& pConfigName)
{
	ConfigurationPtr activeConfig = GetConfiguration(pConfigName);

	if(activeConfig == nullptr)
	{
		return false;
	}

	// See if there are any dependant projects that need to be built first.
	const StringMap& DependantProjects = activeConfig->GetDependantProjects();
	for( auto& proj : DependantProjects )
	{
		const std::string ProjectFile = proj.first;
		std::cout << "Checking dependency \'" << ProjectFile << "\'" << std::endl;

		if( !FileExists(ProjectFile) )
		{
			std::cout << "Error: Project file " << ProjectFile << " is not found." << std::endl << "Current working dir " << GetCurrentWorkingDirectory() << std::endl;
			return false;
		}

		Project TheProject(ProjectFile,mNumThreads,mLoggingMode,mRebuild,mTruncateOutput);
		if( TheProject )
		{
			std::string configname = proj.second;
			if( configname.empty() )
				configname = activeConfig->GetName();

			if( TheProject.Build(configname) == false )
			{
				return false;
			}

			// That worked, lets add it's output file to our input libs.
			ConfigurationPtr DepConfig = TheProject.GetConfiguration(configname);
			assert( DepConfig != nullptr );// Should not happen, it was built ok...		
			if( DepConfig->GetTargetType() == TARGET_LIBRARY )
			{
				const std::string RelativeOutputpath = GetPath(DepConfig->GetPathedTargetName());
				mDependencyLibraryFiles.push_back(DepConfig->GetOutputName());
				mDependencyLibrarySearchPaths.push_back(RelativeOutputpath);

				if( mLoggingMode >= LOG_VERBOSE )
					std::cout << "Adding dependency for lib \'" << DepConfig->GetOutputName() << "\' located at \'" << RelativeOutputpath << "\'" << std::endl;
			}
		}
		else
		{
			std::cout << "There was an error in the project file \'" << ProjectFile << "\'" << std::endl;
			return false;
		}		
	}

	// We know what config to build with, lets go.
    if( mLoggingMode >= LOG_INFO )
    {
    	std::cout << "Compiling configuration \'" << activeConfig->GetName() << "\'" << std::endl;
    }

	BuildTaskStack BuildTasks;

	// See if we need to build the resoure files first.
	SourceFiles GeneratedResourceFiles(mProjectDir,mLoggingMode);
	if( mResourceFiles.size() > 0 )
	{
		// We run this build task now as it's a prebuild step and will need to make new tasks of it's own.
		BuildTaskResourceFiles* ResourceTask = new BuildTaskResourceFiles("Resource Files",mResourceFiles,activeConfig->GetOutputPath(),mLoggingMode);

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
	if( activeConfig->GetBuildTasks(mSourceFiles,GeneratedResourceFiles,mRebuild,BuildTasks,mDependencies,OutputFiles) )
	{
		if( BuildTasks.size() > 0 )
		{
			if( !CompileSource(activeConfig,BuildTasks) )
			{
				return false;
			}
		}

		switch(activeConfig->GetTargetType())
		{
		case TARGET_EXEC:
			// Link. May just do a link if none of the source files needed to be build.
			return LinkTarget(activeConfig,OutputFiles);

		case TARGET_LIBRARY:
			return ArchiveLibrary(activeConfig,OutputFiles);

		case TARGET_SHARED_LIBRARY:
			return LinkSharedLibrary(activeConfig,OutputFiles);

		case TARGET_NOT_SET:
			std::cerr << "Target type not set, unable to compile configuration \'" << activeConfig->GetName() << "\' in project \'" << mPathedProjectFilename << "\'" << std::endl;
			break;
		}
	}
	else
	{
		std::cerr << "Unable to create build tasks for the configuration \'" << activeConfig->GetName() << "\' in project \'" << mPathedProjectFilename << "\'" << std::endl;
	}

	return false;
}

bool Project::RunOutputFile(const std::string& pConfigName)
{
	ConfigurationPtr activeConfig = GetConfiguration(pConfigName);
	if(activeConfig == nullptr)
	{
		return false;
	}

	std::string PathedTargetName;
	if( activeConfig->GetTargetType() == TARGET_EXEC )
	{
		char command[256];
		command[0] = 0;

		strcat(command,activeConfig->GetPathedTargetName().c_str());

		char* TheArgs[]={command,NULL};
		return execvp(TheArgs[0],TheArgs) == 0;
	}

	return false;
}

void Project::Write(rapidjson::Document& pDocument)const
{
	rapidjson::Document::AllocatorType& alloc = pDocument.GetAllocator();

	rapidjson::Value configurations = rapidjson::Value(rapidjson::kObjectType);

	for( const auto& conf : mBuildConfigurations )
	{
		AddMember(configurations,conf.first,conf.second->Write(alloc),alloc);
	}
	pDocument.AddMember("configurations",configurations,alloc);
	pDocument.AddMember("source_files",mSourceFiles.Write(alloc),alloc);

	if( mResourceFiles.size() > 0 )
		pDocument.AddMember("resource_files",mResourceFiles.Write(alloc),alloc);

}

std::string Project::FindDefaultConfigurationName()const
{
	for( auto conf : mBuildConfigurations )
		if( conf.second->GetIsDefaultConfig() )
			return conf.first;

	if( mBuildConfigurations.size() == 1 )
		return mBuildConfigurations.begin()->first;

	std::cerr << "No configuration was specified to build, your choices are:-" << std::endl;
	for(auto conf : mBuildConfigurations )
	{
		std::cout << conf.first << std::endl;
	}
	std::cerr << "Use -c [config name] to specify which to build. Consider using \"default\":true in the configuration that you build the most to make life easier." << std::endl;

	return "";
}

ConfigurationPtr Project::GetConfiguration(const std::string& pName)const
{
	if( pName.size() > 0 )
	{
		auto FoundConfig = mBuildConfigurations.find(pName);
		if( FoundConfig != mBuildConfigurations.end() )
		{
			return FoundConfig->second;
		}
		std::cerr << "The configuration \'" << pName << "\' to build was not found in the project file \'" << mPathedProjectFilename << "\'" << std::endl;
	}

	return nullptr;
}

bool Project::ReadConfigurations(const rapidjson::Value& pConfigs)
{
	assert(pConfigs.IsObject());

	// Use the name of the project, without it's extension, as the default out bin name.
	// This is done as this tool supports minimal project files that just list source files.
	const std::string defaultOutputName = GetFileName(mPathedProjectFilename,true);

	if( pConfigs.IsObject() )
	{
		for(auto& configs : pConfigs.GetObject() )
		{
			const std::string name = configs.name.GetString();
			const rapidjson::Value& val = configs.value;

			if( mBuildConfigurations.find(name) == mBuildConfigurations.end() )
			{
				mBuildConfigurations[name] = std::make_shared<Configuration>(name,val,defaultOutputName,mProjectDir,mLoggingMode);
				if( mBuildConfigurations[name]->GetOk() == false )
				{
					std::cerr << "Configuration \'"<< name << "\' failed to load, error in project " << mPathedProjectFilename << std::endl;
					return false;
				}
			}
			else
			{
				std::cerr << "Configuration \'"<< name << "\' not unique, names must be unique, error in project " << mPathedProjectFilename << std::endl;
				return false;
			}
		}
	}
	else if( pConfigs.IsArray() )
	{
		std::cerr << "Configuration is not an object, it is an array, do not use an array, error in project " << mPathedProjectFilename << std::endl;
		return false;
	}
	else
	{
		std::cerr << "Configuration is not an object, error in project " << mPathedProjectFilename << std::endl;
		return false;
	}

	return true;
}

bool Project::CompileSource(ConfigurationPtr pConfig,BuildTaskStack& pBuildTasks)
{
	assert(pConfig);
	if( !pConfig )
		return false;

	assert( pBuildTasks.size() > 0 );

	// I always delete the target if something needs to be build so there is no exec to run if the source has failed to build.
	remove(pConfig->GetPathedTargetName().c_str());

    if( mLoggingMode >= LOG_INFO )
    {
    	std::cout << "Building: " << pBuildTasks.size() << " file" << (pBuildTasks.size()>1?"s.":".");
    }

	const size_t ThreadCount = std::max((size_t)1,std::min(mNumThreads,pBuildTasks.size()));

    if( mLoggingMode >= LOG_INFO )
    {
        if( ThreadCount > 1 )
            std::cout << " Num threads " << ThreadCount;

        std::cout << " Output path " << pConfig->GetOutputPath();
        std::cout << std::endl;
    }

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
					std::cerr << std::endl << "Unexpected output from task: " << (*task)->GetTaskName() << std::endl;
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

    if( mLoggingMode >= LOG_INFO )
	    std::cout << "Build finished" << std::endl;

	return CompileOk;
}

bool Project::LinkTarget(ConfigurationPtr pConfig,const StringVec& pOutputFiles)
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

    if( mLoggingMode >= LOG_INFO )
	    std::cout << "Linking: " << pConfig->GetPathedTargetName() << std::endl;

	if( mLoggingMode >= LOG_VERBOSE )
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
    {
        if( mLoggingMode >= LOG_INFO )
            std::cout << "Linked ok" << std::endl;
    }
    else
    {
        std::cerr << Results << std::endl;
    }
    

	return ok;
}

bool Project::ArchiveLibrary(ConfigurationPtr pConfig,const StringVec& pOutputFiles)
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

    if( mLoggingMode >= LOG_INFO )
	    std::cout << "Archiving: " << pConfig->GetPathedTargetName() << std::endl;
	
	if( mLoggingMode >= LOG_VERBOSE )
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
    {
        if( mLoggingMode >= LOG_INFO )
            std::cout << "Linked ok" << std::endl;
    }
    else
    {
        std::cerr << Results << std::endl;
    }
    
	return ok;
}

bool Project::LinkSharedLibrary(ConfigurationPtr pConfig,const StringVec& pOutputFiles)
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
