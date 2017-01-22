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

#ifndef _BUILD_TASK_H_
#define _BUILD_TASK_H_

#include <assert.h>
#include <atomic>
#include <thread>
#include <list>
#include <stack>

#include "string_types.h"

//////////////////////////////////////////////////////////////////////////
// Holds the information for each build task.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{

class BuildTask
{
public:
	BuildTask(const std::string& pTaskName,const std::string& pOutputFilename, const std::string& pCommand, const StringVec& pArgs);
	~BuildTask();

	void Execute();
	const std::string& GetResults()const{return mResults;}
	const std::string& GetOutputFilename()const{return mOutputFilename;}
	bool GetIsCompleted()const{return mCompleted;}
	bool GetOk(){return mOk;}

private:
	BuildTask(const BuildTask& pOther):mOk(false) { assert(false); }// Prevent this from being called.
	const BuildTask& operator =(const BuildTask& pOther){ assert(false); return *this;}// Prevent this from being called.

	void Main();
	static void CallMain(BuildTask* pTask);

	const std::string mTaskName;// The name of the task. At a later data I may make new task types instead of all being a compile task. Some work on the design needed.
	const std::string mCWD;
	const std::string mCommand; // What needs to be done.
	const StringVec mArgs;
	const std::string mOutputFilename;
	std::string mResults;
	bool mOk;

	std::atomic<bool> mCompleted;
	std::thread thread;
};

class BuildTaskStack : public std::stack<BuildTask*>
{
};

class RunningBuildTasks : public std::list<BuildTask*>
{
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
