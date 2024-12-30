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

#include "json.h"
#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

const std::string& GetProjectSchema()
{
// This odd #define thing is to stop editors thinking there is an error when there is not. ALLOW_INCLUDE_OF_SCHEMA is defined in the build settings.
#ifdef ALLOW_INCLUDE_OF_SCHEMA
static const std::string projectSchema =
#include "project-schema.json.string"
;
#else
    static const std::string projectSchema = "{}";
#endif

    return projectSchema;
}

static const std::string& GetProjectDefault()
{
// This odd #define thing is to stop editors thinking there is an error when there is not. ALLOW_INCLUDE_OF_SCHEMA is defined in the build settings.
#ifdef ALLOW_INCLUDE_OF_SCHEMA
static const std::string projectDefault =
#include "project-default.json.string"
;
#else
    static const std::string projectDefault = "{}";
#endif

    return projectDefault;
}

bool ValidateJsonAgainstSchema(const tinyjson::JsonValue& pJson,bool pVerbose)
{
    try
    {
        // First make sure all values have a type set.
        if( pJson.GetType() != tinyjson::JsonValueType::OBJECT )
        {
            THROW_APPBUILD_EXCEPTION("JsonValue passed to ValidateJsonAgainstSchema is not an object");
        }

        for( const auto& o : pJson )
        {
            if( o.second.GetType() == tinyjson::JsonValueType::OBJECT )
            {
                if( ValidateJsonAgainstSchema(o.second,pVerbose) == false )
                {
                    return false;
                }
            }
            else if( pJson.GetType() == tinyjson::JsonValueType::INVALID )
            {
                THROW_APPBUILD_EXCEPTION("A value in the object " + o.first + " has not type set.");
            }
        }
    }
    catch(const std::exception& e)
    {
        if( pVerbose )
        {
            std::cerr << "Project file failed validation: " << e.what() << "\n";
        }
        return false;
    }
    
    return true;
}

static bool AddMissingMembers(tinyjson::JsonValue& pDst,const tinyjson::JsonValue& pSrc,bool pVerbose)
{
    if( pSrc.GetType() != tinyjson::JsonValueType::OBJECT )
    {
        THROW_APPBUILD_EXCEPTION("The json value is not an object.");
    }

    // Now merge the two json trees together.
    for( const auto& obj : pSrc )
    {
        if(pVerbose){std::cout << "Adding for: " << obj.first << "\n";}
        if( pDst.HasValue(obj.first) )
        {
            // If they both have the same type, so continue enumeration deaper down.
            // we do not replace any values or arrays if they match.
            // We're only filling in the blanks here!
            if( obj.second.IsObject() && pDst.IsObject() )
            {
                if(!AddMissingMembers(pDst[obj.first], obj.second,pVerbose))
                    return false;
            }
        }
        else
        {// The source object was not found in dest, so copy it over.
            pDst[obj.first] = obj.second;
        }
    }
    return true;
}

/**
 * @brief Finds the first member that has the "default":true entry, if not found returns first entry.
 * Assumes that there is at least one child object.
 * @param pConfigurations 
 * @return const rapidjson::Value& 
 */
static const tinyjson::JsonValue& FindDefaultConfiguration(const tinyjson::JsonValue& pConfigurations)
{
    for( const auto& conf : pConfigurations.GetObject() )
    {
        if( conf.second.HasValue("default") && conf.second["default"].IsBool() && conf.second["default"].GetBoolean() == true )
        {
            return conf.second;
        }
    }

    // None listed as default, so return the first one.
    return pConfigurations.GetObject().begin()->second;
}

/**
 * @brief Searches for a configuration to use that matches the name or uses the best default.
 * 
 * @param pConfigurations 
 * @return const rapidjson::Value& 
 */
static const tinyjson::JsonValue& FindNamedConfigurationOrDefault(const tinyjson::JsonValue& pConfigurations,const std::string& pName)
{
    for( const auto& conf : pConfigurations.GetObject() )
    {
        if( CompareNoCase(conf.first.c_str(),pName.c_str()) )
        {
            return conf.second;
        }
    }

    return FindDefaultConfiguration(pConfigurations);
}

void UpdateJsonProjectWithDefaults(tinyjson::JsonValue& pJson,bool pVerbose)
{
    if(pVerbose){std::cout << "Adding missing defaults to project.\n";}
    tinyjson::JsonProcessor defaultJson(GetProjectDefault());

    if( defaultJson.GetRoot().GetType() != tinyjson::JsonValueType::OBJECT )
    {
        THROW_APPBUILD_EXCEPTION("The root for the defaults for a project is not an object.");
    }

    // This function expects the defaultJson to be of a specific format that the project loader expects.
    // It will apply parts of the defaults to parts of the passed in file.
    // We can not just do a insertion of missing parts as some parts have different names depending on the target.

    // Get the defaults for the configuration.
    // here we make the assumption that there is just one in the defaults.
    for( const auto& o : defaultJson.GetRoot() )
    {
        if(pVerbose){std::cout << "Checking for: " << o.first << "\n";}
        if( pJson.HasValue(o.first) == false )
        {
            if(pVerbose){std::cout << "No \"" << o.first << "\", adding default\n";}
            pJson[o.first] = o.second;
        }
        else if( CompareNoCase(o.first.c_str(),"configurations") )
        {
            if(pVerbose){std::cout << "Merging configurations\n";}
            // See what needs to be added to the configurations that there are.
            tinyjson::JsonValue& configs = pJson["configurations"];
            for( auto& conf : configs )
            {
                const tinyjson::JsonValue& defaults = FindNamedConfigurationOrDefault(defaultJson["configurations"],conf.first);

                // If their configs match one of the configs in the defaults then we'll use that to fill in the gaps.
                // If no match is made, we'll just use the first one marked as default in the json. If none mound first one used.
                AddMissingMembers(conf.second,defaults,pVerbose);
            }
        }
    }
/*
//debug testing
    std::ofstream file("test.json");
    if( file.is_open() )
    {
        tinyjson::JsonWriter(file,pJson,true);
    }
*/
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
