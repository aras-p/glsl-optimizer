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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"

#include "lp_debug.h"
#include "lp_state.h"
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"
#include "lp_bld_debug.h"
#include "lp_bin.h"



/**
 * Begin the rasterization phase.
 * Map the framebuffer surfaces.  Initialize the 'rast' state.
 */
static boolean
lp_rast_begin( struct lp_rasterizer *rast,
                       struct pipe_surface *cbuf,
                       struct pipe_surface *zsbuf,
                       boolean write_color,
                       boolean write_zstencil,
                       unsigned width,
                       unsigned height )
{
   struct pipe_screen *screen = rast->screen;

   LP_DBG(DEBUG_RAST, "%s %dx%d\n", __FUNCTION__, width, height);

   pipe_surface_reference(&rast->state.cbuf, cbuf);
   pipe_surface_reference(&rast->state.zsbuf, zsbuf);

   rast->width = width;
   rast->height = height;
   rast->state.write_zstencil = write_zstencil;
   rast->state.write_color = write_color;

   rast->check_for_clipped_tiles = (width % TILE_SIZE != 0 ||
                                    height % TILE_SIZE != 0);

   if (cbuf) {
      rast->cbuf_transfer = screen->get_tex_transfer(rast->screen,
                                                     cbuf->texture,
                                                     cbuf->face,
                                                     cbuf->level,
                                                     cbuf->zslice,
                                                     PIPE_TRANSFER_READ_WRITE,
                                                     0, 0, width, height);
      if (!rast->cbuf_transfer)
         return FALSE;

      rast->cbuf_map = screen->transfer_map(rast->screen, 
                                            rast->cbuf_transfer);
      if (!rast->cbuf_map)
         return FALSE;
   }

   if (zsbuf) {
      rast->zsbuf_transfer = screen->get_tex_transfer(rast->screen,
                                                     zsbuf->texture,
                                                     zsbuf->face,
                                                     zsbuf->level,
                                                     zsbuf->zslice,
                                                     PIPE_TRANSFER_READ_WRITE,
                                                     0, 0, width, height);
      if (!rast->zsbuf_transfer)
         return FALSE;

      rast->zsbuf_map = screen->transfer_map(rast->screen, 
                                            rast->zsbuf_transfer);
      if (!rast->zsbuf_map)
         return FALSE;
   }

   return TRUE;
}


/**
 * Finish the rasterization phase.
 * Unmap framebuffer surfaces.
 */
static void
lp_rast_end( struct lp_rasterizer *rast )
{
   struct pipe_screen *screen = rast->screen;

   if (rast->cbuf_map) 
      screen->transfer_unmap(screen, rast->cbuf_transfer);

   if (rast->zsbuf_map) 
      screen->transfer_unmap(screen, rast->zsbuf_transfer);

   if (rast->cbuf_transfer)
      screen->tex_transfer_destroy(rast->cbuf_transfer);

   if (rast->zsbuf_transfer)
      screen->tex_transfer_destroy(rast->zsbuf_transfer);

   rast->cbuf_transfer = NULL;
   rast->zsbuf_transfer = NULL;
   rast->cbuf_map = NULL;
   rast->zsbuf_map = NULL;
}


/**
 * Begining rasterization of a tile.
 * \param x  window X position of the tile, in pixels
 * \param y  window Y position of the tile, in pixels
 */
static void
lp_rast_start_tile( struct lp_rasterizer *rast,
                    unsigned thread_index,
                    unsigned x, unsigned y )
{
   LP_DBG(DEBUG_RAST, "%s %d,%d\n", __FUNCTION__, x, y);

   rast->tasks[thread_index].x = x;
   rast->tasks[thread_index].y = y;
}


/**
 * Clear the rasterizer's current color tile.
 * This is a bin command called during bin processing.
 */
