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

/**
 * Tiling engine.
 *
 * Builds per-tile display lists and executes them on calls to
 * lp_setup_flush().
 */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "util/u_surface.h"
#include "lp_scene.h"
#include "lp_scene_queue.h"
#include "lp_buffer.h"
#include "lp_texture.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_rast.h"
#include "lp_setup_context.h"
#include "lp_screen.h"
#include "lp_winsys.h"

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"


static void set_scene_state( struct setup_context *, unsigned );


struct lp_scene *
lp_setup_get_current_scene(struct setup_context *setup)
{
   if (!setup->scene) {

      /* wait for a free/empty scene
       */
      setup->scene = lp_scene_dequeue(setup->empty_scenes, TRUE);

      assert(lp_scene_is_empty(setup->scene));

      lp_scene_begin_binning(setup->scene,
                             &setup->fb );
   }
   return setup->scene;
}


static void
first_triangle( struct setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_triangle( setup );
   setup->triangle( setup, v0, v1, v2 );
}

static void
first_line( struct setup_context *setup,
	    const float (*v0)[4],
	    const float (*v1)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_line( setup );
   setup->line( setup, v0, v1 );
}

static void
first_point( struct setup_context *setup,
	     const float (*v0)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_point( setup );
   setup->point( setup, v0 );
}

static void reset_context( struct setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   /* Reset derived state */
   setup->constants.stored_size = 0;
   setup->constants.stored_data = NULL;
   setup->fs.stored = NULL;
   setup->dirty = ~0;

   /* no current bin */
   setup->scene = NULL;

   /* Reset some state:
    */
   setup->clear.flags = 0;

   /* Have an explicit "start-binning" call and get rid of this
    * pointer twiddling?
    */
   setup->line = first_line;
   setup->point = first_point;
   setup->triangle = first_triangle;
}


/** Rasterize all scene's bins */
static void
lp_setup_rasterize_scene( struct setup_context *setup,
                          boolean write_depth )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);

   lp_scene_rasterize(scene,
                      setup->rast,
                      write_depth);

   reset_context( setup );

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}



static void
begin_binning( struct setup_context *setup )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);

   LP_DBG(DEBUG_SETUP, "%s color: %s depth: %s\n", __FUNCTION__,
          (setup->clear.flags & PIPE_CLEAR_COLOR) ? "clear": "load",
          (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) ? "clear": "load");

   if (setup->fb.nr_cbufs) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR)
         lp_scene_bin_everywhere( scene, 
				  lp_rast_clear_color, 
				  setup->clear.color );
      else
         lp_scene_bin_everywhere( scene,
				  lp_rast_load_color,
				  lp_rast_arg_null() );
   }

   if (setup->fb.zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         lp_scene_bin_everywhere( scene, 
				  lp_rast_clear_zstencil, 
				  setup->clear.zstencil );
   }

   LP_DBG(DEBUG_SETUP, "%s done\n", __FUNCTION__);
}


/* This basically bins and then flushes any outstanding full-screen
 * clears.  
 *
 * TODO: fast path for fullscreen clears and no triangles.
 */
static void
execute_clears( struct setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   begin_binning( setup );
   lp_setup_rasterize_scene( setup, TRUE );
}


static void
set_scene_state( struct setup_context *setup,
           unsigned new_state )
{
   unsigned old_state = setup->state;

   if (old_state == new_state)
      return;
       
   LP_DBG(DEBUG_SETUP, "%s old %d new %d\n", __FUNCTION__, old_state, new_state);

   switch (new_state) {
   case SETUP_ACTIVE:
      begin_binning( setup );
      break;

   case SETUP_CLEARED:
      if (old_state == SETUP_ACTIVE) {
         assert(0);
         return;
      }
      break;
      
   case SETUP_FLUSHED:
      if (old_state == SETUP_CLEARED)
         execute_clears( setup );
      else
         lp_setup_rasterize_scene( setup, TRUE );
      break;
   }

   setup->state = new_state;
}


void
lp_setup_flush( struct setup_context *setup,
                unsigned flags )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   set_scene_state( setup, SETUP_FLUSHED );
}


void
lp_setup_bind_framebuffer( struct setup_context *setup,
                           const struct pipe_framebuffer_state *fb )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   /* Flush any old scene.
    */
   set_scene_state( setup, SETUP_FLUSHED );

   /* Set new state.  This will be picked up later when we next need a
    * scene.
    */
   util_copy_framebuffer_state(&setup->fb, fb);
}


