/* $Id: varray.h,v 1.12 2001/03/12 00:48:39 gareth Exp $ */

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


#ifndef VARRAY_H
#define VARRAY_H


#include "mtypes.h"


extern void
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride,
                    const GLvoid *ptr);

extern void
_mesa_UnlockArraysEXT( void );

extern void
_mesa_LockArraysEXT(GLint first, GLsizei count);


extern void
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr);


extern void
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr);


extern void
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr);


extern void
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr);


extern void
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr);


extern void
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr);


extern void
_mesa_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr);

extern void
_mesa_SecondaryColorPointerEXT(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr);


extern void
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);



#endif
