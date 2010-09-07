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

#include "util/u_rect.h"
#include "util/u_surface.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_limits.h"
#include "lp_surface.h"
#include "lp_texture.h"


/**
 * Adjust x, y, width, height to lie on tile bounds.
 */
static void
adjust_to_tile_bounds(unsigned x, unsigned y, unsigned width, unsigned height,
                      unsigned *x_tile, unsigned *y_tile,
                      unsigned *w_tile, unsigned *h_tile)
{
   *x_tile = x & ~(TILE_SIZE - 1);
   *y_tile = y & ~(TILE_SIZE - 1);
   *w_tile = ((x + width + TILE_SIZE - 1) & ~(TILE_SIZE - 1)) - *x_tile;
   *h_tile = ((y + height + TILE_SIZE - 1) & ~(TILE_SIZE - 1)) - *y_tile;
}



static void
lp_resource_copy(struct pipe_context *pipe,
                 struct pipe_resource *dst, struct pipe_subresource subdst,
                 unsigned dstx, unsigned dsty, unsigned dstz,
                 struct pipe_resource *src, struct pipe_subresource subsrc,
                 unsigned srcx, unsigned srcy, unsigned srcz,
                 unsigned width, unsigned height)
{
   /* XXX what about the dstz/srcz parameters - zslice wasn't used... */
   struct llvmpipe_resource *src_tex = llvmpipe_resource(src);
   struct llvmpipe_resource *dst_tex = llvmpipe_resource(dst);
   const enum pipe_format format = src_tex->base.format;

   llvmpipe_flush_resource(pipe,
                           dst, subdst.face, subdst.level,
                           0, /* flush_flags */
                           FALSE, /* read_only */
                           TRUE, /* cpu_access */
                           FALSE, /* do_not_block */
                           "blit dest");

   llvmpipe_flush_resource(pipe,
                           src, subsrc.face, subsrc.level,
                           0, /* flush_flags */
                           TRUE, /* read_only */
                           TRUE, /* cpu_access */
                           FALSE, /* do_not_block */
                           "blit src");

   /*
   printf("surface copy from %u to %u: %u,%u to %u,%u %u x %u\n",
          src_tex->id, dst_tex->id,
          srcx, srcy, dstx, dsty, width, height);
   */

   /* set src tiles to linear layout */
   {
      unsigned tx, ty, tw, th;
      unsigned x, y;

      adjust_to_tile_bounds(srcx, srcy, width, height, &tx, &ty, &tw, &th);

      for (y = 0; y < th; y += TILE_SIZE) {
         for (x = 0; x < tw; x += TILE_SIZE) {
            (void) llvmpipe_get_texture_tile_linear(src_tex,
                                                    subsrc.face, subsrc.level,
                                                    LP_TEX_USAGE_READ,
                                                    tx + x, ty + y);
         }
      }
   }

   /* set dst tiles to linear layout */
   {
      unsigned tx, ty, tw, th;
      unsigned x, y;
      enum lp_texture_usage usage;

      adjust_to_tile_bounds(dstx, dsty, width, height, &tx, &ty, &tw, &th);

      for (y = 0; y < th; y += TILE_SIZE) {
         boolean contained_y = ty + y >= dsty &&
                               ty + y + TILE_SIZE <= dsty + height ?
                               TRUE : FALSE;

         for (x = 0; x < tw; x += TILE_SIZE) {
            boolean contained_x = tx + x >= dstx &&
                                  tx + x + TILE_SIZE <= dstx + width ?
                                  TRUE : FALSE;

            /*
             * Set the usage mode to WRITE_ALL for the tiles which are
             * completely contained by the dest rectangle.
             */
            if (contained_y && contained_x)
               usage = LP_TEX_USAGE_WRITE_ALL;
            else
               usage = LP_TEX_USAGE_READ_WRITE;

            (void) llvmpipe_get_texture_tile_linear(dst_tex,
                                                    subdst.face, subdst.level,
                                                    usage,
                                                    tx + x, ty + y);
         }
      }
   }

   /* copy */
   {
      const ubyte *src_linear_ptr
         = llvmpipe_get_texture_image_address(src_tex, subsrc.face,
                                              subsrc.level,
                                              LP_TEX_LAYOUT_LINEAR);
      ubyte *dst_linear_ptr
         = llvmpipe_get_texture_image_address(dst_tex, subdst.face,
                                              subdst.level,
                                              LP_TEX_LAYOUT_LINEAR);

      if (dst_linear_ptr && src_linear_ptr) {
         util_copy_rect(dst_linear_ptr, format,
                        llvmpipe_resource_stride(&dst_tex->base, subdst.level),
                        dstx, dsty,
                        width, height,
                        src_linear_ptr,
                        llvmpipe_resource_stride(&src_tex->base, subsrc.level),
                        srcx, srcy);
      }
   }
}


void
llvmpipe_init_surface_functions(struct llvmpipe_context *lp)
{
   lp->pipe.resource_copy_region = lp_resource_copy;
   lp->pipe.clear_render_target = util_clear_render_target;
   lp->pipe.clear_depth_stencil = util_clear_depth_stencil;
}
