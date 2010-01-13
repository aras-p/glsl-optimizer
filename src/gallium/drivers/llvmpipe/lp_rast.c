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

#include <limits.h>
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"
#include "util/u_surface.h"

#include "lp_scene_queue.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"
#include "lp_bld_debug.h"
#include "lp_scene.h"


/**
 * Begin the rasterization phase.
 * Map the framebuffer surfaces.  Initialize the 'rast' state.
 */
static boolean
lp_rast_begin( struct lp_rasterizer *rast,
               const struct pipe_framebuffer_state *fb,
               boolean write_color,
               boolean write_zstencil )
{
   struct pipe_screen *screen = rast->screen;
   struct pipe_surface *cbuf, *zsbuf;
   int i;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   util_copy_framebuffer_state(&rast->state.fb, fb);

   rast->state.write_zstencil = write_zstencil;
   rast->state.write_color = write_color;

   rast->check_for_clipped_tiles = (fb->width % TILE_SIZE != 0 ||
                                    fb->height % TILE_SIZE != 0);

   
   for (i = 0; i < rast->state.fb.nr_cbufs; i++) {
      cbuf = rast->state.fb.cbufs[i];
      if (cbuf) {
	 rast->cbuf_transfer[i] = screen->get_tex_transfer(rast->screen,
							   cbuf->texture,
							   cbuf->face,
							   cbuf->level,
							   cbuf->zslice,
							   PIPE_TRANSFER_READ_WRITE,
							   0, 0,
							   cbuf->width, 
							   cbuf->height);
	 if (!rast->cbuf_transfer[i])
	    goto fail;

	 rast->cbuf_map[i] = screen->transfer_map(rast->screen, 
						  rast->cbuf_transfer[i]);
	 if (!rast->cbuf_map[i])
	    goto fail;
      }
   }

   zsbuf = rast->state.fb.zsbuf;
   if (zsbuf) {
      rast->zsbuf_transfer = screen->get_tex_transfer(rast->screen,
                                                      zsbuf->texture,
                                                      zsbuf->face,
                                                      zsbuf->level,
                                                      zsbuf->zslice,
                                                      PIPE_TRANSFER_READ_WRITE,
                                                      0, 0,
                                                      zsbuf->width,
						      zsbuf->height);
      if (!rast->zsbuf_transfer)
         goto fail;

      rast->zsbuf_map = screen->transfer_map(rast->screen, 
                                            rast->zsbuf_transfer);
      if (!rast->zsbuf_map)
	 goto fail;
   }

   return TRUE;

fail:
   /* Unmap and release transfers?
    */
   return FALSE;
}


/**
 * Finish the rasterization phase.
 * Unmap framebuffer surfaces.
 */
