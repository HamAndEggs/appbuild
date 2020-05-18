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
inline bool SaveJson(const std::string& pFilename,rapidjson::Document& pJson)
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

inline bool ReadJson(const std::string& pFilename,rapidjson::Document& pJson)
{
   std::ifstream jsonFile(pFilename);
   if( jsonFile.is_open() )
   {
      std::stringstream jsonStream;
      jsonStream << jsonFile.rdbuf();// Read the whole file in...

      pJson.Parse(jsonStream.str().c_str());

      // Have to invert result as I want tru if it worked, false if it failed.
      return pJson.HasParseError() == false;
   }

   return false;
}

// Will add the json pKey:pValue to pContainer. So if pContainer is an object and you call this once, you get {"key":"value"}....
// Handy for putting key pairs into an array, which is not really ideal. But....
// Also good when the key is a std::string
template <typename __TYPE__>void AddMember(rapidjson::Value& pContainer,const std::string& pKey,const __TYPE__& pValue,rapidjson::Document::AllocatorType& pAlloc)
{
   pContainer.AddMember(rapidjson::Value(pKey,pAlloc).Move(),pValue,pAlloc);
}

// Template specialisation, for when value is a string, needs to be wrapped in a value like the key is.
inline void AddMember(rapidjson::Value& pContainer,const std::string& pKey,const std::string& pValue,rapidjson::Document::AllocatorType& pAlloc)
{
   pContainer.AddMember(rapidjson::Value(pKey,pAlloc),rapidjson::Value(pValue,pAlloc),pAlloc);
}

// Template specialisation, for when value is a rapidjson::Value
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
               std::cout << pKey << " set to [" << value << "]" << std::endl;                                                 \
         }                                                                                                                    \
         else                                                                                                                                                 \
            std::cout << "json read error, " << pKey << " is not a " << #__TYPE__ << " type, it will be ignored, correct the projects json." << std::endl;    \
      }                                                                                                                                                       \
      else if( Verbose )                                                                                                      \
      {                                                                                                                       \
         std::cout << pKey << " not found, set to default [" << value << "]" << std::endl;                                    \
      }                                                                                                                       \
      return value;                                                                                                           \
   }

// Build all util get functions that allow you to define a default if the item is not in the json stream.
UTIL_GET_WITH_DEFAULT_FUNCTIONS

#undef ADD_FUNCTION
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild


#endif //__JSON_H__