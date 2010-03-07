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


#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S			0x0C60
#define GL_TEXTURE_GEN_T			0x0C61
#define GL_TEXTURE_GEN_R			0x0C62
#endif


extern void GL_APIENTRY _es_Disable(GLenum cap);
extern void GL_APIENTRY _es_Enable(GLenum cap);
extern GLboolean GL_APIENTRY _es_IsEnabled(GLenum cap);

extern void GL_APIENTRY _mesa_Disable(GLenum cap);
extern void GL_APIENTRY _mesa_Enable(GLenum cap);
extern GLboolean GL_APIENTRY _mesa_IsEnabled(GLenum cap);


void GL_APIENTRY
_es_Disable(GLenum cap)
{
   switch (cap) {
   case GL_TEXTURE_GEN_STR_OES:
      /* disable S, T, and R at the same time */
      _mesa_Disable(GL_TEXTURE_GEN_S);
      _mesa_Disable(GL_TEXTURE_GEN_T);
      _mesa_Disable(GL_TEXTURE_GEN_R);
      break;
   default:
      _mesa_Disable(cap);
      break;
   }
}


void GL_APIENTRY
_es_Enable(GLenum cap)
{
   switch (cap) {
   case GL_TEXTURE_GEN_STR_OES:
      /* enable S, T, and R at the same time */
      _mesa_Enable(GL_TEXTURE_GEN_S);
      _mesa_Enable(GL_TEXTURE_GEN_T);
      _mesa_Enable(GL_TEXTURE_GEN_R);
      break;
   default:
      _mesa_Enable(cap);
      break;
   }
}


GLboolean GL_APIENTRY
_es_IsEnabled(GLenum cap)
{
   switch (cap) {
   case GL_TEXTURE_GEN_STR_OES:
      cap = GL_TEXTURE_GEN_S;
   default:
      break;
   }

   return _mesa_IsEnabled(cap);
}