void lp_rast_clear_color( struct lp_rasterizer *rast,
                          unsigned thread_index,
                          const union lp_rast_cmd_arg arg )
{
   const uint8_t *clear_color = arg.clear_color;
   uint8_t *color_tile = rast->tasks[thread_index].tile.color;
   
   LP_DBG(DEBUG_RAST, "%s 0x%x,0x%x,0x%x,0x%x\n", __FUNCTION__, 
              clear_color[0],
              clear_color[1],
              clear_color[2],
              clear_color[3]);

   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      memset(color_tile, clear_color[0], TILE_SIZE * TILE_SIZE * 4);
   }
   else {
      unsigned x, y, chan;
      for (y = 0; y < TILE_SIZE; y++)
         for (x = 0; x < TILE_SIZE; x++)
            for (chan = 0; chan < 4; ++chan)
               TILE_PIXEL(color_tile, x, y, chan) = clear_color[chan];
   }
}


/**
 * Clear the rasterizer's current z/stencil tile.
 * This is a bin command called during bin processing.
 */
void lp_rast_clear_zstencil( struct lp_rasterizer *rast,
                             unsigned thread_index,
                             const union lp_rast_cmd_arg arg)
{
   unsigned i, j;
   uint32_t *depth_tile = rast->tasks[thread_index].tile.depth;
   
   LP_DBG(DEBUG_RAST, "%s 0x%x\n", __FUNCTION__, arg.clear_zstencil);

   for (i = 0; i < TILE_SIZE; i++)
      for (j = 0; j < TILE_SIZE; j++)
	 depth_tile[i*TILE_SIZE + j] = arg.clear_zstencil;
}


/**
 * Load tile color from the framebuffer surface.
 * This is a bin command called during bin processing.
 */
void lp_rast_load_color( struct lp_rasterizer *rast,
                         unsigned thread_index,
                         const union lp_rast_cmd_arg arg)
{
   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   /* call u_tile func to load colors from surface */
}


/**
 * Load tile z/stencil from the framebuffer surface.
 * This is a bin command called during bin processing.
 */
void lp_rast_load_zstencil( struct lp_rasterizer *rast,
                            unsigned thread_index,
                            const union lp_rast_cmd_arg arg )
{
   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   /* call u_tile func to load depth (and stencil?) from surface */
}


void lp_rast_set_state( struct lp_rasterizer *rast,
                        unsigned thread_index,
                        const union lp_rast_cmd_arg arg )
{
   const struct lp_rast_state *state = arg.set_state;

   LP_DBG(DEBUG_RAST, "%s %p\n", __FUNCTION__, (void *) state);

   /* just set the current state pointer for this rasterizer */
   rast->tasks[thread_index].current_state = state;
}



/* Within a tile:
 */

/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle.
 * This is a bin command called during bin processing.
 */
void lp_rast_shade_tile( struct lp_rasterizer *rast,
                         unsigned thread_index,
                         const union lp_rast_cmd_arg arg )
{
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   const unsigned tile_x = rast->tasks[thread_index].x;
   const unsigned tile_y = rast->tasks[thread_index].y;
   const unsigned mask = ~0;
   unsigned x, y;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   /* Use the existing preference for 4x4 (four quads) shading:
    */
   for (y = 0; y < TILE_SIZE; y += 4)
      for (x = 0; x < TILE_SIZE; x += 4)
         lp_rast_shade_quads( rast,
                              thread_index,
                              inputs,
                              tile_x + x,
                              tile_y + y,
                              mask);
}


/**
 * Compute shading for a 4x4 block of pixels.
 * This is a bin command called during bin processing.
 */
