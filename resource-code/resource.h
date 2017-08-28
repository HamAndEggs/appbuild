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

#ifndef _APPBUILD_RESOURCE_H_
#define _APPBUILD_RESOURCE_H_

#include <string>

namespace AppBuildResource{
//////////////////////////////////////////////////////////////////////////
class MemoryStream;
class ResourceFile : public std::istream
{
	ResourceFile(const std::string& pFilename);
	virtual ~ResourceFile();

	bool is_open()const{return mTheStream != NULL;}
	operator bool()const{return is_open();}

private:
	MemoryStream* mTheStream;
};
//////////////////////////////////////////////////////////////////////////
};//AppBuildResource{

#endif //#ifndef _APPBUILD_RESOURCE_H_
