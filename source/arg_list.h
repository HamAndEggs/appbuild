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
   
#ifndef __ARG_LIST_H__
#define __ARG_LIST_H__

#include "json.h"
#include "misc.h"

namespace appbuild{
/////////////////////////////////////////////////////////////////////////
class ArgList
{
public:
	ArgList();
	ArgList(const ArgList& pOther);
	~ArgList();

	void AddArg(const StringVec& pStrings);
	void AddArg(const std::string& pArg);
	void AddIncludeSearchPath(const StringVec& pPaths);
	void AddLibrarySearchPath(const std::string& pPath);
	void AddLibrarySearchPaths(const StringVec& pPaths);
	void AddLibraryFiles(const StringVec& pLibs);
	void AddDefines(const StringVec& pDefines);

	operator const StringVec&()const{return mArguments;}


private:
	StringVec mArguments;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild


#endif
