//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//

#ifndef _COMMON_INCLUDED_
#define _COMMON_INCLUDED_

#ifdef _WIN32
    #include <basetsd.h>
#elif defined (solaris)
    #include <sys/int_types.h>
    #define UINT_PTR uintptr_t
#else
    #include <stdint.h>
    #define UINT_PTR uintptr_t
#endif

/* windows only pragma */
#ifdef _MSC_VER
    #pragma warning(disable : 4786) // Don't warn about too long identifiers
    #pragma warning(disable : 4514) // unused inline method
    #pragma warning(disable : 4201) // nameless union
#endif

//
// Doing the push and pop below for warnings does not leave the warning state
// the way it was.  This seems like a defect in the compiler.  We would like
// to do this, but since it does not work correctly right now, it is turned
// off.
//
//??#pragma warning(push, 3)

	#include <set>
    #include <vector>
    #include <map>
    #include <list>
    #include <string>
    #include <stdio.h>

//??#pragma warning(pop)

typedef int TSourceLoc;

#include <assert.h>
#include "PoolAlloc.h"

//
// Put POOL_ALLOCATOR_NEW_DELETE in base classes to make them use this scheme.
//
#define POOL_ALLOCATOR_NEW_DELETE(A)                                  \
    void* operator new(size_t s) { return (A).allocate(s); }          \
    void* operator new(size_t, void *_Where) { return (_Where);	}     \
    void operator delete(void*) { }                                   \
    void operator delete(void *, void *) { }                          \
    void* operator new[](size_t s) { return (A).allocate(s); }        \
    void* operator new[](size_t, void *_Where) { return (_Where);	} \
    void operator delete[](void*) { }                                 \
    void operator delete[](void *, void *) { }

#ifdef _M_AMD64
//
// The current version of STL that comes with the PSDK (as required for the AMD64 compiler)
// has a very old version of the STL which is very out of date.  As a result, various additions needed
// making to it to get the compilers compiling!
//

//
// A new version of the Map template class - the operator[] now returns the correct type reference
//
template <class _K, class _Ty, class _Pr = std::less<_K>, class _A = std::allocator<_Ty> >
class TBaseMap : public std::map <_K, _Ty, _Pr, _A >
{
public :
	_Ty& operator[] (const _K& _Kv)
	{
		iterator _P = insert(value_type(_Kv, _Ty())).first;
		return ((*_P).second); 
	}
	
	explicit TBaseMap(const _Pr& _Pred = _Pr(), const _A& _Al = _A())
		: std::map<_K, _Ty, _Pr, _A >(_Pred, _Al) {};


};

//
// A new version of the List template class - the begin function now checks for NULL to eliminate access violations
//
template <class _Ty, class _A = std::allocator<_Ty> >
class TBaseList : public std::list <_Ty, _A >
{
public :
	iterator begin()
	{
		return (iterator(_Head == 0 ? 0 : _Acc::_Next(_Head))); 
	}
	
	const_iterator begin() const
	{
		return (const_iterator(_Head == 0 ? 0 : _Acc::_Next(_Head))); 
	}
	
	// 
	// These are required - apparently!
	//
	explicit TBaseList(const _A& _Al = _A()) 
		: std::list<_Ty, _A >(_Al) {};
	explicit TBaseList(size_type _N, const _Ty& _V = _Ty(),	const _A& _Al = _A())
		: std::list<_Ty, _A >(N, _V, _Al) {};

};

//
// A new version of the set class - this defines the required insert method
//
template<class _K, class _Pr = std::less<_K>, class _A = std::allocator<_K> >
class TBaseSet : public std::set <_K, _Pr, _A>
{
public :

	//
	// This method wasn't defined
	//
	template<class _Iter>
	void insert(_Iter _First, _Iter _Last)
	{	// insert [_First, _Last)
		for (; _First != _Last; ++_First)
			this->insert(*_First);
	}
	
