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
#include "util/u_rect.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"

#include "os/os_time.h"

#include "lp_scene_queue.h"
#include "lp_context.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_perf.h"
#include "lp_query.h"
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_scene.h"
#include "lp_tex_sample.h"


#ifdef DEBUG
int jit_line = 0;
const struct lp_rast_state *jit_state = NULL;
const struct lp_rasterizer_task *jit_task = NULL;
#endif


/**
 * Begin rasterizing a scene.
 * Called once per scene by one thread.
 */
static void
lp_rast_begin( struct lp_rasterizer *rast,
               struct lp_scene *scene )
{
   rast->curr_scene = scene;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   lp_scene_begin_rasterization( scene );
   lp_scene_bin_iter_begin( scene );
}


static void
lp_rast_end( struct lp_rasterizer *rast )
{
   lp_scene_end_rasterization( rast->curr_scene );

   rast->curr_scene = NULL;
}


/**
 * Begining rasterization of a tile.
 * \param x  window X position of the tile, in pixels
 * \param y  window Y position of the tile, in pixels
 */
static void
lp_rast_tile_begin(struct lp_rasterizer_task *task,
                   const struct cmd_bin *bin,
                   int x, int y)
{
   LP_DBG(DEBUG_RAST, "%s %d,%d\n", __FUNCTION__, x, y);

   task->bin = bin;
   task->x = x * TILE_SIZE;
   task->y = y * TILE_SIZE;
   task->width = TILE_SIZE + x * TILE_SIZE > task->scene->fb.width ?
                    task->scene->fb.width - x * TILE_SIZE : TILE_SIZE;
   task->height = TILE_SIZE + y * TILE_SIZE > task->scene->fb.height ?
                    task->scene->fb.height - y * TILE_SIZE : TILE_SIZE;

   task->thread_data.vis_counter = 0;
   task->ps_invocations = 0;

   /* reset pointers to color and depth tile(s) */
   memset(task->color_tiles, 0, sizeof(task->color_tiles));
   task->depth_tile = NULL;
}


/**
 * Clear the rasterizer's current color tile.
 * This is a bin command called during bin processing.
 * Clear commands always clear all bound layers.
 */
static void
lp_rast_clear_color(struct lp_rasterizer_task *task,
                    const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   unsigned cbuf = arg.clear_rb->cbuf;
   union util_color uc;
   enum pipe_format format;

   /* we never bin clear commands for non-existing buffers */
   assert(cbuf < scene->fb.nr_cbufs);
   assert(scene->fb.cbufs[cbuf]);

   format = scene->fb.cbufs[cbuf]->format;
   uc = arg.clear_rb->color_val;

   /*
    * this is pretty rough since we have target format (bunch of bytes...) here.
    * dump it as raw 4 dwords.
    */
   LP_DBG(DEBUG_RAST, "%s clear value (target format %d) raw 0x%x,0x%x,0x%x,0x%x\n",
          __FUNCTION__, format, uc.ui[0], uc.ui[1], uc.ui[2], uc.ui[3]);


   util_fill_box(scene->cbufs[cbuf].map,
                 format,
                 scene->cbufs[cbuf].stride,
                 scene->cbufs[cbuf].layer_stride,
                 task->x,
                 task->y,
                 0,
                 task->width,
                 task->height,
                 scene->fb_max_layer + 1,
                 &uc);

   /* this will increase for each rb which probably doesn't mean much */
   LP_COUNT(nr_color_tile_clear);
}


/**
 * Clear the rasterizer's current z/stencil tile.
 * This is a bin command called during bin processing.
 * Clear commands always clear all bound layers.
 */
