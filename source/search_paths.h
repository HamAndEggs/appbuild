#ifndef search_paths_h__
#define search_paths_h__

#include "string_types.h"
#include "json.h"
#include "misc.h"

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
struct SearchPaths : public StringVec
{
   SearchPaths(const std::string& pProjectDir):mProjectDir(pProjectDir){}

   void AddPath(const std::string& pPath)
   {
      const std::string path = PreparePath(pPath);
      if( path.size() > 0 )
      {
         push_back(path);
      }
   }

   bool AddPaths(const rapidjson::Value& pPaths)
   {
      if( pPaths.IsArray() )
      {
         for( const auto& val : pPaths.GetArray() )
         {
            AddPath(val.GetString());
         }
         return true;
      }
      return false;
   }

   bool Add(const rapidjson::Value& pLibs)
   {
      if( pLibs.IsArray() )
      {
         for( const auto& val : pLibs.GetArray() )
         {
            push_back(val.GetString());
         }
         return true;
      }
      return false;
   }

   rapidjson::Value Write(rapidjson::Document::AllocatorType& pAllocator)const
   {
      return BuildStringArray(*this,pAllocator);
   }

private:
   const std::string mProjectDir;

   const std::string PreparePath(const std::string& pPath)
   {
      assert(pPath.size() > 0);
      if( pPath.size() > 0 )
      {
         // Before we add it, see if it's an absolute path, if not add it to the project dir, then check if it exists.
         std::string ArgPath;
         if( pPath.front() != '/' )
            ArgPath = mProjectDir;

         ArgPath += pPath;
         if( DirectoryExists(ArgPath) )
         {
            if( ArgPath.back() != '/' )
               ArgPath += "/";
            return CleanPath(ArgPath);
         }
      }
      return std::string();
   }
};
//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif // search_paths_h__