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

	Project(const std::string& pFilename,size_t pNumThreads,int pLoggingMode,bool pRebuild,size_t pTruncateOutput);
	~Project();

	operator bool ()const{return mOk;}

	bool Build(const Configuration* pActiveConfig);
	bool RunOutputFile(const Configuration*pActiveConfig);
	void Write(rapidjson::Document& pDocument)const;
	const Configuration* GetActiveConfiguration(const std::string& pConfigName)const; // Tries to return a sutible configuration to build with if pConfigName is not found.

	const std::string& GetPathedProjectFilename()const{return mPathedProjectFilename;}

private:

	bool ReadConfigurations(const rapidjson::Value& pConfigs);

	bool CompileSource(const Configuration* pConfig,BuildTaskStack& pBuildTasks);
	bool LinkTarget(const Configuration* pConfig,const StringVec& pOutputFiles);
	bool ArchiveLibrary(const Configuration* pConfig,const StringVec& pOutputFiles);
	bool LinkSharedLibrary(const Configuration* pConfig,const StringVec& pOutputFiles);


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

	// Not a fan of statics, but this is a good use case.
	// I use it to trap dependacy loops.
	static StringSet sLoadedProjects;

};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
