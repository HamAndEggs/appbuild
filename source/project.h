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
#include "search_paths.h"

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
	/**
	 * @brief Construct a new Project object from the filename passed in, the file has to be JSON formatted and contain the correct tokens.
	 * 
	 * @param pProjectJson The root json for the project definition.
	 * @param pProjectName The name of the project that will uniquely identify it within a group of loaded projects.
	 * @param pProjectPath The root path that all paths in the project are relative too.
	 * @param pNumThreads The number of threads to build the project with.
	 * @param pLoggingMode Sets the logging mode for when passing the json file.
	 * @param pRebuild If true then the build process will be a full rebuild.
	 * @param pTruncateOutput Sometimes the errors from the compiler can be too long, this will cause these errors to be truncated.
	 */
	Project(const tinyjson::JsonValue& pProjectJson,const std::string& pProjectName,const std::string& pProjectPath,size_t pNumThreads,int pLoggingMode,bool pRebuild,size_t pTruncateOutput);

	/**
	 * @brief Destroy the Project object
	 * 
	 */
	~Project();

	operator bool ()const{return mOk;}

	bool Build(const std::string& pConfigName);
	bool RunOutputFile(const std::string& pConfigName)const;
	void Write(tinyjson::JsonValue& pDocument)const;

	/**
	 * @brief Tries to find a configuration that can be used if the user did not specify one.
	 * 
	 * @return std::string The name of a configuration to use or null if no default could be determined.
	 */
	std::string FindDefaultConfigurationName()const;

	/**
	 * @brief Returns a vector of all the configuration names in the project.
	 * 
	 * @return const StringVec 
	 */
	const StringVec GetConfigurationNames()const;
	
	/**
	 * @brief Ghe name of the project that will uniquely identify it within a group of loaded projects.
	 * 
	 * @return const std::string& 
	 */
	const std::string& GetProjectName()const{return mProjectName;}

	/**
	 * @brief Get the root path that all paths in the project are relative too.
	 * 
	 * @return const std::string& 
	 */
	const std::string& GetProjectDir()const{return mProjectDir;}

	/**
	 * @brief Adds a generic file to the list of file dates to be tested against.
	 * This is used for the project file, if there is one, and maybe some embedded resource files.
	 * 
	 * @param pPathedFileName Full path to the file, the file is NOT parsed in anyway. If it's date is younger than the object file being tested a rebuild will be triggered.
	 */
	void AddGenericFileDependency(const std::string& pPathedFileName);

private:

	ConfigurationPtr GetConfiguration(const std::string& pName)const;

	bool ReadConfigurations(const tinyjson::JsonValue& pConfigs);

	bool CompileSource(ConfigurationPtr pConfig,BuildTaskStack& pBuildTasks);
	bool LinkTarget(ConfigurationPtr pConfig,const StringVec& pOutputFiles);
	bool ArchiveLibrary(ConfigurationPtr pConfig,const StringVec& pOutputFiles);
	bool LinkSharedObject(ConfigurationPtr pConfig,const StringVec& pOutputFiles);

	/**
	 * @brief Returns a 32bit value that represents the version string passed in.
	 * @param pString The version string, must be in the format of NUMBER.NUMBER.NUMBER where the first two numbers are 0 -> 255 and the last 0 -> 65384
	 * @return uint32_t The version number.
	 */
	uint32_t ParseVersion(const std::string& pString);

	// Some options that are passed into the constructor.
	const size_t mNumThreads;
	const int mLoggingMode;
	const bool mRebuild;
	const size_t mTruncateOutput;
	
	// This project file, fully pathed.
	const std::string mProjectName; //!< The name of the project that will uniquely identify it within a group of loaded projects.
	const std::string mProjectDir; //!< The root path that all paths in the project are relative too.
	
	Dependencies mDependencies;
	SourceFiles mSourceFiles;
	SourceFiles mResourceFiles;
	BuildConfigurations mBuildConfigurations;

	StringVec mDependencyLibrarySearchPaths;
	StringVec mDependencyLibraryFiles;
	std::string mSharedObjectPaths;		//!< Used to set the search paths to the putput of any shared object files that dependant projects create.

	SearchPaths mIncludeSearchPaths; //!< Global include search paths for the project, used in all configurations.
	SearchPaths mLibrarySearchPaths; //!< Global lib search paths for the project, used in all configurations.
	SearchPaths mLibraryFiles;

	/**
	 * @brief This is the version of the project, this is required for generation of the correct .so file contents. 
	 * See https://docs.oracle.com/cd/E19683-01/817-3677/chapter4-2-rason/index.html
	 * This is calculated at load time so that we can emit the correct errors if the version string is invalid.
	 * We also add a define into the compile stream so that all files can test it.
	 * It is added as a string, APP_VERSION.
	 * Not allowed to be zero, zero is treated as an error.
	 */
	int mProjectVersion;

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
