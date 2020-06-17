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

#ifndef _BUILD_TASK_RESOURCE_FILES_H_
#define _BUILD_TASK_RESOURCE_FILES_H_

#include <assert.h>
#include <atomic>
#include <thread>
#include <list>
#include <stack>

#include "json.h"
#include "build_task.h"
#include "mem_buffer.h"

//////////////////////////////////////////////////////////////////////////
// Holds the information for each build task.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{
class SourceFiles;

struct ResourceFilesTOC;
class BuildTaskResourceFiles : public BuildTask
{
public:

	BuildTaskResourceFiles(const SourceFiles& pResourceFiles,const std::string& pOutputPath,int pLoggingMode);
	virtual ~BuildTaskResourceFiles();

	virtual const std::string& GetOutputFilename()const{return mOutputPath;}
	
	bool GetGeneratedResourceFiles(SourceFiles& rGeneratedResourceFiles);
	
private:
	typedef MemoryBuffer<char,1024,1024> CompressionSourceBuffer;

	virtual bool Main();

	void WriteSupportingCodeFile(const ResourceFilesTOC& pFile,bool pWriteToProjectFileLocation);
	char* DecompressSupportingCodeFile(const ResourceFilesTOC& pFile,int &pSizeToWrite)const;
	bool ResourceFileIsUpToDate(const std::string& pResourceOutputFilename)const;

	const std::string mOutputPath;
	const bool mIncludeLZ4Code;
	StringVec mResourceFiles;
	CompressionSourceBuffer mSourceBuffer;
	StringVec mGeneratedResourceFiles;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
