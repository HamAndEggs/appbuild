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


//////////////////////////////////////////////////////////////////////////
// Holds the information for each build configuration.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{

enum eTargetType
{
	TARGET_NOT_SET,
	TARGET_EXEC,
	TARGET_LIBRARY,
	TARGET_SHARED_LIBRARY,
};

class Dependencies;
class BuildTaskStack;

class Configuration
{
public:
	Configuration(const std::string& pProjectDir,const std::string& pProjectName);// Creates a default configuration suitable for simple c++11 projects.
	Configuration(const std::string& pConfigName,const JSONValue* pConfig,const std::string& pPathedProjectFilename,const std::string& pProjectDir,bool pVerboseOutput);
	~Configuration();

	bool GetOk()const{return mOk;}
	eTargetType GetTargetType()const{return mTargetType;}

	const std::string& GetPathedTargetName()const{return mPathedTargetName;}
	const std::string& GetName()const{return mConfigName;}
	const std::string& GetLinker()const{return mLinker;}
	const std::string& GetArchiver()const{return mArchiver;}
	const std::string& GetOutputPath()const{return mOutputPath;}
	const std::string& GetOutputName()const{return mOutputName;}
	const StringVec& GetLibraryFiles()const{return mLibraryFiles;}
	const ArgList& GetLibrarySearchPaths()const{return mLibrarySearchPaths;}
	const StringMap& GetDependantProjects()const{return mDependantProjects;}


	bool GetBuildTasks(const StringVecMap& pSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const;

private:

	bool AddIncludeSearchPaths(const JSONValue* pPaths);
	void AddIncludeSearchPath(const std::string& pPath);

	bool AddLibrarySearchPaths(const JSONValue* pPaths);
	void AddLibrarySearchPath(const std::string& pPath);

	bool AddLibraries(const JSONValue* pLibs);
	void AddLibrary(const std::string& pLib);

	bool AddDefines(const JSONValue* pDefines);
	void AddDefine(const std::string& pDefine);

	bool AddDependantProjects(const JSONValue* pLibs);

	const std::string mConfigName;
	const std::string mProjectDir;		// The path to where the project file was loaded. All relative paths start in this folder.

	bool mOk;
	eTargetType mTargetType;

	std::string mComplier;
	std::string mLinker;
	std::string mArchiver;
	std::string mPathedTargetName;	// This is the final output fully pathed filename.
	std::string mOutputPath;
	std::string mOutputName; //The string read from output_name

	ArgList mCompileArguments;
	ArgList mLibrarySearchPaths;
	StringVec mLibraryFiles;

	// The projects that this project needs.
	// Will check and build them if they need to be also will add their output filenames to the this projects.
	// Not sure about adding source folders into the include folders.
	// The key is the path to the project file, the value is the configuration in that project we're dependant on.'
	StringMap mDependantProjects;
};

class BuildConfigurations : public std::map<std::string,const Configuration*>
{
public:
	virtual ~BuildConfigurations()
	{
		for(auto& config : *this )
		{
			delete config.second;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
