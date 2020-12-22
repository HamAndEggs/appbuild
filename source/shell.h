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
#ifndef __SHELL_H__
#define __SHELL_H__

#include <string>
#include <vector>
#include <map>

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

/**
 * @brief Calls and waits for the command in pCommand with the arguments pArgs and addictions to environment variables in pEnv.
 * Uses the function ExecuteCommand below but first forks the process so that current execution can continue.
 * https://linux.die.net/man/3/execvp
 * 
 * Blocking. If you need a non blocking then just call in a worker thread of your own. This makes it cleaner and more flexable.
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param pEnv Extra environment variables, string pair (name,value), to append to the current processes environment variables, maybe empty if you wish. An example use is setting LD_LIBRARY_PATH
 * @param rOutput The output from the executed command, if there was any.
 * @return true if calling the command worked, says nothing of the command itself.
 * @return false Something went wrong. Does not represent return value of command.
 */
extern bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv, std::string& rOutput);

/**
 * @brief Calls and waits for the command in pCommand with the arguments pArgs.
 * Uses the function ExecuteCommand below but first forks the process so that current execution can continue.
 * https://linux.die.net/man/3/execvp
 * 
 * Blocking. If you need a non blocking then just call in a worker thread of your own. This makes it cleaner and more flexable.
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param rOutput The output from the executed command, if there was any.
 * @return true if calling the command worked, says nothing of the command itself.
 * @return false Something went wrong. Does not represent return value of command.
 */
inline bool ExecuteShellCommand(const std::string& pCommand,const std::vector<std::string>& pArgs, std::string& rOutput)
{
    const std::map<std::string,std::string> empty;
    return ExecuteShellCommand(pCommand,pArgs,empty,rOutput);
}

/**
 * @brief Replaces the current process image with the command in pCommand with the arguments pArgs and addictions to environment variables in pEnv.
 * Uses the execvp command.
 * https://linux.die.net/man/3/execvp
 * 
 * Replaces the current process, so not returning from this function!
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 * @param pEnv Extra environment variables, string pair (name,value), to append to the current processes environment variables, maybe empty if you wish. An example use is setting LD_LIBRARY_PATH
 */
extern void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs,const std::map<std::string,std::string>& pEnv);

/**
 * @brief Replaces the current process image with the command in pCommand with the arguments pArgs.
 * Uses the execvp command.
 * https://linux.die.net/man/3/execvp
 * 
 * Replaces the current process, so not returning from this function!
 * 
 * @param pCommand The pathed command to run.
 * @param pArgs The arguments to be passed to the command.
 */
inline void ExecuteCommand(const std::string& pCommand,const std::vector<std::string>& pArgs)
{
    const std::map<std::string,std::string> empty;
    ExecuteCommand(pCommand,pArgs,empty);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __SHELL_H__
