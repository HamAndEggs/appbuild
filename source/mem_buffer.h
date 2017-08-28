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

#ifndef __MEM_BUFFER_H__
#define __MEM_BUFFER_H__

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

template <class MEMORY_TYPE,int startSize = 16,int OverAllocate = 16>class MemoryBuffer
{
public:
	MemoryBuffer():mArray(NULL),mSize(0)
	{
		data(startSize);
	}

	~MemoryBuffer()
	{
		delete []mArray;
	}

	/*
	 * The size of the array. The actual buffer maybe bigger.
	 */
	int size()const{return mSize;}

	/*
	 * Returns a buffer that you can use in the old school C way.
	 * Exsisting values may or may not be lost. It will reallocate if it needs too.
	 */
	MEMORY_TYPE* data(int pLength)
	{
		if( mSize < pLength )
		{
			mSize = pLength + OverAllocate;
			delete []mArray;
			mArray = new MEMORY_TYPE[mSize];
		}
		return mArray;
	}

private:
	MEMORY_TYPE* mArray;
	int mSize;//This is the size in number of 'types' and not byte size.
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild

#endif //#ifndef __MISC_H__