void
lp_setup_clear( struct setup_context *setup,
                const float *color,
                double depth,
                unsigned stencil,
                unsigned flags )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s state %d\n", __FUNCTION__, setup->state);


   if (flags & PIPE_CLEAR_COLOR) {
      for (i = 0; i < 4; ++i)
         setup->clear.color.clear_color[i] = float_to_ubyte(color[i]);
   }

   if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
      setup->clear.zstencil.clear_zstencil = 
         util_pack_z_stencil(setup->fb.zsbuf->format, 
                             depth,
                             stencil);
   }

   if (setup->state == SETUP_ACTIVE) {
      /* Add the clear to existing scene.  In the unusual case where
       * both color and depth-stencil are being cleared when there's
       * already been some rendering, we could discard the currently
       * binned scene and start again, but I don't see that as being
       * a common usage.
       */
      if (flags & PIPE_CLEAR_COLOR)
         lp_scene_bin_everywhere( scene, 
                                  lp_rast_clear_color,
                                  setup->clear.color );

      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         lp_scene_bin_everywhere( scene, 
                                  lp_rast_clear_zstencil,
                                  setup->clear.zstencil );
   }
   else {
      /* Put ourselves into the 'pre-clear' state, specifically to try
       * and accumulate multiple clears to color and depth_stencil
       * buffers which the app or state-tracker might issue
       * separately.
       */
      set_scene_state( setup, SETUP_CLEARED );

      setup->clear.flags |= flags;
   }
}


/**
 * Emit a fence.
 */
struct pipe_fence_handle *
lp_setup_fence( struct setup_context *setup )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   const unsigned rank = lp_scene_get_num_bins( scene ); /* xxx */
   struct lp_fence *fence = lp_fence_create(rank);

   LP_DBG(DEBUG_SETUP, "%s rank %u\n", __FUNCTION__, rank);

   set_scene_state( setup, SETUP_ACTIVE );

   /* insert the fence into all command bins */
   lp_scene_bin_everywhere( scene,
			    lp_rast_fence,
			    lp_rast_arg_fence(fence) );

   return (struct pipe_fence_handle *) fence;
}


void 
lp_setup_set_triangle_state( struct setup_context *setup,
                             unsigned cull_mode,
                             boolean ccw_is_frontface,
                             boolean scissor )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->ccw_is_frontface = ccw_is_frontface;
   setup->cullmode = cull_mode;
   setup->triangle = first_triangle;
   setup->scissor_test = scissor;
}



void
lp_setup_set_fs_inputs( struct setup_context *setup,
                        const struct lp_shader_input *input,
                        unsigned nr )
{
   LP_DBG(DEBUG_SETUP, "%s %p %u\n", __FUNCTION__, (void *) input, nr);

   memcpy( setup->fs.input, input, nr * sizeof input[0] );
   setup->fs.nr_inputs = nr;
}

void
lp_setup_set_fs_functions( struct setup_context *setup,
                           lp_jit_frag_func jit_function0,
                           lp_jit_frag_func jit_function1,
                           boolean opaque )
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__, (void *) jit_function0);
   /* FIXME: reference count */

   setup->fs.current.jit_function[0] = jit_function0;
   setup->fs.current.jit_function[1] = jit_function1;
   setup->fs.current.opaque = opaque;
   setup->dirty |= LP_SETUP_NEW_FS;
}

void
lp_setup_set_fs_constants(struct setup_context *setup,
                          struct pipe_buffer *buffer)
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__, (void *) buffer);

   pipe_buffer_reference(&setup->constants.current, buffer);

   setup->dirty |= LP_SETUP_NEW_CONSTANTS;
}


void
lp_setup_set_alpha_ref_value( struct setup_context *setup,
                              float alpha_ref_value )
{
   LP_DBG(DEBUG_SETUP, "%s %f\n", __FUNCTION__, alpha_ref_value);

   if(setup->fs.current.jit_context.alpha_ref_value != alpha_ref_value) {
      setup->fs.current.jit_context.alpha_ref_value = alpha_ref_value;
      setup->dirty |= LP_SETUP_NEW_FS;
   }
}

void
lp_setup_set_blend_color( struct setup_context *setup,
                          const struct pipe_blend_color *blend_color )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(blend_color);

   if(memcmp(&setup->blend_color.current, blend_color, sizeof *blend_color) != 0) {
      memcpy(&setup->blend_color.current, blend_color, sizeof *blend_color);
      setup->dirty |= LP_SETUP_NEW_BLEND_COLOR;
   }
}