static void
lp_rast_clear_zstencil(struct lp_rasterizer_task *task,
                       const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   uint64_t clear_value64 = arg.clear_zstencil.value;
   uint64_t clear_mask64 = arg.clear_zstencil.mask;
   uint32_t clear_value = (uint32_t) clear_value64;
   uint32_t clear_mask = (uint32_t) clear_mask64;
   const unsigned height = task->height;
   const unsigned width = task->width;
   const unsigned dst_stride = scene->zsbuf.stride;
   uint8_t *dst;
   unsigned i, j;
   unsigned block_size;

   LP_DBG(DEBUG_RAST, "%s: value=0x%08x, mask=0x%08x\n",
           __FUNCTION__, clear_value, clear_mask);

   /*
    * Clear the area of the depth/depth buffer matching this tile.
    */

   if (scene->fb.zsbuf) {
      unsigned layer;
      uint8_t *dst_layer = lp_rast_get_unswizzled_depth_tile_pointer(task, LP_TEX_USAGE_READ_WRITE);
      block_size = util_format_get_blocksize(scene->fb.zsbuf->format);

      clear_value &= clear_mask;

      for (layer = 0; layer <= scene->fb_max_layer; layer++) {
         dst = dst_layer;

         switch (block_size) {
         case 1:
            assert(clear_mask == 0xff);
            memset(dst, (uint8_t) clear_value, height * width);
            break;
         case 2:
            if (clear_mask == 0xffff) {
               for (i = 0; i < height; i++) {
                  uint16_t *row = (uint16_t *)dst;
                  for (j = 0; j < width; j++)
                     *row++ = (uint16_t) clear_value;
                  dst += dst_stride;
               }
            }
            else {
               for (i = 0; i < height; i++) {
                  uint16_t *row = (uint16_t *)dst;
                  for (j = 0; j < width; j++) {
                     uint16_t tmp = ~clear_mask & *row;
                     *row++ = clear_value | tmp;
                  }
                  dst += dst_stride;
               }
            }
            break;
         case 4:
            if (clear_mask == 0xffffffff) {
               for (i = 0; i < height; i++) {
                  uint32_t *row = (uint32_t *)dst;
                  for (j = 0; j < width; j++)
                     *row++ = clear_value;
                  dst += dst_stride;
               }
            }
            else {
               for (i = 0; i < height; i++) {
                  uint32_t *row = (uint32_t *)dst;
                  for (j = 0; j < width; j++) {
                     uint32_t tmp = ~clear_mask & *row;
                     *row++ = clear_value | tmp;
                  }
                  dst += dst_stride;
               }
            }
            break;
         case 8:
            clear_value64 &= clear_mask64;
            if (clear_mask64 == 0xffffffffffULL) {
               for (i = 0; i < height; i++) {
                  uint64_t *row = (uint64_t *)dst;
                  for (j = 0; j < width; j++)
                     *row++ = clear_value64;
                  dst += dst_stride;
               }
            }
            else {
               for (i = 0; i < height; i++) {
                  uint64_t *row = (uint64_t *)dst;
                  for (j = 0; j < width; j++) {
                     uint64_t tmp = ~clear_mask64 & *row;
                     *row++ = clear_value64 | tmp;
                  }
                  dst += dst_stride;
               }
            }
            break;

         default:
            assert(0);
            break;
         }
         dst_layer += scene->zsbuf.layer_stride;
      }
   }
}



/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_shade_tile(struct lp_rasterizer_task *task,
                   const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   const struct lp_rast_state *state;
   struct lp_fragment_shader_variant *variant;
   const unsigned tile_x = task->x, tile_y = task->y;
   unsigned x, y;

   if (inputs->disable) {
      /* This command was partially binned and has been disabled */
      return;
   }

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   state = task->state;
   assert(state);
   if (!state) {
      return;
   }
   variant = state->variant;

   /* render the whole 64x64 tile in 4x4 chunks */
   for (y = 0; y < task->height; y += 4){
      for (x = 0; x < task->width; x += 4) {
         uint8_t *color[PIPE_MAX_COLOR_BUFS];
         unsigned stride[PIPE_MAX_COLOR_BUFS];
         uint8_t *depth = NULL;
         unsigned depth_stride = 0;
         unsigned i;

         /* color buffer */
         for (i = 0; i < scene->fb.nr_cbufs; i++){
            if (scene->fb.cbufs[i]) {
               stride[i] = scene->cbufs[i].stride;
               color[i] = lp_rast_get_unswizzled_color_block_pointer(task, i, tile_x + x,
                                                                     tile_y + y, inputs->layer);
            }
            else {
               stride[i] = 0;
               color[i] = NULL;
            }
         }

         /* depth buffer */
         if (scene->zsbuf.map) {
            depth = lp_rast_get_unswizzled_depth_block_pointer(task, tile_x + x,
                                                               tile_y + y, inputs->layer);
            depth_stride = scene->zsbuf.stride;
         }

         /* Propagate non-interpolated raster state. */
         task->thread_data.raster_state.viewport_index = inputs->viewport_index;

         /* run shader on 4x4 block */
         BEGIN_JIT_CALL(state, task);
         variant->jit_function[RAST_WHOLE]( &state->jit_context,
                                            tile_x + x, tile_y + y,
                                            inputs->frontfacing,
                                            GET_A0(inputs),
                                            GET_DADX(inputs),
                                            GET_DADY(inputs),
                                            color,
                                            depth,
                                            0xffff,
                                            &task->thread_data,
                                            stride,
                                            depth_stride);
         END_JIT_CALL();
      }
   }
}


/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle, and the shader is opaque.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_shade_tile_opaque(struct lp_rasterizer_task *task,
                          const union lp_rast_cmd_arg arg)
{
   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   assert(task->state);
   if (!task->state) {
      return;
   }

   lp_rast_shade_tile(task, arg);
}


