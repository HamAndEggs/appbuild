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
   
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include "misc.h"

#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
Dependencies::Dependencies(const std::string& pProjectFile)
{
	bool Ok = GetFileTime(pProjectFile,mProjectFileTime);
	assert( Ok );
}

bool Dependencies::RequiresRebuild(const std::string& pSourceFile,const std::string& pObjectFile,const StringVec& pIncludePaths)
{
	// Add the path of the source file we're checking to the include paths. Has to be done in a way so that we don't pollute the passed in paths. Hence the copy and the passing in of the params as const. Stops bugs!!!!
	StringVec IncludePaths = pIncludePaths;
	const std::string srcPath = GetPath(pSourceFile);
	if( !srcPath.empty() )
		IncludePaths.push_back(srcPath);


	// Everything works from the object file's modification date. If any of the dependencies are younger than the object file then the source file needs building.
	timespec ObjFileTime;

	// We have to reset this for each source file as it is information respect to the source files object file date.
	// This map is also what helps to stop us getting into recursive loop. When it is not in this map we parse it too see what headers it has.
	// After that it is added with the state of being older or newer. This also prevents it being parsed a second time.
	// File headers found in a file are cached for each file and don't need to be and are not cleared. So the header is only parsed once.
	mFileDependencyState.clear();
	mFileCheckedState.clear();// This one is used to stop recursion.

	// Get the object files info, if this fails then the file is not there, if it is not a regular file then that is wrong and so will rebuild it too.
	if( GetFileTime(pObjectFile,ObjFileTime) )
	{
		// Before scanning the file, check against the project file time.
		if( FileYoungerThanObjectFile(mProjectFileTime,ObjFileTime) )
			return true;

		return CheckDependencies(pSourceFile,ObjFileTime,IncludePaths);
	}

	// Obj not there, so build it.
	return true;
}

bool Dependencies::CheckDependencies(const std::string& pSourceFile,const timespec& pObjFileTime,const StringVec& pIncludePaths)
{
	// Check that we have not already checked this file.
	auto found = mFileDependencyState.find(pSourceFile);
	if( found != mFileDependencyState.end() )
	{// File already been processed so stop and return the outcome.
		return found->second;
	}

	// Start the source file for a start then move onto scanning the file.
	if( FileYoungerThanObjectFile(pSourceFile,pObjFileTime) )
		return true;

	// Get all the includes from the file.
	StringSet Includes;
	if( GetIncludesFromFile(pSourceFile,pIncludePaths,Includes) )
	{
		// Now check their dependencies.....
		for( std::string filename : Includes )
		{
			if( mFileCheckedState[filename] == false )
			{
				mFileCheckedState[filename] = true;
				bool result = CheckDependencies(filename,pObjFileTime,pIncludePaths);
				mFileDependencyState[filename] = result;
				if( result )
					return true;
			}
		}
	}

	// Get here then all is up to date.
	return false;
}

bool Dependencies::GetFileTime(const std::string& pFilename,timespec& rFileTime)
{// I cache file times and the headers found in a file. Gives a very nice speed up.
	FileTimeMap::iterator found = mFileTimes.find(pFilename);
	if( found != mFileTimes.end() )
	{
		rFileTime = found->second;
		return true;
	}

	FileStats Stats;
	if( stat(pFilename.c_str(), &Stats) == 0 && S_ISREG(Stats.st_mode) )
	{
		rFileTime = Stats.st_mtim;
		mFileTimes[pFilename] = Stats.st_mtim;
		return true;
	}
	// File not found.
	return false;
}

bool Dependencies::FileYoungerThanObjectFile(const std::string& pFilename,const timespec& pObjFileTime)
{
	timespec OtherTime;
	// Get the dependency file's info, if this fails then the file is not there.
	// Unlike the object file, if not here then that is an error and I need to invoke a rebuild of the source file.
	// If I do not do this then you could delete a used header and not know that the file does not build till you modify it.
	if( GetFileTime(pFilename,OtherTime) )
	{
		return FileYoungerThanObjectFile(OtherTime,pObjFileTime);
	}
	// Dependency not found, cause a rebuild of source file.
	return true;
}

bool Dependencies::FileYoungerThanObjectFile(const timespec& pOtherTime,const timespec& pObjFileTime)const
{
	if(pOtherTime.tv_sec == pObjFileTime.tv_sec)
		return pOtherTime.tv_nsec > pObjFileTime.tv_nsec;
	else
		return pOtherTime.tv_sec > pObjFileTime.tv_sec;
}

bool Dependencies::GetIncludesFromFile(const std::string& pFilename,const StringVec& pIncludePaths,StringSet& rIncludes)
{
	assert( pFilename.size() > 0 );
	assert( pIncludePaths.size() > 0 );
	assert( rIncludes.size() < 1000  );

	// First see if we have not already parsed this header, if so send back the stuff we found.
	// Caches the found headers in a file between each dependency check is a very nice speed up.
	DependencyMap::const_iterator already_done = mDependencies.find(pFilename);
	if( already_done != mDependencies.end() )
	{
		rIncludes = already_done->second;
		return true;
	}

//	std::cout << "GetIncludesFromFile(" << pFilename << ")" << std::endl;


	assert(mDependencies.size() < 1000 );

	std::ifstream file(pFilename);
	if( file.is_open() )
	{
		while( file.eof() == false )
		{
			std::string aLine;
			std::getline(file,aLine);
			if( aLine.size() >= 12 )// Has to be >= than 12 for #include <a> the shortest incarnation.
			{
				std::size_t found = aLine.find("#include");
				if( found != std::string::npos )
				{
					found += 8;// Skip #include
					// Now scan from here to find a " or a <.
					// I can not use another find as the line maybe malformed.
					while( found < aLine.size() )
					{
						char terminator = 0;
						if( aLine[found] == '\"' )
							terminator = '\"';
						else if( aLine[found] == '<' )
							terminator = '>';

						found++;// Next char.

						// If we have a terminator then scan to the end and treat that as the filename.
						if( terminator != 0 )
						{
							std::size_t start = found;
							for(; found < aLine.size() && aLine[found] != terminator ; found++);

							// Did we find the end?
							if( aLine[found] == terminator )
							{
								std::string includefile = aLine.substr(start,found-start);


								// Now see if we can find it.
								for(const std::string& path : pIncludePaths )
								{
									std::string PathedInclude = path + includefile;
									if( FileExists(PathedInclude) )
									{
										rIncludes.insert(PathedInclude);
										break;
									}
								}
							}
							// And done. Next line please.
							found = aLine.size();
						}
					}
				}
			}
		}
		// done :)
		// Record the files found for this file.
		mDependencies[pFilename] = rIncludes;
		return true;
	}

	// Dependency not found, cause a rebuild of source file.
	return false;
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
