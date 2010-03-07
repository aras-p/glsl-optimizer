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

#include "os/os_thread.h"
#include "util/u_format.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_rast.h"
#include "lp_tile_soa.h"


#define MAX_THREADS 8  /* XXX probably temporary here */


struct lp_rasterizer;


/**
 * A tile's color and depth memory.
 * We can choose whatever layout for the internal tile storage we prefer.
 */
struct lp_rast_tile
{
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
};


/**
 * Per-thread rasterization state
 */
struct lp_rasterizer_task
{
   struct lp_rast_tile tile;   /** Tile color/z/stencil memory */

   unsigned x, y;          /**< Pos of this tile in framebuffer, in pixels */

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
   boolean exit_flag;

   /* Framebuffer stuff
    */
   struct {
      void *map;
      unsigned stride;
      unsigned width;
      unsigned height;
      enum pipe_format format;
   } cbuf[PIPE_MAX_COLOR_BUFS];

   struct {
      uint8_t *map;
      unsigned stride;
      unsigned blocksize;
   } zsbuf;

   struct {
      unsigned nr_cbufs;
      boolean write_color;
      boolean write_zstencil;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;

   /** The incoming queue of scenes ready to rasterize */
   struct lp_scene_queue *full_scenes;

   /**
    * The outgoing queue of processed scenes to return to setup module
    *
    * XXX: while scenes are per-context but the rasterizer is
    * (potentially) shared, these empty scenes should be returned to
    * the context which created them rather than retained here.
    */
   struct lp_scene_queue *empty_scenes;

   /** The scene currently being rasterized by the threads */
   struct lp_scene *curr_scene;

   /** A task object for each rasterization thread */
   struct lp_rasterizer_task tasks[MAX_THREADS];

   unsigned num_threads;
   pipe_thread threads[MAX_THREADS];

   /** For synchronizing the rasterization threads */
   pipe_barrier barrier;
};


void lp_rast_shade_quads( struct lp_rasterizer_task *task,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          int32_t c1, int32_t c2, int32_t c3);


/**
 * Get the pointer to the depth buffer for a block.
 * \param x, y location of 4x4 block in window coords
 */
static INLINE void *
lp_rast_depth_pointer( struct lp_rasterizer *rast,
                       unsigned x, unsigned y )
{
   void * depth;

   assert((x % TILE_VECTOR_WIDTH) == 0);
   assert((y % TILE_VECTOR_HEIGHT) == 0);

   if (!rast->zsbuf.map)
      return NULL;

   depth = (rast->zsbuf.map +
            rast->zsbuf.stride * y +
            rast->zsbuf.blocksize * x * TILE_VECTOR_HEIGHT);

   assert(lp_check_alignment(depth, 16));
   return depth;
}



/**
 * Shade all pixels in a 4x4 block.  The fragment code omits the
 * triangle in/out tests.
 * \param x, y location of 4x4 block in window coords
 */
static INLINE void
lp_rast_shade_quads_all( struct lp_rasterizer_task *task,
                         const struct lp_rast_shader_inputs *inputs,
                         unsigned x, unsigned y )
{
   struct lp_rasterizer *rast = task->rast;
   const struct lp_rast_state *state = task->current_state;
   struct lp_rast_tile *tile = &task->tile;
   const unsigned ix = x % TILE_SIZE, iy = y % TILE_SIZE;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   void *depth;
   unsigned block_offset, i;

   /* offset of the containing 16x16 pixel block within the tile */
   block_offset = (iy / 4) * (16 * 16) + (ix / 4) * 16;

   /* color buffer */
   for (i = 0; i < rast->state.nr_cbufs; i++)
      color[i] = tile->color[i] + 4 * block_offset;

   depth = lp_rast_depth_pointer(rast, x, y);

   /* run shader */
   state->jit_function[0]( &state->jit_context,
                           x, y,
                           inputs->a0,
                           inputs->dadx,
                           inputs->dady,
                           color,
                           depth,
                           INT_MIN, INT_MIN, INT_MIN,
                           NULL, NULL, NULL );
}


#endif
