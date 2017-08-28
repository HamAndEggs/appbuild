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

#include <streambuf>
#include <iostream>
#include <assert.h>
#include "resource.h"
#include "lz4.h"


namespace AppBuildResource{
//////////////////////////////////////////////////////////////////////////
struct ResourceFilesTOC
{
  const char* mFilename;
  int mOffset;
  int mCompressedSize;
  int mFileSize;
};

//*****-APPBUILD-INSERT-RESOURCE-HERE-*****

static const ResourceFilesTOC* FindFile(const std::string& pFilename)
{
	//First see if we can find the file. For now, do it slow. Later i'll sort and do a divide and conqor search, which will be a lot quicker..
	const ResourceFilesTOC* FileTOC = NULL;
	for( size_t n = 0 ; n < (sizeof(table_of_contents) / sizeof(table_of_contents[0])) && FileTOC == NULL ; n++ )
	{
		if( table_of_contents[n].mFilename == pFilename )
		{
			return table_of_contents + n;
		}
	}
	return NULL;
}

class MemoryStream : public std::streambuf
{
public:
	MemoryStream(const ResourceFilesTOC* pTOC) : mMemory(NULL),mMemoryEnd(NULL)
	{
		assert(pTOC);
		mMemory = new char[pTOC->mFileSize];
		mMemoryEnd = mMemory + pTOC->mFileSize;

		LZ4_decompress_safe((const char*)(resource_file_data + pTOC->mOffset),mMemory,pTOC->mCompressedSize,pTOC->mFileSize);
	
		this->setg(mMemory, mMemory, mMemoryEnd);
	}

	virtual ~MemoryStream()
	{
		delete []mMemory;
	}

	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
	{
		if(dir == std::ios_base::cur)
			gbump(off);
		else if(dir == std::ios_base::end)
			setg(mMemory, mMemoryEnd+off, mMemoryEnd);
		else if(dir == std::ios_base::beg)
			setg(mMemory, mMemory+off, mMemoryEnd);

		return gptr() - eback();
	}

	virtual pos_type seekpos(std::streampos pos, std::ios_base::openmode mode) override
	{
		return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, mode);
	}

	char *mMemory;
	char *mMemoryEnd;
};


ResourceFile::ResourceFile(const std::string& pFilename) : mTheStream(NULL)
{
	const ResourceFilesTOC* FileTOC = FindFile(pFilename);
	if( FileTOC )
	{
		mTheStream = new MemoryStream(FileTOC);
	}
}

ResourceFile::~ResourceFile()
{
	delete mTheStream;
}

// Following two are to allow you to read as a block of ram. You are responcible for allocating and deleting the destination memory.
bool CopyResourceToMemory(const std::string& pFilename,uint8_t* rDestination,int pDestinationSize)
{
	const ResourceFilesTOC* FileTOC = FindFile(pFilename);
	if(FileTOC && FileTOC->mFileSize <= pDestinationSize)
	{
		LZ4_decompress_safe((const char*)(resource_file_data + FileTOC->mOffset),(char*)rDestination,FileTOC->mCompressedSize,FileTOC->mFileSize);		
		return true;
	}
	return false;
}

int GetResourceFileSize(const std::string& pFilename)
{
	const ResourceFilesTOC* FileTOC = FindFile(pFilename);
	if(FileTOC)
		return FileTOC->mFileSize;
	return -1;
}

//////////////////////////////////////////////////////////////////////////
};//AppBuildResource
