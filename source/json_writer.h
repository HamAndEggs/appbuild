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

#ifndef __JSON_WRITER_H__
#define __JSON_WRITER_H__

#include <sstream>

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
class JsonWriter
{    
public:
	JsonWriter();
	~JsonWriter();

	const std::string Get()const{return json.str();}

    void StartObject(const std::string& pName){StartObject(pName.c_str());}
    void StartObject(const char* pName = NULL);
    void EndObject();

	void AddObjectItem(const std::string& pName,const std::string& pValue){AddObjectItem(pName.c_str(),pValue.c_str());}
	void AddObjectItem(const char* pName,const std::string& pValue){AddObjectItem(pName,pValue.c_str());}
	void AddObjectItem(const char* pName,const char* pValue);
	void AddObjectItem(const char* pName,const double pValue);
	void AddObjectItem(const char* pName,const float pValue);
	void AddObjectItem(const char* pName,const int pValue);
	void AddObjectItem(const char* pName,const bool pValue);

    void StartArray(const char* pName);
    void EndArray();
    
	void AddArrayItem(const std::string& pValue){AddArrayItem(pValue.c_str());}
    void AddArrayItem(const char* pValue);
    void AddArrayItem(const double pValue);
    void AddArrayItem(const float pValue);
    void AddArrayItem(const int pValue);
    void AddArrayItem(const bool pValue);


private:
	void AddTabs();

    std::stringstream json;
	std::string seperator;
	int mIndent;
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __JSON_WRITER_H__
