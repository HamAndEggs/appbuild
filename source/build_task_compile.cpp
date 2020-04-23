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
   
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include "build_task_compile.h"
#include "misc.h"
#include "logging.h"
#include "shell.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

BuildTaskCompile::BuildTaskCompile(const std::string& pTaskName, const std::string& pOutputFilename, const std::string& pCommand, const StringVec& pArgs,int pLoggingMode):
	BuildTask(pTaskName,pLoggingMode),
	mCommand(pCommand), mArgs(pArgs), mOutputFilename(pOutputFilename)
{
}

BuildTaskCompile::~BuildTaskCompile()
{
}

bool BuildTaskCompile::Main()
{
	if(mLoggingMode >= appbuild::LOG_VERBOSE)
	{
		std::cout << mCommand << " ";
		for( const auto& arg : mArgs )
			std::cout << arg << " ";

		std::cout << std::endl;
	}

	return ExecuteShellCommand(mCommand, mArgs, mResults);
}


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
