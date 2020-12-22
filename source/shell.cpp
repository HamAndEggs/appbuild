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

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>

#include "shell.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
static char* CopyArg(const std::string& pString)
{
    char *newString = nullptr;
    const size_t len = pString.length();
    if( len > 0 )
    {
        newString = new char[len + 1];
        for( size_t n = 0 ; n < len ; n++ )
            newString[n] = pString.at(n);
        newString[len] = 0;
    }

    return newString;
}

static void BuildArgArray(char** pTheArgs, const std::vector<std::string>& pArgs)
{    
    for (const std::string& Arg : pArgs)
    {
        char* str = CopyArg(Arg);
        if(str)
        {
            //Trim leading white space.
            while(isspace(str[0]) && str[0])
                str++;

            if(str[0])
            {
                *pTheArgs = str;
                pTheArgs++;
            }
        }
    }
    *pTheArgs = nullptr;
}

bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv, std::string& rOutput)
{
    const bool VERBOSE = false;
    if (pCommand.size() == 0 )
    {
        std::cerr << "ExecuteShellCommand Command name for was zero length! No command given!\n";
        return false;
    }

    int pipeSTDOUT[2];
    int result = pipe(pipeSTDOUT);
    if (result < 0)
    {
        perror("pipe");
        exit(-1);
    }

    int pipeSTDERR[2];
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
        std::cout << "ExecuteShellCommand Fork failed" << std::endl;
        return false;
    }

    /* fork() == 0 for child process */
    if (pid == 0)
    {
        dup2(pipeSTDOUT[1], STDOUT_FILENO ); /* Duplicate writing end to stdout */
        close(pipeSTDOUT[0]);
        close(pipeSTDOUT[1]);

        dup2(pipeSTDERR[1], STDERR_FILENO ); /* Duplicate writing end to stdout */
        close(pipeSTDERR[0]);
        close(pipeSTDERR[1]);

        ExecuteCommand(pCommand,pArgs,pEnv);
    }

    /*
     * parent process
     */

    /* Parent process */
    close(pipeSTDOUT[1]); /* Close writing end of pipes, don't need them */
    close(pipeSTDERR[1]); /* Close writing end of pipes, don't need them */

    size_t BufSize = 1000;
    char buf[BufSize+1];
    buf[BufSize] = 0;

    struct pollfd Pipes[] =
    {
        {pipeSTDOUT[0],POLLIN,0},
        {pipeSTDERR[0],POLLIN,0},
    };

    int NumPipesOk = 2;
    std::stringstream outputStream;
    do
    {
        int ret = poll(Pipes,2,1000);
        if( ret < 0 )
        {
            rOutput = "Error, pipes failed. Can't capture process output";
            NumPipesOk = 0;
        }
        else if(ret  > 0 )
        {
            for(int n = 0 ; n < 2 ; n++ )
            {
                if( (Pipes[n].revents&POLLIN) != 0 )
                {
                    ssize_t num = read(Pipes[n].fd,buf,BufSize);
                    if( num > 0 )
                    {
                        buf[num] = 0;
                        outputStream << buf;
                    }
                }
                else if( (Pipes[n].revents&(POLLERR|POLLHUP|POLLNVAL)) != 0 && Pipes[n].events != 0 )
                {
                    Pipes[n].fd = -1;
                    NumPipesOk--;
                }
            }
        }
    }while(NumPipesOk>0);
    rOutput = outputStream.str();

    int status;
    bool Worked = false;
    if( wait(&status) == -1 )
    {
        std::cout << "Failed to wait for child process." << std::endl;
    }
    else
    {
        if(WIFEXITED(status) && WEXITSTATUS(status) != 0)//did the child terminate normally?
        {
            if( VERBOSE )
                std::cout << (long)pid << " exited with return code " << WEXITSTATUS(status) << std::endl;
        }
        else if (WIFSIGNALED(status))// was the child terminated by a signal?
        {
            if( VERBOSE )
                std::cout << (long)pid << " terminated because it didn't catch signal number " << WTERMSIG(status) << std::endl;
        }
        else
        {// Get here, then all is ok.
            Worked = true;
        }
    }

    return Worked;
}

void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv)
{
    // +1 for the NULL and +1 for the file name as per convention, see https://linux.die.net/man/3/execlp.
    char** TheArgs = new char*[pArgs.size() + 2];
    TheArgs[0] = CopyArg(pCommand);
    BuildArgArray(TheArgs+1,pArgs);

    // Build the environment variables.
    for( const auto& var : pEnv )
    {
        setenv(var.first.c_str(),var.second.c_str(),1);
    }

    // This replaces the current process so no need to clean up the memory leaks before here. ;)
    execvp(TheArgs[0], TheArgs);

    const char* errorString = strerror(errno);

    std::cerr << "ExecuteCommand execvp() failure!\n" << "    Error: " << errorString << "\n    This print is after execvp() and should not have been executed if execvp were successful!\n";

    // Should never get here!
    throw std::runtime_error("Command execution failed! Should not have returned! " + pCommand + " Error: " + errorString);
    // Really make sure the process is gone...
    _exit(1);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
