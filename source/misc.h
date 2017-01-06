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

#include "string_types.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
void Debug(const char* pMessage,...);	// debug messages, only written in debug build. Some extra text to make it show up.

bool FileExists(const char* pFilename);	//Will return false if file name is a path! We want to know if the file exists!
bool DirectoryExists(const char* pDirname);	//Will return false if name name is a file! We want to know if the dir exists!

inline bool FileExists(const std::string& pFilename){return FileExists(pFilename.c_str());}
inline bool DirectoryExists(const std::string& pDirname){return DirectoryExists(pDirname.c_str());}

// Splits string into list have names to be made into directories, so don't include any filenames as they will be made into a folder.
// Returns true if all was ok. 
bool MakeDir(const std::string& pPath);

std::string GetFileName(const std::string& pPathedFileName);
std::string GetCurrentWorkingDirectory();

/**
 * pCWD can be null.
 */
bool ExecuteShellCommand(const std::string& pCommand,const StringVec& pArgs, std::string& rOutput);
bool ExecuteShellCommand(const std::string& pCommand,const StringVec& pArgs, std::string& rOutput);


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

char* CopyString(const char* pString);
inline char* CopyString(const std::string& pString){return CopyString(pString.c_str());}
StringVec SplitString(const std::string& pString,const char* pSeperator);

inline std::string TrimWhiteSpace(const std::string &s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && isspace(*it))
		it++;

	std::string::const_reverse_iterator rit = s.rbegin();
	while (rit.base() != it && isspace(*rit))
		rit++;

	return std::string(it, rit.base());
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __MISC_H__