void
lp_setup_set_scissor( struct setup_context *setup,
                      const struct pipe_scissor_state *scissor )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(scissor);

   if (memcmp(&setup->scissor.current, scissor, sizeof(*scissor)) != 0) {
      setup->scissor.current = *scissor; /* struct copy */
      setup->dirty |= LP_SETUP_NEW_SCISSOR;
   }
}


void 
lp_setup_set_flatshade_first( struct setup_context *setup,
                              boolean flatshade_first )
{
   setup->flatshade_first = flatshade_first;
}


void 
lp_setup_set_vertex_info( struct setup_context *setup,
                          struct vertex_info *vertex_info )
{
   /* XXX: just silently holding onto the pointer:
    */
   setup->vertex_info = vertex_info;
}


/**
 * Called during state validation when LP_NEW_TEXTURE is set.
 */
void
lp_setup_set_sampler_textures( struct setup_context *setup,
                               unsigned num, struct pipe_texture **texture)
{
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      if(tex) {
         struct llvmpipe_texture *lp_tex = llvmpipe_texture(tex);
         struct lp_jit_texture *jit_tex;
         jit_tex = &setup->fs.current.jit_context.textures[i];
         jit_tex->width = tex->width0;
         jit_tex->height = tex->height0;
         jit_tex->depth = tex->depth0;
         jit_tex->last_level = tex->last_level;
         jit_tex->stride = lp_tex->stride[0];
         if(!lp_tex->dt) {
            jit_tex->data = lp_tex->data;
         }
         else {
            /*
             * XXX: Where should this be unmapped?
             */

            struct llvmpipe_screen *screen = llvmpipe_screen(tex->screen);
            struct llvmpipe_winsys *winsys = screen->winsys;
            jit_tex->data = winsys->displaytarget_map(winsys, lp_tex->dt,
                                                      PIPE_BUFFER_USAGE_CPU_READ);
            assert(jit_tex->data);
         }

         /* the scene references this texture */
         {
            struct lp_scene *scene = lp_setup_get_current_scene(setup);
            lp_scene_texture_reference(scene, tex);
         }
      }
   }

   setup->dirty |= LP_SETUP_NEW_FS;
}


/**
 * Is the given texture referenced by any scene?
 * Note: we have to check all scenes including any scenes currently
 * being rendered and the current scene being built.
 */