void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          unsigned thread_index,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          unsigned mask)
{
#if 1
   const struct lp_rast_state *state = rast->tasks[thread_index].current_state;
   struct lp_rast_tile *tile = &rast->tasks[thread_index].tile;
   void *color;
   void *depth;
   uint32_t ALIGN16_ATTRIB masks[2][2][2][2];
   unsigned ix, iy;
   int block_offset;

   assert(state);

   /* Sanity checks */
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   /* mask: the rasterizer wants to treat pixels in 4x4 blocks, but
    * the pixel shader wants to swizzle them into 4 2x2 quads.
    * 
    * Additionally, the pixel shader wants masks as full dword ~0,
    * while the rasterizer wants to pack per-pixel bits tightly.
    */
#if 0
   unsigned qx, qy;
   for (qy = 0; qy < 2; ++qy)
      for (qx = 0; qx < 2; ++qx)
	 for (iy = 0; iy < 2; ++iy)
	    for (ix = 0; ix < 2; ++ix)
	       masks[qy][qx][iy][ix] = mask & (1 << (qy*8+iy*4+qx*2+ix)) ? ~0 : 0;
#else
   masks[0][0][0][0] = mask & (1 << (0*8+0*4+0*2+0)) ? ~0 : 0;
   masks[0][0][0][1] = mask & (1 << (0*8+0*4+0*2+1)) ? ~0 : 0;
   masks[0][0][1][0] = mask & (1 << (0*8+1*4+0*2+0)) ? ~0 : 0;
   masks[0][0][1][1] = mask & (1 << (0*8+1*4+0*2+1)) ? ~0 : 0;
   masks[0][1][0][0] = mask & (1 << (0*8+0*4+1*2+0)) ? ~0 : 0;
   masks[0][1][0][1] = mask & (1 << (0*8+0*4+1*2+1)) ? ~0 : 0;
   masks[0][1][1][0] = mask & (1 << (0*8+1*4+1*2+0)) ? ~0 : 0;
   masks[0][1][1][1] = mask & (1 << (0*8+1*4+1*2+1)) ? ~0 : 0;

   masks[1][0][0][0] = mask & (1 << (1*8+0*4+0*2+0)) ? ~0 : 0;
   masks[1][0][0][1] = mask & (1 << (1*8+0*4+0*2+1)) ? ~0 : 0;
   masks[1][0][1][0] = mask & (1 << (1*8+1*4+0*2+0)) ? ~0 : 0;
   masks[1][0][1][1] = mask & (1 << (1*8+1*4+0*2+1)) ? ~0 : 0;
   masks[1][1][0][0] = mask & (1 << (1*8+0*4+1*2+0)) ? ~0 : 0;
   masks[1][1][0][1] = mask & (1 << (1*8+0*4+1*2+1)) ? ~0 : 0;
   masks[1][1][1][0] = mask & (1 << (1*8+1*4+1*2+0)) ? ~0 : 0;
   masks[1][1][1][1] = mask & (1 << (1*8+1*4+1*2+1)) ? ~0 : 0;
#endif

   assert((x % 2) == 0);
   assert((y % 2) == 0);

   ix = x % TILE_SIZE;
   iy = y % TILE_SIZE;

   /* offset of the 16x16 pixel block within the tile */
   block_offset = ((iy/4)*(16*16) + (ix/4)*16);

   /* color buffer */
   color = tile->color + 4 * block_offset;

   /* depth buffer */
   depth = tile->depth + block_offset;

   /* XXX: This will most likely fail on 32bit x86 without -mstackrealign */
   assert(lp_check_alignment(masks, 16));

   assert(lp_check_alignment(depth, 16));
   assert(lp_check_alignment(color, 16));
   assert(lp_check_alignment(state->jit_context.blend_color, 16));

   /* run shader */
   state->jit_function( &state->jit_context,
                        x, y,
                        inputs->a0,
                        inputs->dadx,
                        inputs->dady,
                        &masks[0][0][0][0],
                        color,
                        depth);
#else
   struct lp_rast_tile *tile = &rast->tile;
   unsigned chan_index;
   unsigned q, ix, iy;

   x %= TILE_SIZE;
   y %= TILE_SIZE;

   /* mask */
   for (q = 0; q < 4; ++q)
      for(iy = 0; iy < 2; ++iy)
         for(ix = 0; ix < 2; ++ix)
            if(masks[q] & (1 << (iy*2 + ix)))
               for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
                  TILE_PIXEL(tile->color, x + q*2 + ix, y + iy, chan_index) = 0xff;

#endif
}


/* End of tile:
 */


/**
 * Write the rasterizer's color tile to the framebuffer.
 */