static void
lp_rast_end( struct lp_rasterizer *rast )
{
   struct pipe_screen *screen = rast->screen;
   unsigned i;

   for (i = 0; i < rast->state.fb.nr_cbufs; i++) {
      if (rast->cbuf_map[i]) 
	 screen->transfer_unmap(screen, rast->cbuf_transfer[i]);

      if (rast->cbuf_transfer[i])
	 screen->tex_transfer_destroy(rast->cbuf_transfer[i]);

      rast->cbuf_transfer[i] = NULL;
      rast->cbuf_map[i] = NULL;
   }

   if (rast->zsbuf_map) 
      screen->transfer_unmap(screen, rast->zsbuf_transfer);

   if (rast->zsbuf_transfer)
      screen->tex_transfer_destroy(rast->zsbuf_transfer);

   rast->zsbuf_transfer = NULL;
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
   uint8_t **color_tile = rast->tasks[thread_index].tile.color;
   unsigned i;

   LP_DBG(DEBUG_RAST, "%s 0x%x,0x%x,0x%x,0x%x\n", __FUNCTION__, 
              clear_color[0],
              clear_color[1],
              clear_color[2],
              clear_color[3]);

   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      for (i = 0; i < rast->state.fb.nr_cbufs; i++) {
	 memset(color_tile[i], clear_color[0], TILE_SIZE * TILE_SIZE * 4);
      }
   }
   else {
      unsigned x, y, chan;
      for (i = 0; i < rast->state.fb.nr_cbufs; i++)
	 for (y = 0; y < TILE_SIZE; y++)
	    for (x = 0; x < TILE_SIZE; x++)
	       for (chan = 0; chan < 4; ++chan)
		  TILE_PIXEL(color_tile[i], x, y, chan) = clear_color[chan];
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
   struct lp_rasterizer_task *task = &rast->tasks[thread_index];
   const unsigned x = task->x;
   const unsigned y = task->y;
   unsigned i;

   LP_DBG(DEBUG_RAST, "%s at %u, %u\n", __FUNCTION__, x, y);

   for (i = 0; i < rast->state.fb.nr_cbufs; i++) {
      struct pipe_transfer *transfer = rast->cbuf_transfer[i];
      int w = TILE_SIZE;
      int h = TILE_SIZE;

      if (x >= transfer->width)
	 continue;

      if (y >= transfer->height)
	 continue;
      /* XXX: require tile-size aligned render target dimensions:
       */
      if (x + w > transfer->width)
	 w -= x + w - transfer->width;

      if (y + h > transfer->height)
	 h -= y + h - transfer->height;

      assert(w >= 0);
      assert(h >= 0);
      assert(w <= TILE_SIZE);
      assert(h <= TILE_SIZE);

      lp_tile_read_4ub(transfer->texture->format,
		       rast->tasks[thread_index].tile.color[i],
		       rast->cbuf_map[i], 
		       transfer->stride,
		       x, y,
		       w, h);
   }
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
   /* Set c1,c2,c3 to large values so the in/out test always passes */
   const int32_t c1 = INT_MIN, c2 = INT_MIN, c3 = INT_MIN;
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   const unsigned tile_x = rast->tasks[thread_index].x;
   const unsigned tile_y = rast->tasks[thread_index].y;
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
                              c1, c2, c3);
}


/**
 * Compute shading for a 4x4 block of pixels.
 * This is a bin command called during bin processing.
 */
