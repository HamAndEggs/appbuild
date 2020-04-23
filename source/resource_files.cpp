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

#include <iostream>
#include <assert.h>
#include <string.h>

#include "resource_files.h"
#include "misc.h"

namespace appbuild{

//////////////////////////////////////////////////////////////////////////
ResourceFiles::ResourceFiles()
{

}

ResourceFiles::ResourceFiles(const ResourceFiles& pOther)
{
	*this = pOther;
}

const ResourceFiles& ResourceFiles::operator = (const ResourceFiles& pOther)
{
	mFiles = pOther.mFiles;
	return pOther;
}

bool ResourceFiles::Read(const rapidjson::Value& pSourceElement,const std::string& pPathedProjectFilename)
{
	if( pSourceElement.IsArray() )
	{
		for( const auto& file : pSourceElement.GetArray() )
		{
			AddFile(file.GetString(),pPathedProjectFilename);
		}
	}
	return true;
}

const rapidjson::Value ResourceFiles::Write(rapidjson::Document::AllocatorType& pAllocator)const
{
	return BuildStringArray(mFiles,pAllocator);
}

void ResourceFiles::AddFile(const std::string& pFileName,const std::string& pPathedProjectFilename)
{
	if( pFileName.length() > 0 )
	{
		const std::string ProjectDir = GetPath(pPathedProjectFilename);
		const std::string InputFilename = ProjectDir + pFileName;
		// If the source file exists then we'll continue, else show an error.
		if( FileExists(InputFilename) )
		{
			mFiles.insert(pFileName);
		}
		else
		{
			std::cout << "Resource filename \'" << pFileName << " not found at \'" << InputFilename << "\'" << std::endl;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
