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

#ifndef _RESOURCE_FILES_H_
#define _RESOURCE_FILES_H_

#include "string_types.h"
#include "json.h"
#include "build_task.h"
#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////


class ResourceFiles
{
public:
    typedef StringSet::const_iterator const_iterator;

	ResourceFiles();
	ResourceFiles(const ResourceFiles& pOther);

	const ResourceFiles& operator = (const ResourceFiles& pOther);

    const_iterator begin()const{return mFiles.begin();}
    const_iterator end()const{return mFiles.end();}

	bool Read(const rapidjson::Value& pSourceElement,const std::string& pPathedProjectFilename);
	const rapidjson::Value Write(rapidjson::Document::AllocatorType& pAllocator)const;

	bool GetNeedsRebuild()const{return mFiles.size() > 0;} // Checks the date time of the resource files against the cpp file that is created. If any are newer then the resource file is rebuilt.
    bool GetIncludeLZ4Code()const{return true;} // Not  implemented yet, but almost is.....
private:
	void AddFile(const std::string&,const std::string& pPathedProjectFilename);

	StringSet mFiles;

};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
