/* $Id: rastpos.h,v 1.2 1999/11/11 01:22:27 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#ifndef RASTPOS_H
#define RASTPOS_H


#include "glheader.h"


extern void
_mesa_RasterPos2d(GLdouble x, GLdouble y);

extern void
_mesa_RasterPos2f(GLfloat x, GLfloat y);

extern void
_mesa_RasterPos2i(GLint x, GLint y);

extern void
_mesa_RasterPos2s(GLshort x, GLshort y);

extern void
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z);

extern void
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z);

extern void
_mesa_RasterPos3i(GLint x, GLint y, GLint z);

extern void
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z);

extern void
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w);

extern void
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);

extern void
_mesa_RasterPos2dv(const GLdouble *v);

extern void
_mesa_RasterPos2fv(const GLfloat *v);

extern void
_mesa_RasterPos2iv(const GLint *v);

extern void
_mesa_RasterPos2sv(const GLshort *v);

extern void
_mesa_RasterPos3dv(const GLdouble *v);

extern void
_mesa_RasterPos3fv(const GLfloat *v);

extern void
_mesa_RasterPos3iv(const GLint *v);

extern void
_mesa_RasterPos3sv(const GLshort *v);

extern void
_mesa_RasterPos4dv(const GLdouble *v);

extern void
_mesa_RasterPos4fv(const GLfloat *v);

extern void
_mesa_RasterPos4iv(const GLint *v);

extern void
_mesa_RasterPos4sv(const GLshort *v);


#endif

