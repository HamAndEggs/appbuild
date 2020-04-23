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

#ifndef __MISC_H__
#define __MISC_H__

#include <inttypes.h>
#include <stdint.h>
#include <chrono>

#include "string_types.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
bool FileExists(const char* pFilename);	//Will return false if file name is a path! We want to know if the file exists!
bool DirectoryExists(const char* pDirname);	//Will return false if name name is a file! We want to know if the dir exists!

inline bool FileExists(const std::string& pFilename){return FileExists(pFilename.c_str());}
inline bool DirectoryExists(const std::string& pDirname){return DirectoryExists(pDirname.c_str());}

// Splits string into list have names to be made into directories, so don't include any filenames as they will be made into a folder.
// Returns true if all was ok.
bool MakeDir(const std::string& pPath);

std::string GetFileName(const std::string& pPathedFileName,bool RemoveExtension = false);
std::string GetPath(const std::string& pPathedFileName);
std::string GetCurrentWorkingDirectory();
std::string CleanPath(const std::string& pPath);
std::string GetExtension(const std::string& pFileName,bool pToLower = true);
StringVec FindFiles(const std::string& pPath,const std::string& pFilter = "*");


// If pNumChars == 0 then full length is used.
// assert(cppmake::CompareNoCase("onetwo","one",3) == true);
// assert(cppmake::CompareNoCase("onetwo","ONE",3) == true);
// assert(cppmake::CompareNoCase("OneTwo","one",3) == true);
// assert(cppmake::CompareNoCase("onetwo","oneX",3) == true);
// assert(cppmake::CompareNoCase("OnE","oNe") == true);
// assert(cppmake::CompareNoCase("onetwo","one") == false);
// assert(cppmake::CompareNoCase("onetwo","onetwothree",6) == true);
// assert(cppmake::CompareNoCase("onetwo","onetwothreeX",6) == true);
// assert(cppmake::CompareNoCase("onetwo","onetwothree") == false);
// assert(cppmake::CompareNoCase("onetwo","onetwo") == true);
bool CompareNoCase(const char* pA,const char* pB,int pLength=0);

char* CopyString(const char* pString, size_t pMaxLength);
inline char* CopyString(const std::string& pString){return CopyString(pString.c_str(),pString.size());}
StringVec SplitString(const std::string& pString,const char* pSeperator);
std::string ReplaceString(const std::string& pString,const std::string& pSearch,const std::string& pReplace);
std::string TrimWhiteSpace(const std::string &s);

// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
// for more information about date/time format
std::string GetTimeString(const char* pFormat = "%d-%m-%Y %X");

std::string GetTimeDifference(const std::chrono::system_clock::time_point& pStart,const std::chrono::system_clock::time_point& pEnd);

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __MISC_H__
