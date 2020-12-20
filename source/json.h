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

#include <iostream>
#include <fstream>
#include <sstream>

#include "string_types.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

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
 * @brief Get the Default Project json string.
 * 
 * @return const std::string& 
 */
extern const std::string& GetProjectDefault();

/**
 * @brief Saves the passed in json object to the file name.
 * 
 * @param pFilename 
 * @param pJson 
 * @return true 
 * @return false 
 */
extern bool SaveJson(const std::string& pFilename,rapidjson::Document& pJson);

/**
 * @brief Reads the file from disk, checks it, and returns the document to it.
 * 
 * @param pFilename 
 * @param pJson 
 * @return true 
 * @return false 
 */
extern bool ReadJson(const std::string& pFilename,rapidjson::Document& pJson);

/**
 * @brief Create a Json Project From Source Files passed in, used mainly for she bang. 
 * Uses the default json project to fill in the blanks.
 * @param pFiles 
 * @param pJson 
 * @return true 
 * @return false 
 */
extern bool CreateJsonProjectFromSourceFiles(const StringSet& pFiles,rapidjson::Document& pJson);

/**
 * @brief Checks the passed in json against the internal application schema.
 * 
 * @param pJson 
 * @param pVerbose If true will output information on the validation process.
 * @return true 
 * @return false 
 */
extern bool ValidateJsonAgainstSchema(rapidjson::Value& pJson,bool pVerbose);

/**
 * @brief For every entry in the project file pJson that is missing the default value will be added.
 * Uses the internal application project-default.json file. 
 * This is more than just inserting missing parts. It has to understand the structure so that some parts in the default file
 * are applied to multiple entries.
 * This allows a user to create a minimal project files.
 * @param pJson 
 * @param pSchema 
 */
extern void UpdateJsonProjectWithDefaults(rapidjson::Document& pJson);

// Will add the json pKey:pValue to pContainer. So if pContainer is an object and you call this once, you get {"key":"value"}....
// Handy for putting key pairs into an array, which is not really ideal. But....
// Also good when the key is a std::string
template <typename __TYPE__>void AddMember(rapidjson::Value& pContainer,const std::string& pKey,const __TYPE__& pValue,rapidjson::Document::AllocatorType& pAlloc)
{
   pContainer.AddMember(rapidjson::Value(pKey,pAlloc).Move(),pValue,pAlloc);
}

// Template specialization, for when value is a string, needs to be wrapped in a value like the key is.
inline void AddMember(rapidjson::Value& pContainer,const std::string& pKey,const std::string& pValue,rapidjson::Document::AllocatorType& pAlloc)
{
   pContainer.AddMember(rapidjson::Value(pKey,pAlloc),rapidjson::Value(pValue,pAlloc),pAlloc);
}

// Template specialization, for when value is a rapidjson::Value
inline void AddMember(rapidjson::Value& pContainer,const std::string& pKey,rapidjson::Value pValue,rapidjson::Document::AllocatorType& pAlloc)
{
   pContainer.AddMember(rapidjson::Value(pKey,pAlloc),rapidjson::Value(pValue,pAlloc),pAlloc);
}

inline void PushBack(rapidjson::Value& pArray,const std::string& Value,rapidjson::Document::AllocatorType& pAlloc)
{
   pArray.PushBack(rapidjson::Value(Value,pAlloc),pAlloc);
}

inline rapidjson::Value BuildStringArray(const StringVec& pStrings,rapidjson::Document::AllocatorType& pAlloc)
{
   rapidjson::Value array = rapidjson::Value(rapidjson::kArrayType);   
   for( const auto& str : pStrings )
   {
      PushBack(array,str,pAlloc);
   }
   return array;
}

inline rapidjson::Value BuildStringArray(const StringSet& pStrings,rapidjson::Document::AllocatorType& pAlloc)
{
   rapidjson::Value array = rapidjson::Value(rapidjson::kArrayType);
   for( const auto& str : pStrings )
   {
      PushBack(array,str,pAlloc);
   }
   return array;
}

