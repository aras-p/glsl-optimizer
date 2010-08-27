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

#include <limits.h>

#include "pipe/p_defines.h"
#include "util/u_framebuffer.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "lp_context.h"
#include "lp_memory.h"
#include "lp_scene.h"
#include "lp_scene_queue.h"
#include "lp_texture.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_query.h"
#include "lp_rast.h"
#include "lp_setup_context.h"
#include "lp_screen.h"
#include "lp_state.h"
#include "state_tracker/sw_winsys.h"

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"


static void set_scene_state( struct lp_setup_context *, enum setup_state );


struct lp_scene *
lp_setup_get_current_scene(struct lp_setup_context *setup)
{
   if (!setup->scene) {
      set_scene_state( setup, SETUP_EMPTY );
   }
   return setup->scene;
}


/**
 * Check if the size of the current scene has exceeded the limit.
 * If so, flush/render it.
 */
static void
setup_check_scene_size_and_flush(struct lp_setup_context *setup)
{
   if (setup->scene) {
      struct lp_scene *scene = lp_setup_get_current_scene(setup);
      unsigned size = lp_scene_get_size(scene);

      if (size > LP_MAX_SCENE_SIZE) {
         /*printf("LLVMPIPE: scene size = %u, flushing.\n", size);*/
         set_scene_state( setup, SETUP_FLUSHED );
         /*assert(lp_scene_get_size(scene) == 0);*/
      }
   }
}


static void
first_triangle( struct lp_setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_triangle( setup );
   setup->triangle( setup, v0, v1, v2 );
}

static void
first_line( struct lp_setup_context *setup,
	    const float (*v0)[4],
	    const float (*v1)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_line( setup );
   setup->line( setup, v0, v1 );
}

static void
first_point( struct lp_setup_context *setup,
	     const float (*v0)[4])
{
   set_scene_state( setup, SETUP_ACTIVE );
   lp_setup_choose_point( setup );
   setup->point( setup, v0 );
}

static void reset_context( struct lp_setup_context *setup )
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
   setup->clear.clearzs.clearzs_mask = 0;

   /* Have an explicit "start-binning" call and get rid of this
    * pointer twiddling?
    */
   setup->line = first_line;
   setup->point = first_point;
   setup->triangle = first_triangle;
}


/** Rasterize all scene's bins */
static void
lp_setup_rasterize_scene( struct lp_setup_context *setup )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   struct llvmpipe_screen *screen = llvmpipe_screen(scene->pipe->screen);

   pipe_mutex_lock(screen->rast_mutex);
   lp_scene_rasterize(scene, screen->rast);
   pipe_mutex_unlock(screen->rast_mutex);

   reset_context( setup );

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}



static void
begin_binning( struct lp_setup_context *setup )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   boolean need_zsload = FALSE;
   if (setup->fb.zsbuf &&
       ((setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
        util_format_is_depth_and_stencil(setup->fb.zsbuf->format))
      need_zsload = TRUE;

   LP_DBG(DEBUG_SETUP, "%s color: %s depth: %s\n", __FUNCTION__,
          (setup->clear.flags & PIPE_CLEAR_COLOR) ? "clear": "load",
          need_zsload ? "clear": "load");

   if (setup->fb.nr_cbufs) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR) {
         lp_scene_bin_everywhere( scene, 
				  lp_rast_clear_color, 
				  setup->clear.color );
         scene->has_color_clear = TRUE;
      }
   }

   if (setup->fb.zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) {
         if (!need_zsload)
            scene->has_depthstencil_clear = TRUE;
         lp_scene_bin_everywhere( scene,
                                  lp_rast_clear_zstencil,
                                  lp_rast_arg_clearzs(&setup->clear.clearzs) );
      }
   }

   LP_DBG(DEBUG_SETUP, "%s done\n", __FUNCTION__);
}


/* This basically bins and then flushes any outstanding full-screen
 * clears.  
 *
 * TODO: fast path for fullscreen clears and no triangles.
 */
static void
execute_clears( struct lp_setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   begin_binning( setup );
   lp_setup_rasterize_scene( setup );
}


static void
set_scene_state( struct lp_setup_context *setup,
                 enum setup_state new_state )
{
   unsigned old_state = setup->state;

   if (old_state == new_state)
      return;
       
   LP_DBG(DEBUG_SETUP, "%s old %d new %d\n", __FUNCTION__, old_state, new_state);

