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
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>


#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
void Debug(const char* fmt, ...)
{
#ifdef _DEBUG
	char buf[256];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	std::cout << "DEBUG: :" << buf << std::endl;
#endif
}

bool FileExists(const char* pFileName)
{
	struct stat file_info;
	return stat(pFileName, &file_info) == 0 && S_ISREG(file_info.st_mode);
}

bool DirectoryExists(const char* pDirname)
{
	struct stat dir_info;
	if( stat(pDirname, &dir_info) == 0 )
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
	for(std::string path : folders )
	{
		CurrentPath += seperator;
		CurrentPath += path;
		seperator = "/";
		if(DirectoryExists(CurrentPath) == false)
		if( mkdir(CurrentPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 )
			return false;
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

std::string GetCurrentWorkingDirectory()
{
	char buf[PATH_MAX];
	std::string path = getcwd(buf,PATH_MAX);
	return path;
}


bool ExecuteShellCommand(const std::string& pCommand, const StringVec& pArgs, std::string& rOutput)
{
	if (pCommand.size() == 0 )
	{
		Debug("Command parameter was zero length");
		return false;
	}

	int pipeSTDOUT[2];
	int pipeSTDERR[2];
	int result = pipe(pipeSTDOUT);
	if (result < 0)
	{
		perror("pipe");
		exit(-1);
	}

	result = pipe(pipeSTDERR);
	if (result < 0)
	{
		perror("pipe");
		exit(-1);
	}

	/* print error message if fork() fails */
	pid_t pid = fork();
	if (pid < 0)
	{
		Debug("fork failure");
		return false;
	}

	/* fork() == 0 for child process */
	if (pid == 0)
	{
		dup2(pipeSTDERR[1], STDERR_FILENO); /* Duplicate writing end to stdout */
		close(pipeSTDERR[0]);
		close(pipeSTDERR[1]);

		dup2(pipeSTDOUT[1], STDOUT_FILENO); /* Duplicate writing end to stdout */
		close(pipeSTDOUT[0]);
		close(pipeSTDOUT[1]);

		char** TheArgs = new char*[pArgs.size() + 2];// +1 for the NULL and +1 for the file name as per convention, see https://linux.die.net/man/3/execlp.
		int c = 0;
		TheArgs[c++] = CopyString(pCommand.c_str());
		for (const std::string& Arg : pArgs)
		{
			char* str = CopyString(Arg.c_str());
			if(str)
			{
				//Trim leading white space.
				while(isspace(str[0]) && str[0])
					str++;
				if(str[0])
					TheArgs[c++] = str;
			}
		}
		TheArgs[c++] = NULL;

		// This replaces the current process so no need to clearn up the memory leaks before here. ;)
		execvp(TheArgs[0], TheArgs);

		Debug("execl() failure!\n\n");
		Debug("This print is after execl() and should not have been executed if execl were successful! \n\n");
		_exit(1);
	}

	/*
	 * parent process
	 */

	/* Parent process */
	close(pipeSTDOUT[1]); /* Close writing end of pipes, don't need them */
	close(pipeSTDERR[1]); /* Close writing end of pipes, don't need them */

	FILE *cmd_output = fdopen(pipeSTDOUT[0], "r");
	char buf[128];

	while( fgets(buf,sizeof(buf), cmd_output) != NULL )
	{
		rOutput += buf;
	}

	cmd_output = fdopen(pipeSTDERR[0], "r");
	while( fgets(buf,sizeof(buf), cmd_output) != NULL )
	{
		rOutput += buf;
	}

	int status;
	bool Worked = false;
	if( wait(&status) == -1 )
	{
		Debug("Failed to wait for child process");
	}
	else
	{
		if(WIFEXITED(status) && WEXITSTATUS(status) != 0)//did the child terminate normally?
		{
			Debug("%ld exited with return code %d\n",(long)pid, WEXITSTATUS(status));
			Worked = WEXITSTATUS(status) == 0;
		}
		else if (WIFSIGNALED(status))// was the child terminated by a signal?
		{
			Debug("%ld terminated because it didn't catch signal number %d\n",(long)pid, WTERMSIG(status));
		}
		else
		{// Get here, then all is ok.
			Worked = true;
		}
	}

	return Worked;
}

bool CompareNoCase(const char* pA,const char* pB,int pLength)
{
	assert( pA != NULL || pB != NULL );// Note only goes pop if both are null.
// If either or both NULL, then say no. A bit like a divide by zero as null strings are not strings.
	if( pA == NULL || pB == NULL )
		return false;

// If same memory then yes they match, doh!
	if( pA == pB )
		return true;

	if( pLength <= 0 )
		pLength = INT_MAX;

	while( (*pA != 0 || *pB != 0) && pLength > 0 )
	{
		// Get here are one of the strings has hit a null then not the same.
		// The while loop condition would not allow us to get here if both are null.
		if( *pA == 0 || *pB == 0 )
		{// Check my assertion above that should not get here if both are null. Note only goes pop if both are null.
			assert( pA != NULL || pB != NULL );
			return false;
		}

		if( tolower(*pA) != tolower(*pB) )
			return false;

		pA++;
		pB++;
		pLength--;
	};

	// Get here, they are the same.
	return true;
}

char* CopyString(const char* pString)
{
	char *newString = NULL;
	if( pString != NULL && pString[0] != 0 )
	{
		int len = strlen(pString);
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

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