/**
 * Compute shading for a 4x4 block of pixels inside a triangle.
 * This is a bin command called during bin processing.
 * \param x  X position of quad in window coords
 * \param y  Y position of quad in window coords
 */
void
lp_rast_shade_quads_mask(struct lp_rasterizer_task *task,
                         const struct lp_rast_shader_inputs *inputs,
                         unsigned x, unsigned y,
                         unsigned mask)
{
   const struct lp_rast_state *state = task->state;
   struct lp_fragment_shader_variant *variant = state->variant;
   const struct lp_scene *scene = task->scene;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   unsigned stride[PIPE_MAX_COLOR_BUFS];
   uint8_t *depth = NULL;
   unsigned depth_stride = 0;
   unsigned i;

   assert(state);

   /* Sanity checks */
   assert(x < scene->tiles_x * TILE_SIZE);
   assert(y < scene->tiles_y * TILE_SIZE);
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   assert((x % 4) == 0);
   assert((y % 4) == 0);

   /* color buffer */
   for (i = 0; i < scene->fb.nr_cbufs; i++) {
      if (scene->fb.cbufs[i]) {
         stride[i] = scene->cbufs[i].stride;
         color[i] = lp_rast_get_unswizzled_color_block_pointer(task, i, x, y,
                                                               inputs->layer);
      }
      else {
         stride[i] = 0;
         color[i] = NULL;
      }
   }

   /* depth buffer */
   if (scene->zsbuf.map) {
      depth_stride = scene->zsbuf.stride;
      depth = lp_rast_get_unswizzled_depth_block_pointer(task, x, y, inputs->layer);
   }

   assert(lp_check_alignment(state->jit_context.u8_blend_color, 16));

   /*
    * The rasterizer may produce fragments outside our
    * allocated 4x4 blocks hence need to filter them out here.
    */
   if ((x % TILE_SIZE) < task->width && (y % TILE_SIZE) < task->height) {
      /* not very accurate would need a popcount on the mask */
      /* always count this not worth bothering? */
      task->ps_invocations += 1 * variant->ps_inv_multiplier;

      /* Propagate non-interpolated raster state. */
      task->thread_data.raster_state.viewport_index = inputs->viewport_index;

      /* run shader on 4x4 block */
      BEGIN_JIT_CALL(state, task);
      variant->jit_function[RAST_EDGE_TEST](&state->jit_context,
                                            x, y,
                                            inputs->frontfacing,
                                            GET_A0(inputs),
                                            GET_DADX(inputs),
                                            GET_DADY(inputs),
                                            color,
                                            depth,
                                            mask,
                                            &task->thread_data,
                                            stride,
                                            depth_stride);
      END_JIT_CALL();
   }
}



/**
 * Begin a new occlusion query.
 * This is a bin command put in all bins.
 * Called per thread.
 */
static void
lp_rast_begin_query(struct lp_rasterizer_task *task,
                    const union lp_rast_cmd_arg arg)
{
   struct llvmpipe_query *pq = arg.query_obj;

   switch (pq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      pq->start[task->thread_index] = task->thread_data.vis_counter;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      pq->start[task->thread_index] = task->ps_invocations;
      break;
   default:
      assert(0);
      break;
   }
}


/**
 * End the current occlusion query.
 * This is a bin command put in all bins.
 * Called per thread.
 */
static void
lp_rast_end_query(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg)
{
   struct llvmpipe_query *pq = arg.query_obj;

   switch (pq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      pq->end[task->thread_index] +=
         task->thread_data.vis_counter - pq->start[task->thread_index];
      pq->start[task->thread_index] = 0;
      break;
   case PIPE_QUERY_TIMESTAMP:
      pq->end[task->thread_index] = os_time_get_nano();
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      pq->end[task->thread_index] +=
         task->ps_invocations - pq->start[task->thread_index];
      pq->start[task->thread_index] = 0;
      break;
   default:
      assert(0);
      break;
   }
}


void
lp_rast_set_state(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg)
{
   task->state = arg.state;
}



/**
 * Called when we're done writing to a color tile.
 */
static void
lp_rast_tile_end(struct lp_rasterizer_task *task)
{
   unsigned i;

   for (i = 0; i < task->scene->num_active_queries; ++i) {
      lp_rast_end_query(task, lp_rast_arg_query(task->scene->active_queries[i]));
   }

   /* debug */
   memset(task->color_tiles, 0, sizeof(task->color_tiles));
   task->depth_tile = NULL;

   task->bin = NULL;
}

