/* $Id: bufferobj.c,v 1.1 2003/03/29 17:01:00 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex_buffer_object extension.
 * \author Brian Paul
 */


#include "glheader.h"
#include "imports.h"
#include "bufferobj.h"


void
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
}

void
_mesa_DeleteBuffersARB(GLsizei n, const GLuint * buffer)
{
}

void
_mesa_GenBuffersARB(GLsizei n, GLuint * buffer)
{
}

GLboolean
_mesa_IsBufferARB(GLuint buffer)
{
   return GL_FALSE;
}

void
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
}

void
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
}

void
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
}

void
_mesa_MapBufferARB(GLenum target, GLenum access)
{
}

GLboolean
_mesa_UnmapBufferARB(GLenum target)
{
   return GL_FALSE;
}

void
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
}

void
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
}