	// 
	// These methods were not resolved if I declared the previous method??
	//
	_Pairib insert(const value_type& _X)
	{
		_Imp::_Pairib _Ans = _Tr.insert(_X);
		return (_Pairib(_Ans.first, _Ans.second)); 
	}
	
	iterator insert(iterator _P, const value_type& _X)
	{
		return (_Tr.insert((_Imp::iterator&)_P, _X)); 
	}
	
	void insert(_It _F, _It _L)
	{
		for (; _F != _L; ++_F)
			_Tr.insert(*_F); 
	}

};

#else

#define TBaseMap std::map
#define TBaseList std::list
#define TBaseSet std::set

#endif //_M_AMD64

//
// Pool version of string.
//
typedef pool_allocator<char> TStringAllocator;
typedef std::basic_string <char, std::char_traits<char>, TStringAllocator > TString;
inline TString* NewPoolTString(const char* s)
{
	void* memory = GlobalPoolAllocator.allocate(sizeof(TString));
	return new(memory) TString(s);
}

//
// Pool allocator versions of vectors, lists, and maps
//
template <class T> class TVector : public std::vector<T, pool_allocator<T> > {
public:
    typedef typename std::vector<T, pool_allocator<T> >::size_type size_type;
    TVector() : std::vector<T, pool_allocator<T> >() {}
    TVector(const pool_allocator<T>& a) : std::vector<T, pool_allocator<T> >(a) {}
    TVector(size_type i): std::vector<T, pool_allocator<T> >(i) {}
};

template <class T> class TList   : public TBaseList  <T, pool_allocator<T> > {
public:
    typedef typename TBaseList<T, pool_allocator<T> >::size_type size_type;
    TList() : TBaseList<T, pool_allocator<T> >() {}
    TList(const pool_allocator<T>& a) : TBaseList<T, pool_allocator<T> >(a) {}
    TList(size_type i): TBaseList<T, pool_allocator<T> >(i) {}
};

// This is called TStlSet, because TSet is taken by an existing compiler class.
template <class T, class CMP> class TStlSet : public std::set<T, CMP, pool_allocator<T> > {
    // No pool allocator versions of constructors in std::set.
};


template <class K, class D, class CMP = std::less<K> > class TMap :
    public TBaseMap<K, D, CMP, pool_allocator<std::pair<K, D> > > {
public:
    typedef pool_allocator<std::pair <K, D> > tAllocator;

    TMap() : TBaseMap<K, D, CMP, tAllocator >() {}
/*
    TMap(const tAllocator& a) : TBaseMap<K, D, CMP, tAllocator >(key_compare(), a) {}
*/
    TMap(const tAllocator& a) : TBaseMap<K, D, CMP, tAllocator >() {}
};

//
// Persistent string memory.  Should only be used for strings that survive
// across compiles/links.
//
typedef std::basic_string<char> TPersistString;

//
// templatized min and max functions.
//
template <class T> T Min(const T a, const T b) { return a < b ? a : b; }
template <class T> T Max(const T a, const T b) { return a > b ? a : b; }

//
// Create a TString object from an integer.
//
inline const TString String(const int i, const int base = 10)
{
    char text[16];     // 32 bit ints are at most 10 digits in base 10
    
    #ifdef _WIN32
        itoa(i, text, base);
    #else
        // we assume base 10 for all cases
        sprintf(text, "%d", i);
    #endif

    return text;
}

const unsigned int SourceLocLineMask = 0xffff;
const unsigned int SourceLocStringShift = 16;

__inline TPersistString FormatSourceLoc(const TSourceLoc loc)
{
    char locText[64];

    int string = loc >> SourceLocStringShift;
    int line = loc & SourceLocLineMask;

    if (line)
        sprintf(locText, "%d:%d", string, line);
    else
        sprintf(locText, "%d:? ", string);

    return TPersistString(locText);
}
typedef TMap<TString, TString> TPragmaTable;
typedef TMap<TString, TString>::tAllocator TPragmaTableAllocator;

#endif // _COMMON_INCLUDED_
