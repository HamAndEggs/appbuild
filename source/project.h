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
#include "arg_list.h"

//////////////////////////////////////////////////////////////////////////
// Holds all the information about the project file.
// If any filename variable is fully pathed then the name with start with Pathed.
// If not then it maybe just the filename or part pathed.
// All pathed filenames are from root file system. '/'
//////////////////////////////////////////////////////////////////////////

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class Options;
class JSONValue;
class BuildTaskVec;

class Project
{
public:

	Project(const std::string& pFilename,size_t pNumThreads,bool pVerboseOutput,bool pRebuild);
	~Project();

	operator bool ()const{return mOk;}

	bool Build();
	bool RunOutputFile(bool pAsSudo);

private:

	void ReadGroupFiles(const char* pGroupName,const JSONValue* pFiles);
	void ReadSettings(const JSONValue* pSettings);

	const std::string MakeOutputFilename(const std::string& pFilename, const std::string& pFolder)const;

	bool CompileSource();
	bool LinkTarget();

	bool AddIncludeSearchPaths(const JSONValue* pPaths);
	void AddIncludeSearchPath(const std::string& pPath);

	bool AddLibrarySearchPaths(const JSONValue* pPaths);
	void AddLibrarySearchPath(const std::string& pPath);

	bool AddLibraries(const JSONValue* pLibs);
	void AddLibrary(const std::string& pLib);

	enum eTargetType
	{
		TARGET_NOT_SET,
		TARGET_EXEC,
		TARGET_LIBRARY,
		TARGET_SHARED_LIBRARY,
	};

	// Some options that are passed into the constructor.
	const size_t mNumThreads;
	const bool mVerboseOutput;
	const bool mRebuild;

	// This project file, fully pathed.
	std::string mPathedProjectFilename;
	std::string mProjectDir;

	// The file I make to test for dependency changes.
	// All dates for files built and their dependencies are recorded in here.
	// Written to the output folder as it's rebuilt if missing.
	std::string mPathedDatesFilename;

	std::string mOutputPath;
	std::string mComplier;
	std::string mLinker;
	std::string mArchiver;
	std::string mPathedTargetName;	// This is the final output fully pathed filename.

	BuildTaskStack mBuildTasks;
	Dependencies mDependencies;
	ArgList mCompileArguments;
	ArgList mLinkerArguments;
	StringVec mLibraryFiles;
	StringVec mObjectFiles;

	bool mOk;	// Project loaded ok.
	eTargetType mTargetType;

};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
