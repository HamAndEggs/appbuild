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

#include "build_task.h"
#include "dependencies.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

class JSONValue;
class JsonWriter;

class ResourceFiles
{
public:
    typedef StringMap::const_iterator const_iterator;

	ResourceFiles();
	ResourceFiles(const ResourceFiles& pOther);

	const ResourceFiles& operator = (const ResourceFiles& pOther);

    const_iterator begin()const{return mFiles.begin();}
    const_iterator end()const{return mFiles.end();}

	bool Read(const JSONValue* pSourceElement,const std::string& pPathedProjectFilename);
	bool Write(JsonWriter& rJsonOutput)const;

private:
	void AddFile(const char* pFileName,const char* pGroupName,const std::string& pPathedProjectFilename);

	StringMap mFiles;
	bool mWriteAsJsonArray;	// So if we write it out again, we know to write as an array.
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
