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

#ifndef H_MINIFS
#define H_MINIFS

#include "parray.h"

#define vsprintf_s vsprintf

class FlashString : public PArray<char> {
public:
	// string like stuff hack
	void Set ( const char *src ) {
		Reset ( ); 
		AppendString ( src );
	}

	void operator = (const char *src ) {
		Set(src);		
	}

	const char * CStr() { 
		return mem; 
	}

	void Format ( const char *fmt, ... ) {
		char buf[4096];
		va_list args;
		va_start (args, fmt);
		vsprintf_s (buf,fmt, args);		
		Set ( buf ); 
		va_end (args);
	}

	void AppendFormat ( const char *fmt, ... ) {
		char buf[4096];
		va_list args;
		va_start (args, fmt);
		vsprintf_s (buf,fmt, args);		
		AppendString ( buf ); 
		va_end (args);
	}

	void AppendChar ( char src ) {
		AppendFormat ( "%c", src ); 
	}

	void AppendString ( const char *src ) {
		if ( !src ) return; 
		if ( length >= 1 && mem[length-1]==0 ) length--;
		int l = 0;
		while ( src[l] ) l++; 
		AppendArray ( src, l+1 ); 
	}	
};

#endif