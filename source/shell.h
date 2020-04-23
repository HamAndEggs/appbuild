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
#ifndef __SHELL_H__
#define __SHELL_H__

#include <string>
#include <vector>

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs, std::string& rOutput);

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __SHELL_H__
