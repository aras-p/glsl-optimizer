
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * This file allows the GLU code to be compiled either with the Mesa
 * headers or with the real OpenGL headers.
 */


#ifndef GLUP_H
#define GLUP_H


#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>


#if defined(_WIN32) && !defined(__WIN32__)
#	define __WIN32__
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) || defined(__CYGWIN__))
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

/* compatibility guard so we don't need to change client code */

#if defined(_WIN32) && !defined(_WINDEF_) && !defined(_GNU_H_WINDOWS32_BASE) && !defined(OPENSTEP)
#	define CALLBACK GLCALLBACK
typedef int (GLAPIENTRY *PROC)();
typedef void *HGLRC;
typedef void *HDC;
typedef unsigned long COLORREF;
#endif

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
#include <gl/mesa_wgl.h>
#endif



#ifndef GLU_TESS_ERROR9
   /* If we're using the real OpenGL header files... */
#  define GLU_TESS_ERROR9	100159
#endif


#define GLU_NO_ERROR		GL_NO_ERROR


/* for Sun: */
#ifdef SUNOS4
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (char *) (DST), (char *) (SRC), (int) (BYTES) )
#else
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (void *) (DST), (void *) (SRC), (size_t) (BYTES) )
#endif


#ifndef NULL
#  define NULL 0
#endif


#endif
