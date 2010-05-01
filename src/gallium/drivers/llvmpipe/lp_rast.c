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
#include "util/u_surface.h"

#include "lp_scene_queue.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_perf.h"
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_scene.h"


/**
 * Begin rasterizing a scene.
 * Called once per scene by one thread.
 */
static void
lp_rast_begin( struct lp_rasterizer *rast,
               struct lp_scene *scene )
{
   const struct pipe_framebuffer_state *fb = &scene->fb;
   int i;

   rast->curr_scene = scene;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   rast->state.nr_cbufs = scene->fb.nr_cbufs;
   
   for (i = 0; i < rast->state.nr_cbufs; i++) {
      struct pipe_surface *cbuf = scene->fb.cbufs[i];
      rast->cbuf[i].format = cbuf->texture->format;
      rast->cbuf[i].tiles_per_row = align(cbuf->width, TILE_SIZE) / TILE_SIZE;
      rast->cbuf[i].blocksize = 
         util_format_get_blocksize(cbuf->texture->format);
      rast->cbuf[i].map = llvmpipe_resource_map(cbuf->texture,
                                               cbuf->face,
                                               cbuf->level,
                                               cbuf->zslice,
                                               LP_TEX_USAGE_READ_WRITE,
                                               LP_TEX_LAYOUT_NONE);
   }

   if (fb->zsbuf) {
      struct pipe_surface *zsbuf = scene->fb.zsbuf;
      rast->zsbuf.stride = llvmpipe_resource_stride(zsbuf->texture, zsbuf->level);
      rast->zsbuf.blocksize = 
         util_format_get_blocksize(zsbuf->texture->format);

      rast->zsbuf.map = llvmpipe_resource_map(zsbuf->texture,
                                             zsbuf->face,
                                             zsbuf->level,
                                             zsbuf->zslice,
                                             LP_TEX_USAGE_READ_WRITE,
                                             LP_TEX_LAYOUT_NONE);
      assert(rast->zsbuf.map);
   }

   lp_scene_bin_iter_begin( scene );
}


static void
lp_rast_end( struct lp_rasterizer *rast )
{
   struct lp_scene *scene = rast->curr_scene;
   unsigned i;

   /* Unmap color buffers */
   for (i = 0; i < rast->state.nr_cbufs; i++) {
      struct pipe_surface *cbuf = scene->fb.cbufs[i];
      llvmpipe_resource_unmap(cbuf->texture,
                             cbuf->face,
                             cbuf->level,
                             cbuf->zslice);
      rast->cbuf[i].map = NULL;
   }

   /* Unmap z/stencil buffer */
   if (rast->zsbuf.map) {
      struct pipe_surface *zsbuf = scene->fb.zsbuf;
      llvmpipe_resource_unmap(zsbuf->texture,
                             zsbuf->face,
                             zsbuf->level,
                             zsbuf->zslice);
      rast->zsbuf.map = NULL;
   }

   lp_scene_reset( rast->curr_scene );

   rast->curr_scene = NULL;

#ifdef DEBUG
   if (0)
      debug_printf("Post render scene: tile unswizzle: %u tile swizzle: %u\n",
                   lp_tile_unswizzle_count, lp_tile_swizzle_count);
#endif
}


/**
 * Begining rasterization of a tile.
 * \param x  window X position of the tile, in pixels
 * \param y  window Y position of the tile, in pixels
 */
