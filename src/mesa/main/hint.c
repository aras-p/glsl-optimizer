/* $Id: hint.c,v 1.10 2001/05/21 16:41:03 brianp Exp $ */

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
#include "enums.h"
#include "context.h"
#include "hint.h"
#include "state.h"
#endif



void
_mesa_Hint( GLenum target, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   (void) _mesa_try_Hint( ctx, target, mode );
}


GLboolean
_mesa_try_Hint( GLcontext *ctx, GLenum target, GLenum mode )
{
   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glHint %s %d\n", _mesa_lookup_enum_by_nr(target), mode);

   if (mode != GL_NICEST && mode != GL_FASTEST && mode != GL_DONT_CARE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glHint(mode)");
      return GL_FALSE;
   }

   switch (target) {
      case GL_FOG_HINT:
         if (ctx->Hint.Fog == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.Fog = mode;
         break;
      case GL_LINE_SMOOTH_HINT:
         if (ctx->Hint.LineSmooth == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.LineSmooth = mode;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
         if (ctx->Hint.PerspectiveCorrection == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.PerspectiveCorrection = mode;
         break;
      case GL_POINT_SMOOTH_HINT:
         if (ctx->Hint.PointSmooth == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.PointSmooth = mode;
         break;
      case GL_POLYGON_SMOOTH_HINT:
         if (ctx->Hint.PolygonSmooth == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.PolygonSmooth = mode;
         break;

      /* GL_EXT_clip_volume_hint */
      case GL_CLIP_VOLUME_CLIPPING_HINT_EXT:
         if (ctx->Hint.ClipVolumeClipping == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
         ctx->Hint.ClipVolumeClipping = mode;
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSION_HINT_ARB:
         if (!ctx->Extensions.ARB_texture_compression) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glHint(target)");
	    return GL_FALSE;
         }
	 if (ctx->Hint.TextureCompression == mode)
	    return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
	 ctx->Hint.TextureCompression = mode;
         break;

      /* GL_SGIS_generate_mipmap */
      case GL_GENERATE_MIPMAP_HINT_SGIS:
         if (!ctx->Extensions.SGIS_generate_mipmap) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glHint(target)");
	    return GL_FALSE;
         }
         if (ctx->Hint.GenerateMipmap == mode)
            return GL_TRUE;
	 FLUSH_VERTICES(ctx, _NEW_HINT);
	 ctx->Hint.GenerateMipmap = mode;
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glHint(target)");
         return GL_FALSE;
   }

   if (ctx->Driver.Hint) {
      (*ctx->Driver.Hint)( ctx, target, mode );
   }

   return GL_TRUE;
}
