
#include "json.h"
#include "rapidjson/schema.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

const std::string& GetProjectFileSchema()
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
      return pJson.HasParseError() == false;
   }

   return false;
}

bool ValidateJsonAgainstSchema(rapidjson::Document& pJson)
{
    rapidjson::Document sd;
    if (sd.Parse(GetProjectFileSchema()).HasParseError())
    {
        std::cout << "Schema Json failed to read..." << std::endl;
        // all ok.
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

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
