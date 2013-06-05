/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DRAWPIX_H
#define DRAWPIX_H


#include "compiler.h"

struct _glapi_table;


void GLAPIENTRY
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels );
void GLAPIENTRY
_mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLenum type );
void GLAPIENTRY
_mesa_Bitmap( GLsizei width, GLsizei height,
              GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,
              const GLubyte *bitmap );


#endif /* DRAWPIX_H */