static void lp_rast_store_color( struct lp_rasterizer *rast,
                                 unsigned thread_index)
{
   const unsigned x = rast->tasks[thread_index].x;
   const unsigned y = rast->tasks[thread_index].y;
   int w = TILE_SIZE;
   int h = TILE_SIZE;

   if (x + w > rast->width)
      w -= x + w - rast->width;

   if (y + h > rast->height)
      h -= y + h - rast->height;

   assert(w >= 0);
   assert(h >= 0);
   assert(w <= TILE_SIZE);
   assert(h <= TILE_SIZE);

   LP_DBG(DEBUG_RAST, "%s [%u] %d,%d %dx%d\n", __FUNCTION__,
          thread_index, x, y, w, h);

   lp_tile_write_4ub(rast->cbuf_transfer->format,
                     rast->tasks[thread_index].tile.color,
                     rast->cbuf_map, 
                     rast->cbuf_transfer->stride,
                     x, y,
                     w, h);
}


static void
lp_tile_write_z32(const uint32_t *src, uint8_t *dst, unsigned dst_stride,
                  unsigned x0, unsigned y0, unsigned w, unsigned h)
{
   unsigned x, y;
   uint8_t *dst_row = dst + y0*dst_stride;
   for (y = 0; y < h; ++y) {
      uint32_t *dst_pixel = (uint32_t *)(dst_row + x0*4);
      for (x = 0; x < w; ++x) {
         *dst_pixel++ = *src++;
      }
      dst_row += dst_stride;
   }
}

/**
 * Write the rasterizer's z/stencil tile to the framebuffer.
 */
static void lp_rast_store_zstencil( struct lp_rasterizer *rast,
                                    unsigned thread_index )
{
   const unsigned x = rast->tasks[thread_index].x;
   const unsigned y = rast->tasks[thread_index].y;
   unsigned w = TILE_SIZE;
   unsigned h = TILE_SIZE;

   if (x + w > rast->width)
      w -= x + w - rast->width;

   if (y + h > rast->height)
      h -= y + h - rast->height;

   LP_DBG(DEBUG_RAST, "%s %d,%d %dx%d\n", __FUNCTION__, x, y, w, h);

   assert(rast->zsbuf_transfer->format == PIPE_FORMAT_Z32_UNORM);
   lp_tile_write_z32(rast->tasks[thread_index].tile.depth,
                     rast->zsbuf_map, 
                     rast->zsbuf_transfer->stride,
                     x, y, w, h);
}


/**
 * Write the rasterizer's tiles to the framebuffer.
 */
static void
lp_rast_end_tile( struct lp_rasterizer *rast,
                  unsigned thread_index )
{
   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   if (rast->state.write_color)
      lp_rast_store_color(rast, thread_index);

   if (rast->state.write_zstencil)
      lp_rast_store_zstencil(rast, thread_index);
}


/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 */
static void
rasterize_bin( struct lp_rasterizer *rast,
               unsigned thread_index,
               const struct cmd_bin *bin,
               int x, int y)
{
   const struct cmd_block_list *commands = &bin->commands;
   struct cmd_block *block;
   unsigned k;

   lp_rast_start_tile( rast, thread_index, x, y );

   /* simply execute each of the commands in the block list */
   for (block = commands->head; block; block = block->next) {
      for (k = 0; k < block->count; k++) {
         block->cmd[k]( rast, thread_index, block->arg[k] );
      }
   }

   lp_rast_end_tile( rast, thread_index );
}


/**
 * Rasterize/execute all bins.
 */
static void
rasterize_bins( struct lp_rasterizer *rast,
                unsigned thread_index,
                struct lp_bins *bins,
                const struct pipe_framebuffer_state *fb,
                bool write_depth )
{
   /* loop over tile bins, rasterize each */
#if 0
   {
      unsigned i, j;
      for (i = 0; i < bins->tiles_x; i++) {
         for (j = 0; j < bins->tiles_y; j++) {
            struct cmd_bin *bin = lp_get_bin(bins, i, j);
            rasterize_bin( rast, thread_index,
                           bin, i * TILE_SIZE, j * TILE_SIZE );
         }
      }
   }
#else
   {
      struct cmd_bin *bin;
      int x, y;

      while ((bin = lp_bin_iter_next(bins, &x, &y))) {
         rasterize_bin( rast, thread_index, bin, x * TILE_SIZE, y * TILE_SIZE);
      }
   }
#endif
}


/**
 * Called by rasterizer when it has something for us to render.
 */
void
lp_rasterize_bins( struct lp_rasterizer *rast,
                   struct lp_bins *bins,
                   const struct pipe_framebuffer_state *fb,
                   bool write_depth )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   lp_rast_begin( rast,
                  fb->cbufs[0], 
                  fb->zsbuf,
                  fb->cbufs[0] != NULL,
                  fb->zsbuf != NULL && write_depth,
                  fb->width,
                  fb->height );

   if (rast->num_threads == 0) {
      /* no threading */
      lp_bin_iter_begin( bins );
      rasterize_bins( rast, 0, bins, fb, write_depth );
   }
   else {
      /* threaded rendering! */
      unsigned i;

      rast->bins = bins;
      rast->fb = fb;
      rast->write_depth = write_depth;

      lp_bin_iter_begin( bins );

      /* signal the threads that there's work to do */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_signal(&rast->tasks[i].work_ready);
      }

      /* wait for work to complete */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_wait(&rast->tasks[i].work_done);
      }
   }

   lp_rast_end( rast );

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}


/**
 * This is the thread's main entrypoint.
 * It's a simple loop:
 *   1. wait for work
 *   2. do work
 *   3. signal that we're done
 */
static void *
thread_func( void *init_data )
{
   struct lp_rasterizer_task *task = (struct lp_rasterizer_task *) init_data;
   struct lp_rasterizer *rast = task->rast;
   int debug = 0;

   while (1) {
      /* wait for work */
      if (debug)
         debug_printf("thread %d waiting for work\n", task->thread_index);
      pipe_semaphore_wait(&task->work_ready);

      /* do work */
      if (debug)
         debug_printf("thread %d doing work\n", task->thread_index);
      rasterize_bins(rast, task->thread_index,
                     rast->bins, rast->fb, rast->write_depth);

      /* signal done with work */
      if (debug)
         debug_printf("thread %d done working\n", task->thread_index);
      pipe_semaphore_signal(&task->work_done);
   }

   return NULL;
}


/**
 * Initialize semaphores and spawn the threads.
 */
static void
create_rast_threads(struct lp_rasterizer *rast)
{
   unsigned i;

   rast->num_threads = util_cpu_caps.nr_cpus;
   rast->num_threads = debug_get_num_option("LP_NUM_THREADS", rast->num_threads);
   rast->num_threads = MIN2(rast->num_threads, MAX_THREADS);

   /* NOTE: if num_threads is zero, we won't use any threads */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_init(&rast->tasks[i].work_ready, 0);
      pipe_semaphore_init(&rast->tasks[i].work_done, 0);
      rast->threads[i] = pipe_thread_create(thread_func,
                                            (void *) &rast->tasks[i]);
   }
}



struct lp_rasterizer *lp_rast_create( struct pipe_screen *screen )
{
   struct lp_rasterizer *rast;
   unsigned i;

   rast = CALLOC_STRUCT(lp_rasterizer);
   if(!rast)
      return NULL;

   rast->screen = screen;

   for (i = 0; i < Elements(rast->tasks); i++) {
      rast->tasks[i].tile.color = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );
      rast->tasks[i].tile.depth = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );
      rast->tasks[i].rast = rast;
      rast->tasks[i].thread_index = i;
   }

   create_rast_threads(rast);

   return rast;
}


/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   unsigned i;

   pipe_surface_reference(&rast->state.cbuf, NULL);
   pipe_surface_reference(&rast->state.zsbuf, NULL);

   for (i = 0; i < Elements(rast->tasks); i++) {
      align_free(rast->tasks[i].tile.depth);
      align_free(rast->tasks[i].tile.color);
   }

   FREE(rast);
}