static void
lp_rast_tile_begin(struct lp_rasterizer_task *task,
                   unsigned x, unsigned y)
{
   struct lp_rasterizer *rast = task->rast;
   struct lp_scene *scene = rast->curr_scene;
   enum lp_texture_usage usage;
   unsigned buf;

   LP_DBG(DEBUG_RAST, "%s %d,%d\n", __FUNCTION__, x, y);

   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   task->x = x;
   task->y = y;

   if (scene->has_color_clear)
      usage = LP_TEX_USAGE_WRITE_ALL;
   else
      usage = LP_TEX_USAGE_READ_WRITE;

   /* get pointers to color tile(s) */
   for (buf = 0; buf < rast->state.nr_cbufs; buf++) {
      struct pipe_surface *cbuf = rast->curr_scene->fb.cbufs[buf];
      struct llvmpipe_resource *lpt;
      assert(cbuf);
      lpt = llvmpipe_resource(cbuf->texture);
      task->color_tiles[buf] = llvmpipe_get_texture_tile(lpt,
                                                         cbuf->face + cbuf->zslice,
                                                         cbuf->level,
                                                         usage,
                                                         x, y);
      assert(task->color_tiles[buf]);
   }

   /* get pointer to depth/stencil tile */
   {
      struct pipe_surface *zsbuf = rast->curr_scene->fb.zsbuf;
      if (zsbuf) {
         struct llvmpipe_resource *lpt = llvmpipe_resource(zsbuf->texture);

         if (scene->has_depth_clear)
            usage = LP_TEX_USAGE_WRITE_ALL;
         else
            usage = LP_TEX_USAGE_READ_WRITE;

         /* "prime" the tile: convert data from linear to tiled if necessary
          * and update the tile's layout info.
          */
         (void) llvmpipe_get_texture_tile(lpt,
                                          zsbuf->face + zsbuf->zslice,
                                          zsbuf->level,
                                          usage,
                                          x, y);
         /* Get actual pointer to the tile data.  Note that depth/stencil
          * data is tiled differently than color data.
          */
         task->depth_tile = lp_rast_get_depth_block_pointer(rast, x, y);

         assert(task->depth_tile);
      }
      else {
         task->depth_tile = NULL;
      }
   }
}


/**
 * Clear the rasterizer's current color tile.
 * This is a bin command called during bin processing.
 */
void
lp_rast_clear_color(struct lp_rasterizer_task *task,
                    const union lp_rast_cmd_arg arg)
{
   struct lp_rasterizer *rast = task->rast;
   const uint8_t *clear_color = arg.clear_color;

   unsigned i;

   LP_DBG(DEBUG_RAST, "%s 0x%x,0x%x,0x%x,0x%x\n", __FUNCTION__, 
              clear_color[0],
              clear_color[1],
              clear_color[2],
              clear_color[3]);

   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      /* clear to grayscale value {x, x, x, x} */
      for (i = 0; i < rast->state.nr_cbufs; i++) {
         uint8_t *ptr = task->color_tiles[i];
	 memset(ptr, clear_color[0], TILE_SIZE * TILE_SIZE * 4);
      }
   }
   else {
      /* Non-gray color.
       * Note: if the swizzled tile layout changes (see TILE_PIXEL) this code
       * will need to change.  It'll be pretty obvious when clearing no longer
       * works.
       */
      const unsigned chunk = TILE_SIZE / 4;
      for (i = 0; i < rast->state.nr_cbufs; i++) {
         uint8_t *c = task->color_tiles[i];
         unsigned j;

         for (j = 0; j < 4 * TILE_SIZE; j++) {
            memset(c, clear_color[0], chunk);
            c += chunk;
            memset(c, clear_color[1], chunk);
            c += chunk;
            memset(c, clear_color[2], chunk);
            c += chunk;
            memset(c, clear_color[3], chunk);
            c += chunk;
         }
      }
   }

   LP_COUNT(nr_color_tile_clear);
}


/**
 * Clear the rasterizer's current z/stencil tile.
 * This is a bin command called during bin processing.
 */
void
lp_rast_clear_zstencil(struct lp_rasterizer_task *task,
                       const union lp_rast_cmd_arg arg)
{
   struct lp_rasterizer *rast = task->rast;
   const unsigned height = TILE_SIZE / TILE_VECTOR_HEIGHT;
   const unsigned width = TILE_SIZE * TILE_VECTOR_HEIGHT;
   const unsigned block_size = rast->zsbuf.blocksize;
   const unsigned dst_stride = rast->zsbuf.stride * TILE_VECTOR_HEIGHT;
   uint8_t *dst;
   unsigned i, j;

   LP_DBG(DEBUG_RAST, "%s 0x%x\n", __FUNCTION__, arg.clear_zstencil);

   /*
    * Clear the aera of the swizzled depth/depth buffer matching this tile, in
    * stripes of TILE_VECTOR_HEIGHT x TILE_SIZE at a time.
    *
    * The swizzled depth format is such that the depths for
    * TILE_VECTOR_HEIGHT x TILE_VECTOR_WIDTH pixels have consecutive offsets.
    */

   dst = task->depth_tile;

   assert(dst == lp_rast_get_depth_block_pointer(rast, task->x, task->y));

   switch (block_size) {
   case 1:
      memset(dst, (uint8_t) arg.clear_zstencil, height * width);
      break;
   case 2:
      for (i = 0; i < height; i++) {
         uint16_t *row = (uint16_t *)dst;
         for (j = 0; j < width; j++)
            *row++ = (uint16_t) arg.clear_zstencil;
         dst += dst_stride;
      }
      break;
   case 4:
      for (i = 0; i < height; i++) {
         uint32_t *row = (uint32_t *)dst;
         for (j = 0; j < width; j++)
            *row++ = arg.clear_zstencil;
         dst += dst_stride;
      }
      break;
   default:
      assert(0);
      break;
   }
}