   switch (new_state) {
   case SETUP_EMPTY:
      assert(old_state == SETUP_FLUSHED);
      assert(setup->scene == NULL);

      /* wait for a free/empty scene
       */
      setup->scene = lp_scene_dequeue(setup->empty_scenes, TRUE);
      assert(lp_scene_is_empty(setup->scene));
      lp_scene_begin_binning(setup->scene,
                             &setup->fb );
      break;

   case SETUP_CLEARED:
      assert(old_state == SETUP_EMPTY);
      assert(setup->scene != NULL);
      break;

   case SETUP_ACTIVE:
      assert(old_state == SETUP_EMPTY ||
             old_state == SETUP_CLEARED);
      assert(setup->scene != NULL);
      begin_binning( setup );
      break;

   case SETUP_FLUSHED:
      if (old_state == SETUP_CLEARED)
         execute_clears( setup );
      else
         lp_setup_rasterize_scene( setup );
      assert(setup->scene == NULL);
      break;

   default:
      assert(0 && "invalid setup state mode");
   }

   setup->state = new_state;
}


/**
 * \param flags  bitmask of PIPE_FLUSH_x flags
 */
void
lp_setup_flush( struct lp_setup_context *setup,
                unsigned flags,
                struct pipe_fence_handle **fence,
                const char *reason)
{
   LP_DBG(DEBUG_SETUP, "%s %s\n", __FUNCTION__, reason);

   if (setup->scene) {
      if (fence) {
         /* if we're going to flush the setup/rasterization modules, emit
          * a fence.
          */
         *fence = lp_setup_fence( setup );
      }

      if (setup->scene->fence)
         setup->scene->fence->issued = TRUE;
   }

   set_scene_state( setup, SETUP_FLUSHED );
}


void
lp_setup_bind_framebuffer( struct lp_setup_context *setup,
                           const struct pipe_framebuffer_state *fb )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   /* Flush any old scene.
    */
   set_scene_state( setup, SETUP_FLUSHED );

   /*
    * Ensure the old scene is not reused.
    */
   assert(!setup->scene);

   /* Set new state.  This will be picked up later when we next need a
    * scene.
    */
   util_copy_framebuffer_state(&setup->fb, fb);
   setup->framebuffer.x0 = 0;
   setup->framebuffer.y0 = 0;
   setup->framebuffer.x1 = fb->width-1;
   setup->framebuffer.y1 = fb->height-1;
   setup->dirty |= LP_SETUP_NEW_SCISSOR;
}


void
lp_setup_clear( struct lp_setup_context *setup,
                const float *color,
                double depth,
                unsigned stencil,
                unsigned flags )
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   unsigned i;
   boolean full_zs_clear = TRUE;
   uint32_t mask = 0;

   LP_DBG(DEBUG_SETUP, "%s state %d\n", __FUNCTION__, setup->state);


   if (flags & PIPE_CLEAR_COLOR) {
      for (i = 0; i < 4; ++i)
         setup->clear.color.clear_color[i] = float_to_ubyte(color[i]);
   }

   if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
      if (setup->fb.zsbuf &&
          ((flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
           util_format_is_depth_and_stencil(setup->fb.zsbuf->format))
         full_zs_clear = FALSE;

      if (full_zs_clear) {
         setup->clear.clearzs.clearzs_value =
            util_pack_z_stencil(setup->fb.zsbuf->format,
                                depth,
                                stencil);
         setup->clear.clearzs.clearzs_mask = 0xffffffff;
      }
      else {
         /* hmm */
         uint32_t tmpval;
         if (flags & PIPE_CLEAR_DEPTH) {
            tmpval = util_pack_z(setup->fb.zsbuf->format,
                                 depth);
            switch (setup->fb.zsbuf->format) {
            case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
               mask = 0xffffff;
               break;
            case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
               mask = 0xffffff00;
               break;
            default:
               assert(0);
            }
         }
         else {
            switch (setup->fb.zsbuf->format) {
            case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
               mask = 0xff000000;
               tmpval = stencil << 24;
               break;
            case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
               mask = 0xff;
               tmpval = stencil;
               break;
            default:
               assert(0);
               tmpval = 0;
            }
         }
         setup->clear.clearzs.clearzs_mask |= mask;
         setup->clear.clearzs.clearzs_value =
            (setup->clear.clearzs.clearzs_value & ~mask) | (tmpval & mask);
      }
   }

   if (setup->state == SETUP_ACTIVE) {
      /* Add the clear to existing scene.  In the unusual case where
       * both color and depth-stencil are being cleared when there's
       * already been some rendering, we could discard the currently
       * binned scene and start again, but I don't see that as being
       * a common usage.
       */
      if (flags & PIPE_CLEAR_COLOR) {
         lp_scene_bin_everywhere( scene, 
                                  lp_rast_clear_color,
                                  setup->clear.color );
         scene->has_color_clear = TRUE;
      }

      if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
         if (full_zs_clear)
            scene->has_depthstencil_clear = TRUE;
         else
            setup->clear.clearzs.clearzs_mask = mask;
         lp_scene_bin_everywhere( scene,
                                  lp_rast_clear_zstencil,
                                  lp_rast_arg_clearzs(&setup->clear.clearzs) );


      }

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
lp_setup_fence( struct lp_setup_context *setup )
{
   if (setup->scene == NULL)
      return NULL;
   else if (setup->num_threads == 0)
      return NULL;
   else
   {
      struct lp_scene *scene = lp_setup_get_current_scene(setup);
      const unsigned rank = setup->num_threads;

      set_scene_state( setup, SETUP_ACTIVE );
      
      assert(scene->fence == NULL);

      /* The caller gets a reference, we keep a copy too, so need to
       * bump the refcount:
       */
      lp_fence_reference(&scene->fence, lp_fence_create(rank));

      LP_DBG(DEBUG_SETUP, "%s rank %u\n", __FUNCTION__, rank);

      return (struct pipe_fence_handle *) scene->fence;
   }
}


void 
lp_setup_set_triangle_state( struct lp_setup_context *setup,
                             unsigned cull_mode,
                             boolean ccw_is_frontface,
                             boolean scissor,
                             boolean gl_rasterization_rules)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->ccw_is_frontface = ccw_is_frontface;
   setup->cullmode = cull_mode;
   setup->triangle = first_triangle;
   setup->pixel_offset = gl_rasterization_rules ? 0.5f : 0.0f;

   if (setup->scissor_test != scissor) {
      setup->dirty |= LP_SETUP_NEW_SCISSOR;
      setup->scissor_test = scissor;
   }
}

