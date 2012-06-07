/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef H_PARRAY
#define H_PARRAY

#include <stdarg.h>
#include <stdio.h>

#include "MiniFlashTypes.h"
#include "MiniFlashAssert.h" 

template <class T> class PArray {
public:

	static const uint32_t InvalidIndex = (unsigned int)(-1); 
	
	PArray ( size_t maxsize=0 ) {
		mem = 0;		
		Allocate ( maxsize );
	}

	~PArray ( ) {
		Free ( );
	}

	void Free() { 
		Reset ( );
		delete [] mem;
		mem=0; 		 
		maxlength=0; 		
	}

	void Reset() {
		length = 0;	
	}

	bool Grow ( size_t maxsize ) { 
		if ( maxlength >= maxsize ) return true;
		T *newmem = new T[maxsize];
		PLAYERASSERT ( newmem );
		if ( length )
			memcpy ( newmem, mem, sizeof(T)*length );
		delete [] mem;
		mem = newmem;
		maxlength=maxsize; 		
		return true;
	}	

	void Trim ( ) {
		if ( maxlength == length || length == 0 ) return;
		T *newmem = new T[length];
		PLAYERASSERT ( newmem );
		memcpy ( newmem, mem, sizeof(T)*length );
		delete [] mem;
		mem = newmem;
		maxlength=length;
	}

	void SwapBack ( uint32_t i ) {
		PLAYERASSERT(length>0 && i<length );
		mem[i] = mem[length-1];
		length--;
	}

	void Pop ( ) {
		PLAYERASSERT(length>0);
		length--;
	}

	void PushUnique ( T &value ) {
		if ( Find(value)<length ) return;
		if ( length==maxlength ) {
			EnsureSpace ( 1 );			
		}
		mem[length] = value;
		length++;
	}

	void PushByValue ( T value ) {
		if ( length==maxlength ) {
			EnsureSpace ( 1 );
		}			
		mem[length] = value;
		length++;
	}

	void Push ( T &value ) {
		if ( length==maxlength ) {
			EnsureSpace ( 1 );
		}			
		mem[length] = value;
		length++;
	}

	T &Last() const {
		PLAYERASSERT(length);
		return mem[length-1];
	}

	T &First() const {
		PLAYERASSERT(length);
		return mem[0];
	}

	T &operator[] ( uint32_t i ) const {
		PLAYERASSERT(i<length);
		return mem[i];
	}

	bool EnsureSpace ( size_t nmoare ) {		
		if ( nmoare+length < maxlength ) return true;
		size_t news = nmoare+length;
		if ( news<16 ) news = 16;
		size_t grows = maxlength<<1;//+256 //(maxlength*3)>>1;
		if ( news<grows ) news = grows;
		return Grow ( news );
	}

	bool Allocate ( size_t maxsize ) { 
		Free(); 
		if ( maxsize ) { 			
			mem = new T[maxsize];
			maxlength=maxsize; 			
		} 		
		return true;
	}

	size_t Count ( ) const {
		return length;
	}

	bool IsEmpty ( ) const {
		return length==0;
	}

	uint32_t Find ( T &value ) const {
		for ( uint32_t i=0; i<length; i++ )
			if ( mem[i]==value ) return i;
		//PLAYERASSERT(false);
		return InvalidIndex;
	}

	uint32_t Insert ( T &value, uint32_t index ) {
		PLAYERASSERT(index<=length && length<maxlength);
		for ( uint32_t i=(uint32_t)length; i>index; i-- )
			mem[i] = mem[i-1];
		mem[index] = value;
		length++;
		return index;
	}

	void Remove ( uint32_t index ) {
		PLAYERASSERT(length>0 && index<length);
		for ( uint32_t i=index+1; i<length; i++ )
			mem[i-1] = mem[i];
		length--;
	}

	size_t MemoryUsed ( ) const {
		return maxlength * sizeof(T);
	}

	bool AppendArray ( const T *src, size_t n ) {
		PLAYERASSERT ( src && n );
		if ( !EnsureSpace ( n ) ) return false;
		memcpy ( mem+length, src, n*sizeof(T) );
		length += n;
		return true;
	}

	bool IsEqual ( const PArray<T> &other ) {
		if ( other.length != length ) return false;
		for ( size_t i=0; i<length; i++ )
			if ( mem[i] != other.mem[i] ) return false;
		return true; 
	}

	PArray<T> * MakeDeepCopy ( ) {
		PArray<T> *r = new PArray<T>; 
		if ( length ) r->AppendArray ( mem, uint32_t(length&0xFFFFFFFF) );
		return r; 
	}

	T *mem;
	size_t maxlength;
	size_t length;	
};

#endif // H_PARRAY

