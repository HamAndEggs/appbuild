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
   
#ifndef __JSON_H__
#define __JSON_H__

#include "TinyJson.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


namespace appbuild{
//////////////////////////////////////////////////////////////////////////
// Some rapid json helpers.

/**
 * @brief Get the Project Schema json string.
 * 
 * @return const std::string& 
 */
extern const std::string& GetProjectSchema();

/**
 * @brief Checks the passed in json against the internal application schema.
 * 
 * @param pJson 
 * @param pVerbose If true will output information on the validation process.
 * @return true 
 * @return false 
 */
extern bool ValidateJsonAgainstSchema(const tinyjson::JsonValue& pJson,bool pVerbose);

/**
 * @brief For every entry in the project file pJson that is missing the default value will be added.
 * Uses the internal application project-default.json file. 
 * This is more than just inserting missing parts. It has to understand the structure so that some parts in the default file
 * are applied to multiple entries.
 * This allows a user to create a minimal project files.
 * @param pJson 
 */
extern void UpdateJsonProjectWithDefaults(tinyjson::JsonValue& pJson,bool pVerbose);

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild


#endif //__JSON_H__