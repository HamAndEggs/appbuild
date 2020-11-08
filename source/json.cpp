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
#include "rapidjson/schema.h"
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

const std::string& GetProjectDefault()
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

bool SaveJson(const std::string& pFilename,rapidjson::Document& pJson)
{
   std::ofstream file(pFilename);
   if( file.is_open() )
   {
      rapidjson::OStreamWrapper osw(file);
      rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
      return pJson.Accept(writer);
   }
   else
   {
      std::cout << "Failed to open json file  " << pFilename << " for writing" << std::endl;
   }
   return false;
}

bool ReadJson(const std::string& pFilename,rapidjson::Document& pJson)
{
    std::ifstream jsonFile(pFilename);
    if( jsonFile.is_open() )
    {
        std::stringstream jsonStream;
        jsonStream << jsonFile.rdbuf();// Read the whole file in...

        pJson.Parse(jsonStream.str().c_str());

        // Have to invert result as I want true if it worked, false if it failed.
        if( pJson.HasParseError() == false )
        {
            // Add out defaults for members that are missing.
            UpdateJsonProjectWithDefaults(pJson);
            return true;
        }
    }

    return false;
}

bool CreateJsonProjectFromSourceFiles(const StringSet& pFiles,rapidjson::Document& pJson)
{
    if( pFiles.size() > 0 )
    {
        std::stringstream jsonStream;
        jsonStream << "{\"source_files\":[";
        char comma = ' ';
        for( auto f : pFiles )
        {
            jsonStream << comma << f;
            comma = ',';
        }
        jsonStream << "]}";

        // Now make it a rapidjson object.
        pJson.Parse(jsonStream.str().c_str());

        // Have to invert result as I want true if it worked, false if it failed.
        if( pJson.HasParseError() == false )
        {
            // Add out defaults for members that are missing.
            UpdateJsonProjectWithDefaults(pJson);
            return true;
        }
    }

    return false;
}

bool ValidateJsonAgainstSchema(rapidjson::Value& pJson)
{
    rapidjson::Document sd;
    if (sd.Parse(GetProjectSchema()).HasParseError())
    {
        std::cout << "Schema Json failed to read..." << std::endl;
        // Whoops, that failed!
        return false;
    }
    rapidjson::SchemaDocument schema(sd); // Compile a Document to SchemaDocument
    // sd is no longer needed here.
    
    
    rapidjson::SchemaValidator validator(schema);
    if( pJson.Accept(validator) )
    {
        std::cout << "Json passed schema test" << std::endl;
        // all ok.
        return true;
    }

    // Input JSON is invalid according to the schema
    // Output diagnostic information
    rapidjson::StringBuffer sb;
    validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
    std::cout << "Invalid schema: " << sb.GetString() << std::endl;
    std::cout << "Invalid keyword: " << validator.GetInvalidSchemaKeyword() << std::endl;
    sb.Clear();
    validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
    std::cout << "Invalid document: " << sb.GetString() << std::endl;
    return false;

}

/**
 * @brief Adds the members, and their values that are in srcObject, into dstObject.
 * 
 * @param dstObject Object to be altered.
 * @param srcObject The const object that contains the values to be added to dstObject.
 * @param allocator For the memory allocation. Normally dstObject.GetAllocator()
 * @return true 
 * @return false 
 */
static bool AddMissingMembers(rapidjson::Value &dstObject,const rapidjson::Value &srcObject, rapidjson::Document::AllocatorType &allocator)
{
    for (auto srcIt = srcObject.MemberBegin(); srcIt != srcObject.MemberEnd(); ++srcIt)
    {
        auto dstIt = dstObject.FindMember(srcIt->name);

        // The source object was not found in dest, so copy it over.
        if (dstIt == dstObject.MemberEnd())
        {
            rapidjson::Value dstName ;
            dstName.CopyFrom(srcIt->name, allocator);
            rapidjson::Value dstVal ;
            dstVal.CopyFrom(srcIt->value, allocator) ;

            dstObject.AddMember(dstName, dstVal, allocator);

            dstName.CopyFrom(srcIt->name, allocator);
            dstIt = dstObject.FindMember(dstName);
            if (dstIt == dstObject.MemberEnd())
            {
                return false;
            }
        }
        else
        {
            // They both have the same type, so continue enumeration deaper down.
            // we do not replace any values or arrays if they match.
            // We're only filling in the blanks here!
            if( dstIt->value.IsObject() && srcIt->value.IsObject() )
            {
                if(!AddMissingMembers(dstIt->value, srcIt->value, allocator))
                    return false;
            }
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
static const rapidjson::Value& FindDefaultConfiguration(const rapidjson::Value& pConfigurations)
{
    for( const auto& conf : pConfigurations.GetObject() )
    {
        if( conf.value.HasMember("default") && conf.value["default"].IsBool() && conf.value["default"].GetBool() == true )
        {
            return conf.value;
        }
    }

    // None listed as default, so return the first one.
    return pConfigurations.GetObject().begin()->value;
}

/**
 * @brief Searches for a configuration to use that matches the name or uses the best default.
 * 
 * @param pConfigurations 
 * @return const rapidjson::Value& 
 */
static const rapidjson::Value& FindNamedConfigurationOrDefault(const rapidjson::Value& pConfigurations,const char* pName)
{
    for( const auto& conf : pConfigurations.GetObject() )
    {
        if( CompareNoCase(conf.name.GetString(),pName) )
        {
            return conf.value;
        }
    }

    return FindDefaultConfiguration(pConfigurations);
}

void UpdateJsonProjectWithDefaults(rapidjson::Document& pJson)
{
    // Get our defaults file.
    rapidjson::Document defaultJson;
    if( defaultJson.Parse(GetProjectDefault()).HasParseError() )
    {
        std::cout << "Schema Json failed to read..." << std::endl;
        return;
    }

    // This function expects the defaultJson to be of a specific format that the project loader expects.
    // It will apply parts of the defaults to parts of the passed in file.
    // We can not just do a insertion of missing parts as some parts have different names depending on the target.

    // Get the defaults for the configuration.
    // here we make the assumption that there is just one in the defaults.
    const rapidjson::Value& configurationDefaults = defaultJson["configurations"];

    if( pJson.HasMember("configurations") )
    {
        // See what needs to be added to the configurations that there are.
        rapidjson::Value& configs = pJson["configurations"];
        for( auto& conf : configs.GetObject() )
        {
            const rapidjson::Value& defaults = FindNamedConfigurationOrDefault(configurationDefaults,conf.name.GetString());

            // If their configs match one of the configs in the defaults then we'll use that to fill in the gaps.
            // If no match is made, we'll just use the first one marked as default in the json. If none mound first one used.
            AddMissingMembers(conf.value,defaults,pJson.GetAllocator());
        }
    }
    else
    {
        // No configurations at all, so just add the default one.
        rapidjson::Value configuration = rapidjson::Value(rapidjson::kObjectType);
        AddMissingMembers(configuration,configurationDefaults,pJson.GetAllocator());
        pJson.AddMember("configurations",configuration,pJson.GetAllocator());
    }

//for debugging
// SaveJson("./test.json",pJson);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
