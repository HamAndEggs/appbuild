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


//////////////////////////////////////////////////////////////////////////
// Holds the information for each build task.
//////////////////////////////////////////////////////////////////////////
namespace appbuild{
class BuildTaskStack;
class Configuration;
class BuildTask
{
public:
	BuildTask(const std::string& pTaskName,int pLoggingMode);
	virtual ~BuildTask();

	virtual const std::string& GetOutputFilename()const = 0;

	void Execute();
	const std::string& GetResults()const{return mResults;}
	const std::string& GetTaskName()const{return mTaskName;}
	bool GetIsCompleted()const{return mCompleted;}
	bool GetOk(){return mOk;}

protected:
	virtual bool Main() = 0;

	const int mLoggingMode;
	std::string mResults;

private:
	static void CallMain(BuildTask* pTask);

	const std::string mTaskName;// The name of the task. At a later data I may make new task types instead of all being a compile task. Some work on the design needed.
	bool mOk;

	std::atomic<bool> mCompleted;
	std::thread thread;
};

// As a class and not a type def so that in some headers that only need a reference to these I can do a forward reference instead of including this header.
class BuildTaskStack : public std::stack<BuildTask*>{};
class RunningBuildTasks : public std::list<BuildTask*>{};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
