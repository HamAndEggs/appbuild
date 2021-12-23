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

#ifndef __STRING_TYPES_H__
#define __STRING_TYPES_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>

// This is here to allow to_string to be used in macros where the input type could be a basic type or a more complex one, such as a string.
// I can call std::to_string and not worry about the type by doing this.
namespace std
{
   inline const std::string& to_string(const std::string& pString){return pString;}
};

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
typedef std::vector<std::string> StringVec;
typedef std::map<std::string,std::string> StringMap;
typedef std::set<std::string> StringSet;
typedef std::stack<std::string> StringStack;
typedef std::map<std::string,StringVec> StringVecMap;
typedef std::map<std::string,StringSet> StringSetMap;
typedef std::map<std::string,int> StringIntMap;

inline StringVec GetKeys(const StringMap& pMap)
{
   StringVec keys;

   for( const auto& key : pMap )
      keys.push_back(key.first);

   return keys;
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif
