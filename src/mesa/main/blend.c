/* $Id: blend.c,v 1.28 2001/02/26 22:07:03 brianp Exp $ */

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
#include "blend.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "mtypes.h"
#endif


void
_mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glBlendFunc %s %s\n",
	      gl_lookup_enum_by_nr(sfactor),
	      gl_lookup_enum_by_nr(dfactor));

   switch (sfactor) {
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(sfactor)" );
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(sfactor)" );
         return;
   }

   switch (dfactor) {
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(dfactor)" );
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendFunc(dfactor)" );
         return;
   }

   if (ctx->Color.BlendDstRGB == dfactor &&
       ctx->Color.BlendSrcRGB == sfactor &&
       ctx->Color.BlendDstA == dfactor &&
       ctx->Color.BlendSrcA == sfactor)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.BlendDstRGB = ctx->Color.BlendDstA = dfactor;
   ctx->Color.BlendSrcRGB = ctx->Color.BlendSrcA = sfactor;

   if (ctx->Driver.BlendFunc) 
      ctx->Driver.BlendFunc( ctx, sfactor, dfactor );
}


/* GL_EXT_blend_func_separate */
void
_mesa_BlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB,
                            GLenum sfactorA, GLenum dfactorA )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glBlendFuncSeperate %s %s %s %s\n",
	      gl_lookup_enum_by_nr(sfactorRGB),
	      gl_lookup_enum_by_nr(dfactorRGB),
	      gl_lookup_enum_by_nr(sfactorA),
	      gl_lookup_enum_by_nr(dfactorA));

   switch (sfactorRGB) {
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(sfactorRGB)");
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(sfactorRGB)");
         return;
   }

   switch (dfactorRGB) {
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(dfactorRGB)");
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(dfactorRGB)");
         return;
   }

   switch (sfactorA) {
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(sfactorA)");
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(sfactorA)");
         return;
   }

   switch (dfactorA) {
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(dfactorA)");
            return;
         }
         /* fall-through */
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendFuncSeparate(dfactorA)" );
         return;
   }

   if (ctx->Color.BlendSrcRGB == sfactorRGB &&
       ctx->Color.BlendDstRGB == dfactorRGB &&
       ctx->Color.BlendSrcA == sfactorA &&
       ctx->Color.BlendDstA == dfactorA)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);

   ctx->Color.BlendSrcRGB = sfactorRGB;
   ctx->Color.BlendDstRGB = dfactorRGB;
   ctx->Color.BlendSrcA = sfactorA;
   ctx->Color.BlendDstA = dfactorA;

   if (ctx->Driver.BlendFuncSeparate) {
      (*ctx->Driver.BlendFuncSeparate)( ctx, sfactorRGB, dfactorRGB,
					sfactorA, dfactorA );
   }
}



/* This is really an extension function! */
void
_mesa_BlendEquation( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glBlendEquation %s\n",
	      gl_lookup_enum_by_nr(mode));

   switch (mode) {
      case GL_FUNC_ADD_EXT:
         break;
      case GL_MIN_EXT:
      case GL_MAX_EXT:
         if (!ctx->Extensions.EXT_blend_minmax &&
             !ctx->Extensions.ARB_imaging) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
            return;
         }
         break;
      case GL_LOGIC_OP:
         if (!ctx->Extensions.EXT_blend_logic_op) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
            return;
         }
         break;
      case GL_FUNC_SUBTRACT_EXT:
      case GL_FUNC_REVERSE_SUBTRACT_EXT:
         if (!ctx->Extensions.EXT_blend_subtract &&
             !ctx->Extensions.ARB_imaging) {
            gl_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
            return;
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBlendEquation" );
         return;
   }

   if (ctx->Color.BlendEquation == mode)
      return;
   
   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.BlendEquation = mode;

   /* This is needed to support 1.1's RGB logic ops AND
    * 1.0's blending logicops.
    */
   ctx->Color.ColorLogicOpEnabled = (mode==GL_LOGIC_OP && 
				     ctx->Color.BlendEnabled);

   if (ctx->Driver.BlendEquation)
      ctx->Driver.BlendEquation( ctx, mode );
}



void
_mesa_BlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
   GLfloat tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = CLAMP( red,   0.0, 1.0 );
   tmp[1] = CLAMP( green, 0.0, 1.0 );
   tmp[2] = CLAMP( blue,  0.0, 1.0 );
   tmp[3] = CLAMP( alpha, 0.0, 1.0 );

   if (TEST_EQ_4V(tmp, ctx->Color.BlendColor))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4FV( ctx->Color.BlendColor, tmp );
}

