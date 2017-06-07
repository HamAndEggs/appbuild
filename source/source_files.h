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

#ifndef _SOURCE_FILES_H_
#define _SOURCE_FILES_H_

#include "build_task.h"
#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class JSONValue;

class SourceFiles
{
public:
    typedef StringMap::const_iterator const_iterator;

	SourceFiles();
	SourceFiles(const SourceFiles& pOther);

	const SourceFiles& operator = (const SourceFiles& pOther);

//    operator const StringMap&(){return mSourceFiles;}
    const_iterator begin()const{return mSourceFiles.begin();}
    const_iterator end()const{return mSourceFiles.end();}

	bool Read(const JSONValue* pSourceElement,const std::string& pPathedProjectFilename);

private:
	void AddFile(const char* pFileName,const char* pGroupName,const std::string& pPathedProjectFilename);

	StringMap mSourceFiles;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
