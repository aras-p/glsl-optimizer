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
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_surface.h"
#include "lp_texture.h"
#include "lp_tile_image.h"
#include "lp_tile_size.h"


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
lp_surface_copy(struct pipe_context *pipe,
                struct pipe_surface *dst, unsigned dstx, unsigned dsty,
                struct pipe_surface *src, unsigned srcx, unsigned srcy,
                unsigned width, unsigned height)
{
   struct llvmpipe_resource *src_tex = llvmpipe_resource(src->texture);
   struct llvmpipe_resource *dst_tex = llvmpipe_resource(dst->texture);
   const enum pipe_format format = src_tex->base.format;

   llvmpipe_flush_texture(pipe,
                          dst->texture, dst->face, dst->level,
                          0, /* flush_flags */
                          FALSE, /* read_only */
                          FALSE, /* cpu_access */
                          FALSE); /* do_not_flush */

   llvmpipe_flush_texture(pipe,
                          src->texture, src->face, src->level,
                          0, /* flush_flags */
                          TRUE, /* read_only */
                          FALSE, /* cpu_access */
                          FALSE); /* do_not_flush */

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
                                                    src->face, src->level,
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

      /* XXX for the tiles which are completely contained by the
       * dest rectangle, we could set the usage mode to WRITE_ALL.
       * Just test for the case of replacing the whole dest region for now.
       */
      if (width == dst_tex->base.width0 && height == dst_tex->base.height0)
         usage = LP_TEX_USAGE_WRITE_ALL;
      else
         usage = LP_TEX_USAGE_READ_WRITE;

      adjust_to_tile_bounds(dstx, dsty, width, height, &tx, &ty, &tw, &th);

      for (y = 0; y < th; y += TILE_SIZE) {
         for (x = 0; x < tw; x += TILE_SIZE) {
            (void) llvmpipe_get_texture_tile_linear(dst_tex,
                                                    dst->face, dst->level,
                                                    usage,
                                                    tx + x, ty + y);
         }
      }
   }

   /* copy */
   {
      const ubyte *src_linear_ptr
         = llvmpipe_get_texture_image_address(src_tex, src->face,
                                              src->level,
                                              LP_TEX_LAYOUT_LINEAR);
      ubyte *dst_linear_ptr
         = llvmpipe_get_texture_image_address(dst_tex, dst->face,
                                              dst->level,
                                              LP_TEX_LAYOUT_LINEAR);

      util_copy_rect(dst_linear_ptr, format,
                     dst_tex->stride[dst->level],
                     dstx, dsty,
                     width, height,
                     src_linear_ptr, src_tex->stride[src->level],
                     srcx, srcy);
   }
}


void
lp_init_surface_functions(struct llvmpipe_context *lp)
{
   lp->pipe.surface_copy = lp_surface_copy;
   lp->pipe.surface_fill = util_surface_fill;
}