void 
lp_setup_set_line_state( struct lp_setup_context *setup,
			 float line_width)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->line_width = line_width;
}

void 
lp_setup_set_point_state( struct lp_setup_context *setup,
                          float point_size)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->point_size = point_size;
}

void
lp_setup_set_fs_inputs( struct lp_setup_context *setup,
                        const struct lp_shader_input *input,
                        unsigned nr )
{
   LP_DBG(DEBUG_SETUP, "%s %p %u\n", __FUNCTION__, (void *) input, nr);

   memcpy( setup->fs.input, input, nr * sizeof input[0] );
   setup->fs.nr_inputs = nr;
}

void
lp_setup_set_fs_variant( struct lp_setup_context *setup,
                         struct lp_fragment_shader_variant *variant)
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__,
          variant);
   /* FIXME: reference count */

   setup->fs.current.variant = variant;
   setup->dirty |= LP_SETUP_NEW_FS;
}

void
lp_setup_set_fs_constants(struct lp_setup_context *setup,
                          struct pipe_resource *buffer)
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__, (void *) buffer);

   pipe_resource_reference(&setup->constants.current, buffer);

   setup->dirty |= LP_SETUP_NEW_CONSTANTS;
}


void
lp_setup_set_alpha_ref_value( struct lp_setup_context *setup,
                              float alpha_ref_value )
{
   LP_DBG(DEBUG_SETUP, "%s %f\n", __FUNCTION__, alpha_ref_value);

   if(setup->fs.current.jit_context.alpha_ref_value != alpha_ref_value) {
      setup->fs.current.jit_context.alpha_ref_value = alpha_ref_value;
      setup->dirty |= LP_SETUP_NEW_FS;
   }
}

void
lp_setup_set_stencil_ref_values( struct lp_setup_context *setup,
                                 const ubyte refs[2] )
{
   LP_DBG(DEBUG_SETUP, "%s %d %d\n", __FUNCTION__, refs[0], refs[1]);

   if (setup->fs.current.jit_context.stencil_ref_front != refs[0] ||
       setup->fs.current.jit_context.stencil_ref_back != refs[1]) {
      setup->fs.current.jit_context.stencil_ref_front = refs[0];
      setup->fs.current.jit_context.stencil_ref_back = refs[1];
      setup->dirty |= LP_SETUP_NEW_FS;
   }
}

