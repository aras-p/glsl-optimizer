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

#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "intel_tex.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * Get the intel_region which is the source for any glCopyTex[Sub]Image call.
 *
 * Do the best we can using the blitter.  A future project is to use
 * the texture engine and fragment programs for these copies.
 */
static struct intel_renderbuffer *
get_teximage_readbuffer(struct intel_context *intel, GLenum internalFormat)
{
   DBG("%s %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat));

   if (_mesa_is_depth_format(internalFormat) ||
       _mesa_is_depthstencil_format(internalFormat))
      return intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);

   return intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
}


GLboolean
intel_copy_texsubimage(struct intel_context *intel,
                       GLenum target,
                       struct intel_texture_image *intelImage,
                       GLenum internalFormat,
                       GLint dstx, GLint dsty,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_context *ctx = &intel->ctx;
   struct intel_renderbuffer *irb;
   bool copy_supported = false;
   bool copy_supported_with_alpha_override = false;

   intel_prepare_render(intel);

   irb = get_teximage_readbuffer(intel, internalFormat);
   if (!intelImage->mt || !irb || !irb->region) {
      if (unlikely(INTEL_DEBUG & DEBUG_FALLBACKS))
	 fprintf(stderr, "%s fail %p %p (0x%08x)\n",
		 __FUNCTION__, intelImage->mt, irb, internalFormat);
      return GL_FALSE;
   }

   copy_supported = intelImage->base.TexFormat == irb->Base.Format;

   /* Converting ARGB8888 to XRGB8888 is trivial: ignore the alpha bits */
   if (irb->Base.Format == MESA_FORMAT_ARGB8888 &&
       intelImage->base.TexFormat == MESA_FORMAT_XRGB8888) {
      copy_supported = true;
   }

   /* Converting XRGB8888 to ARGB8888 requires setting the alpha bits to 1.0 */
   if (irb->Base.Format == MESA_FORMAT_XRGB8888 &&
       intelImage->base.TexFormat == MESA_FORMAT_ARGB8888) {
      copy_supported_with_alpha_override = true;
   }

   if (!copy_supported && !copy_supported_with_alpha_override) {
      if (unlikely(INTEL_DEBUG & DEBUG_FALLBACKS))
	 fprintf(stderr, "%s mismatched formats %s, %s\n",
		 __FUNCTION__,
		 _mesa_get_format_name(intelImage->base.TexFormat),
		 _mesa_get_format_name(irb->Base.Format));
      return GL_FALSE;
   }

   {
      drm_intel_bo *dst_bo = intel_region_buffer(intel,
						 intelImage->mt->region,
						 INTEL_WRITE_PART);
      GLuint image_x, image_y;
      GLshort src_pitch;

      /* get dest x/y in destination texture */
      intel_miptree_get_image_offset(intelImage->mt,
				     intelImage->level,
				     intelImage->face,
				     0,
				     &image_x, &image_y);

      /* The blitter can't handle Y-tiled buffers. */
      if (intelImage->mt->region->tiling == I915_TILING_Y) {
	 return GL_FALSE;
      }

      if (ctx->ReadBuffer->Name == 0) {
	 /* Flip vertical orientation for system framebuffers */
	 y = ctx->ReadBuffer->Height - (y + height);
	 src_pitch = -irb->region->pitch;
      } else {
	 /* reading from a FBO, y is already oriented the way we like */
	 src_pitch = irb->region->pitch;
      }

      /* blit from src buffer to texture */
      if (!intelEmitCopyBlit(intel,
			     intelImage->mt->cpp,
			     src_pitch,
			     irb->region->buffer,
			     0,
			     irb->region->tiling,
			     intelImage->mt->region->pitch,
			     dst_bo,
			     0,
			     intelImage->mt->region->tiling,
			     irb->draw_x + x, irb->draw_y + y,
			     image_x + dstx, image_y + dsty,
			     width, height,
			     GL_COPY)) {
	 return GL_FALSE;
      }
   }

   if (copy_supported_with_alpha_override)
      intel_set_teximage_alpha_to_one(ctx, intelImage);

   return GL_TRUE;
}


static void
intelCopyTexImage1D(struct gl_context * ctx, GLenum target, GLint level,
                    GLenum internalFormat,
                    GLint x, GLint y, GLsizei width, GLint border)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   int srcx, srcy, dstx, dsty, height;

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                          width, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);
   srcx = x;
   srcy = y;
   dstx = 0;
   dsty = 0;
   height = 1;
   if (!_mesa_clip_copytexsubimage(ctx,
				   &dstx, &dsty,
				   &srcx, &srcy,
				   &width, &height))
      return;

   if (!intel_copy_texsubimage(intel_context(ctx), target,
                               intel_texture_image(texImage),
                               internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
   fallback_debug("%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexImage1D(ctx, target, level, internalFormat, x, y,
                             width, border);
}


static void
intelCopyTexImage2D(struct gl_context * ctx, GLenum target, GLint level,
                    GLenum internalFormat,
                    GLint x, GLint y, GLsizei width, GLsizei height,
                    GLint border)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   int srcx, srcy, dstx, dsty;

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.
    */
   ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                          width, height, border,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   srcx = x;
   srcy = y;
   dstx = 0;
   dsty = 0;
   if (!_mesa_clip_copytexsubimage(ctx,
				   &dstx, &dsty,
				   &srcx, &srcy,
				   &width, &height))
      return;

   if (!intel_copy_texsubimage(intel_context(ctx), target,
                               intel_texture_image(texImage),
                               internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
   fallback_debug("%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexImage2D(ctx, target, level, internalFormat, x, y,
                             width, height, border);
}


static void
intelCopyTexSubImage1D(struct gl_context * ctx, GLenum target, GLint level,
                       GLint xoffset, GLint x, GLint y, GLsizei width)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;

   /* XXX need to check <border> as in above function? */

   /* Need to check texture is compatible with source format. 
    */

   if (!intel_copy_texsubimage(intel_context(ctx), target,
                               intel_texture_image(texImage),
                               internalFormat, xoffset, 0, x, y, width, 1)) {
      fallback_debug("%s - fallback to swrast\n", __FUNCTION__);
      _mesa_meta_CopyTexSubImage1D(ctx, target, level, xoffset, x, y, width);
   }
}


static void
intelCopyTexSubImage2D(struct gl_context * ctx, GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;

   /* Need to check texture is compatible with source format. 
    */

   if (!intel_copy_texsubimage(intel_context(ctx), target,
                               intel_texture_image(texImage),
                               internalFormat,
                               xoffset, yoffset, x, y, width, height)) {
      fallback_debug("%s - fallback to swrast\n", __FUNCTION__);
      _mesa_meta_CopyTexSubImage2D(ctx, target, level,
                                   xoffset, yoffset, x, y, width, height);
   }
}


void
intelInitTextureCopyImageFuncs(struct dd_function_table *functions)
{
   functions->CopyTexImage1D = intelCopyTexImage1D;
   functions->CopyTexImage2D = intelCopyTexImage2D;
   functions->CopyTexSubImage1D = intelCopyTexSubImage1D;
   functions->CopyTexSubImage2D = intelCopyTexSubImage2D;
}
