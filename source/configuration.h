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
	Configuration(const std::string& pConfigName,const JSONValue* pConfig,const std::string& pPathedProjectFilename,const std::string& pProjectDir);
	~Configuration();

	bool GetOk()const{return mOk;}
	eTargetType GetTargetType()const{return mTargetType;}

	const std::string& GetPathedTargetName()const{return mPathedTargetName;}
	const std::string& GetName()const{return mConfigName;}
	const std::string& GetLinker()const{return mLinker;}
	const StringVec& GetLibraryFiles()const{return mLibraryFiles;}
	const ArgList& GetLibrarySearchPaths()const{return mLibrarySearchPaths;}


	bool GetBuildTasks(const StringVecMap& pSourceFiles,bool pRebuildAll,BuildTaskStack& rBuildTasks,Dependencies& rDependencies,StringVec& rOutputFiles)const;

private:

	bool AddIncludeSearchPaths(const JSONValue* pPaths);
	void AddIncludeSearchPath(const std::string& pPath);

	bool AddLibrarySearchPaths(const JSONValue* pPaths);
	void AddLibrarySearchPath(const std::string& pPath);

	bool AddLibraries(const JSONValue* pLibs);
	void AddLibrary(const std::string& pLib);

	const std::string mConfigName;
	const std::string mProjectDir;		// The path to where the project file was loaded. All relative paths start in this folder.

	bool mOk;
	eTargetType mTargetType;

	std::string mComplier;
	std::string mLinker;
	std::string mArchiver;
	std::string mPathedTargetName;	// This is the final output fully pathed filename.
	std::string mOutputPath;

	ArgList mCompileArguments;
	ArgList mLibrarySearchPaths;
	StringVec mLibraryFiles;
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
