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
#include "misc.h"
#include "logging.h"

namespace appbuild{

//////////////////////////////////////////////////////////////////////////
SourceFiles::SourceFiles(const std::string& pProjectPath,int pLoggingMode) :mProjectDir(pProjectPath), mLoggingMode(pLoggingMode)
{

}

SourceFiles::SourceFiles(const SourceFiles& pOther)
{
	*this = pOther;
}

const SourceFiles& SourceFiles::operator = (const SourceFiles& pOther)
{
	mProjectDir = pOther.mProjectDir;
	mFiles = pOther.mFiles;
	mLoggingMode = pOther.mLoggingMode;
	return pOther;
}

bool SourceFiles::Read(const rapidjson::Value& pSourceElement)
{
	if( pSourceElement.IsArray() )
	{
		for( const auto& file : pSourceElement.GetArray() )
		{
			AddFile(file.GetString());
		}
	}
	return true;
}

rapidjson::Value SourceFiles::Write(rapidjson::Document::AllocatorType& pAllocator)const
{
	return BuildStringArray(mFiles,pAllocator);
}

bool SourceFiles::AddFile(const std::string& pFileName)
{
	if(pFileName.size() > 0)
	{
		const bool isAbsolute = GetIsPathAbsolute(pFileName);
		if( mLoggingMode == LOG_INFO && isAbsolute )
		{
			std::cout << "SourceFiles::AddFile passed a file with an absolute path, for projects files this is not a good idea. Are you sure you wish to do this? File was " << pFileName << std::endl;
		}

		// Make the string we're going to check a constant so we can be sure we don't accidentally change it.
		const std::string InputFilename = (isAbsolute) ? (pFileName) : (mProjectDir + pFileName);
		// If the source file exists then we'll continue, else show an error.
		if( FileExists(InputFilename) )
		{
			mFiles.insert(pFileName);
			return true;
		}
		else if( isAbsolute )
		{
			std::cout << "Input filename with absolute path was not found, is this what you indented? Filename is " << pFileName << std::endl;
		}
		else
		{
			std::cout << "Input filename " << pFileName << " not found at " << InputFilename << "" << std::endl;
		}
	}
	else if( mLoggingMode == LOG_INFO )
	{
		std::cout << "SourceFiles::AddFile passed with zero length string" << std::endl;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
