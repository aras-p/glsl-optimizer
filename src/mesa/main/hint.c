/* $Id: hint.c,v 1.4 2000/05/23 20:10:50 brianp Exp $ */

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
#include "enums.h"
#include "context.h"
#include "hint.h"
#include "state.h"
#endif



void
_mesa_Hint( GLenum target, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   (void) _mesa_try_Hint( ctx, target, mode );
}


GLboolean
_mesa_try_Hint( GLcontext *ctx, GLenum target, GLenum mode )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glHint", GL_FALSE);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glHint %s %d\n", gl_lookup_enum_by_nr(target), mode);

   if (mode != GL_NICEST && mode != GL_FASTEST && mode != GL_DONT_CARE) {
      gl_error(ctx, GL_INVALID_ENUM, "glHint(mode)");
      return GL_FALSE;
   }

   switch (target) {
      case GL_FOG_HINT:
         ctx->Hint.Fog = mode;
         break;
      case GL_LINE_SMOOTH_HINT:
         ctx->Hint.LineSmooth = mode;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
         ctx->Hint.PerspectiveCorrection = mode;
         break;
      case GL_POINT_SMOOTH_HINT:
         ctx->Hint.PointSmooth = mode;
         break;
      case GL_POLYGON_SMOOTH_HINT:
         ctx->Hint.PolygonSmooth = mode;
         break;
      case GL_PREFER_DOUBLEBUFFER_HINT_PGI:
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
         break;
      case GL_STRICT_LIGHTING_HINT_PGI:
         ctx->Hint.StrictLighting = mode;
         break;
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
      case GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI:
      case GL_NATIVE_GRAPHICS_END_HINT_PGI:
      case GL_CONSERVE_MEMORY_HINT_PGI:
      case GL_RECLAIM_MEMORY_HINT_PGI:
         break;
      case GL_ALWAYS_FAST_HINT_PGI:
         if (mode) {
            ctx->Hint.AllowDrawWin = GL_TRUE;
            ctx->Hint.AllowDrawFrg = GL_FALSE;
            ctx->Hint.AllowDrawMem = GL_FALSE;
         } else {
            ctx->Hint.AllowDrawWin = GL_TRUE;
            ctx->Hint.AllowDrawFrg = GL_TRUE;
            ctx->Hint.AllowDrawMem = GL_TRUE;
         } 
         break;
      case GL_ALWAYS_SOFT_HINT_PGI:
         ctx->Hint.AllowDrawWin = GL_TRUE;
         ctx->Hint.AllowDrawFrg = GL_TRUE;
         ctx->Hint.AllowDrawMem = GL_TRUE;
         break;
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
         break;
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
         ctx->Hint.AllowDrawWin = mode;
         break;
      case GL_ALLOW_DRAW_FRG_HINT_PGI:
         ctx->Hint.AllowDrawFrg = mode;
         break;
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
         ctx->Hint.AllowDrawMem = mode;
         break;
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
      case GL_WIDE_LINE_HINT_PGI:
      case GL_BACK_NORMALS_HINT_PGI:
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
         break;

      /* GL_EXT_clip_volume_hint */
      case GL_CLIP_VOLUME_CLIPPING_HINT_EXT:
         ctx->Hint.ClipVolumeClipping = mode;
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         if (ctx->Extensions.HaveTextureCompression) {
            ctx->Hint.TextureCompression = mode;
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glHint(target)");
         }
         break;

      default:
         gl_error(ctx, GL_INVALID_ENUM, "glHint(target)");
         return GL_FALSE;
   }

   ctx->NewState |= NEW_ALL;   /* just to be safe */

   if (ctx->Driver.Hint) {
      (*ctx->Driver.Hint)( ctx, target, mode );
   }
   
   return GL_TRUE;
}


void
_mesa_HintPGI( GLenum target, GLint mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glHintPGI");

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glHintPGI %s %d\n", gl_lookup_enum_by_nr(target), mode);

   if (mode != GL_NICEST && mode != GL_FASTEST && mode != GL_DONT_CARE) {
      gl_error(ctx, GL_INVALID_ENUM, "glHintPGI(mode)");
      return;
   }

   switch (target) {
      case GL_PREFER_DOUBLEBUFFER_HINT_PGI:
      case GL_STRICT_DEPTHFUNC_HINT_PGI:
      case GL_STRICT_LIGHTING_HINT_PGI:
      case GL_STRICT_SCISSOR_HINT_PGI:
      case GL_FULL_STIPPLE_HINT_PGI:
      case GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI:
      case GL_NATIVE_GRAPHICS_END_HINT_PGI:
      case GL_CONSERVE_MEMORY_HINT_PGI:
      case GL_RECLAIM_MEMORY_HINT_PGI:
      case GL_ALWAYS_FAST_HINT_PGI:
      case GL_ALWAYS_SOFT_HINT_PGI:
      case GL_ALLOW_DRAW_OBJ_HINT_PGI:
      case GL_ALLOW_DRAW_WIN_HINT_PGI:
      case GL_ALLOW_DRAW_FRG_HINT_PGI:
      case GL_ALLOW_DRAW_MEM_HINT_PGI:
      case GL_CLIP_NEAR_HINT_PGI:
      case GL_CLIP_FAR_HINT_PGI:
      case GL_WIDE_LINE_HINT_PGI:
      case GL_BACK_NORMALS_HINT_PGI:
      case GL_NATIVE_GRAPHICS_HANDLE_PGI:
         (void) _mesa_try_Hint(ctx, target, (GLenum) mode);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glHintPGI(target)" );
         return;
   }
}

   
