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

#ifndef _PROJECT_H_
#define _PROJECT_H_

#include <vector>

#include "json.h"

#include "build_task.h"
#include "dependencies.h"
#include "configuration.h"
#include "source_files.h"


//////////////////////////////////////////////////////////////////////////
// Holds all the information about the project file.
// If any filename variable is fully pathed then the name with start with Pathed.
// If not then it maybe just the filename or part pathed.
// All pathed filenames are from root file system. '/'
//////////////////////////////////////////////////////////////////////////

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class Project
{
public:
/****** TODO and NOTE!!! I need to change code from passing in pFilename to a project root path. Does need to know the file it came from. But should know it's working directory ******/
 
	/**
	 * @brief Construct a new basic project object that can build the passed in source file.
	 *  This constructure is mainly used for the shebang code and auto generation of projects when creating a new applications / libraries.
	 * @param pFilename The filename of the project file that was loaded.
	 * @param pSourceFiles The source files to add to the new project.
	 * @param pConfigurations The configurations to add to this project.
	 * @param pLoggingMode Sets the logging mode for when passing the json file.
	 */
	Project(const std::string& pFilename,const SourceFiles& pSourceFiles,ConfigurationsVec& pConfigurations,int pLoggingMode);

	/**
	 * @brief Construct a new Project object from the filename passed in, the file has to be JSON formatted and contain the correct tokens.
	 * 
	 * @param pFilename The filename of the project file that was loaded.
	 * @param pNumThreads The number of threads to build the project with.
	 * @param pLoggingMode Sets the logging mode for when passing the json file.
	 * @param pRebuild If true then the build process will be a full rebuild.
	 * @param pTruncateOutput Sometimes the errors from the compiler can be too long, this will cause these errors to be truncated.
	 */
	Project(const std::string& pFilename,size_t pNumThreads,int pLoggingMode,bool pRebuild,size_t pTruncateOutput);

	/**
	 * @brief Destroy the Project object
	 * 
	 */
	~Project();

	operator bool ()const{return mOk;}

	bool Build(const std::string& pConfigName);
	bool RunOutputFile(const std::string& pConfigName);
	void Write(rapidjson::Document& pDocument)const;

	/**
	 * @brief Tries to find a configuration that can be used if the user did not specify one.
	 * 
	 * @return std::string The name of a configuration to use or null if no default could be determined.
	 */
	std::string FindDefaultConfigurationName()const;
	const std::string& GetPathedProjectFilename()const{return mPathedProjectFilename;}

private:

	ConfigurationPtr GetConfiguration(const std::string& pName)const;

	bool ReadConfigurations(const rapidjson::Value& pConfigs);

	bool CompileSource(ConfigurationPtr pConfig,BuildTaskStack& pBuildTasks);
	bool LinkTarget(ConfigurationPtr pConfig,const StringVec& pOutputFiles);
	bool ArchiveLibrary(ConfigurationPtr pConfig,const StringVec& pOutputFiles);
	bool LinkSharedLibrary(ConfigurationPtr pConfig,const StringVec& pOutputFiles);


	// Some options that are passed into the constructor.
	const size_t mNumThreads;
	const int mLoggingMode;
	const bool mRebuild;
	const size_t mTruncateOutput;
	
	// This project file, fully pathed.
	const std::string mPathedProjectFilename;
	const std::string mProjectDir;

	Dependencies mDependencies;
	SourceFiles mSourceFiles;
	SourceFiles mResourceFiles;
	BuildConfigurations mBuildConfigurations;

	StringVec mDependencyLibrarySearchPaths;
	StringVec mDependencyLibraryFiles;


	bool mOk;	// Project loaded ok.

	/**
	 * @brief This is used to prevent recursive project references.
	 * For example ProjectA is dependant on ProjectB which is dependant on ProjectC which is dependant on ProjectA
	 * This is a rare case where a static is useful. I have made sure it still remains behind the closed doors for the class.
	 * Not a fan of statics, but this is a good use case.
	 */
	static StringSet sLoadedProjects;

};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
