/* $Id: teximage.h,v 1.2 1999/11/03 17:27:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


#ifndef TEXIMAGE_H
#define TEXIMAGE_H


#include "types.h"


/*** Internal functions ***/


extern struct gl_texture_image *gl_alloc_texture_image( void );


extern void gl_free_texture_image( struct gl_texture_image *teximage );




/*** API entry points ***/


extern void gl_TexImage1D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLint border, GLenum format,
                           GLenum type, const GLvoid *pixels );


extern void gl_TexImage2D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLsizei height, GLint border,
                           GLenum format, GLenum type,
                           const GLvoid *pixels );


extern void gl_TexImage3D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels );


extern void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, GLvoid *pixels );



extern void gl_TexSubImage1D( GLcontext *ctx,
                              GLenum target, GLint level, GLint xoffset,
                              GLsizei width, GLenum format, GLenum type,
                              const GLvoid *pixels );


extern void gl_TexSubImage2D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type,
                              const GLvoid *pixels );


extern void gl_TexSubImage3D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLenum format, GLenum type,
                              const GLvoid *pixels );


extern void gl_CopyTexImage1D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat,
                               GLint x, GLint y,
                               GLsizei width, GLint border );


extern void gl_CopyTexImage2D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat, GLint x, GLint y,
                               GLsizei width, GLsizei height,
                               GLint border );


extern void gl_CopyTexSubImage1D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint x, GLint y,
                                  GLsizei width );


extern void gl_CopyTexSubImage2D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLint x, GLint y,
                                  GLsizei width, GLsizei height );


extern void gl_CopyTexSubImage3D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLint zoffset,
                                  GLint x, GLint y,
                                  GLsizei width, GLsizei height );

#endif

