/* $Id: glheader.h,v 1.21 2001/06/15 15:22:07 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef GLHEADER_H
#define GLHEADER_H


/*
 * This is the top-most include file of the Mesa sources.
 * It includes gl.h and all system headers which are needed.
 * Other Mesa source files should _not_ directly include any system
 * headers.  This allows Mesa to be integrated into XFree86 and
 * allows system-dependent hacks/work-arounds to be collected in one place.
 *
 * If you touch this file, everything gets recompiled!
 *
 * This file should be included before any other header in the .c files.
 *
 * Put compiler/OS/assembly pragmas and macros here to avoid
 * cluttering other source files.
 */



#ifdef XFree86LOADER
#include "xf86_ansic.h"
#else
#include <assert.h>
#include <ctype.h>
/* If we can use Compaq's Fast Math Library on Alpha */
#if defined(__alpha__) && defined(CCPML)
#include <cpml.h>
#else
#include <math.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__) && defined(__i386__)
#include <fpu_control.h>
#endif
#endif
#include <float.h>


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif



#if defined(_WIN32) && !defined(__WIN32__) && !defined(__CYGWIN__)
#	define __WIN32__
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__))
#  pragma warning( disable : 4068 ) /* unknown pragma */
#  pragma warning( disable : 4710 ) /* function 'foo' not inlined */
#  pragma warning( disable : 4711 ) /* function 'foo' selected for automatic inline expansion */
#  pragma warning( disable : 4127 ) /* conditional expression is constant */
#  if defined(MESA_MINWARN)
#    pragma warning( disable : 4244 ) /* '=' : conversion from 'const double ' to 'float ', possible loss of data */
#    pragma warning( disable : 4018 ) /* '<' : signed/unsigned mismatch */
#    pragma warning( disable : 4305 ) /* '=' : truncation from 'const double ' to 'float ' */
#    pragma warning( disable : 4550 ) /* 'function' undefined; assuming extern returning int */
#    pragma warning( disable : 4761 ) /* integral size mismatch in argument; conversion supplied */
#  endif
#  if defined(_MSC_VER) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#    define GLAPI __declspec(dllexport)
#    define WGLAPI __declspec(dllexport)
#  elif defined(_MSC_VER) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#    define GLAPI __declspec(dllimport)
#    define WGLAPI __declspec(dllimport)
#  else /* for use with static link lib build of Win32 edition only */
#    define GLAPI extern
#    define WGLAPI __declspec(dllimport)
#  endif /* _STATIC_MESA support */
#  define GLAPIENTRY __stdcall
#  define GLAPIENTRYP __stdcall *
#  define GLCALLBACK __stdcall
#  define GLCALLBACKP __stdcall *
#  if defined(__CYGWIN__)
#    define GLCALLBACKPCAST *
#  else
#    define GLCALLBACKPCAST __stdcall *
#  endif
#  define GLWINAPI __stdcall
#  define GLWINAPIV __cdecl
#else
/* non-Windows compilation */
#  define GLAPI extern
#  define GLAPIENTRY
#  define GLAPIENTRYP *
#  define GLCALLBACK
#  define GLCALLBACKP *
#  define GLCALLBACKPCAST *
#  define GLWINAPI
#  define GLWINAPIV
#endif /* WIN32 / CYGWIN bracket */

/* compatability guard so we don't need to change client code */

#if defined(_WIN32) && !defined(_WINDEF_) && !defined(_GNU_H_WINDOWS32_BASE) && !defined(OPENSTEP) && !defined(__CYGWIN__)
#if 0
#	define CALLBACK GLCALLBACK
typedef void *HGLRC;
typedef void *HDC;
#endif
typedef int (GLAPIENTRY *PROC)();
typedef unsigned long COLORREF;
#endif


/* Make sure we include glext.h from gl.h */
#define GL_GLEXT_PROTOTYPES


#if defined(_WIN32) && !defined(_WINGDI_) && !defined(_GNU_H_WINDOWS32_DEFINES) && !defined(OPENSTEP)
#	define WGL_FONT_LINES      0
#	define WGL_FONT_POLYGONS   1
#ifndef _GNU_H_WINDOWS32_FUNCTIONS
#	ifdef UNICODE
#		define wglUseFontBitmaps  wglUseFontBitmapsW
#		define wglUseFontOutlines  wglUseFontOutlinesW
#	else
#		define wglUseFontBitmaps  wglUseFontBitmapsA
#		define wglUseFontOutlines  wglUseFontOutlinesA
#	endif /* !UNICODE */
#endif /* _GNU_H_WINDOWS32_FUNCTIONS */
typedef struct tagLAYERPLANEDESCRIPTOR LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, *LPLAYERPLANEDESCRIPTOR;
typedef struct _GLYPHMETRICSFLOAT GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, *LPGLYPHMETRICSFLOAT;
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;
#if !defined(GLX_USE_MESA)
#include <gl/mesa_wgl.h>
#endif
#endif


/* This is a macro on IRIX */
#ifdef _P
#undef _P
#endif



#include "GL/gl.h"
#include "GL/glext.h"


#ifndef CAPI
#define CAPI
#endif
#include <GL/internal/glcore.h>



/* Disable unreachable code warnings for Watcom C++ */
#ifdef	__WATCOMC__
#pragma disable_message(201)
#endif


/* Turn off macro checking systems used by other libraries */
#ifdef CHECK
#undef CHECK
#endif


/* Create a macro so that asm functions can be linked into compilers other
 * than GNU C
 */
#ifndef _ASMAPI
#if !defined( __GNUC__ ) && !defined( VMS )
#define _ASMAPI __cdecl
#else
#define _ASMAPI
#endif
#ifdef	PTR_DECL_IN_FRONT
#define	_ASMAPIP * _ASMAPI
#else
#define	_ASMAPIP _ASMAPI *
#endif
#endif

#ifdef USE_X86_ASM
#define _NORMAPI _ASMAPI
#define _NORMAPIP _ASMAPIP
#else
#define _NORMAPI
#define _NORMAPIP *
#endif


/* Function inlining */
#if defined(__GNUC__)
#  define INLINE __inline__
#elif defined(__MSC__)
#  define INLINE __inline
#else
#  define INLINE
#endif


/* Some compilers don't like some of Mesa's const usage */
#ifdef NO_CONST
#  define CONST
#else
#  define CONST const
#endif


#ifdef DEBUG
#  define ASSERT(X)   assert(X)
#else
#  define ASSERT(X)
#endif


/*
 * Sometimes we treat GLfloats as GLints.  On x86 systems, moving a float
 * as a int (thereby using integer registers instead of fp registers) is
 * a performance win.  Typically, this can be done with ordinary casts.
 * But with gcc's -fstrict-aliasing flag (which defaults to on in gcc 3.0)
 * these casts generate warnings.
 * The following union typedef is used to solve that.
 */
typedef union { GLfloat f; GLint i; } fi_type;


#endif /* GLHEADER_H */
