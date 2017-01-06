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

#ifndef _JSON_READER_H_
#define _JSON_READER_H_

#include <map>
#include <vector>
#include <string>


namespace appbuild{
//////////////////////////////////////////////////////////////////////////
class JSONObject;
class JSONValue;

typedef std::vector<JSONValue*> JSONValueVec;

class JSONValue
{
public:
	// The types that I support. Sort of fit with the standard but I have added some more friendly ones.
	enum Type
	{
		STRING,
		INT32,	// Signed 32 bit int.
		FLOAT,	// Box standard floating point number.
		BOOLEAN,
		OBJECT,
		ARRAY,
		NULL_VALUE,
	};

	JSONValue(){};
	virtual ~JSONValue(){};

	virtual Type GetType()const = 0;
	virtual int GetArraySize()const = 0;

	// pIndex can only be non zero for array types. And then the element in the array has to match the type requested. Some types maybe interchangable. And int can be read as a float.
	virtual  const char* GetString(int pIndex = 0)const = 0;
	virtual  const int GetInt32(int pIndex = 0)const = 0;
	virtual  const float GetFloat(int pIndex = 0)const = 0;
	virtual  const bool GetBoolean(int pIndex = 0)const = 0;
	virtual  const JSONObject* GetObject(int pIndex = 0)const = 0;

	// Only works if the value is a JSONObject.
	virtual const JSONValue* Find(const char* pName)const = 0;	// Returns NULL if not found.
};

class Compare
{
public:
	bool operator() (const char* lhs, const char* rhs) const
	{
		assert(lhs && rhs);
		while( *lhs && *rhs && tolower(*lhs) == tolower(*rhs) )
		{
			lhs++;
			rhs++;
		};

		return tolower(*lhs) < tolower(*rhs);
	}
};
typedef std::map<const char*,JSONValueVec,Compare> ValueMap;


class JSONObject
{
public:


	virtual ~JSONObject(){};

	// If key not found, vector will be null. Else returns all the elements in this object that use this key.
	virtual const JSONValue*	Find(const char* pName)const=0;
	virtual const JSONValueVec& FindAll(const char* pName)const=0;

	virtual const ValueMap& GetChildren()const = 0;
};

class Json
{
public:
	Json(std::ifstream& pFile);
	Json(const std::string& pString);// A Json string, EG {"name":"fred","age":21}
	~Json();

	const JSONValue*	Find(const char* pName)const;		// Returns NULL if not found.
	const JSONValueVec& FindAll(const char* pName)const;	// Returns an empty vector if not found.

	operator bool()const{return mFileData && mRoot;}

private:
	const JSONObject* Read(const std::string& pJson);

	const JSONObject *mRoot;
	char* mFileData;	//Copy of the incoming JSON so that I can modify it.
	const JSONValueVec EmptyVec;
};


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif
