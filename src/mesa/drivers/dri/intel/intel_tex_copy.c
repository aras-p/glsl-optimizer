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
#include "main/mipmap.h"

#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_buffers.h"
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
static const struct intel_region *
get_teximage_source(struct intel_context *intel, GLenum internalFormat)
{
   struct intel_renderbuffer *irb;

   DBG("%s %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat));

   switch (internalFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 2)
         return irb->region;
      return NULL;
   case GL_DEPTH24_STENCIL8_EXT:
   case GL_DEPTH_STENCIL_EXT:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 4)
         return irb->region;
      return NULL;
   case GL_RGBA:
   case GL_RGBA8:
   case GL_RGB:
   case GL_RGB8:
      return intel_readbuf_region(intel);
   default:
      return NULL;
   }
}


static GLboolean
do_copy_texsubimage(struct intel_context *intel,
		    GLenum target,
                    struct intel_texture_image *intelImage,
                    GLenum internalFormat,
                    GLint dstx, GLint dsty,
                    GLint x, GLint y, GLsizei width, GLsizei height)
{
   GLcontext *ctx = &intel->ctx;
   const struct intel_region *src = get_teximage_source(intel, internalFormat);

   if (!intelImage->mt || !src) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "%s fail %p %p (0x%08x)\n",
		 __FUNCTION__, intelImage->mt, src, internalFormat);
      return GL_FALSE;
   }

   if (intelImage->mt->cpp != src->cpp) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "%s fail %d vs %d cpp\n",
		 __FUNCTION__, intelImage->mt->cpp, src->cpp);
      return GL_FALSE;
   }

   /* intelFlush(ctx); */
   intel_prepare_render(intel);
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

      /* Can't blit to tiled buffers with non-tile-aligned offset. */
      if (intelImage->mt->region->tiling == I915_TILING_Y) {
	 return GL_FALSE;
      }

      if (ctx->ReadBuffer->Name == 0) {
	 /* reading from a window, adjust x, y */
	 const __DRIdrawable *dPriv = intel->driReadDrawable;
	 y = dPriv->y + (dPriv->h - (y + height));
	 x += dPriv->x;

	 /* Invert the data coming from the source rectangle due to GL
	  * and hardware disagreeing on where y=0 is.
	  *
	  * It appears that our offsets and pitches get mangled
	  * appropriately by the hardware, and we don't need to adjust them
	  * on our own.
	  */
	 src_pitch = -src->pitch;
      } else {
	 /* reading from a FBO, y is already oriented the way we like */
	 src_pitch = src->pitch;
      }

      /* blit from src buffer to texture */
      if (!intelEmitCopyBlit(intel,
			     intelImage->mt->cpp,
			     src_pitch,
			     src->buffer,
			     0,
			     src->tiling,
			     intelImage->mt->pitch,
			     dst_bo,
			     0,
			     intelImage->mt->region->tiling,
			     src->draw_x + x, src->draw_y + y,
			     image_x + dstx, image_y + dsty,
			     width, height,
			     GL_COPY)) {
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}


static void
intelCopyTexImage1D(GLcontext * ctx, GLenum target, GLint level,
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

   if (!do_copy_texsubimage(intel_context(ctx), target,
                            intel_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
   if (INTEL_DEBUG & DEBUG_FALLBACKS)
      fprintf(stderr, "%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexImage1D(ctx, target, level, internalFormat, x, y,
                             width, border);
}


static void
intelCopyTexImage2D(GLcontext * ctx, GLenum target, GLint level,
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

   if (!do_copy_texsubimage(intel_context(ctx), target,
                            intel_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
   if (INTEL_DEBUG & DEBUG_FALLBACKS)
      fprintf(stderr, "%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexImage2D(ctx, target, level, internalFormat, x, y,
                             width, height, border);
}


static void
intelCopyTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
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

   if (!do_copy_texsubimage(intel_context(ctx), target,
                            intel_texture_image(texImage),
                            internalFormat, xoffset, 0, x, y, width, 1)) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
         fprintf(stderr, "%s - fallback to swrast\n", __FUNCTION__);
      _mesa_meta_CopyTexSubImage1D(ctx, target, level, xoffset, x, y, width);
   }
}


static void
intelCopyTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
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

   if (!do_copy_texsubimage(intel_context(ctx), target,
                            intel_texture_image(texImage),
                            internalFormat,
                            xoffset, yoffset, x, y, width, height)) {

      if (INTEL_DEBUG & DEBUG_FALLBACKS)
         fprintf(stderr, "%s - fallback to swrast\n", __FUNCTION__);
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
