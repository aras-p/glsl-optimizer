/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


/**
 * \file glheader.h
 * Wrapper for GL/gl.h and GL/glext.h
 */


#ifndef GLHEADER_H
#define GLHEADER_H


#ifdef WGLAPI
#undef WGLAPI
#endif


#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__)) && !defined(BUILD_FOR_SNAP)
#  if (defined(_MSC_VER) || defined(__MINGW32__)) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#    define WGLAPI __declspec(dllexport)
#  elif (defined(_MSC_VER) || defined(__MINGW32__)) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#    define WGLAPI __declspec(dllimport)
#  else /* for use with static link lib build of Win32 edition only */
#    define WGLAPI __declspec(dllimport)
#  endif /* _STATIC_MESA support */
#endif /* WIN32 / CYGWIN bracket */


#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/internal/glcore.h"


#ifndef GL_FIXED
#define GL_FIXED 0x140C
typedef int GLfixed;
typedef int GLclampx;
#endif


#ifndef GL_OES_EGL_image
typedef void *GLeglImageOES;
#endif


#ifndef GL_OES_point_size_array
#define GL_POINT_SIZE_ARRAY_OES                                 0x8B9C
#define GL_POINT_SIZE_ARRAY_TYPE_OES                            0x898A
#define GL_POINT_SIZE_ARRAY_STRIDE_OES                          0x898B
#define GL_POINT_SIZE_ARRAY_POINTER_OES                         0x898C
#define GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES                  0x8B9F
#endif


#ifndef GL_OES_draw_texture
#define GL_TEXTURE_CROP_RECT_OES  0x8B9D
#endif


#ifndef GL_PROGRAM_BINARY_LENGTH_OES
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#endif


/**
 * Special, internal token
 */
#define GL_SHADER_PROGRAM_MESA 0x9999


#endif /* GLHEADER_H */