void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          unsigned thread_index,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          int32_t c1, int32_t c2, int32_t c3)
{
   const struct lp_rast_state *state = rast->tasks[thread_index].current_state;
   struct lp_rast_tile *tile = &rast->tasks[thread_index].tile;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   void *depth;
   unsigned i;
   unsigned ix, iy;
   int block_offset;

#ifdef DEBUG
   assert(state);

   /* Sanity checks */
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   assert((x % 4) == 0);
   assert((y % 4) == 0);
#endif

   ix = x % TILE_SIZE;
   iy = y % TILE_SIZE;

   /* offset of the 16x16 pixel block within the tile */
   block_offset = ((iy/4)*(16*16) + (ix/4)*16);

   /* color buffer */
   for (i = 0; i < rast->state.fb.nr_cbufs; i++)
      color[i] = tile->color[i] + 4 * block_offset;

   /* depth buffer */
   depth = tile->depth + block_offset;



#ifdef DEBUG
   assert(lp_check_alignment(tile->depth, 16));
   assert(lp_check_alignment(tile->color[0], 16));
   assert(lp_check_alignment(state->jit_context.blend_color, 16));

   assert(lp_check_alignment(inputs->step[0], 16));
   assert(lp_check_alignment(inputs->step[1], 16));
   assert(lp_check_alignment(inputs->step[2], 16));
#endif

   /* run shader */
   state->jit_function( &state->jit_context,
                        x, y,
                        inputs->a0,
                        inputs->dadx,
                        inputs->dady,
                        color,
                        depth,
                        c1, c2, c3,
                        inputs->step[0], inputs->step[1], inputs->step[2]);
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
   unsigned i;

   for (i = 0; i < rast->state.fb.nr_cbufs; i++) {
      struct pipe_transfer *transfer = rast->cbuf_transfer[i];
      int w = TILE_SIZE;
      int h = TILE_SIZE;

      if (x >= transfer->width)
	 continue;

      if (y >= transfer->height)
	 continue;

      /* XXX: require tile-size aligned render target dimensions:
       */
      if (x + w > transfer->width)
	 w -= x + w - transfer->width;

      if (y + h > transfer->height)
	 h -= y + h - transfer->height;

      assert(w >= 0);
      assert(h >= 0);
      assert(w <= TILE_SIZE);
      assert(h <= TILE_SIZE);

      LP_DBG(DEBUG_RAST, "%s [%u] %d,%d %dx%d\n", __FUNCTION__,
	     thread_index, x, y, w, h);

      lp_tile_write_4ub(transfer->texture->format,
			rast->tasks[thread_index].tile.color[i],
			rast->cbuf_map[i], 
			transfer->stride,
			x, y,
			w, h);
   }
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

   if (x + w > rast->state.fb.width)
      w -= x + w - rast->state.fb.width;

   if (y + h > rast->state.fb.height)
      h -= y + h - rast->state.fb.height;

   LP_DBG(DEBUG_RAST, "%s %d,%d %dx%d\n", __FUNCTION__, x, y, w, h);

   assert(rast->zsbuf_transfer->texture->format == PIPE_FORMAT_Z32_UNORM);
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
 * Signal on a fence.  This is called during bin execution/rasterization.
 * Called per thread.
 */
void lp_rast_fence( struct lp_rasterizer *rast,
                    unsigned thread_index,
                    const union lp_rast_cmd_arg arg )
{
   struct lp_fence *fence = arg.fence;

   pipe_mutex_lock( fence->mutex );

   fence->count++;
   assert(fence->count <= fence->rank);

   LP_DBG(DEBUG_RAST, "%s count=%u rank=%u\n", __FUNCTION__,
          fence->count, fence->rank);

   pipe_condvar_signal( fence->signalled );

   pipe_mutex_unlock( fence->mutex );
}


/**
 * When all the threads are done rasterizing a scene, one thread will
 * call this function to reset the scene and put it onto the empty queue.
 */
static void
release_scene( struct lp_rasterizer *rast,
	       struct lp_scene *scene )
{
   util_unreference_framebuffer_state( &scene->fb );

   lp_scene_reset( scene );
   lp_scene_enqueue( rast->empty_scenes, scene );
   rast->curr_scene = NULL;
}


/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 * Called per thread.
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

static boolean
is_empty_bin( struct lp_rasterizer *rast,
              const struct cmd_bin *bin )
{
   const struct cmd_block *head = bin->commands.head;
   int i;

   if (head->next != NULL ||
       head->count > PIPE_MAX_COLOR_BUFS + 1)
      return FALSE;

   for (i = 0; i < head->count; i++)
      if (head->cmd[i] != lp_rast_load_color &&
          head->cmd[i] != lp_rast_load_zstencil)
         return FALSE;

   return TRUE;
}



/**
 * Rasterize/execute all bins within a scene.
 * Called per thread.
 */
static void
rasterize_scene( struct lp_rasterizer *rast,
                unsigned thread_index,
                struct lp_scene *scene,
                bool write_depth )
{
   /* loop over scene bins, rasterize each */
#if 0
   {
      unsigned i, j;
      for (i = 0; i < scene->tiles_x; i++) {
         for (j = 0; j < scene->tiles_y; j++) {
            struct cmd_bin *bin = lp_get_bin(scene, i, j);
            rasterize_bin( rast, thread_index,
                           bin, i * TILE_SIZE, j * TILE_SIZE );
         }
      }
   }
#else
   {
      struct cmd_bin *bin;
      int x, y;

      assert(scene);
      while ((bin = lp_scene_bin_iter_next(scene, &x, &y))) {
         if (!is_empty_bin( rast, bin ))
            rasterize_bin( rast, thread_index, bin, x * TILE_SIZE, y * TILE_SIZE);
      }
   }
#endif
}


/**
 * Called by setup module when it has something for us to render.
 */
void
lp_rasterize_scene( struct lp_rasterizer *rast,
                   struct lp_scene *scene,
                   const struct pipe_framebuffer_state *fb,
                   bool write_depth )
{
   boolean debug = false;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   if (debug) {
      unsigned x, y;
      printf("rasterize scene:\n");
      printf("  data size: %u\n", lp_scene_data_size(scene));
      for (y = 0; y < scene->tiles_y; y++) {
         for (x = 0; x < scene->tiles_x; x++) {
            printf("  bin %u, %u size: %u\n", x, y,
                   lp_scene_bin_size(scene, x, y));
         }
      }
   }

   /* save framebuffer state in the bin */
   util_copy_framebuffer_state(&scene->fb, fb);
   scene->write_depth = write_depth;

   if (rast->num_threads == 0) {
      /* no threading */

      lp_rast_begin( rast, fb,
                     fb->nr_cbufs != 0, /* always write color if cbufs present */
                     fb->zsbuf != NULL && write_depth );

      lp_scene_bin_iter_begin( scene );
      rasterize_scene( rast, 0, scene, write_depth );

      release_scene( rast, scene );

      lp_rast_end( rast );
   }
   else {
      /* threaded rendering! */
      unsigned i;

      lp_scene_enqueue( rast->full_scenes, scene );

      /* signal the threads that there's work to do */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_signal(&rast->tasks[i].work_ready);
      }

      /* wait for work to complete */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_wait(&rast->tasks[i].work_done);
      }
   }

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
   boolean debug = false;

   while (1) {
      /* wait for work */
      if (debug)
         debug_printf("thread %d waiting for work\n", task->thread_index);
      pipe_semaphore_wait(&task->work_ready);

      if (task->thread_index == 0) {
         /* thread[0]:
          *  - get next scene to rasterize
          *  - map the framebuffer surfaces
          */
         const struct pipe_framebuffer_state *fb;
         boolean write_depth;

         rast->curr_scene = lp_scene_dequeue( rast->full_scenes );

         lp_scene_bin_iter_begin( rast->curr_scene );

         fb = &rast->curr_scene->fb;
         write_depth = rast->curr_scene->write_depth;

         lp_rast_begin( rast, fb,
                        fb->nr_cbufs != 0,
                        fb->zsbuf != NULL && write_depth );
      }

      /* Wait for all threads to get here so that threads[1+] don't
       * get a null rast->curr_scene pointer.
       */
      pipe_barrier_wait( &rast->barrier );

      /* do work */
      if (debug)
         debug_printf("thread %d doing work\n", task->thread_index);
      rasterize_scene(rast, 
		     task->thread_index,
                     rast->curr_scene, 
		     rast->curr_scene->write_depth);
      
      /* wait for all threads to finish with this scene */
      pipe_barrier_wait( &rast->barrier );

      if (task->thread_index == 0) {
         /* thread[0]:
          * - release the scene object
          * - unmap the framebuffer surfaces
          */
         release_scene( rast, rast->curr_scene );
         lp_rast_end( rast );
      }

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



/**
 * Create new lp_rasterizer.
 * \param empty  the queue to put empty scenes on after we've finished
 *               processing them.
 */
struct lp_rasterizer *
lp_rast_create( struct pipe_screen *screen, struct lp_scene_queue *empty )
{
   struct lp_rasterizer *rast;
   unsigned i, cbuf;

   rast = CALLOC_STRUCT(lp_rasterizer);
   if(!rast)
      return NULL;

   rast->screen = screen;

   rast->empty_scenes = empty;
   rast->full_scenes = lp_scene_queue_create();

   for (i = 0; i < Elements(rast->tasks); i++) {
      for (cbuf = 0; cbuf < PIPE_MAX_COLOR_BUFS; cbuf++ )
	 rast->tasks[i].tile.color[cbuf] = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );

      rast->tasks[i].tile.depth = align_malloc( TILE_SIZE*TILE_SIZE*4, 16 );
      rast->tasks[i].rast = rast;
      rast->tasks[i].thread_index = i;
   }

   create_rast_threads(rast);

   /* for synchronizing rasterization threads */
   pipe_barrier_init( &rast->barrier, rast->num_threads );

   return rast;
}


/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   unsigned i, cbuf;

   util_unreference_framebuffer_state(&rast->state.fb);

   for (i = 0; i < Elements(rast->tasks); i++) {
      align_free(rast->tasks[i].tile.depth);
      for (cbuf = 0; cbuf < PIPE_MAX_COLOR_BUFS; cbuf++ )
	 align_free(rast->tasks[i].tile.color[cbuf]);
   }

   /* for synchronizing rasterization threads */
   pipe_barrier_destroy( &rast->barrier );

   FREE(rast);
}


/** Return number of rasterization threads */
unsigned
lp_rast_get_num_threads( struct lp_rasterizer *rast )
{
   return rast->num_threads;
}
