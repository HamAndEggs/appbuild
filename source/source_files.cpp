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
#include "misc.h"

namespace appbuild{

//////////////////////////////////////////////////////////////////////////
SourceFiles::SourceFiles()
{

}

SourceFiles::SourceFiles(const SourceFiles& pOther)
{
	*this = pOther;
}

const SourceFiles& SourceFiles::operator = (const SourceFiles& pOther)
{
	mSourceFiles = pOther.mSourceFiles;
	return pOther;
}

bool SourceFiles::Read(const JSONValue* pSourceElement,const std::string& pPathedProjectFilename)
{
	if( pSourceElement )
	{
		// Instead of looking for specific items here we'll enumerate them.
		for( auto group : pSourceElement->GetObject()->GetChildren() )
		{// Can be any number of json elements with the same name at the same level. So we have a vector.
			for(auto x : group.second)
				ReadGroupSourceFiles(group.first,x,pPathedProjectFilename);
		}
	}
	return true;
}

void SourceFiles::ReadGroupSourceFiles(const char* pGroupName,const JSONValue* pFiles,const std::string& pPathedProjectFilename)
{
	const std::string ProjectDir = GetPath(pPathedProjectFilename);
	if(pFiles && pGroupName)// Can be null.
	{
		if( pFiles->GetType() != JSONValue::ARRAY )
		{
			std::cout << "The \'files\' object in the \'group\' object of this project file \'" << pPathedProjectFilename << "\' is not an array" << std::endl;
		}
		else
		{
			for( int n = 0 ; n < pFiles->GetArraySize() ; n++ )
			{
				const char* filename = pFiles->GetString(n);
				if(filename)
				{
					const std::string InputFilename = ProjectDir + filename;
					// If the source file exists then we'll continue, else show an error.
					if( FileExists(InputFilename) )
					{
						mSourceFiles[pGroupName].insert(filename);
					}
					else
					{
						std::cout << "Input filename \'" << filename << "\' in group \'" << pGroupName << "\' not found at \'" << InputFilename << "\'" << std::endl;
					}
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
