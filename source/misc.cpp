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

#include <assert.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <algorithm>

#include "misc.h"


namespace appbuild{
//////////////////////////////////////////////////////////////////////////
bool FileExists(const char* pFileName)
{
    struct stat file_info;
    return stat(pFileName, &file_info) == 0 && S_ISREG(file_info.st_mode);
}

bool DirectoryExists(const char* pDirname)
{
// Had and issue where a path had an odd char at the end of it. So do this to make sure it's clean.
	const std::string clean(TrimWhiteSpace(pDirname));

    struct stat dir_info;
    if( stat(clean.c_str(), &dir_info) == 0 )
    {
        return S_ISDIR(dir_info.st_mode);
    }
    return false;
}

bool MakeDir(const std::string& pPath)
{
    StringVec folders = SplitString(pPath,"/");

    std::string CurrentPath;
    const char* seperator = "";
    for(const std::string& path : folders )
    {
        CurrentPath += seperator;
        CurrentPath += path;
        seperator = "/";
        if(DirectoryExists(CurrentPath) == false)
        {
            if( mkdir(CurrentPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 )
            {
                std::cout << "Making folders failed for " << pPath << std::endl;
                std::cout << "Failed AT " << path << std::endl;
                return false;
            }
        }
    }
    return true;
}

std::string GetFileName(const std::string& pPathedFileName,bool RemoveExtension/* = false*/)
{
    std::string result = pPathedFileName;
    // If / is the last char then it is just a path, so do nothing.
    if(result.back() != '/' )
    {
            // String after the last / char is the file name.
        std::size_t found = result.rfind("/");
        if(found != std::string::npos)
        {
            result = result.substr(found+1);
        }

        if( RemoveExtension )
        {
            found = result.rfind(".");
            if(found != std::string::npos)
            {
                result = result.substr(0,found);
            }
        }
    }

    // Not found, and so is just the file name.
    return result;
}

std::string GetPath(const std::string& pPathedFileName)
{
    // String after the last / char is the file name.
    std::size_t found = pPathedFileName.find_last_of("/");
    if(found != std::string::npos)
    {
        std::string result = pPathedFileName.substr(0,found);
        result += '/';
        return CleanPath(result);
    }
    return "./";
}

std::string GetCurrentWorkingDirectory()
{
    char buf[PATH_MAX];
    std::string path = getcwd(buf,PATH_MAX);
    return path;
}

std::string CleanPath(const std::string& pPath)
{
    return ReplaceString(ReplaceString(pPath,"/./","/"),"//","/");
}

std::string GetExtension(const std::string& pFileName,bool pToLower)
{
    std::string result;
    std::size_t found = pFileName.rfind(".");
    if(found != std::string::npos)
    {
        result = pFileName.substr(found,pFileName.size()-found);
        if( pToLower )
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    }
    return result;
}

static bool FilterMatch(const std::string& pFilename,const std::string& pFilter)
{
    size_t wildcardpos=pFilter.find("*");
    if( wildcardpos != std::string::npos )
    {
        if( wildcardpos > 0 )
        {
            wildcardpos = pFilename.find(pFilter.substr(0,wildcardpos));
            if( wildcardpos == std::string::npos )
                return false;
        }
        return pFilename.find(pFilter.substr(wildcardpos+1)) != std::string::npos;
    }
    return pFilename.find(pFilter) != std::string::npos;
}

StringVec FindFiles(const std::string& pPath,const std::string& pFilter)
{
    StringVec FoundFiles;

    DIR *dir = opendir(pPath.c_str());
    if(dir)
    {
        struct dirent *ent;
        while((ent = readdir(dir)) != nullptr)
        {
            const std::string fname = ent->d_name;
            if( FilterMatch(fname,pFilter) )
                FoundFiles.push_back(fname);
        }
    }
    else
    {
        std::cout << "FindFiles \'" << pPath << "\' was not found or is not a path." << std::endl;
    }

    return FoundFiles;
}

char* CopyString(const char* pString, size_t pMaxLength)
{
    char *newString = nullptr;
    if( pString != nullptr && pString[0] != 0 )
    {
        size_t len = strnlen(pString,pMaxLength);
        if( len > 0 )
        {
            newString = new char[len + 1];
            strncpy(newString,pString,len);
            newString[len] = 0;
        }
    }

    return newString;
}

StringVec SplitString(const std::string& pString, const char* pSeperator)
{
    StringVec res;
    for (size_t p = 0, q = 0; p != pString.npos; p = q)
        res.push_back(pString.substr(p + (p != 0), (q = pString.find(pSeperator, p + 1)) - p - (p != 0)));
    return res;
}


std::string ReplaceString(const std::string& pString,const std::string& pSearch,const std::string& pReplace)
{
    std::string Result = pString;
    // Make sure we can't get stuck in an infinite loop if replace includes the search string or both are the same.
    if( pSearch != pReplace && pSearch.find(pReplace) != pSearch.npos )
    {
        for( std::size_t found = Result.find(pSearch) ; found != Result.npos ; found = Result.find(pSearch) )
        {
            Result.erase(found,pSearch.length());
            Result.insert(found,pReplace);
        }
    }
    return Result;
}

std::string TrimWhiteSpace(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}


std::string GetTimeString(const char* pFormat)
{
    assert(pFormat);
    if(!pFormat)
        pFormat = "%d-%m-%Y %X";

    time_t     now = time(nullptr);
    struct tm  tstruct;
    char       buf[128];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), pFormat, &tstruct);
    return buf;
}

std::string GetTimeDifference(const std::chrono::system_clock::time_point& pStart,const std::chrono::system_clock::time_point& pEnd)
{
    assert( pStart <= pEnd );
    if( pStart >= pEnd )
        return "0 Seconds";

    const std::chrono::duration<double> timeTaken(pEnd - pStart);

    const int TotalSeconds = (int)timeTaken.count();
    const int Seconds = TotalSeconds % 60;
    const int Minutes = (TotalSeconds/60) % 60;
    const int Hours = (TotalSeconds/60/60) % 60;
    const int Days = (TotalSeconds/60/60/24);

    std::string time;

    if( Days == 1 )
        time += "1 Day ";
    else if( Days > 1 )
        time += std::to_string(Days) + " Days ";

    if( Hours == 1 )
        time += "1 hour ";
    else if( Hours > 1 )
        time += std::to_string(Hours) + " Hours ";

    if( Minutes == 1 )
        time += "1 Minute ";
    else if( Minutes > 1 )
        time += std::to_string(Minutes) + " Minutes ";

    if( Seconds == 1 )
        time += "1 second ";
    else if( Seconds > 1 )
        time += std::to_string(Seconds) + " Seconds ";

    return time;
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