void
lp_setup_set_blend_color( struct lp_setup_context *setup,
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
lp_setup_set_scissor( struct lp_setup_context *setup,
                      const struct pipe_scissor_state *scissor )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(scissor);

   setup->scissor.x0 = scissor->minx;
   setup->scissor.x1 = scissor->maxx-1;
   setup->scissor.y0 = scissor->miny;
   setup->scissor.y1 = scissor->maxy-1;
   setup->dirty |= LP_SETUP_NEW_SCISSOR;
}


void 
lp_setup_set_flatshade_first( struct lp_setup_context *setup,
                              boolean flatshade_first )
{
   setup->flatshade_first = flatshade_first;
}


void 
lp_setup_set_vertex_info( struct lp_setup_context *setup,
                          struct vertex_info *vertex_info )
{
   /* XXX: just silently holding onto the pointer:
    */
   setup->vertex_info = vertex_info;
}


/**
 * Called during state validation when LP_NEW_SAMPLER_VIEW is set.
 */
void
lp_setup_set_fragment_sampler_views(struct lp_setup_context *setup,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      if(view) {
         struct pipe_resource *tex = view->texture;
         struct llvmpipe_resource *lp_tex = llvmpipe_resource(tex);
         struct lp_jit_texture *jit_tex;
         jit_tex = &setup->fs.current.jit_context.textures[i];
         jit_tex->width = tex->width0;
         jit_tex->height = tex->height0;
         jit_tex->depth = tex->depth0;
         jit_tex->last_level = tex->last_level;

         /* We're referencing the texture's internal data, so save a
          * reference to it.
          */
         pipe_resource_reference(&setup->fs.current_tex[i], tex);

         if (!lp_tex->dt) {
            /* regular texture - setup array of mipmap level pointers */
            int j;
            for (j = 0; j <= tex->last_level; j++) {
               jit_tex->data[j] =
                  llvmpipe_get_texture_image_all(lp_tex, j, LP_TEX_USAGE_READ,
                                                 LP_TEX_LAYOUT_LINEAR);
               jit_tex->row_stride[j] = lp_tex->row_stride[j];
               jit_tex->img_stride[j] = lp_tex->img_stride[j];

               if (!jit_tex->data[j]) {
                  /* out of memory - use dummy tile memory */
                  jit_tex->data[j] = lp_dummy_tile;
                  jit_tex->width = TILE_SIZE;
                  jit_tex->height = TILE_SIZE;
                  jit_tex->depth = 1;
                  jit_tex->last_level = 0;
                  jit_tex->row_stride[j] = 0;
                  jit_tex->img_stride[j] = 0;
               }
            }
         }
         else {
            /* display target texture/surface */
            /*
             * XXX: Where should this be unmapped?
             */
            struct llvmpipe_screen *screen = llvmpipe_screen(tex->screen);
            struct sw_winsys *winsys = screen->winsys;
            jit_tex->data[0] = winsys->displaytarget_map(winsys, lp_tex->dt,
							 PIPE_TRANSFER_READ);
            jit_tex->row_stride[0] = lp_tex->row_stride[0];
            jit_tex->img_stride[0] = lp_tex->img_stride[0];
            assert(jit_tex->data[0]);
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
lp_setup_is_resource_referenced( const struct lp_setup_context *setup,
                                const struct pipe_resource *texture )
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
      if (lp_scene_is_resource_referenced(setup->scenes[i], texture)) {
         return PIPE_REFERENCED_FOR_READ;
      }
   }

   return PIPE_UNREFERENCED;
}


/**
 * Called by vbuf code when we're about to draw something.
 */
void
lp_setup_update_state( struct lp_setup_context *setup )
{
   struct lp_scene *scene;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup_check_scene_size_and_flush(setup);

   scene = lp_setup_get_current_scene(setup);

   assert(setup->fs.current.variant);

   /* Some of the 'draw' pipeline stages may have changed some driver state.
    * Make sure we've processed those state changes before anything else.
    *
    * XXX this is the only place where llvmpipe_context is used in the
    * setup code.  This may get refactored/changed...
    */
   {
      struct llvmpipe_context *lp = llvmpipe_context(scene->pipe);

      /* Will probably need to move this somewhere else, just need  
       * to know about vertex shader point size attribute.
       */
      setup->psize = lp->psize_slot;

      if (lp->dirty) {
         llvmpipe_update_derived(lp);
      }
      assert(lp->dirty == 0);
   }

   if(setup->dirty & LP_SETUP_NEW_BLEND_COLOR) {
      uint8_t *stored;
      unsigned i, j;

      stored = lp_scene_alloc_aligned(scene, 4 * 16, 16);

      if (stored) {
         /* smear each blend color component across 16 ubyte elements */
         for (i = 0; i < 4; ++i) {
            uint8_t c = float_to_ubyte(setup->blend_color.current.color[i]);
            for (j = 0; j < 16; ++j)
               stored[i*16 + j] = c;
         }

         setup->blend_color.stored = stored;

         setup->fs.current.jit_context.blend_color = setup->blend_color.stored;
      }

      setup->dirty |= LP_SETUP_NEW_FS;
   }

   if(setup->dirty & LP_SETUP_NEW_CONSTANTS) {
      struct pipe_resource *buffer = setup->constants.current;

      if(buffer) {
         unsigned current_size = buffer->width0;
         const void *current_data = llvmpipe_resource_data(buffer);

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
         uint i;
         struct lp_rast_state *stored =
            (struct lp_rast_state *) lp_scene_alloc(scene, sizeof *stored);
         if(stored) {
            memcpy(stored,
                   &setup->fs.current,
                   sizeof setup->fs.current);
            setup->fs.stored = stored;
         }

         /* The scene now references the textures in the rasterization
          * state record.  Note that now.
          */
         for (i = 0; i < Elements(setup->fs.current_tex); i++) {
            if (setup->fs.current_tex[i])
               lp_scene_add_resource_reference(scene, setup->fs.current_tex[i]);
         }
      }
   }

   if (setup->dirty & LP_SETUP_NEW_SCISSOR) {
      setup->draw_region = setup->framebuffer;
      if (setup->scissor_test) {
         u_rect_possible_intersection(&setup->scissor,
                                      &setup->draw_region);
      }
   }
                                      
   setup->dirty = 0;

   assert(setup->fs.stored);
}



/* Only caller is lp_setup_vbuf_destroy()
 */
void 
lp_setup_destroy( struct lp_setup_context *setup )
{
   uint i;

   reset_context( setup );

   util_unreference_framebuffer_state(&setup->fb);

   for (i = 0; i < Elements(setup->fs.current_tex); i++) {
      pipe_resource_reference(&setup->fs.current_tex[i], NULL);
   }

   pipe_resource_reference(&setup->constants.current, NULL);

   /* free the scenes in the 'empty' queue */
   while (1) {
      struct lp_scene *scene = lp_scene_dequeue(setup->empty_scenes, FALSE);
      if (!scene)
         break;
      lp_scene_destroy(scene);
   }

   lp_scene_queue_destroy(setup->empty_scenes);

   FREE( setup );
}


/**
 * Create a new primitive tiling engine.  Plug it into the backend of
 * the draw module.  Currently also creates a rasterizer to use with
 * it.
 */
struct lp_setup_context *
lp_setup_create( struct pipe_context *pipe,
                 struct draw_context *draw )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_setup_context *setup = CALLOC_STRUCT(lp_setup_context);
   unsigned i;

   if (!setup)
      return NULL;

   lp_setup_init_vbuf(setup);

   setup->empty_scenes = lp_scene_queue_create();
   if (!setup->empty_scenes)
      goto fail;

   setup->num_threads = screen->num_threads;
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
   if (setup->vbuf)
      ;

   if (setup->empty_scenes)
      lp_scene_queue_destroy(setup->empty_scenes);

   FREE(setup);
   return NULL;
}


