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

#ifndef _VERSION_TOOLS_H_
#define _VERSION_TOOLS_H_

#include <string>

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

// Builds the 32bit number that is a version number that can be used in file transfers as well as sending data to the server / phone.
#define VERSION_MAKE(__MAJOR__,__MINOR__,__PATCH__) ((uint32_t)((((__MAJOR__)&0xff)<<24) | (((__MINOR__)&0xff)<<16) | (((__PATCH__)&0xff)<<0)))

#define VERSION_GET_MAJOR(__VERSION__) ((uint32_t)(((__VERSION__)&0xff000000)>>24))
#define VERSION_GET_MINOR(__VERSION__) ((uint32_t)(((__VERSION__)&0x00ff0000)>>16))
#define VERSION_GET_PATCH(__VERSION__) ((uint32_t)(((__VERSION__)&0x0000ffff)>>0))

#define VERSION_TO_STRING(__VERSION__) (std::string(std::to_string(VERSION_GET_MAJOR(__VERSION__)) + std::string(".") + std::to_string(VERSION_GET_MINOR(__VERSION__)) + std::string(".") + std::to_string(VERSION_GET_PATCH(__VERSION__))))

#endif

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{