unsigned
lp_setup_is_texture_referenced( const struct setup_context *setup,
                                const struct pipe_texture *texture )
{
   unsigned i;

   /* check the render targets */
   for (i = 0; i < setup->fb.nr_cbufs; i++) {
      if (setup->fb.cbufs[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
   }
   if (setup->fb.zsbuf && setup->fb.zsbuf->texture == texture) {
      return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
   }

   /* check textures referenced by the scene */
   for (i = 0; i < Elements(setup->scenes); i++) {
      if (lp_scene_is_texture_referenced(setup->scenes[i], texture)) {
         return PIPE_REFERENCED_FOR_READ;
      }
   }

   return PIPE_UNREFERENCED;
}


/**
 * Called by vbuf code when we're about to draw something.
 */
void
lp_setup_update_state( struct setup_context *setup )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(setup->fs.current.jit_function);

   if(setup->dirty & LP_SETUP_NEW_BLEND_COLOR) {
      uint8_t *stored;
      unsigned i, j;

      stored = lp_scene_alloc_aligned(scene, 4 * 16, 16);

      /* smear each blend color component across 16 ubyte elements */
      for (i = 0; i < 4; ++i) {
         uint8_t c = float_to_ubyte(setup->blend_color.current.color[i]);
         for (j = 0; j < 16; ++j)
            stored[i*16 + j] = c;
      }

      setup->blend_color.stored = stored;

      setup->fs.current.jit_context.blend_color = setup->blend_color.stored;
      setup->dirty |= LP_SETUP_NEW_FS;
   }

   if (setup->dirty & LP_SETUP_NEW_SCISSOR) {
      float *stored;

      stored = lp_scene_alloc_aligned(scene, 4 * sizeof(int32_t), 16);

      stored[0] = (float) setup->scissor.current.minx;
      stored[1] = (float) setup->scissor.current.miny;
      stored[2] = (float) setup->scissor.current.maxx;
      stored[3] = (float) setup->scissor.current.maxy;

      setup->scissor.stored = stored;

      setup->fs.current.jit_context.scissor_xmin = stored[0];
      setup->fs.current.jit_context.scissor_ymin = stored[1];
      setup->fs.current.jit_context.scissor_xmax = stored[2];
      setup->fs.current.jit_context.scissor_ymax = stored[3];

      setup->dirty |= LP_SETUP_NEW_FS;
   }

   if(setup->dirty & LP_SETUP_NEW_CONSTANTS) {
      struct pipe_buffer *buffer = setup->constants.current;

      if(buffer) {
         unsigned current_size = buffer->size;
         const void *current_data = llvmpipe_buffer(buffer)->data;

         /* TODO: copy only the actually used constants? */

         if(setup->constants.stored_size != current_size ||
            !setup->constants.stored_data ||
            memcmp(setup->constants.stored_data,
                   current_data,
                   current_size) != 0) {
            void *stored;

            stored = lp_scene_alloc(scene, current_size);
            if(stored) {
               memcpy(stored,
                      current_data,
                      current_size);
               setup->constants.stored_size = current_size;
               setup->constants.stored_data = stored;
            }
         }
      }
      else {
         setup->constants.stored_size = 0;
         setup->constants.stored_data = NULL;
      }

      setup->fs.current.jit_context.constants = setup->constants.stored_data;
      setup->dirty |= LP_SETUP_NEW_FS;
   }


   if(setup->dirty & LP_SETUP_NEW_FS) {
      if(!setup->fs.stored ||
         memcmp(setup->fs.stored,
                &setup->fs.current,
                sizeof setup->fs.current) != 0) {
         /* The fs state that's been stored in the scene is different from
          * the new, current state.  So allocate a new lp_rast_state object
          * and append it to the bin's setup data buffer.
          */
         struct lp_rast_state *stored =
            (struct lp_rast_state *) lp_scene_alloc(scene, sizeof *stored);
         if(stored) {
            memcpy(stored,
                   &setup->fs.current,
                   sizeof setup->fs.current);
            setup->fs.stored = stored;

            /* put the state-set command into all bins */
            lp_scene_bin_state_command( scene,
					lp_rast_set_state, 
					lp_rast_arg_state(setup->fs.stored) );
         }
      }
   }

   setup->dirty = 0;

   assert(setup->fs.stored);
}



/* Only caller is lp_setup_vbuf_destroy()
 */
void 
lp_setup_destroy( struct setup_context *setup )
{
   reset_context( setup );

   pipe_buffer_reference(&setup->constants.current, NULL);

   /* free the scenes in the 'empty' queue */
   while (1) {
      struct lp_scene *scene = lp_scene_dequeue(setup->empty_scenes, FALSE);
      if (!scene)
         break;
      lp_scene_destroy(scene);
   }

   lp_rast_destroy( setup->rast );

   FREE( setup );
}


/**
 * Create a new primitive tiling engine.  Plug it into the backend of
 * the draw module.  Currently also creates a rasterizer to use with
 * it.
 */
struct setup_context *
lp_setup_create( struct pipe_context *pipe,
                 struct draw_context *draw )
{
   unsigned i;
   struct setup_context *setup = CALLOC_STRUCT(setup_context);

   if (!setup)
      return NULL;

   lp_setup_init_vbuf(setup);

   setup->empty_scenes = lp_scene_queue_create();
   if (!setup->empty_scenes)
      goto fail;

   /* XXX: move this to the screen and share between contexts:
    */
   setup->rast = lp_rast_create();
   if (!setup->rast) 
      goto fail;

   setup->vbuf = draw_vbuf_stage(draw, &setup->base);
   if (!setup->vbuf)
      goto fail;

   draw_set_rasterize_stage(draw, setup->vbuf);
   draw_set_render(draw, &setup->base);

   /* create some empty scenes */
   for (i = 0; i < MAX_SCENES; i++) {
      setup->scenes[i] = lp_scene_create( pipe, setup->empty_scenes );

      lp_scene_enqueue(setup->empty_scenes, setup->scenes[i]);
   }

   setup->triangle = first_triangle;
   setup->line     = first_line;
   setup->point    = first_point;
   
   setup->dirty = ~0;

   return setup;

fail:
   if (setup->rast)
      lp_rast_destroy( setup->rast );
   
   if (setup->vbuf)
      ;

   if (setup->empty_scenes)
      lp_scene_queue_destroy(setup->empty_scenes);

   FREE(setup);
   return NULL;
}

