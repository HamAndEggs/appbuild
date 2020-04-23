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

#include "string_types.h"
#include "json.h"
#include "build_task.h"
#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class SourceFiles
{
public:
    typedef StringSet::const_iterator const_iterator;

	SourceFiles(const std::string& pProjectPath);
	SourceFiles(const SourceFiles& pOther);

	const SourceFiles& operator = (const SourceFiles& pOther);

    const_iterator begin()const{return mFiles.begin();}
    const_iterator end()const{return mFiles.end();}
	const size_t size()const{return mFiles.size();}
	const bool IsEmpty()const{return size() == 0;}

	bool Read(const rapidjson::Value& pSourceElement);
	rapidjson::Value Write(rapidjson::Document::AllocatorType& pAllocator)const;

	void AddFile(const std::string& pFileName);

private:
	std::string mProjectDir;
	StringSet mFiles;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
