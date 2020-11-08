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
   
#ifndef __LOGGING_H__
#define __LOGGING_H__

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

enum LogType
{
    LOG_ERROR,      // Error is always on, hence zero. We use LOG_ERROR for when in shebang mode.
    LOG_INFO,       // Some information about the progress of the build.
    LOG_VERBOSE     // Full progress information to help issues with either the tool or the project file.
};


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif //__LOGGING_H__