/**
 * Load tile color from the framebuffer surface.
 * This is a bin command called during bin processing.
 */
#if 0
void
lp_rast_load_color(struct lp_rasterizer_task *task,
                   const union lp_rast_cmd_arg arg)
{
   struct lp_rasterizer *rast = task->rast;
   unsigned buf;
   enum lp_texture_usage usage;

   LP_DBG(DEBUG_RAST, "%s at %u, %u\n", __FUNCTION__, x, y);

   if (scene->has_color_clear)
      usage = LP_TEX_USAGE_WRITE_ALL;
   else
      usage = LP_TEX_USAGE_READ_WRITE;

   /* Get pointers to color tile(s).
    * This will convert linear data to tiled if needed.
    */
   for (buf = 0; buf < rast->state.nr_cbufs; buf++) {
      struct pipe_surface *cbuf = rast->curr_scene->fb.cbufs[buf];
      struct llvmpipe_texture *lpt;
      assert(cbuf);
      lpt = llvmpipe_texture(cbuf->texture);
      task->color_tiles[buf] = llvmpipe_get_texture_tile(lpt,
                                                         cbuf->face + cbuf->zslice,
                                                         cbuf->level,
                                                         usage,
                                                         task->x, task->y);
      assert(task->color_tiles[buf]);
   }
}
#endif


/**
 * Convert the color tile from tiled to linear layout.
 * This is generally only done when we're flushing the scene just prior to
 * SwapBuffers.  If we didn't do this here, we'd have to convert the entire
 * tiled color buffer to linear layout in the llvmpipe_texture_unmap()
 * function.  It's better to do it here to take advantage of
 * threading/parallelism.
 * This is a bin command which is stored in all bins.
 */
void
lp_rast_store_color( struct lp_rasterizer_task *task,
                     const union lp_rast_cmd_arg arg)
{
   struct lp_rasterizer *rast = task->rast;
   struct lp_scene *scene = rast->curr_scene;
   unsigned buf;

   for (buf = 0; buf < rast->state.nr_cbufs; buf++) {
      struct pipe_surface *cbuf = scene->fb.cbufs[buf];
      const unsigned face = cbuf->face, level = cbuf->level;
      struct llvmpipe_resource *lpt = llvmpipe_resource(cbuf->texture);
      /* this will convert the tiled data to linear if needed */
      (void) llvmpipe_get_texture_tile_linear(lpt, face, level,
                                              LP_TEX_USAGE_READ,
                                              task->x, task->y);
   }
}


/**
 * This is a bin command called during bin processing.
 */
void
lp_rast_set_state(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_state *state = arg.set_state;

   LP_DBG(DEBUG_RAST, "%s %p\n", __FUNCTION__, (void *) state);

   /* just set the current state pointer for this rasterizer */
   task->current_state = state;
}


/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle.
 * This is a bin command called during bin processing.
 */
void
lp_rast_shade_tile(struct lp_rasterizer_task *task,
                   const union lp_rast_cmd_arg arg)
{
   struct lp_rasterizer *rast = task->rast;
   const struct lp_rast_state *state = task->current_state;
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   const unsigned tile_x = task->x, tile_y = task->y;
   unsigned x, y;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   /* render the whole 64x64 tile in 4x4 chunks */
   for (y = 0; y < TILE_SIZE; y += 4){
      for (x = 0; x < TILE_SIZE; x += 4) {
         uint8_t *color[PIPE_MAX_COLOR_BUFS];
         uint32_t *depth;
         unsigned i;

         /* color buffer */
         for (i = 0; i < rast->state.nr_cbufs; i++)
            color[i] = lp_rast_get_color_block_pointer(task, i,
                                                       tile_x + x, tile_y + y);

         /* depth buffer */
         depth = lp_rast_get_depth_block_pointer(rast, tile_x + x, tile_y + y);

         /* run shader on 4x4 block */
         state->jit_function[RAST_WHOLE]( &state->jit_context,
                                          tile_x + x, tile_y + y,
                                          inputs->facing,
                                          inputs->a0,
                                          inputs->dadx,
                                          inputs->dady,
                                          color,
                                          depth,
                                          INT_MIN, INT_MIN, INT_MIN,
                                          NULL, NULL, NULL );
      }
   }
}


