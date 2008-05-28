/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef P_COMPILER_H
#define P_COMPILER_H


#include "p_config.h"

#include <stdlib.h>
#include <string.h>


#if defined(_WIN32) && !defined(__WIN32__)
#define __WIN32__
#endif

#if defined(_MSC_VER) && !defined(__MSC__)
#define __MSC__
#endif


#if defined(__MSC__)

/* Avoid 'expression is always true' warning */
#pragma warning(disable: 4296)

#endif /* __MSC__ */


typedef unsigned int       uint;
typedef unsigned char      ubyte;
typedef unsigned char      boolean;
typedef unsigned short     ushort;
typedef unsigned long long uint64;


#if defined(__MSC__)

typedef char               int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef long               int32_t;
typedef unsigned long      uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

#if defined(_WIN64)
typedef __int64            intptr_t;
typedef unsigned __int64   uintptr_t;
#else
typedef int                intptr_t;
typedef unsigned int       uintptr_t;
#endif

#else
#include <stdint.h>
#endif


#define TRUE  1
#define FALSE 0


/* Function inlining */
#if defined(__GNUC__)
#  define INLINE __inline__
#elif defined(__MSC__)
#  define INLINE __inline
#elif defined(__ICL)
#  define INLINE __inline
#elif defined(__INTEL_COMPILER)
#  define INLINE inline
#elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#  define INLINE __inline
#else
#  define INLINE
#endif


/* This should match linux gcc cdecl semantics everywhere, so that we
 * just codegen one calling convention on all platforms.
 */
#ifdef WIN32
#define PIPE_CDECL __cdecl
#else
#define PIPE_CDECL
#endif



#if defined __GNUC__
#define ALIGN16_DECL(TYPE, NAME, SIZE)  TYPE NAME##___aligned[SIZE] __attribute__(( aligned( 16 ) ))
#define ALIGN16_ASSIGN(NAME) NAME##___aligned
#define ALIGN16_ATTRIB  __attribute__(( aligned( 16 ) ))
#else
#define ALIGN16_DECL(TYPE, NAME, SIZE)  TYPE NAME##___unaligned[SIZE + 1]
#define ALIGN16_ASSIGN(NAME) align16(NAME##___unaligned)
#define ALIGN16_ATTRIB
#endif



/** 
 * For calling code-gen'd functions, phase out in favor of
 * PIPE_CDECL, above, which really means cdecl on all platforms, not
 * like the below...
 */
#if !defined(XSTDCALL) 
#if defined(WIN32)
#define XSTDCALL __stdcall      /* phase this out */
#else
#define XSTDCALL                /* XXX: NOTE! not STDCALL! */
#endif
#endif


#endif /* P_COMPILER_H */
