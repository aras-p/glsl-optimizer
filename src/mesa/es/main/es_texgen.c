/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "GLES/gl.h"
#include "GLES/glext.h"

#include "main/compiler.h" /* for ASSERT */


#ifndef GL_S
#define GL_S					0x2000
#define GL_T					0x2001
#define GL_R					0x2002
#endif


extern void GL_APIENTRY _es_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
extern void GL_APIENTRY _es_TexGenf(GLenum coord, GLenum pname, GLfloat param);
extern void GL_APIENTRY _es_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);

extern void GL_APIENTRY _mesa_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
extern void GL_APIENTRY _mesa_TexGenf(GLenum coord, GLenum pname, GLfloat param);
extern void GL_APIENTRY _mesa_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);


void GL_APIENTRY
_es_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   _mesa_GetTexGenfv(GL_S, pname, params);
}


void GL_APIENTRY
_es_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   /* set S, T, and R at the same time */
   _mesa_TexGenf(GL_S, pname, param);
   _mesa_TexGenf(GL_T, pname, param);
   _mesa_TexGenf(GL_R, pname, param);
}


void GL_APIENTRY
_es_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
   ASSERT(coord == GL_TEXTURE_GEN_STR_OES);
   /* set S, T, and R at the same time */
   _mesa_TexGenfv(GL_S, pname, params);
   _mesa_TexGenfv(GL_T, pname, params);
   _mesa_TexGenfv(GL_R, pname, params);
}
