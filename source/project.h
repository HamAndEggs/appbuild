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

#include "build_task.h"
#include "dependencies.h"
#include "configuration.h"

//////////////////////////////////////////////////////////////////////////
// Holds all the information about the project file.
// If any filename variable is fully pathed then the name with start with Pathed.
// If not then it maybe just the filename or part pathed.
// All pathed filenames are from root file system. '/'
//////////////////////////////////////////////////////////////////////////

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class JSONValue;

class Project
{
public:

	Project(const std::string& pFilename,size_t pNumThreads,bool pVerboseOutput,bool pRebuild);
	~Project();

	operator bool ()const{return mOk;}

	bool Build(const Configuration* pActiveConfig);
	bool RunOutputFile(const Configuration*pActiveConfig,bool pAsSudo);
	const Configuration* FindConfiguration(const std::string& pConfigName)const;

	const std::string& GetPathedProjectFilename()const{return mPathedProjectFilename;}

private:

	void ReadSourceFiles(const char* pGroupName,const JSONValue* pFiles);
	void ReadConfigurations(const JSONValue* pSettings);

	bool CompileSource(const Configuration* pConfig,BuildTaskStack& pBuildTasks);
	bool LinkTarget(const Configuration* pConfig,const StringVec& pOutputFiles);
	bool ArchiveLibrary(const Configuration* pConfig,const StringVec& pOutputFiles);
	bool LinkSharedLibrary(const Configuration* pConfig,const StringVec& pOutputFiles);


	// Some options that are passed into the constructor.
	const size_t mNumThreads;
	const bool mVerboseOutput;
	const bool mRebuild;

	// This project file, fully pathed.
	std::string mPathedProjectFilename;
	std::string mProjectDir;

	Dependencies mDependencies;
	StringVecMap mSourceFiles;
	BuildConfigurations mBuildConfigurations;

	bool mOk;	// Project loaded ok.

};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif