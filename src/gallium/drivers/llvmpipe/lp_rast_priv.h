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

#include "pipe/p_thread.h"
#include "lp_rast.h"


#define MAX_THREADS 8  /* XXX probably temporary here */


struct pipe_transfer;
struct pipe_screen;
struct lp_rasterizer;


/**
 * A tile's color and depth memory.
 * We can choose whatever layout for the internal tile storage we prefer.
 */
struct lp_rast_tile
{
   uint8_t *color;

   uint32_t *depth;
};


/**
 * Per-thread rasterization state
 */
struct lp_rasterizer_task
{
   struct lp_rast_tile tile;   /** Tile color/z/stencil memory */

   unsigned x, y;          /**< Pos of this tile in framebuffer, in pixels */

   /* Pixel blocks produced during rasterization
    */
   unsigned nr_blocks;
   struct {
      unsigned x;
      unsigned y;
      unsigned mask;
   } blocks[256];

   const struct lp_rast_state *current_state;

   /** "back" pointer */
   struct lp_rasterizer *rast;

   /** "my" index */
   unsigned thread_index;

   pipe_semaphore work_ready;
   pipe_semaphore work_done;
};


/**
 * This is the state required while rasterizing tiles.
 * Note that this contains per-thread information too.
 * The tile size is TILE_SIZE x TILE_SIZE pixels.
 */
struct lp_rasterizer
{
   boolean clipped_tile;
   boolean check_for_clipped_tiles;

   /* Framebuffer stuff
    */
   struct pipe_screen *screen;
   struct pipe_transfer *cbuf_transfer;
   struct pipe_transfer *zsbuf_transfer;
   void *cbuf_map;
   void *zsbuf_map;

   struct {
      struct pipe_framebuffer_state fb;
      boolean write_color;
      boolean write_zstencil;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;

   /** The incoming queue of filled bins to rasterize */
   struct lp_bins_queue *full_bins;
   /** The outgoing queue of emptied bins to return to setup modulee */
   struct lp_bins_queue *empty_bins;

   /** The bins currently being rasterized by the threads */
   struct lp_bins *curr_bins;

   /** A task object for each rasterization thread */
   struct lp_rasterizer_task tasks[MAX_THREADS];

   unsigned num_threads;
   pipe_thread threads[MAX_THREADS];

   /** For synchronizing the rasterization threads */
   pipe_barrier barrier;
};


void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          unsigned thread_index,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          unsigned masks);

#endif
