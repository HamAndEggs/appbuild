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

#include "build_task_resource_files.h"
#include "json.h"
#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

BuildTaskResourceFiles::BuildTaskResourceFiles(const std::string& pTaskName, const StringVec& pResourceFiles,bool pVerboseOutput):
	BuildTask(pTaskName,pVerboseOutput),
	mResourceFiles(pResourceFiles)
{
}

BuildTaskResourceFiles::~BuildTaskResourceFiles()
{
}

bool BuildTaskResourceFiles::Main()
{
	if(mVerboseOutput)
	{
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
