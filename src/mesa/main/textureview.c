/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */


/**
 * \file textureview.c
 * GL_ARB_texture_view functions
 */

#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "imports.h"
#include "macros.h"
#include "teximage.h"
#include "texobj.h"
#include "texstorage.h"
#include "textureview.h"
#include "mtypes.h"

/**
 * glTextureView (ARB_texture_view)
 * If an error is found, record it with _mesa_error()
 * \return none.
 */
void GLAPIENTRY
_mesa_TextureView(GLuint texture, GLenum target, GLuint origtexture,
                  GLenum internalformat,
                  GLuint minlevel, GLuint numlevels,
                  GLuint minlayer, GLuint numlayers)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTextureView (unfinished) %d %s %d %s %d %d %d %d\n",
                  texture, _mesa_lookup_enum_by_nr(target), origtexture,
                  _mesa_lookup_enum_by_nr(internalformat),
                  minlevel, numlevels, minlayer, numlayers);


}
