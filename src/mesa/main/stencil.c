/* $Id: stencil.c,v 1.22 2000/10/31 18:09:45 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "types.h"
#include "enable.h"
#endif




void
_mesa_ClearStencil( GLint s )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearStencil");
   ctx->Stencil.Clear = (GLstencil) s;
   ctx->NewState |= _NEW_STENCIL;

   if (ctx->Driver.ClearStencil) {
      (*ctx->Driver.ClearStencil)( ctx, s );
   }
}



void
_mesa_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint maxref;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glStencilFunc");

   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_GEQUAL:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
         ctx->Stencil.Function = func;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glStencilFunc" );
         return;
   }

   maxref = (1 << STENCIL_BITS) - 1;
   ctx->Stencil.Ref = (GLstencil) CLAMP( ref, 0, maxref );
   ctx->Stencil.ValueMask = (GLstencil) mask;
   ctx->NewState |= _NEW_STENCIL;

   if (ctx->Driver.StencilFunc) {
      (*ctx->Driver.StencilFunc)( ctx, func, ctx->Stencil.Ref, mask );
   }
}



void
_mesa_StencilMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glStencilMask");
   ctx->Stencil.WriteMask = (GLstencil) mask;
   ctx->NewState |= _NEW_STENCIL;

   if (ctx->Driver.StencilMask) {
      (*ctx->Driver.StencilMask)( ctx, mask );
   }
}



void
_mesa_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glStencilOp");
   switch (fail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.FailFunc = fail;
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            ctx->Stencil.FailFunc = fail;
            break;
         }
         /* FALL-THROUGH */
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zfail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.ZFailFunc = zfail;
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            ctx->Stencil.ZFailFunc = zfail;
            break;
         }
         /* FALL-THROUGH */
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zpass) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         ctx->Stencil.ZPassFunc = zpass;
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            ctx->Stencil.ZPassFunc = zpass;
            break;
         }
         /* FALL-THROUGH */
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }

   ctx->NewState |= _NEW_STENCIL;

   if (ctx->Driver.StencilOp) {
      (*ctx->Driver.StencilOp)(ctx, fail, zfail, zpass);
   }
}

