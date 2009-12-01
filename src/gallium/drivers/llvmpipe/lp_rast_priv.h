/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef LP_RAST_PRIV_H
#define LP_RAST_PRIV_H

#include "lp_rast.h"

struct pipe_transfer;
struct pipe_screen;

/* We can choose whatever layout for the internal tile storage we
 * prefer:
 */
struct lp_rast_tile
{
   uint8_t *color;

   uint32_t *depth;
};


struct lp_rasterizer {

   /* We can choose whatever layout for the internal tile storage we
    * prefer:
    */
   struct lp_rast_tile tile;

   unsigned x;
   unsigned y;
   boolean clipped_tile;

   boolean check_for_clipped_tiles;
   unsigned width;
   unsigned height;

   struct pipe_screen *screen;
   struct pipe_transfer *cbuf_transfer;
   struct pipe_transfer *zsbuf_transfer;
   void *cbuf_map;
   void *zsbuf_map;

   struct {
      struct pipe_surface *cbuf;
      struct pipe_surface *zsbuf;
      boolean write_color;
      boolean write_zstencil;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;

   unsigned nr_blocks;
   struct {
      unsigned x;
      unsigned y;
      unsigned mask;
   } blocks[256];
};


void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          unsigned masks);

#endif
