/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/image.h"
#include "main/pbo.h"
#include "main/imports.h"
#include "main/readpix.h"
#include "main/enums.h"
#include "main/framebuffer.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "st_cb_fbo.h"
#include "st_atom.h"
#include "st_context.h"
#include "st_cb_bitmap.h"
#include "st_cb_readpixels.h"
#include "state_tracker/st_cb_texture.h"
#include "state_tracker/st_format.h"
#include "state_tracker/st_texture.h"


/**
 * This uses a blit to copy the read buffer to a texture format which matches
 * the format and type combo and then a fast read-back is done using memcpy.
 * We can do arbitrary X/Y/Z/W/0/1 swizzling here as long as there is
 * a format which matches the swizzling.
 *
 * If such a format isn't available, we fall back to _mesa_readpixels.
 *
 * NOTE: Some drivers use a blit to convert between tiled and linear
 *       texture layouts during texture uploads/downloads, so the blit
 *       we do here should be free in such cases.
 */
static void
st_readpixels(struct gl_context *ctx, GLint x, GLint y,
              GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *pack,
              GLvoid *pixels)
{
   struct st_context *st = st_context(ctx);
   struct gl_renderbuffer *rb =
         _mesa_get_read_renderbuffer_for_format(ctx, format);
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *src;
   struct pipe_resource *dst = NULL;
   struct pipe_resource dst_templ;
   enum pipe_format dst_format, src_format;
   struct pipe_blit_info blit;
   unsigned bind = PIPE_BIND_TRANSFER_READ;
   struct pipe_transfer *tex_xfer;
   ubyte *map = NULL;

   /* Validate state (to be sure we have up-to-date framebuffer surfaces)
    * and flush the bitmap cache prior to reading. */
   st_validate_state(st);
   st_flush_bitmap_cache(st);

   if (!st->prefer_blit_based_texture_transfer) {
      goto fallback;
   }

   /* This must be done after state validation. */
   src = strb->texture;

   /* XXX Fallback for depth-stencil formats due to an incomplete
    * stencil blit implementation in some drivers. */
   if (format == GL_DEPTH_STENCIL) {
      goto fallback;
   }

   /* We are creating a texture of the size of the region being read back.
    * Need to check for NPOT texture support. */
   if (!screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES) &&
       (!util_is_power_of_two(width) ||
        !util_is_power_of_two(height))) {
      goto fallback;
   }

   /* If the base internal format and the texture format don't match, we have
    * to use the slow path. */
   if (rb->_BaseFormat !=
       _mesa_get_format_base_format(rb->Format)) {
      goto fallback;
   }

   /* See if the texture format already matches the format and type,
    * in which case the memcpy-based fast path will likely be used and
    * we don't have to blit. */
   if (_mesa_format_matches_format_and_type(rb->Format, format,
                                            type, pack->SwapBytes)) {
      goto fallback;
   }

   if (_mesa_readpixels_needs_slow_path(ctx, format, type, GL_TRUE)) {
      goto fallback;
   }

   /* Convert the source format to what is expected by ReadPixels
    * and see if it's supported. */
   src_format = util_format_linear(src->format);
   src_format = util_format_luminance_to_red(src_format);
   src_format = util_format_intensity_to_red(src_format);

   if (!src_format ||
       !screen->is_format_supported(screen, src_format, src->target,
                                    src->nr_samples,
                                    PIPE_BIND_SAMPLER_VIEW)) {
      printf("fallback: src format unsupported %s\n", util_format_short_name(src_format));
      goto fallback;
   }

   if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
      bind |= PIPE_BIND_DEPTH_STENCIL;
   else
      bind |= PIPE_BIND_RENDER_TARGET;

   /* Choose the destination format by finding the best match
    * for the format+type combo. */
   dst_format = st_choose_matching_format(screen, bind, format, type,
                                          pack->SwapBytes);
   if (dst_format == PIPE_FORMAT_NONE) {
      printf("fallback: no matching format for %s, %s\n",
             _mesa_lookup_enum_by_nr(format), _mesa_lookup_enum_by_nr(type));
      goto fallback;
   }

   /* create the destination texture */
   memset(&dst_templ, 0, sizeof(dst_templ));
   dst_templ.target = PIPE_TEXTURE_2D;
   dst_templ.format = dst_format;
   dst_templ.bind = bind;
   dst_templ.usage = PIPE_USAGE_STAGING;

   st_gl_texture_dims_to_pipe_dims(GL_TEXTURE_2D, width, height, 1,
                                   &dst_templ.width0, &dst_templ.height0,
                                   &dst_templ.depth0, &dst_templ.array_size);

   dst = screen->resource_create(screen, &dst_templ);
   if (!dst) {
      goto fallback;
   }

   blit.src.resource = src;
   blit.src.level = strb->rtt_level;
   blit.src.format = src_format;
   blit.dst.resource = dst;
   blit.dst.level = 0;
   blit.dst.format = dst->format;
   blit.src.box.x = x;
   blit.dst.box.x = 0;
   blit.src.box.y = y;
   blit.dst.box.y = 0;
   blit.src.box.z = strb->rtt_face + strb->rtt_slice;
   blit.dst.box.z = 0;
   blit.src.box.width = blit.dst.box.width = width;
   blit.src.box.height = blit.dst.box.height = height;
   blit.src.box.depth = blit.dst.box.depth = 1;
   blit.mask = st_get_blit_mask(rb->_BaseFormat, format);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
   blit.scissor_enable = FALSE;

   if (st_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      blit.src.box.y = rb->Height - blit.src.box.y;
      blit.src.box.height = -blit.src.box.height;
   }

   /* blit */
   st->pipe->blit(st->pipe, &blit);

   /* map resources */
   pixels = _mesa_map_pbo_dest(ctx, pack, pixels);

   map = pipe_transfer_map_3d(pipe, dst, 0, PIPE_TRANSFER_READ,
                              0, 0, 0, width, height, 1, &tex_xfer);
   if (!map) {
      _mesa_unmap_pbo_dest(ctx, pack);
      pipe_resource_reference(&dst, NULL);
      goto fallback;
   }

   /* memcpy data into a user buffer */
   {
      const uint bytesPerRow = width * util_format_get_blocksize(dst_format);
      GLuint row;

      for (row = 0; row < height; row++) {
         GLvoid *dest = _mesa_image_address3d(pack, pixels,
                                              width, height, format,
                                              type, 0, row, 0);
         memcpy(dest, map, bytesPerRow);
         map += tex_xfer->stride;
      }
   }

   pipe_transfer_unmap(pipe, tex_xfer);
   _mesa_unmap_pbo_dest(ctx, pack);
   pipe_resource_reference(&dst, NULL);
   return;

fallback:
   _mesa_readpixels(ctx, x, y, width, height, format, type, pack, pixels);
}

void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}
