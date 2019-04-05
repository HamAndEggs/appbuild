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

#ifndef _BUILD_TASK_COMPILE_H_
#define _BUILD_TASK_COMPILE_H_

#include <assert.h>
#include <atomic>
#include <thread>
#include <list>
#include <stack>

#include "string_types.h"
#include "build_task.h"

//////////////////////////////////////////////////////////////////////////
// Holds the information for each build task.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{

class BuildTaskCompile : public BuildTask
{
public:
	BuildTaskCompile(const std::string& pTaskName,const std::string& pOutputFilename, const std::string& pCommand, const StringVec& pArgs,int pLoggingMode);
	virtual ~BuildTaskCompile();

	virtual const std::string& GetOutputFilename()const{return mOutputFilename;}

private:
	virtual bool Main();

	const std::string mCommand; // What needs to be done.
	const StringVec mArgs;
	const std::string mOutputFilename;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
