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

/**
 * @brief Checks if the path passed is absolute.
 * abstracted out like this in case it needs to be changed / ported.
 * Just checks first char.
 * 
 * @param pPath 
 * @return true 
 * @return false 
 */
inline bool GetIsPathAbsolute(const std::string& pPath)
{
   return pPath.size() > 0 && pPath.at(0) == '/';
}

// Splits string into list have names to be made into directories, so don't include any filenames as they will be made into a folder.
// Returns true if all was ok.
bool MakeDir(const std::string& pPath);

/**
 * @brief Takes the pathed file name and creates any missing folders that it needs.
 * 
 * @param pPathedFilename Can either be absolute or relative to current working directory. 
 * If folders already exist then nothing will change.
 * @return true The folders were made ok.
 * @return false There was an issue.
 */
bool MakeDirForFile(const std::string& pPathedFilename);


std::string GetFileName(const std::string& pPathedFileName,bool RemoveExtension = false);
std::string GetPath(const std::string& pPathedFileName);
std::string GetCurrentWorkingDirectory();
std::string CleanPath(const std::string& pPath);

/**
 * @brief Based on the string passed in will attempt to make a reasonble guess for an output file name.
 * 
 * @param pProjectName A string that should contain something to work with. A reasonble prompt.
 * @return std::string If no guess can be made, the string "binary" will be returned.
 */
std::string GuessOutputName(const std::string& pProjectName);


/**
 * @brief Get the Extension of the passed filename,
 * E.G. hello.text will return text
 * 
 * @param pFileName 
 * @param pToLower 
 * @return std::string The extension found or a zero length string. The extension returned does not include the dot.
 */
std::string GetExtension(const std::string& pFileName,bool pToLower = true);

/**
 * @brief Find the files in path that match the filter. Does not enter child folders.
 * 
 * @param pPath 
 * @param pFilter 
 * @return StringVec 
 */
StringVec FindFiles(const std::string& pPath,const std::string& pFilter = "*");

/**
 * @brief Get the Relative Path based on the passed absolute paths.
 * In debug the following errors will result in an assertion.
 * Examples
 *     GetRelativePath("/home/richard/games/chess","/home/richard/games/chess/game1") == "./game1/"
 *     GetRelativePath("/home/richard/games","/home/richard/music") == "../music/"
 * @param pCWD The directory that you want the full path to be relative too. This path should also start with a / if it does not the function will just return the full path unchanged.
 * @param pFullPath The full path, if it does not start with a / it will be assumed to be already relative and so just returned.
 * @return std::string If either input string is empty './' will be returned.
 */
std::string GetRelativePath(const std::string& pCWD,const std::string& pFullPath);


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
bool DoMiscUnitTests();

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __MISC_H__
