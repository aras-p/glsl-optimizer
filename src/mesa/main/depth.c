/* $Id: depth.c,v 1.24 2000/11/22 07:32:16 joukj Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
#include "enums.h"
#include "depth.h"
#include "macros.h"
#include "mem.h"
#include "mtypes.h"
#endif



/**********************************************************************/
/*****                          API Functions                     *****/
/**********************************************************************/



void
_mesa_ClearDepth( GLclampd depth )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearDepth");
   ctx->Depth.Clear = (GLfloat) CLAMP( depth, 0.0, 1.0 );
   if (ctx->Driver.ClearDepth)
      (*ctx->Driver.ClearDepth)( ctx, ctx->Depth.Clear );
   ctx->NewState |= _NEW_DEPTH;
}



void
_mesa_DepthFunc( GLenum func )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDepthFunc");

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glDepthFunc %s\n", gl_lookup_enum_by_nr(func));

   switch (func) {
      case GL_LESS:    /* (default) pass if incoming z < stored z */
      case GL_GEQUAL:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_NOTEQUAL:
      case GL_EQUAL:
      case GL_ALWAYS:
	 if (ctx->Depth.Func != func) {
	    ctx->Depth.Func = func;
	    ctx->NewState |= _NEW_DEPTH;
	    ctx->_TriangleCaps &= ~DD_Z_NEVER;
	    if (ctx->Driver.DepthFunc) {
	       (*ctx->Driver.DepthFunc)( ctx, func );
	    }
	 }
         break;
      case GL_NEVER:
	 if (ctx->Depth.Func != func) {
	    ctx->Depth.Func = func;
	    ctx->NewState |= _NEW_DEPTH;
	    ctx->_TriangleCaps |= DD_Z_NEVER;
	    if (ctx->Driver.DepthFunc) {
	       (*ctx->Driver.DepthFunc)( ctx, func );
	    }
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDepth.Func" );
   }
}



void
_mesa_DepthMask( GLboolean flag )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDepthMask");

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glDepthMask %d\n", flag);

   /*
    * GL_TRUE indicates depth buffer writing is enabled (default)
    * GL_FALSE indicates depth buffer writing is disabled
    */
   if (ctx->Depth.Mask != flag) {
      ctx->Depth.Mask = flag;
      ctx->NewState |= _NEW_DEPTH;
      if (ctx->Driver.DepthMask) {
	 (*ctx->Driver.DepthMask)( ctx, flag );
      }
   }
}


