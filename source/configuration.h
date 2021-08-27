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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <map>
#include <string>

#include "string_types.h"
#include "json.h"
#include "arg_list.h"
#include "source_files.h"


//////////////////////////////////////////////////////////////////////////
// Holds the information for each build configuration.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{

enum eTargetType
{
	TARGET_NOT_SET,
	TARGET_EXEC,
	TARGET_LIBRARY,
	TARGET_SHARED_OBJECT,
};

class Dependencies;
class BuildTaskStack;
class JsonWriter;
class Project;
class SourceFiles;

class Configuration
{
public:
	Configuration(const std::string& pConfigName,const Project* pParentProject,int pLoggingMode,const rapidjson::Value& pConfig);
	~Configuration();

	rapidjson::Value Write(rapidjson::Document::AllocatorType& pAllocator)const;

	bool GetIsDefaultConfig()const{return mIsDefaultConfig;}
	bool GetOk()const{return mOk;}
	eTargetType GetTargetType()const{return mTargetType;}

	const std::string GetPathedTargetName()const{return mOutputPath + mOutputName;}
	const std::string& GetName()const{return mConfigName;}
	const std::string& GetLinker()const{return mLinker;}
	const std::string& GetArchiver()const{return mArchiver;}
	const std::string& GetOutputPath()const{return mOutputPath;}
	const std::string& GetOutputName()const{return mOutputName;}
	const StringVec GetLibraryFiles()const;
	const StringVec& GetLibrarySearchPaths()const{return mLibrarySearchPaths;}
	const StringMap& GetDependantProjects()const{return mDependantProjects;}

	bool GetBuildTasks(const SourceFiles& pProjectSourceFiles,const SourceFiles& pGeneratedResourceFiles,bool pRebuildAll,const ArgList& pAdditionalArgs,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const;

	void AddDefine(const std::string& pDefine);
	void AddLibrary(const std::string& pLib);

	/**
	 * @brief Replaces the current process image with the command and arguments arguments defined in the project.
	 * Used to run the build result if the build worked.
	 * 
	 * @param pSharedObjectPaths - If any dependency creates a shared object then this will include their output paths to set LD_LIBRARY_PATH with.
	 * @return true - This will never happen as it it works current process will be replace with the command.
	 * @return false - Something was wrong.
	 */
	bool RunOutputFile(const std::string& pSharedObjectPaths)const;

private:
	/**
	 * @brief Adds an BuildTaskCompile object for every source file that needs to be built.
	 * 
	 * @param pSourceFiles The source files that 'may' need to be built.
	 * @param pRebuildAll If true forces a rebuild of all source files.
	 * @param pAdditionalArgs Some extra build arguments that the calling function may need to add. Other args are added in this function.
	 * @param rBuildTasks The location that the new BuildTaskCompile is added too.
	 * @param rDependencies A dependency cache, when pRebuildAll is false this is used to test if a file needs building.
	 * @param rOutputFiles The list of files that will be written.
	 * @param rInputFilesSeen A list to ensure the same file is not compiled more than once.
	 * @return true 
	 * @return false 
	 */
	bool AddCompileTasks(const SourceFiles& pSourceFiles,bool pRebuildAll,const ArgList& pAdditionalArgs,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles,StringSet& rInputFilesSeen)const;

	bool AddIncludeSearchPaths(const rapidjson::Value& pPaths);
	void AddIncludeSearchPath(const std::string& pPath);

	bool AddLibrarySearchPaths(const rapidjson::Value& pPaths);
	void AddLibrarySearchPath(const std::string& pPath);

	bool AddLibraries(const rapidjson::Value& pLibs);

	bool AddDefines(const rapidjson::Value& pDefines);

	bool AddDependantProjects(const rapidjson::Value& pLibs);

	const std::string PreparePath(const std::string& pPath);// Makes the path relative to the project if it is not absolute. Cleans it up a bit too.
	bool AddIncludesFromPKGConfig(StringVec& pIncludeSearchPaths,const std::string& pVersion)const;
	bool AddLibrariesFromPKGConfig(StringVec& pLibraryFiles,const std::string& pVersion)const;

	const std::string TargetTypeToString(eTargetType pTarget)const;


	const std::string mConfigName;
	const std::string mProjectDir;	//!< The path to where the project file was loaded. All relative paths start in this folder.
	const int mLoggingMode;
	bool mIsDefaultConfig;			//!< If true then this configuration is selected to be built when there are multiple configurations and none have been specified on the commandline.
	bool mOk;
	eTargetType mTargetType;

	std::string mComplier;
	std::string mLinker;
	std::string mArchiver;

	std::string mOutputPath;
	std::string mOutputName; 		//!< The string read from output_name
	std::string mCppStandard;		//!< The c++ used standard.
	std::string mOptimisation; 		//!< The level of optimisation used, for gcc will be 0,1 or 2.
	std::string mDebugLevel; 		//!< Request debugging information and also use level to specify how much information.
	std::string mGTKVersion;		//!< The version of GTK, currently 2.0 or 3.0. Is used in a call to "pkg-config --cflags gtk+-[VERSION]". If empty not called and not added.
	bool mWarningsAsErrors;			//!< If true then any warnings will become errors using the compiler option -Werror
	bool mEnableAllWarnings;		//!< If true then the option -Wall is used.
	bool mFatalErrors;				//!< If true, -Wfatal-errors, is added to the build args.
	StringVec mIncludeSearchPaths;
	StringVec mLibrarySearchPaths;
	StringVec mLibraryFiles;
	StringVec mDefines;
	StringVec mExtraCompilerArgs;	//!< Allows the user to add extra compiler options that I may not have included.
	StringVec mExecuteParams;		//!< If the build exec is to be ran then these are the commandlines for that process.
	SourceFiles mSourceFiles; 		//!< Source files that are build just for a specific configuration. Allows targeting of different platforms.

	//!< The projects that this project needs.
	//!< Will check and build them if they need to be also will add their output filenames to the this projects.
	//!< Not sure about adding source folders into the include folders.
	//!< The key is the path to the project file, the value is the configuration in that project we're dependant on.'
	StringMap mDependantProjects;
};

typedef std::shared_ptr<const Configuration> ConfigurationPtr;
typedef std::map<std::string,ConfigurationPtr> BuildConfigurations;
typedef std::vector<ConfigurationPtr> ConfigurationsVec;


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