inline rapidjson::Value BuildStringArray(const StringMap& pStrings,rapidjson::Document::AllocatorType& pAlloc)
{
   rapidjson::Value array = rapidjson::Value(rapidjson::kArrayType);   
   for( const auto str : pStrings )
   {
      rapidjson::Value entry = rapidjson::Value(rapidjson::kObjectType);
      AddMember(entry,str.first,str.second,pAlloc);
      array.PushBack(entry,pAlloc);
   }
   return array;
}

// Not using overloads as I want to keep to the rapid json naming. Also means I can create them all with some macro magic.
#define UTIL_GET_WITH_DEFAULT_FUNCTIONS            \
   ADD_FUNCTION(GetBool,IsBool,bool)               \
   ADD_FUNCTION(GetInt,IsInt,int)                  \
   ADD_FUNCTION(GetUint,IsUint,unsigned)           \
   ADD_FUNCTION(GetInt64,IsInt64,int64_t)          \
   ADD_FUNCTION(GetUint64,IsUint64,uint64_t)       \
   ADD_FUNCTION(GetFloat,IsFloat,float)            \
   ADD_FUNCTION(GetDouble,IsDouble,double)         \
   ADD_FUNCTION(GetString,IsString,std::string)    \

#define ADD_FUNCTION(__GET_NAME__,__CHECK_NAME__,__TYPE__)                                                  \
   inline __TYPE__ __GET_NAME__(const rapidjson::Value& pJson,const std::string& pKey,__TYPE__ pDefault)    \
   {                                                                                                        \
      if( pJson.HasMember(pKey) )                                                                           \
      {                                                                                                     \
         assert( pJson[pKey].__CHECK_NAME__() );                                                            \
         if( pJson[pKey].__CHECK_NAME__() )                                                                 \
            return pJson[pKey].__GET_NAME__();                                                              \
      }                                                                                                     \
      return pDefault;                                                                                      \
   }

// Build all util get functions that allow you to define a default if the item is not in the json stream.
UTIL_GET_WITH_DEFAULT_FUNCTIONS

#undef ADD_FUNCTION

//*** These versions log what is going on.
#define ADD_FUNCTION(__GET_NAME__,__CHECK_NAME__,__TYPE__)                                                                    \
   inline __TYPE__ __GET_NAME__##Log(const rapidjson::Value& pJson,const std::string& pKey,__TYPE__ pDefault, bool Verbose)   \
   {                                                                                                                          \
      __TYPE__ value = pDefault;                                                                                              \
      if( pJson.HasMember(pKey) )                                                                                             \
      {                                                                                                                       \
         assert( pJson[pKey].__CHECK_NAME__() );                                                                              \
         if( pJson[pKey].__CHECK_NAME__() )                                                                                   \
         {                                                                                                                    \
            value = pJson[pKey].__GET_NAME__();                                                                               \
            if( Verbose )                                                                                                     \
               {std::clog << pKey << " set to [" << value << "]\n";}                                                          \
         }                                                                                                                    \
         else                                                                                                                                   \
         {std::cerr << "json read error, " << pKey << " is not a " << #__TYPE__ << " type, it will be ignored, correct the projects json.\n";}  \
      }                                                                                                                                         \
      else if( Verbose )                                                                                           \
      {                                                                                                            \
         if( std::to_string(value).size() > 0 )                                                                    \
            {std::clog << pKey << " not found, set to default [" << value << "]\n";}                               \
         else                                                                                                      \
            {std::clog << pKey << " not found, no default set\n";}                                                 \
      }                                                                                                            \
      return value;                                                                                                \
   }

// Build all util get functions that allow you to define a default if the item is not in the json stream.
UTIL_GET_WITH_DEFAULT_FUNCTIONS

#undef ADD_FUNCTION
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild


#endif //__JSON_H__