/**
 * Compute shading for a 4x4 block of pixels.
 * This is a bin command called during bin processing.
 * \param x  X position of quad in window coords
 * \param y  Y position of quad in window coords
 */
void lp_rast_shade_quads( struct lp_rasterizer_task *task,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          int32_t c1, int32_t c2, int32_t c3)
{
   const struct lp_rast_state *state = task->current_state;
   struct lp_rasterizer *rast = task->rast;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   void *depth;
   unsigned i;

   assert(state);

   /* Sanity checks */
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   assert((x % 4) == 0);
   assert((y % 4) == 0);

   /* color buffer */
   for (i = 0; i < rast->state.nr_cbufs; i++) {
      color[i] = lp_rast_get_color_block_pointer(task, i, x, y);
      assert(lp_check_alignment(color[i], 16));
   }

   /* depth buffer */
   depth = lp_rast_get_depth_block_pointer(rast, x, y);


   assert(lp_check_alignment(state->jit_context.blend_color, 16));

   assert(lp_check_alignment(inputs->step[0], 16));
   assert(lp_check_alignment(inputs->step[1], 16));
   assert(lp_check_alignment(inputs->step[2], 16));

   /* run shader on 4x4 block */
   state->jit_function[RAST_EDGE_TEST]( &state->jit_context,
                                        x, y,
                                        inputs->facing,
                                        inputs->a0,
                                        inputs->dadx,
                                        inputs->dady,
                                        color,
                                        depth,
                                        c1, c2, c3,
                                        inputs->step[0],
                                        inputs->step[1],
                                        inputs->step[2]);
}


/**
 * Set top row and left column of the tile's pixels to white.  For debugging.
 */
static void
outline_tile(uint8_t *tile)
{
   const uint8_t val = 0xff;
   unsigned i;

   for (i = 0; i < TILE_SIZE; i++) {
      TILE_PIXEL(tile, i, 0, 0) = val;
      TILE_PIXEL(tile, i, 0, 1) = val;
      TILE_PIXEL(tile, i, 0, 2) = val;
      TILE_PIXEL(tile, i, 0, 3) = val;

      TILE_PIXEL(tile, 0, i, 0) = val;
      TILE_PIXEL(tile, 0, i, 1) = val;
      TILE_PIXEL(tile, 0, i, 2) = val;
      TILE_PIXEL(tile, 0, i, 3) = val;
   }
}


/**
 * Draw grid of gray lines at 16-pixel intervals across the tile to
 * show the sub-tile boundaries.  For debugging.
 */
static void
outline_subtiles(uint8_t *tile)
{
   const uint8_t val = 0x80;
   const unsigned step = 16;
   unsigned i, j;

   for (i = 0; i < TILE_SIZE; i += step) {
      for (j = 0; j < TILE_SIZE; j++) {
         TILE_PIXEL(tile, i, j, 0) = val;
         TILE_PIXEL(tile, i, j, 1) = val;
         TILE_PIXEL(tile, i, j, 2) = val;
         TILE_PIXEL(tile, i, j, 3) = val;

         TILE_PIXEL(tile, j, i, 0) = val;
         TILE_PIXEL(tile, j, i, 1) = val;
         TILE_PIXEL(tile, j, i, 2) = val;
         TILE_PIXEL(tile, j, i, 3) = val;
      }
   }

   outline_tile(tile);
}



/**
 * Called when we're done writing to a color tile.
 */
