/* $Id: stencil.c,v 1.27 2001/05/09 16:34:09 brianp Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "depth.h"
#include "macros.h"
#include "mem.h"
#include "stencil.h"
#include "mtypes.h"
#include "enable.h"
#endif




void
_mesa_ClearStencil( GLint s )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Stencil.Clear == (GLstencil) s)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Clear = (GLstencil) s;

   if (ctx->Driver.ClearStencil) {
      (*ctx->Driver.ClearStencil)( ctx, s );
   }
}



void
_mesa_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint maxref;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_GEQUAL:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glStencilFunc" );
         return;
   }

   maxref = (1 << STENCIL_BITS) - 1;
   ref = (GLstencil) CLAMP( ref, 0, maxref );

   if (ctx->Stencil.Function == func &&
       ctx->Stencil.ValueMask == (GLstencil) mask &&
       ctx->Stencil.Ref == ref)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Function = func;
   ctx->Stencil.Ref = ref;
   ctx->Stencil.ValueMask = (GLstencil) mask;

   if (ctx->Driver.StencilFunc) {
      (*ctx->Driver.StencilFunc)( ctx, func, ctx->Stencil.Ref, mask );
   }
}



void
_mesa_StencilMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Stencil.WriteMask == (GLstencil) mask)
	 return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.WriteMask = (GLstencil) mask;

   if (ctx->Driver.StencilMask) {
      (*ctx->Driver.StencilMask)( ctx, mask );
   }
}



void
_mesa_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (fail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (!ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zfail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zpass) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }

   if (ctx->Stencil.ZFailFunc == zfail &&
       ctx->Stencil.ZPassFunc == zpass &&
       ctx->Stencil.FailFunc == fail)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.ZFailFunc = zfail;
   ctx->Stencil.ZPassFunc = zpass;
   ctx->Stencil.FailFunc = fail;

   if (ctx->Driver.StencilOp) {
      (*ctx->Driver.StencilOp)(ctx, fail, zfail, zpass);
   }
}
