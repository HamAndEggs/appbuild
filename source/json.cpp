
#include "json.h"
#include "rapidjson/schema.h"

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
            UpdateJsonWithDefaults(pJson);
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
            UpdateJsonWithDefaults(pJson);
            return true;
        }
    }

    return false;
}

bool ValidateJsonAgainstSchema(rapidjson::Document& pJson)
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
            // They both have the same key, so continue enumeration deaper down.
            // we do not replace any values or arrays if the match.
            // We're only filling in the blanks here!
            auto srcT = srcIt->value.GetType() ;
            auto dstT = dstIt->value.GetType() ;
            if(srcT != dstT)
                return false;

            if (srcIt->value.IsObject())
            {
                if(!AddMissingMembers(dstIt->value, srcIt->value, allocator))
                    return false;
            }
        }
    }
    return true;
}

void UpdateJsonWithDefaults(rapidjson::Document& pJson)
{
    rapidjson::Document defaultJson;
    if( defaultJson.Parse(GetProjectDefault()).HasParseError() )
    {
        std::cout << "Schema Json failed to read..." << std::endl;
        return;
    }

    AddMissingMembers(pJson,defaultJson,pJson.GetAllocator());

    SaveJson("./test.json",pJson);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