/**
 * Put a BeginQuery command into all bins.
 */
void
lp_setup_begin_query(struct lp_setup_context *setup,
                     struct llvmpipe_query *pq)
{
   struct lp_scene * scene = lp_setup_get_current_scene(setup);
   union lp_rast_cmd_arg cmd_arg;

   /* init the query to its beginning state */
   pq->done = FALSE;
   pq->tile_count = 0;
   pq->num_tiles = scene->tiles_x * scene->tiles_y;
   assert(pq->num_tiles > 0);

   memset(pq->count, 0, sizeof(pq->count));  /* reset all counters */

   set_scene_state( setup, SETUP_ACTIVE );

   cmd_arg.query_obj = pq;
   lp_scene_bin_everywhere(scene, lp_rast_begin_query, cmd_arg);
   pq->binned = TRUE;
}


/**
 * Put an EndQuery command into all bins.
 */
void
lp_setup_end_query(struct lp_setup_context *setup, struct llvmpipe_query *pq)
{
   struct lp_scene * scene = lp_setup_get_current_scene(setup);
   union lp_rast_cmd_arg cmd_arg;

   set_scene_state( setup, SETUP_ACTIVE );

   cmd_arg.query_obj = pq;
   lp_scene_bin_everywhere(scene, lp_rast_end_query, cmd_arg);
}