static void
lp_rast_tile_end(struct lp_rasterizer_task *task)
{
#if DEBUG
   struct lp_rasterizer *rast = task->rast;
   unsigned buf;

   for (buf = 0; buf < rast->state.nr_cbufs; buf++) {
      uint8_t *color = lp_rast_get_color_block_pointer(task, buf,
                                                       task->x, task->y);

      if (LP_DEBUG & DEBUG_SHOW_SUBTILES)
         outline_subtiles(color);
      else if (LP_DEBUG & DEBUG_SHOW_TILES)
         outline_tile(color);
   }
#else
   (void) outline_subtiles;
#endif

   /* debug */
   memset(task->color_tiles, 0, sizeof(task->color_tiles));
   task->depth_tile = NULL;
}



/**
 * Signal on a fence.  This is called during bin execution/rasterization.
 * Called per thread.
 */
void
lp_rast_fence(struct lp_rasterizer_task *task,
              const union lp_rast_cmd_arg arg)
{
   struct lp_fence *fence = arg.fence;
   lp_fence_signal(fence);
}




/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 * Called per thread.
 */
static void
rasterize_bin(struct lp_rasterizer_task *task,
              const struct cmd_bin *bin,
              int x, int y)
{
   const struct cmd_block_list *commands = &bin->commands;
   struct cmd_block *block;
   unsigned k;

   lp_rast_tile_begin( task, x * TILE_SIZE, y * TILE_SIZE );

   /* simply execute each of the commands in the block list */
   for (block = commands->head; block; block = block->next) {
      for (k = 0; k < block->count; k++) {
         block->cmd[k]( task, block->arg[k] );
      }
   }

   lp_rast_tile_end(task);

   /* Free data for this bin.
    */
   lp_scene_bin_reset( task->rast->curr_scene, x, y);
}


#define RAST(x) { lp_rast_##x, #x }

static struct {
   lp_rast_cmd cmd;
   const char *name;
} cmd_names[] = 
{
   RAST(clear_color),
   RAST(clear_zstencil),
   RAST(triangle),
   RAST(shade_tile),
   RAST(set_state),
   RAST(store_color),
   RAST(fence),
};

static void
debug_bin( const struct cmd_bin *bin )
{
   const struct cmd_block *head = bin->commands.head;
   int i, j;

   for (i = 0; i < head->count; i++) {
      debug_printf("%d: ", i);
      for (j = 0; j < Elements(cmd_names); j++) {
         if (head->cmd[i] == cmd_names[j].cmd) {
            debug_printf("%s\n", cmd_names[j].name);
            break;
         }
      }
      if (j == Elements(cmd_names))
         debug_printf("...other\n");
   }

}

/* An empty bin is one that just loads the contents of the tile and
 * stores them again unchanged.  This typically happens when bins have
 * been flushed for some reason in the middle of a frame, or when
 * incremental updates are being made to a render target.
 * 
 * Try to avoid doing pointless work in this case.
 */
static boolean
is_empty_bin( const struct cmd_bin *bin )
{
   const struct cmd_block *head = bin->commands.head;
   int i;
   
   if (0)
      debug_bin(bin);
   
   /* We emit at most two load-tile commands at the start of the first
    * command block.  In addition we seem to emit a couple of
    * set-state commands even in empty bins.
    *
    * As a heuristic, if a bin has more than 4 commands, consider it
    * non-empty.
    */
   if (head->next != NULL ||
       head->count > 4) {
      return FALSE;
   }

   for (i = 0; i < head->count; i++)
      if (head->cmd[i] != lp_rast_set_state) {
         return FALSE;
      }

   return TRUE;
}



/**
 * Rasterize/execute all bins within a scene.
 * Called per thread.
 */
static void
rasterize_scene(struct lp_rasterizer_task *task,
                struct lp_scene *scene)
{
   /* loop over scene bins, rasterize each */
#if 0
   {
      unsigned i, j;
      for (i = 0; i < scene->tiles_x; i++) {
         for (j = 0; j < scene->tiles_y; j++) {
            struct cmd_bin *bin = lp_scene_get_bin(scene, i, j);
            rasterize_bin(task, bin, i, j);
         }
      }
   }
#else
   {
      struct cmd_bin *bin;
      int x, y;

      assert(scene);
      while ((bin = lp_scene_bin_iter_next(scene, &x, &y))) {
         if (!is_empty_bin( bin ))
            rasterize_bin(task, bin, x, y);
      }
   }
#endif
}


/**
 * Called by setup module when it has something for us to render.
 */
