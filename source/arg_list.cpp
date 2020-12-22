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
#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
ArgList::ArgList()
{

}

ArgList::ArgList(const ArgList& pOther)
{
	mArguments = pOther.mArguments;
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

void ArgList::AddIncludeSearchPath(const StringVec& pPaths)
{
	for( const auto& path : pPaths )
	{
		if( DirectoryExists(path) )
		{
			AddArg("-I" + path);
		}
		else
		{
			std::cout << "Include Arg path not found, make sure it's a folder and not a file: " << path << std::endl;
		}
	}
}

void ArgList::AddLibrarySearchPath(const std::string& pPath)
{
	if( DirectoryExists(pPath) )
	{
		AddArg("-L" + pPath);
	}
	else
	{
		std::cout << "Library search path not found, make sure it's a folder and not a file: " << pPath << std::endl;
	}
}

void ArgList::AddLibrarySearchPaths(const StringVec& pPaths)
{
	for(auto path : pPaths)
		AddLibrarySearchPath(path);
}

void ArgList::AddLibraryFiles(const StringVec& pLibs)
{
	for(auto lib : pLibs)
	{
		// Here we force the file name and it's say use the location passed in with -L for the lib.
		// This is because the user could name the lib to anything if they overide the output setting.
		// We have to allow that as they maybe generating an output with a custom name for a custom usecase.
		// https://sourceware.org/binutils/docs-2.18/ld/Options.html
		// This instructs the linker to find and link a library with the exact name libevent.a,
		// in the specified (-Ldir) or default linker search directories,
		// as opposed to the usual convention whereby -lfoo instructs the linker to find and link
		// either libfoo.so (shared library) or libfoo.a (static library),
		// preferring libfoo.so if both are found in the same search directory.
		if( lib.size() > 6 && lib.find("lib") == 0 )
		{
			if( lib.rfind(".a") == lib.size()-2 ||
				lib.rfind(".so") == lib.size()-3 )
			{
				AddArg("-l:" + lib);
			}
		}
		else
		{
			AddArg("-l" + lib);
		}
	}
}

void ArgList::AddDefines(const StringVec& pDefines)
{
	for(auto def : pDefines)
		AddArg("-D" + def);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
