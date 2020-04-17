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

#include "source_files.h"
#include "json.h"
#include "json_writer.h"
#include "misc.h"

namespace appbuild{

//////////////////////////////////////////////////////////////////////////
SourceFiles::SourceFiles(const std::string& pProjectPath) :mProjectDir(pProjectPath), mWriteAsJsonArray(false)
{

}

SourceFiles::SourceFiles(const SourceFiles& pOther)
{
	*this = pOther;
}

const SourceFiles& SourceFiles::operator = (const SourceFiles& pOther)
{
	mProjectDir = pOther.mProjectDir;
	mSourceFiles = pOther.mSourceFiles;
	mWriteAsJsonArray = pOther.mWriteAsJsonArray;
	return pOther;
}

bool SourceFiles::Read(const JSONValue* pSourceElement)
{
	if( pSourceElement )
	{
		// Instead of looking for specific items here we'll enumerate them.
		// Can be either an array of files or an object with a list of child objects where the name is the file name and the value is a virtual folder name.
		if( pSourceElement->GetType() == JSONValue::ARRAY )
		{
			mWriteAsJsonArray = true;
			for(int n=0;n<pSourceElement->GetArraySize();n++)
			{
				AddFile(pSourceElement->GetString(n),"source");
			}
		}
		else if( pSourceElement->GetType() == JSONValue::OBJECT && pSourceElement->GetObject() )
		{
			const ValueMap& files = pSourceElement->GetObject()->GetChildren();
			for(const auto& value : files)
			{
				for(const auto& obj : value.second)
				{
					if( obj->GetType() == JSONValue::STRING )
					{
						AddFile(value.first,obj->GetString());
					}
					else
					{
						std::cout << "The \'source_files\' object has a member \'" << value.first << "\' whos value is not a string in this project file \'" << mProjectDir << std::endl;
						return false;					
					}
				}
			}
		}
		else
		{
			std::cout << "The \'source_files\' object in this project file \'" << mProjectDir << "\' is not a supported json type" << std::endl;
			return false;
		}
	}
	return true;
}

bool SourceFiles::Write(JsonWriter& rJsonOutput)const
{
	if( mSourceFiles.size() > 0 )
	{
		if(mWriteAsJsonArray)
		{
			rJsonOutput.StartArray("source_files");
			for( const auto& file : mSourceFiles )
				rJsonOutput.AddArrayItem(file.first);
			rJsonOutput.EndArray();
		}
		else
		{
			rJsonOutput.StartObject("source_files");
			for( const auto& file : mSourceFiles )
				rJsonOutput.AddObjectItem(file.first,file.second);
			rJsonOutput.EndObject();
		}
	}
	return true;
}

void SourceFiles::AddFile(const char* pFileName,const char* pGroupName)
{
	assert(pFileName);
	assert(pGroupName);
	if(pFileName && pGroupName)
	{
		const std::string InputFilename = mProjectDir + pFileName;
		// If the source file exists then we'll continue, else show an error.
		if( FileExists(InputFilename) )
		{
			// Have we seen it before? If so say NO.
			if( mSourceFiles.find(pFileName) == mSourceFiles.end() )
				mSourceFiles[pFileName] = pGroupName;// Good, not seen before, so add it.
			else
			std::cout << "Input filename \'" << pFileName << "\' in group \'" << pGroupName << "\' is a duplicate" << std::endl;
		}
		else
		{
			std::cout << "Input filename \'" << pFileName << "\' in group \'" << pGroupName << "\' not found at \'" << InputFilename << "\'" << std::endl;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
