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

#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#else
#include "xf86_ansic.h"
#include "xf86_libc.h"
#endif
#include <stddef.h>
#include <stdarg.h>


#if defined(_WIN32) && !defined(__WIN32__)
#define __WIN32__
#endif

#if defined(_MSC_VER)

/* Avoid 'expression is always true' warning */
#pragma warning(disable: 4296)

#endif /* _MSC_VER */


/*
 * Alternative stdint.h and stdbool.h headers are supplied in include/c99 for
 * systems that lack it.
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif
#include <stdint.h>
#include <stdbool.h>


#if !defined(__HAIKU__) && !defined(__USE_MISC)
typedef unsigned int       uint;
typedef unsigned short     ushort;
#endif
typedef unsigned char      ubyte;

typedef unsigned char boolean;
#ifndef TRUE
#define TRUE  true
#endif
#ifndef FALSE
#define FALSE false
#endif


/* Function inlining */
#ifndef INLINE
#  ifdef __cplusplus
#    define INLINE inline
#  elif defined(__GNUC__)
#    define INLINE __inline__
#  elif defined(_MSC_VER)
#    define INLINE __inline
#  elif defined(__ICL)
#    define INLINE __inline
#  elif defined(__INTEL_COMPILER)
#    define INLINE inline
#  elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#    define INLINE __inline
#  elif defined(__SUNPRO_C) && defined(__C99FEATURES__)
#    define INLINE inline
#  elif (__STDC_VERSION__ >= 199901L) /* C99 */
#    define INLINE inline
#  else
#    define INLINE
#  endif
#endif


/* Function visibility */
#ifndef PUBLIC
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PUBLIC __attribute__((visibility("default")))
#  else
#    define PUBLIC
#  endif
#endif


/* The __FUNCTION__ gcc variable is generally only used for debugging.
 * If we're not using gcc, define __FUNCTION__ as a cpp symbol here.
 */
#ifndef __FUNCTION__
# if !defined(__GNUC__)
#  if (__STDC_VERSION__ >= 199901L) /* C99 */ || \
    (defined(__SUNPRO_C) && defined(__C99FEATURES__))
#   define __FUNCTION__ __func__
#  else
#   define __FUNCTION__ "<unknown>"
#  endif
# endif
# if defined(_MSC_VER) && _MSC_VER < 1300
#  define __FUNCTION__ "<unknown>"
# endif
#endif



/* This should match linux gcc cdecl semantics everywhere, so that we
 * just codegen one calling convention on all platforms.
 */
#ifdef _MSC_VER
#define PIPE_CDECL __cdecl
#else
#define PIPE_CDECL
#endif



#if defined(__GNUC__)
#define PIPE_DEPRECATED  __attribute__((__deprecated__))
#else
#define PIPE_DEPRECATED
#endif



/* Macros for data alignment. */
#if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))

/* See http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Type-Attributes.html */
#define PIPE_ALIGN_TYPE(_alignment, _type) _type __attribute__((aligned(_alignment)))

/* See http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Variable-Attributes.html */
#define PIPE_ALIGN_VAR(_alignment) __attribute__((aligned(_alignment)))

#if (__GNUC__ > 4 || (__GNUC__ == 4 &&__GNUC_MINOR__>1)) && !defined(PIPE_ARCH_X86_64)
#define PIPE_ALIGN_STACK __attribute__((force_align_arg_pointer))
#else
#define PIPE_ALIGN_STACK
#endif

#elif defined(_MSC_VER)

/* See http://msdn.microsoft.com/en-us/library/83ythb65.aspx */
#define PIPE_ALIGN_TYPE(_alignment, _type) __declspec(align(_alignment)) _type
#define PIPE_ALIGN_VAR(_alignment) __declspec(align(_alignment))

#define PIPE_ALIGN_STACK

#elif defined(SWIG)

#define PIPE_ALIGN_TYPE(_alignment, _type) _type
#define PIPE_ALIGN_VAR(_alignment)

#define PIPE_ALIGN_STACK

#else

#error "Unsupported compiler"

#endif



#endif /* P_COMPILER_H */