void
lp_rast_queue_scene( struct lp_rasterizer *rast,
                     struct lp_scene *scene)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   if (rast->num_threads == 0) {
      /* no threading */

      lp_rast_begin( rast, scene );

      rasterize_scene( &rast->tasks[0], scene );

      lp_scene_reset( scene );

      lp_rast_end( rast );

      rast->curr_scene = NULL;
   }
   else {
      /* threaded rendering! */
      unsigned i;

      lp_scene_enqueue( rast->full_scenes, scene );

      /* signal the threads that there's work to do */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_signal(&rast->tasks[i].work_ready);
      }
   }

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}


void
lp_rast_finish( struct lp_rasterizer *rast )
{
   if (rast->num_threads == 0) {
      /* nothing to do */
   }
   else {
      int i;

      /* wait for work to complete */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_wait(&rast->tasks[i].work_done);
      }
   }
}


/**
 * This is the thread's main entrypoint.
 * It's a simple loop:
 *   1. wait for work
 *   2. do work
 *   3. signal that we're done
 */
static PIPE_THREAD_ROUTINE( thread_func, init_data )
{
   struct lp_rasterizer_task *task = (struct lp_rasterizer_task *) init_data;
   struct lp_rasterizer *rast = task->rast;
   boolean debug = false;

   while (1) {
      /* wait for work */
      if (debug)
         debug_printf("thread %d waiting for work\n", task->thread_index);
      pipe_semaphore_wait(&task->work_ready);

      if (rast->exit_flag)
         break;

      if (task->thread_index == 0) {
         /* thread[0]:
          *  - get next scene to rasterize
          *  - map the framebuffer surfaces
          */
         lp_rast_begin( rast, 
                        lp_scene_dequeue( rast->full_scenes, TRUE ) );
      }

      /* Wait for all threads to get here so that threads[1+] don't
       * get a null rast->curr_scene pointer.
       */
      pipe_barrier_wait( &rast->barrier );

      /* do work */
      if (debug)
         debug_printf("thread %d doing work\n", task->thread_index);

      rasterize_scene(task,
                      rast->curr_scene);
      
      /* wait for all threads to finish with this scene */
      pipe_barrier_wait( &rast->barrier );

      /* XXX: shouldn't be necessary:
       */
      if (task->thread_index == 0) {
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

   /* NOTE: if num_threads is zero, we won't use any threads */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_init(&rast->tasks[i].work_ready, 0);
      pipe_semaphore_init(&rast->tasks[i].work_done, 0);
      rast->threads[i] = pipe_thread_create(thread_func,
                                            (void *) &rast->tasks[i]);
   }
}



/**
 * Create new lp_rasterizer.  If num_threads is zero, don't create any
 * new threads, do rendering synchronously.
 * \param num_threads  number of rasterizer threads to create
 */
struct lp_rasterizer *
lp_rast_create( unsigned num_threads )
{
   struct lp_rasterizer *rast;
   unsigned i;

   rast = CALLOC_STRUCT(lp_rasterizer);
   if(!rast)
      return NULL;

   rast->full_scenes = lp_scene_queue_create();

   for (i = 0; i < Elements(rast->tasks); i++) {
      struct lp_rasterizer_task *task = &rast->tasks[i];
      task->rast = rast;
      task->thread_index = i;
   }

   rast->num_threads = num_threads;

   create_rast_threads(rast);

   /* for synchronizing rasterization threads */
   pipe_barrier_init( &rast->barrier, rast->num_threads );

   return rast;
}


/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   unsigned i;

   /* Set exit_flag and signal each thread's work_ready semaphore.
    * Each thread will be woken up, notice that the exit_flag is set and
    * break out of its main loop.  The thread will then exit.
    */
   rast->exit_flag = TRUE;
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_signal(&rast->tasks[i].work_ready);
   }

   /* Wait for threads to terminate before cleaning up per-thread data */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_thread_wait(rast->threads[i]);
   }

   /* Clean up per-thread data */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_destroy(&rast->tasks[i].work_ready);
      pipe_semaphore_destroy(&rast->tasks[i].work_done);
   }

   /* for synchronizing rasterization threads */
   pipe_barrier_destroy( &rast->barrier );

   lp_scene_queue_destroy(rast->full_scenes);

   FREE(rast);
}


/** Return number of rasterization threads */
unsigned
lp_rast_get_num_threads( struct lp_rasterizer *rast )
{
   return rast->num_threads;
}
