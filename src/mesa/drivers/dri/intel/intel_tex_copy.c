/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portions
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

#include "main/mtypes.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/fbobject.h"

#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "intel_tex.h"
#include "intel_blit.h"
#ifndef I915
#include "brw_context.h"
#endif

#define FILE_DEBUG_FLAG DEBUG_TEXTURE


bool
intel_copy_texsubimage(struct intel_context *intel,
                       struct intel_texture_image *intelImage,
                       GLint dstx, GLint dsty,
                       struct intel_renderbuffer *irb,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_context *ctx = &intel->ctx;
   const GLenum internalFormat = intelImage->base.Base.InternalFormat;
   bool copy_supported = false;
   bool copy_supported_with_alpha_override = false;

   intel_prepare_render(intel);

   /* glCopyTexSubImage() can be called on a multisampled renderbuffer (if
    * that renderbuffer is associated with the window system framebuffer),
    * however the hardware blitter can't handle this case, so fall back to
    * meta (which can, since it uses ReadPixels).
    */
   if (irb->Base.Base.NumSamples != 0)
      return false;

   /* glCopyTexSubImage() can't be called on a multisampled texture. */
   assert(intelImage->base.Base.NumSamples == 0);

   if (!intelImage->mt || !irb || !irb->mt) {
      if (unlikely(INTEL_DEBUG & DEBUG_PERF))
	 fprintf(stderr, "%s fail %p %p (0x%08x)\n",
		 __FUNCTION__, intelImage->mt, irb, internalFormat);
      return false;
   }

   if (intelImage->base.Base.TexObject->Target == GL_TEXTURE_1D_ARRAY ||
       intelImage->base.Base.TexObject->Target == GL_TEXTURE_2D_ARRAY) {
      perf_debug("no support for array textures\n");
      return false;
   }

   /* glCopyTexImage (and the glBlitFramebuffer() path that reuses this)
    * doesn't do any sRGB conversions.
    */
   gl_format src_format = _mesa_get_srgb_format_linear(intel_rb_format(irb));
   gl_format dst_format = _mesa_get_srgb_format_linear(intelImage->base.Base.TexFormat);

   copy_supported = src_format == dst_format;

   /* Converting ARGB8888 to XRGB8888 is trivial: ignore the alpha bits */
   if (src_format == MESA_FORMAT_ARGB8888 &&
       dst_format == MESA_FORMAT_XRGB8888) {
      copy_supported = true;
   }

   /* Converting XRGB8888 to ARGB8888 requires setting the alpha bits to 1.0 */
   if (src_format == MESA_FORMAT_XRGB8888 &&
       dst_format == MESA_FORMAT_ARGB8888) {
      copy_supported_with_alpha_override = true;
   }

   if (!copy_supported && !copy_supported_with_alpha_override) {
      perf_debug("%s mismatched formats %s, %s\n",
		 __FUNCTION__,
		 _mesa_get_format_name(intelImage->base.Base.TexFormat),
		 _mesa_get_format_name(intel_rb_format(irb)));
      return false;
   }

   /* blit from src buffer to texture */
   if (!intel_miptree_blit(intel,
                           irb->mt, irb->mt_level, irb->mt_layer,
                           x, y, irb->Base.Base.Name == 0,
                           intelImage->mt, intelImage->base.Base.Level,
                           intelImage->base.Base.Face,
                           dstx, dsty, false,
                           width, height, GL_COPY)) {
      return false;
   }

   if (copy_supported_with_alpha_override)
      intel_set_teximage_alpha_to_one(ctx, intelImage);

   return true;
}


static void
intelCopyTexSubImage(struct gl_context *ctx, GLuint dims,
                     struct gl_texture_image *texImage,
                     GLint xoffset, GLint yoffset, GLint slice,
                     struct gl_renderbuffer *rb,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height)
{
   struct intel_context *intel = intel_context(ctx);

   if (slice == 0) {
#ifndef I915
      /* Try BLORP first.  It can handle almost everything. */
      if (brw_blorp_copytexsubimage(intel, rb, texImage, x, y,
                                    xoffset, yoffset, width, height))
         return;
#endif

      /* Next, try the BLT engine. */
      if (intel_copy_texsubimage(intel,
                                 intel_texture_image(texImage),
                                 xoffset, yoffset,
                                 intel_renderbuffer(rb), x, y, width, height))
         return;
   }

   /* Finally, fall back to meta.  This will likely be slow. */
   perf_debug("%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexSubImage(ctx, dims, texImage,
                              xoffset, yoffset, slice,
                              rb, x, y, width, height);
}


void
intelInitTextureCopyImageFuncs(struct dd_function_table *functions)
{
   functions->CopyTexSubImage = intelCopyTexSubImage;
}
