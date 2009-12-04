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
#include "pipe/p_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "lp_debug.h"
#include "lp_state.h"
#include "lp_buffer.h"
#include "lp_texture.h"
#include "lp_setup_context.h"


static void set_state( struct setup_context *, unsigned );



static void
first_triangle( struct setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4])
{
   set_state( setup, SETUP_ACTIVE );
   lp_setup_choose_triangle( setup );
   setup->triangle( setup, v0, v1, v2 );
}

static void
first_line( struct setup_context *setup,
	    const float (*v0)[4],
	    const float (*v1)[4])
{
   set_state( setup, SETUP_ACTIVE );
   lp_setup_choose_line( setup );
   setup->line( setup, v0, v1 );
}

static void
first_point( struct setup_context *setup,
	     const float (*v0)[4])
{
   set_state( setup, SETUP_ACTIVE );
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

   lp_reset_bins(&setup->bins, setup->tiles_x, setup->tiles_y);

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


/**
 * Return last command in the bin
 */
static lp_rast_cmd
lp_get_last_command( const struct cmd_bin *bin )
{
   const struct cmd_block *tail = bin->commands.tail;
   const unsigned i = tail->count;
   if (i > 0)
      return tail->cmd[i - 1];
   else
      return NULL;
}


/**
 * Replace the arg of the last command in the bin.
 */
static void
lp_replace_last_command_arg( struct cmd_bin *bin,
                             const union lp_rast_cmd_arg arg )
{
   struct cmd_block *tail = bin->commands.tail;
   const unsigned i = tail->count;
   assert(i > 0);
   tail->arg[i - 1] = arg;
}



/* Add a command to all active bins.
 */
static void bin_everywhere( struct setup_context *setup,
                            lp_rast_cmd cmd,
                            const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < setup->tiles_x; i++)
      for (j = 0; j < setup->tiles_y; j++)
         lp_bin_command( &setup->bins, i, j, cmd, arg );
}


/**
 * Put a state-change command into all bins.
 * If we find that the last command in a bin was also a state-change
 * command, we can simply replace that one with the new one.
 */
static void
bin_state_command( struct setup_context *setup,
                   lp_rast_cmd cmd,
                   const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < setup->tiles_x; i++) {
      for (j = 0; j < setup->tiles_y; j++) {
         struct cmd_bin *bin = &setup->bins.tile[i][j];
         lp_rast_cmd last_cmd = lp_get_last_command(bin);
         if (last_cmd == cmd) {
            lp_replace_last_command_arg(bin, arg);
         }
         else {
            lp_bin_command( &setup->bins, i, j, cmd, arg );
         }
      }
   }
}


/** Rasterize all tile's bins */
static void
rasterize_bins( struct setup_context *setup,
                boolean write_depth )
{
   lp_rasterize_bins(setup->rast,
                     &setup->bins, setup->tiles_x, setup->tiles_y,
                     setup->fb,
                     write_depth);

   reset_context( setup );

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}



static void
begin_binning( struct setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   if (setup->fb->cbufs[0]) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR)
         bin_everywhere( setup, 
                         lp_rast_clear_color, 
                         setup->clear.color );
      else
         bin_everywhere( setup, lp_rast_load_color, lp_rast_arg_null() );
   }

   if (setup->fb->zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         bin_everywhere( setup, 
                         lp_rast_clear_zstencil, 
                         setup->clear.zstencil );
      else
         bin_everywhere( setup, lp_rast_load_zstencil, lp_rast_arg_null() );
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
   rasterize_bins( setup, TRUE );
}


static void
set_state( struct setup_context *setup,
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
         rasterize_bins( setup, TRUE );
      break;
   }

   setup->state = new_state;
}


void
lp_setup_flush( struct setup_context *setup,
                unsigned flags )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   set_state( setup, SETUP_FLUSHED );
}


void
lp_setup_bind_framebuffer( struct setup_context *setup,
                           const struct pipe_framebuffer_state *fb )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   set_state( setup, SETUP_FLUSHED );

   setup->fb = fb;
   setup->tiles_x = align(setup->fb->width, TILE_SIZE) / TILE_SIZE;
   setup->tiles_y = align(setup->fb->height, TILE_SIZE) / TILE_SIZE;
}


void
lp_setup_clear( struct setup_context *setup,
                const float *color,
                double depth,
                unsigned stencil,
                unsigned flags )
{
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s state %d\n", __FUNCTION__, setup->state);


   if (flags & PIPE_CLEAR_COLOR) {
      for (i = 0; i < 4; ++i)
         setup->clear.color.clear_color[i] = float_to_ubyte(color[i]);
   }

   if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
      setup->clear.zstencil.clear_zstencil = 
         util_pack_z_stencil(setup->fb->zsbuf->format, 
                             depth,
                             stencil);
   }

   if (setup->state == SETUP_ACTIVE) {
      /* Add the clear to existing bins.  In the unusual case where
       * both color and depth-stencilare being cleared, we could
       * discard the currently binned scene and start again, but I
       * don't see that as being a common usage.
       */
      if (flags & PIPE_CLEAR_COLOR)
         bin_everywhere( setup, 
                         lp_rast_clear_color, 
                         setup->clear.color );

      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         bin_everywhere( setup, 
                         lp_rast_clear_zstencil, 
                         setup->clear.zstencil );
   }
   else {
      /* Put ourselves into the 'pre-clear' state, specifically to try
       * and accumulate multiple clears to color and depth_stencil
       * buffers which the app or state-tracker might issue
       * separately.
       */
      set_state( setup, SETUP_CLEARED );

      setup->clear.flags |= flags;
   }
}