static lp_rast_cmd_func dispatch[LP_RAST_OP_MAX] =
{
   lp_rast_clear_color,
   lp_rast_clear_zstencil,
   lp_rast_triangle_1,
   lp_rast_triangle_2,
   lp_rast_triangle_3,
   lp_rast_triangle_4,
   lp_rast_triangle_5,
   lp_rast_triangle_6,
   lp_rast_triangle_7,
   lp_rast_triangle_8,
   lp_rast_triangle_3_4,
   lp_rast_triangle_3_16,
   lp_rast_triangle_4_16,
   lp_rast_shade_tile,
   lp_rast_shade_tile_opaque,
   lp_rast_begin_query,
   lp_rast_end_query,
   lp_rast_set_state,
   lp_rast_triangle_32_1,
   lp_rast_triangle_32_2,
   lp_rast_triangle_32_3,
   lp_rast_triangle_32_4,
   lp_rast_triangle_32_5,
   lp_rast_triangle_32_6,
   lp_rast_triangle_32_7,
   lp_rast_triangle_32_8,
   lp_rast_triangle_32_3_4,
   lp_rast_triangle_32_3_16,
   lp_rast_triangle_32_4_16
};


static void
do_rasterize_bin(struct lp_rasterizer_task *task,
                 const struct cmd_bin *bin,
                 int x, int y)
{
   const struct cmd_block *block;
   unsigned k;

   if (0)
      lp_debug_bin(bin, x, y);

   for (block = bin->head; block; block = block->next) {
      for (k = 0; k < block->count; k++) {
         dispatch[block->cmd[k]]( task, block->arg[k] );
      }
   }
}



/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 * Called per thread.
 */
static void
rasterize_bin(struct lp_rasterizer_task *task,
              const struct cmd_bin *bin, int x, int y )
{
   lp_rast_tile_begin( task, bin, x, y );

   do_rasterize_bin(task, bin, x, y);

   lp_rast_tile_end(task);


   /* Debug/Perf flags:
    */
   if (bin->head->count == 1) {
      if (bin->head->cmd[0] == LP_RAST_OP_SHADE_TILE_OPAQUE)
         LP_COUNT(nr_pure_shade_opaque_64);
      else if (bin->head->cmd[0] == LP_RAST_OP_SHADE_TILE)
         LP_COUNT(nr_pure_shade_64);
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
   return bin->head == NULL;
}


/**
 * Rasterize/execute all bins within a scene.
 * Called per thread.
 */
static void
rasterize_scene(struct lp_rasterizer_task *task,
                struct lp_scene *scene)
{
   task->scene = scene;

   if (!task->rast->no_rast && !scene->discard) {
      /* loop over scene bins, rasterize each */
      {
         struct cmd_bin *bin;
         int i, j;

         assert(scene);
         while ((bin = lp_scene_bin_iter_next(scene, &i, &j))) {
            if (!is_empty_bin( bin ))
               rasterize_bin(task, bin, i, j);
         }
      }
   }


   if (scene->fence) {
      lp_fence_signal(scene->fence);
   }

   task->scene = NULL;
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
      unsigned fpstate = util_fpstate_get();

      /* Make sure that denorms are treated like zeros. This is 
       * the behavior required by D3D10. OpenGL doesn't care.
       */
      util_fpstate_set_denorms_to_zero(fpstate);

      lp_rast_begin( rast, scene );

      rasterize_scene( &rast->tasks[0], scene );

      lp_rast_end( rast );

      util_fpstate_set(fpstate);

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
static PIPE_THREAD_ROUTINE( thread_function, init_data )
{
   struct lp_rasterizer_task *task = (struct lp_rasterizer_task *) init_data;
   struct lp_rasterizer *rast = task->rast;
   boolean debug = false;
   unsigned fpstate = util_fpstate_get();

   /* Make sure that denorms are treated like zeros. This is 
    * the behavior required by D3D10. OpenGL doesn't care.
    */
   util_fpstate_set_denorms_to_zero(fpstate);

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

   return 0;
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
      rast->threads[i] = pipe_thread_create(thread_function,
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
   if (!rast) {
      goto no_rast;
   }

   rast->full_scenes = lp_scene_queue_create();
   if (!rast->full_scenes) {
      goto no_full_scenes;
   }

   for (i = 0; i < Elements(rast->tasks); i++) {
      struct lp_rasterizer_task *task = &rast->tasks[i];
      task->rast = rast;
      task->thread_index = i;
   }

   rast->num_threads = num_threads;

   rast->no_rast = debug_get_bool_option("LP_NO_RAST", FALSE);

   create_rast_threads(rast);

   /* for synchronizing rasterization threads */
   pipe_barrier_init( &rast->barrier, rast->num_threads );

   memset(lp_dummy_tile, 0, sizeof lp_dummy_tile);

   return rast;

no_full_scenes:
   FREE(rast);
no_rast:
   return NULL;
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


