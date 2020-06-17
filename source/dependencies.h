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

#ifndef __DEPENDENCIES_H__
#define __DEPENDENCIES_H__

#include <inttypes.h>
#include <stdint.h>
#include <functional>
#include <unordered_map>

#include "string_types.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class Dependencies
{
public:
	Dependencies();

	// Returns true if the object file date is older than the source file or any of it's dependencies.
	bool RequiresRebuild(const std::string& pSourceFile,const std::string& pObjectFile,const StringVec& pIncludePaths);

	/**
	 * @brief Adds a generic file to the list of file dates to be tested against.
	 * This is used for the project file, if there is one, and maybe some embedded resource files.
	 * 
	 * @param pPathedFileName Full path to the file, the file is NOT parsed in anyway. If it's date is younger than the object file being tested a rebuild will be triggered.
	 */
	void AddGenericFileDependency(const std::string& pPathedFileName);

private:
	bool CheckSourceDependencies(const std::string& pSourceFile,const timespec& pObjFileTime,const StringVec& pIncludePaths);
	bool GetFileTime(const std::string& pFilename,timespec& rFileTime);
	bool FileYoungerThanObjectFile(const std::string& pFilename,const timespec& pObjFileTime);
	bool FileYoungerThanObjectFile(const timespec& pOtherTime,const timespec& pObjFileTime)const;
	bool GetIncludesFromFile(const std::string& pFilename,const StringVec& pIncludePaths,StringSet& rIncludes);

	typedef struct stat FileStats;
	typedef std::unordered_map<std::string,timespec> FileTimeMap;
	typedef std::unordered_map<std::string,StringSet> DependencyMap;
	typedef std::unordered_map<std::string,bool> FileState;	// True if it is out of date and thus the source file needs building, false it is not. If not found we have not checked it yet.


	DependencyMap mDependencies;
	FileTimeMap mFileTimes;
	FileState mFileDependencyState;
	FileState mFileCheckedState;

	FileTimeMap mGenericFileDependencies;	//!< A list of files who's dates are checked against the object file, and if younger will ask for a rebuild of the source file. This is a separate list so we can explcity check these files.
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __DEPENDENCIES_H__
