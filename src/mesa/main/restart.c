/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "restart.h"


/**
 */
void GLAPIENTRY
_mesa_PrimitiveRestart(void)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->VersionMajor * 10 + ctx->VersionMinor < 31) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPrimitiveRestart()");
      return;
   }

   /* XXX call into vbo module */
}



/**
 */
void GLAPIENTRY
_mesa_PrimitiveRestartIndex(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->VersionMajor * 10 + ctx->VersionMinor < 31) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPrimitiveRestartIndex()");
      return;
   }

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

   ctx->Array.RestartIndex = index;
}
