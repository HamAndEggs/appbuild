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

#include <assert.h>
#include "json_writer.h"


namespace appbuild{
//////////////////////////////////////////////////////////////////////////
JsonWriter::JsonWriter()
{
	mIndent = 0;
}

JsonWriter::~JsonWriter()
{
	assert(mIndent == 0);
}

void JsonWriter::StartObject(const char* pName)
{
	if( pName )
	{
		json << seperator;
		AddTabs();	
		json << "\"" << pName << "\":";
	}

	json << "\n";
	AddTabs();	
	json << "{";
	seperator = "\n";
	mIndent++;
}

void JsonWriter::EndObject()
{
	mIndent--;
	json << "\n";
	AddTabs();	
	json << "}";
}

void JsonWriter::AddObjectItem(const char* pName,const char* pValue)
{
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":\"" << pValue << "\"";
	seperator = ",\n";
}

void JsonWriter::AddObjectItem(const char* pName,const double pValue)
{
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":" << pValue;
	seperator = ",\n";
}

void JsonWriter::AddObjectItem(const char* pName,const float pValue)
{
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":" << pValue;
	seperator = ",\n";
}

void JsonWriter::AddObjectItem(const char* pName,const int pValue)
{
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":" << pValue;
	seperator = ",\n";
}

void JsonWriter::AddObjectItem(const char* pName,const bool pValue)
{
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":" << pValue?"true":"false";
	seperator = ",\n";
}

void JsonWriter::StartArray(const char* pName)
{
	assert(pName);
	json << seperator;
	AddTabs();	
	json << "\"" << pName << "\":\n";
	AddTabs();	
	json << "[";
	seperator = "\n";
	mIndent++;
}

void JsonWriter::EndArray()
{
	mIndent--;
	json << "\n";
	AddTabs();	
	json << "]";
}

void JsonWriter::AddArrayItem(const char* pValue)
{
	assert(pValue);
	json << seperator;
	AddTabs();	
	json << "\"" << pValue << "\"";
	seperator = ",\n";
}

void JsonWriter::AddArrayItem(const double pValue)
{
	json << seperator;
	AddTabs();	
	json << pValue << ",\n";
	seperator = ",\n";
}

void JsonWriter::AddArrayItem(const float pValue)
{
	json << seperator;
	AddTabs();	
	json << pValue << ",\n";
	seperator = ",\n";
}

void JsonWriter::AddArrayItem(const int pValue)
{
	json << seperator;
	AddTabs();	
	json << pValue << ",\n";
	seperator = ",\n";
}

void JsonWriter::AddArrayItem(const bool pValue)
{
	json << seperator;
	AddTabs();	
	json << pValue?"true":"false";
	seperator = ",\n";
}

void JsonWriter::AddTabs()
{
	for(int n = 0 ; n < mIndent ; n++ )
		json << "\t";
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