void 
lp_setup_set_triangle_state( struct setup_context *setup,
                             unsigned cull_mode,
                             boolean ccw_is_frontface)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->ccw_is_frontface = ccw_is_frontface;
   setup->cullmode = cull_mode;
   setup->triangle = first_triangle;
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
lp_setup_set_fs( struct setup_context *setup,
                 struct lp_fragment_shader *fs )
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__, (void *) fs);
   /* FIXME: reference count */

   setup->fs.current.jit_function = fs ? fs->current->jit_function : NULL;
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
lp_setup_set_sampler_textures( struct setup_context *setup,
                               unsigned num, struct pipe_texture **texture)
{
   struct pipe_texture *dummy;
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);


   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      /* FIXME: hold on to the reference */
      dummy = NULL;
      pipe_texture_reference(&dummy, tex);

      if(tex) {
         struct llvmpipe_texture *lp_tex = llvmpipe_texture(tex);
         struct lp_jit_texture *jit_tex;
         jit_tex = &setup->fs.current.jit_context.textures[i];
         jit_tex->width = tex->width[0];
         jit_tex->height = tex->height[0];
         jit_tex->stride = lp_tex->stride[0];
         if(!lp_tex->dt)
            jit_tex->data = lp_tex->data;
         else
            /* FIXME: map the rendertarget */
            assert(0);
      }
   }

   setup->dirty |= LP_SETUP_NEW_FS;
}

boolean
lp_setup_is_texture_referenced( struct setup_context *setup,
                                const struct pipe_texture *texture )
{
   /* FIXME */
   return PIPE_UNREFERENCED;
}


static INLINE void
lp_setup_update_shader_state( struct setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(setup->fs.current.jit_function);

   if(setup->dirty & LP_SETUP_NEW_BLEND_COLOR) {
      uint8_t *stored;
      unsigned i, j;

      stored = lp_bin_alloc_aligned(&setup->bins, 4 * 16, 16);

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

            stored = lp_bin_alloc(&setup->bins, current_size);
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
         /* The fs state that's been stored in the bins is different from
          * the new, current state.  So allocate a new lp_rast_state object
          * and append it to the bin's setup data buffer.
          */
         struct lp_rast_state *stored =
            (struct lp_rast_state *) lp_bin_alloc(&setup->bins, sizeof *stored);
         if(stored) {
            memcpy(stored,
                   &setup->fs.current,
                   sizeof setup->fs.current);
            setup->fs.stored = stored;

            /* put the state-set command into all bins */
            bin_state_command( setup, 
                               lp_rast_set_state, 
                               lp_rast_arg_state(setup->fs.stored) );
         }
      }
   }

   setup->dirty = 0;

   assert(setup->fs.stored);
}


/* Stubs for lines & points for now:
 */
void
lp_setup_point(struct setup_context *setup,
		     const float (*v0)[4])
{
   lp_setup_update_shader_state(setup);
   setup->point( setup, v0 );
}

void
lp_setup_line(struct setup_context *setup,
		    const float (*v0)[4],
		    const float (*v1)[4])
{
   lp_setup_update_shader_state(setup);
   setup->line( setup, v0, v1 );
}

void
lp_setup_tri(struct setup_context *setup,
             const float (*v0)[4],
             const float (*v1)[4],
             const float (*v2)[4])
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   lp_setup_update_shader_state(setup);
   setup->triangle( setup, v0, v1, v2 );
}


void 
lp_setup_destroy( struct setup_context *setup )
{
   reset_context( setup );

   pipe_buffer_reference(&setup->constants.current, NULL);

   lp_free_bin_data(&setup->bins);

   lp_rast_destroy( setup->rast );
   FREE( setup );
}


/**
 * Create a new primitive tiling engine.  Currently also creates a
 * rasterizer to use with it.
 */
struct setup_context *
lp_setup_create( struct pipe_screen *screen )
{
   struct setup_context *setup = CALLOC_STRUCT(setup_context);

   setup->rast = lp_rast_create( screen );
   if (!setup->rast) 
      goto fail;

   lp_init_bins(&setup->bins);

   setup->triangle = first_triangle;
   setup->line     = first_line;
   setup->point    = first_point;
   
   setup->dirty = ~0;

   return setup;

fail:
   FREE(setup);
   return NULL;
}

