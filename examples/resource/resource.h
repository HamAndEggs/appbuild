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
#include <iostream>

/*
	All paramiters that ask for a source file will be expecting the same entry
	that was put in the project file.
*/

namespace AppBuildResource{
//////////////////////////////////////////////////////////////////////////

/*
	Access the resource as a stream object.
*/
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

/*
	Returns the size of the resourc. Will return -1 if not found. Used in conjunction with CopyResourceToMemory.
*/
extern int GetResourceFileSize(const std::string& pFilename);

/*
	Following two are to allow you to read as a block of ram. You are responcible for allocating and deleting the destination memory.
*/
extern bool CopyResourceToMemory(const std::string& pFilename,uint8_t* rDestination,int pDestinationSize);// Will return false is file not found or destination size is not enough.

/*
	Returns the resource with it's size in rMemorySize. Will return NULL is resource is not found.
	You have to delete the memory that has been returned.
*/
extern uint8_t* CopyResourceToMemory(const std::string& pFilename,int &rMemorySize);

//////////////////////////////////////////////////////////////////////////
};//AppBuildResource{

#endif //#ifndef _APPBUILD_RESOURCE_H_
