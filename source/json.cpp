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

#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>

#include "json.h"


namespace appbuild{
//////////////////////////////////////////////////////////////////////////
class JSONValuePrivate : public JSONValue
{
public:
	JSONValuePrivate();
	virtual ~JSONValuePrivate();

	virtual Type GetType()const{return theType;}
	virtual Type GetType(int pIndex)const;

	void Read(char* json,size_t& pos,size_t size);
	virtual int GetArraySize()const{return data.Array != NULL ? data.Array->size() : 0;}

	// pIndex can only be non zero for array types. And then the element in the array has to match the type requested. Some types maybe interchangable. And int can be read as a float.
	virtual const char* GetString(int pIndex = 0)const;
	virtual const int GetInt32(int pIndex = 0)const;
	virtual const float GetFloat(int pIndex = 0)const;
	virtual const bool GetBoolean(int pIndex = 0)const;
	virtual const JSONObject* GetObject(int pIndex = 0)const;

	// Only works if the value is a JSONObject.
	virtual const JSONValue* Find(const char* pName)const;	// Returns NULL if not found.

private:
	bool CompareNoCase(const char* pA,const char* pB,int pLength);

	Type theType;
	union TheData
	{
		TheData(){_reserved = 0;}
		const char* String;
		int Int;
		float Float;
		bool Boolean;
		const JSONObject* Object;
		JSONValueVec* Array;
		uint64_t _reserved;
	}data;

};

// Written with simplicity in mind not speed. Having said that unless you're reading mega bytes of data with 1000's of objects I expect you'll never notice.
// Most JSON is from web sites and only a few k in size. The only possible 'slow' bits is the object and std::list / std::map stuff. Don't sweat it. it's very quick on a Raspberry Pi.
class JSONObjectPrivate : public JSONObject
{

public:
	JSONObjectPrivate(char* json,size_t& pos,size_t size);
	virtual ~JSONObjectPrivate();

	virtual const JSONValue*	Find(const char* pName)const;
	virtual const JSONValueVec& FindAll(const char* pName)const;

	virtual const ValueMap& GetChildren()const{return values;}
private:

	bool operator() (const char* lhs, const char* rhs) const;


	ValueMap values;
	const JSONValueVec EmptyVec;

};

JSONValuePrivate::JSONValuePrivate():theType(NULL_VALUE)
{
	assert( sizeof(data) == sizeof(data._reserved) );
}

JSONValuePrivate::~JSONValuePrivate()
{
	switch( theType )
	{
    case NULL_VALUE:
	case STRING:
	case INT32:
	case FLOAT:
	case BOOLEAN:
		break;

	case OBJECT:
		delete data.Object;
		break;

	case ARRAY:
		for (std::vector<JSONValue*>::iterator it = data.Array->begin(); it != data.Array->end(); it++)
		{
			delete *it;
		}
		delete data.Array;
		break;
	}
}

void JSONValuePrivate::Read(char* json,size_t& pos,size_t size)
{
	// Now scan to the first no white space, from that point we can test for a string, number or special word (true, false, null)
	for( ; pos < size && isspace(json[pos]) ; pos++ );
	if( pos == size )return; //Bad JSON file!

	if( json[pos] == '\"' )
	{// We have a string
		pos++;
		size_t end = 0;
		for( end = pos ; end < size && json[end] != '\"' ; end++ );
		if( end == size )return; //Bad JSON file!

		theType = STRING;
		data.String = json + pos;
		json[end] = 0;
		pos = end+1;
	}
	else if( json[pos] == '{' )
	{
		// Starting a new object.
		theType = OBJECT;
		data.Object = new JSONObjectPrivate(json,pos,size);
	}
	else if( json[pos] == '[' )
	{// Reading an array. Which is value after value, the value can not be an name:value. But can be an object {name:value}
		pos++;
		data.Array = new std::vector<JSONValue*>();
		theType = ARRAY;

		// Scan to the first no white space.
		do
		{
			for( ; pos < size && (isspace(json[pos]) || json[pos] == ',') ; pos++ );
			if( pos == size )return; //Bad JSON file!

			if( json[pos] == ']' )
				break; //We are done.

			JSONValuePrivate* newValue = new JSONValuePrivate();
			newValue->Read(json,pos,size);
			data.Array->push_back(newValue);
		}while(pos < size);
	}
	else if( CompareNoCase(json + pos,"true",4) )
	{
		theType = BOOLEAN;
		data.Boolean = true;
		pos += 4;
	}
	else if( CompareNoCase(json + pos,"false",5) )
	{
		theType = BOOLEAN;
		data.Boolean = false;
		pos += 5;
	}
	else if( CompareNoCase(json + pos,"null",5) )
	{
		theType = NULL_VALUE;
		data._reserved = 0;
		pos += 5;
	}
	else
	{// has to be a number then.
		bool isDouble = false;
		size_t end = pos;
		for(  ; end < size && json[end] != ',' && isspace(json[end]) == false ; end++ )
		{
			if( json[pos] == '.' )
			{
				isDouble = true;
			}
		}
		if( pos == size )return; //Bad JSON file!

		if( isDouble )
		{
			theType = FLOAT;
			sscanf(json + pos,"%f",&data.Float);
		}
		else
		{
			theType = INT32;
			sscanf(json + pos,"%d",&data.Int);
		}
		pos = end;
	}
}

// pIndex can only be non zero for array types. And then the element in the array has to match the type requested. Some types maybe interchangable. And int can be read as a float.
JSONValue::Type JSONValuePrivate::GetType(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetType();
	}
	return GetType();
}

