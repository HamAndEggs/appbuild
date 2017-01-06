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
   
#include <assert.h>
#include <iostream>

#include "arg_list.h"
#include "json.h"
#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
ArgList::ArgList()
{

}

ArgList::ArgList(const ArgList& pOther)
{
	mArguments = pOther.mArguments;
	mIncludePaths = pOther.mIncludePaths;
}

ArgList::~ArgList()
{

}

void ArgList::AddArg(const StringVec& pStrings)
{
	for(auto arg : pStrings)
		AddArg(arg);
}

void ArgList::AddArg(const std::string& pArg)
{
	std::string trimed = TrimWhiteSpace(pArg);
	if( trimed.size() > 0 )
		mArguments.push_back(TrimWhiteSpace(trimed));
}

void ArgList::AddIncludeSearchPath(const std::string& pPath,const std::string& pProjectDir)
{
	const std::string path = AddPathArg("-I",pPath,pProjectDir);
	if( path.size() > 0 )
	{
		mIncludePaths.push_back(path);
	}
	else
	{
		std::cout << "Include Arg path not found, make sure it's a folder and not a file: " << path << std::endl;
	}
}

void ArgList::AddLibrarySearchPath(const std::string& pPath,const std::string& pProjectDir)
{
	const std::string path = AddPathArg("-L",pPath,pProjectDir);
	if( path.size() > 0 )
	{

	}
	else
	{
		std::cout << "Library search path not found, make sure it's a folder and not a file: " << path << std::endl;
	}
}

const std::string ArgList::AddPathArg(const std::string& Opt,const std::string& pPath,const std::string& pProjectDir)
{
	assert(pPath.size() > 0);
	if( pPath.size() > 0 )
	{
		// Before we add it, see if it's an absolute path, if not add it to the project dir, then check if it exists.
		std::string ArgPath;
		if( pPath.front() != '/' )
			ArgPath = pProjectDir;

		ArgPath += pPath;
		if( DirectoryExists(ArgPath) )
		{
			if( ArgPath.back() != '/' )
				ArgPath += "/";

			AddArg(Opt + ArgPath);
			return ArgPath;
		}
	}
	return std::string();
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
