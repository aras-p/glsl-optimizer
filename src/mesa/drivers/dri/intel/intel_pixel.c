/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portionsalloc
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/enums.h"
#include "main/state.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_pixel.h"
#include "intel_regions.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static GLenum
effective_func(GLenum func, GLboolean src_alpha_is_one)
{
   if (src_alpha_is_one) {
      if (func == GL_SRC_ALPHA)
	 return GL_ONE;
      if (func == GL_ONE_MINUS_SRC_ALPHA)
	 return GL_ZERO;
   }

   return func;
}

/**
 * Check if any fragment operations are in effect which might effect
 * glDraw/CopyPixels.
 */
GLboolean
intel_check_blit_fragment_ops(GLcontext * ctx, GLboolean src_alpha_is_one)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (ctx->FragmentProgram._Enabled) {
      DBG("fallback due to fragment program\n");
      return GL_FALSE;
   }

   if (ctx->Color.BlendEnabled &&
       (effective_func(ctx->Color.BlendSrcRGB, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.BlendDstRGB, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.BlendEquationRGB != GL_FUNC_ADD ||
	effective_func(ctx->Color.BlendSrcA, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.BlendDstA, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.BlendEquationA != GL_FUNC_ADD)) {
      DBG("fallback due to blend\n");
      return GL_FALSE;
   }

   if (ctx->Texture._EnabledUnits) {
      DBG("fallback due to texturing\n");
      return GL_FALSE;
   }

   if (!(ctx->Color.ColorMask[0][0] &&
	 ctx->Color.ColorMask[0][1] &&
	 ctx->Color.ColorMask[0][2] &&
	 ctx->Color.ColorMask[0][3])) {
      DBG("fallback due to color masking\n");
      return GL_FALSE;
   }

   if (ctx->Color.AlphaEnabled) {
      DBG("fallback due to alpha\n");
      return GL_FALSE;
   }

   if (ctx->Depth.Test) {
      DBG("fallback due to depth test\n");
      return GL_FALSE;
   }

   if (ctx->Fog.Enabled) {
      DBG("fallback due to fog\n");
      return GL_FALSE;
   }

   if (ctx->_ImageTransferState) {
      DBG("fallback due to image transfer\n");
      return GL_FALSE;
   }

   if (ctx->Stencil._Enabled) {
      DBG("fallback due to image stencil\n");
      return GL_FALSE;
   }

   if (ctx->RenderMode != GL_RENDER) {
      DBG("fallback due to render mode\n");
      return GL_FALSE;
   }

   return GL_TRUE;
}

/* The intel_region struct doesn't really do enough to capture the
 * format of the pixels in the region.  For now this code assumes that
 * the region is a display surface and hence is either ARGB8888 or
 * RGB565.
 * XXX FBO: If we'd pass in the intel_renderbuffer instead of region, we'd
 * know the buffer's pixel format.
 *
 * \param format  as given to glDraw/ReadPixels
 * \param type  as given to glDraw/ReadPixels
 */
GLboolean
intel_check_blit_format(struct intel_region * region,
                        GLenum format, GLenum type)
{
   if (region->cpp == 4 &&
       (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
        type == GL_UNSIGNED_BYTE) && format == GL_BGRA) {
      return GL_TRUE;
   }

   if (region->cpp == 2 &&
       type == GL_UNSIGNED_SHORT_5_6_5_REV && format == GL_BGR) {
      return GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s: bad format for blit (cpp %d, type %s format %s)\n",
              __FUNCTION__, region->cpp,
              _mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));

   return GL_FALSE;
}

void
intelInitPixelFuncs(struct dd_function_table *functions)
{
   functions->Accum = _swrast_Accum;
   if (!getenv("INTEL_NO_BLIT")) {
      functions->Bitmap = intelBitmap;
      functions->CopyPixels = intelCopyPixels;
      functions->DrawPixels = intelDrawPixels;
   }
   functions->ReadPixels = intelReadPixels;
}