const char* JSONValuePrivate::GetString(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetString();
	}

	assert( GetType() == STRING );
	return data.String;
}

const int JSONValuePrivate::GetInt32(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetInt32();
	}

	assert( GetType() == INT32 );
	return data.Int;
}

const float JSONValuePrivate::GetFloat(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetFloat();
	}

	assert( GetType() == FLOAT );
	return data.Float;
}

const bool JSONValuePrivate::GetBoolean(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetBoolean();
	}

	assert( GetType() == BOOLEAN );
	return data.Boolean;
}

const JSONObject* JSONValuePrivate::GetObject(int pIndex)const
{
	if( GetType() == ARRAY )
	{
		return (*data.Array)[pIndex]->GetObject();
	}

	assert( GetType() == OBJECT );
	return data.Object;

}

// Only works if the value is a JSONObject.
const JSONValue* JSONValuePrivate::Find(const char* pName)const
{
	assert( data.Object );
	assert( GetType() == OBJECT );
	if( data.Object && GetType() == OBJECT )
		return data.Object->Find(pName);
	return NULL;
}

bool JSONValuePrivate::CompareNoCase(const char* pA,const char* pB,int pLength)
{
	assert( pA != NULL || pB != NULL );// Note only goes pop if both are null.
// If either or both NULL, then say no. A bit like a divide by zero as null strings are not strings.
	if( pA == NULL || pB == NULL )
		return false;

// If same memory then yes they match, doh!
	if( pA == pB )
		return true;

	if( pLength <= 0 )
		pLength = 0x7fffffff;

	while( (*pA != 0 || *pB != 0) && pLength > 0 )
	{
		// Get here are one of the strings has hit a null then not the same.
		// The while loop condition would not allow us to get here if both are null.
		if( *pA == 0 || *pB == 0 )
		{// Check my assertion above that should not get here if both are null. Note only goes pop if both are null.
			assert( pA != NULL || pB != NULL );
			return false;
		}

		if( tolower(*pA) != tolower(*pB) )
			return false;

		pA++;
		pB++;
		pLength--;
	};

	// Get here, they are the same.
	return true;
}

//////////////////////////////////////////////////////////////////////////
// JSON Object code.
//////////////////////////////////////////////////////////////////////////
JSONObjectPrivate::JSONObjectPrivate(char* json,size_t& pos,size_t size)
{
	assert( json[pos] == '{' );
	size_t end = 0;
	// Now scan to the name of the next object.
	while( pos < size )
	{
		switch( json[pos] )
		{
		case '\"':
			{
				pos++;
				// Scan to end of string.
				for( end = pos ; end < size && json[end] != '\"' ; end++ );
				if( end == size )return; //Bad JSON file!
				const char* name = json + pos;
				json[end] = 0;
				pos = end+1;

				// Scan to start of the value.
				for( ; pos < size && json[pos] != ':' ; pos++ );
				if( pos == size )return; //Bad JSON file!

				pos++; // Skip : char.

				// Done like this so I don't need a copy constructor which will slow everything down with extra memory copies etc....
				JSONValuePrivate* val = new JSONValuePrivate();
				val->Read(json,pos,size);//Load it.
				values[name].push_back(val);
			}
			break;

		case '}':
			pos++;
			return;	// all done.

		default:
			// Next please
			pos++;
			break;
		}
	};
	// get here means we hit the end of the data. Whoops.
}

JSONObjectPrivate::~JSONObjectPrivate()
{
	for(ValueMap::const_iterator it = values.begin() ; it != values.end() ; ++it )
	{
		for(JSONValueVec::const_iterator val = it->second.begin() ; val != it->second.begin() ; ++val )
		{
			delete (*val);
		}
	}
}

const JSONValue* JSONObjectPrivate::Find(const char* pName)const
{
	const JSONValueVec& vec = FindAll(pName);
	if( vec.size() > 0 )
		return vec.front();
	return NULL;
}

const JSONValueVec& JSONObjectPrivate::FindAll(const char* pName)const
{
	ValueMap::const_iterator v = values.find(pName);
	if( v == values.end() )
		return EmptyVec;

	return v->second;
}

Json::Json(std::ifstream& pFile):mRoot(NULL),mFileData(NULL)
{
	if( pFile.is_open() )
	{
		std::stringstream s;
		s << pFile.rdbuf();
		mRoot = Read(s.str());
	}
}

Json::Json(const std::string& pString):mRoot(NULL),mFileData(NULL)
{
	mRoot = Read(pString);
}

Json::~Json()
{
	delete mRoot;
	delete []mFileData;
}

const JSONValue*	Json::Find(const char* pName)const
{
	if( mRoot )
		return mRoot->Find(pName);
	return NULL;
}

const JSONValueVec& Json::FindAll(const char* pName)const
{
	if( mRoot )
		return mRoot->FindAll(pName);
	return EmptyVec;
}

const JSONObject* Json::Read(const std::string& pJson)
{
	// Check first that it could be a json file.
	size_t pos = 0;// Used as the starting point of the parsing too.
	for( ; pos < pJson.size() && isspace(pJson[pos]) ; pos++ );

	if( pos >= pJson.size() || pJson[pos] != '{' )
	{
		return NULL;// bad JSON file.
	}

	// Take a copy as I modify it as I go inserting nulls. Saves a lot of memory allocations for strings.
	size_t size = pJson.size();
	mFileData = new char[size+1];
	strncpy(mFileData,pJson.c_str(),size);
	mFileData[size] = 0;

	// Start at the root, this is where the real work starts.
	return new JSONObjectPrivate(mFileData,pos,size);
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{
