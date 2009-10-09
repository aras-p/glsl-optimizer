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
#include "lp_state.h"
#include "lp_buffer.h"
#include "lp_texture.h"
#include "lp_setup_context.h"

static void set_state( struct setup_context *, unsigned );

void lp_setup_new_cmd_block( struct cmd_block_list *list )
{
   struct cmd_block *block = MALLOC_STRUCT(cmd_block);
   list->tail->next = block;
   list->tail = block;
   block->next = NULL;
   block->count = 0;
}

void lp_setup_new_data_block( struct data_block_list *list )
{
   struct data_block *block = MALLOC_STRUCT(data_block);
   list->tail->next = block;
   list->tail = block;
   block->next = NULL;
   block->used = 0;
}


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
   unsigned i, j;

   /* Free binner command lists:
    */
   for (i = 0; i < setup->tiles_x; i++) {
      for (j = 0; j < setup->tiles_y; j++) {
         struct cmd_block_list *list = &setup->tile[i][j];
         struct cmd_block *block;
         struct cmd_block *tmp;
         
         for (block = list->head; block != list->tail; block = tmp) {
            tmp = block->next;
            FREE(block);
         }
         
         list->head = list->tail;
      }
   }

   /* Free binned data:
    */
   {
      struct data_block_list *list = &setup->data;
      struct data_block *block, *tmp;

      for (block = list->head; block != list->tail; block = tmp) {
         tmp = block->next;
         FREE(block);
      }
         
      list->head = list->tail;
   }

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




/* Add a command to all active bins.
 */
static void bin_everywhere( struct setup_context *setup,
                            lp_rast_cmd cmd,
                            const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < setup->tiles_x; i++)
      for (j = 0; j < setup->tiles_y; j++)
         bin_command( &setup->tile[i][j], cmd, arg );
}


static void
rasterize_bins( struct setup_context *setup,
                boolean write_depth )
{
   struct lp_rasterizer *rast = setup->rast;
   struct cmd_block *block;
   unsigned i,j,k;

   lp_rast_begin( rast,
                  setup->fb.cbuf, 
                  setup->fb.zsbuf,
                  setup->fb.cbuf != NULL,
                  setup->fb.zsbuf != NULL && write_depth,
                  setup->fb.width,
                  setup->fb.height );

                       

   for (i = 0; i < setup->tiles_x; i++) {
      for (j = 0; j < setup->tiles_y; j++) {

         lp_rast_start_tile( rast, 
                             i * TILESIZE,
                             j * TILESIZE );

         for (block = setup->tile[i][j].head; block; block = block->next) {
            for (k = 0; k < block->count; k++) {
               block->cmd[k]( rast, block->arg[k] );
            }
         }

         lp_rast_end_tile( rast );
      }
   }

   lp_rast_end( rast );

   reset_context( setup );
}



static void
begin_binning( struct setup_context *setup )
{
   if (!setup->fb.cbuf && !setup->fb.zsbuf) {
      setup->fb.width = 0;
      setup->fb.height = 0;
   }
   else if (!setup->fb.zsbuf) {
      setup->fb.width = setup->fb.cbuf->width;
      setup->fb.height = setup->fb.cbuf->height;
   }
   else if (!setup->fb.cbuf) {
      setup->fb.width = setup->fb.zsbuf->width;
      setup->fb.height = setup->fb.zsbuf->height;
   }
   else {
      /* XXX: not sure what we're really supposed to do for
       * mis-matched color & depth buffer sizes.
       */
      setup->fb.width = MIN2(setup->fb.cbuf->width,
                             setup->fb.zsbuf->width);
      setup->fb.height = MIN2(setup->fb.cbuf->height,
                              setup->fb.zsbuf->height);
   }

   setup->tiles_x = align(setup->fb.width, TILESIZE) / TILESIZE;
   setup->tiles_y = align(setup->fb.height, TILESIZE) / TILESIZE;

   if (setup->fb.cbuf) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR)
         bin_everywhere( setup, 
                         lp_rast_clear_color, 
                         setup->clear.color );
      else
         bin_everywhere( setup, lp_rast_load_color, lp_rast_arg_null() );
   }

   if (setup->fb.zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         bin_everywhere( setup, 
                         lp_rast_clear_zstencil, 
                         setup->clear.zstencil );
      else
         bin_everywhere( setup, lp_rast_load_zstencil, lp_rast_arg_null() );
   }
}


/* This basically bins and then flushes any outstanding full-screen
 * clears.  
 *
 * TODO: fast path for fullscreen clears and no triangles.
 */
static void
execute_clears( struct setup_context *setup )
{
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
       
   switch (new_state) {
   case SETUP_ACTIVE:
      if (old_state == SETUP_FLUSHED)
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
   set_state( setup, SETUP_FLUSHED );
}


void
lp_setup_bind_framebuffer( struct setup_context *setup,
                           struct pipe_surface *color,
                           struct pipe_surface *zstencil )
{
   set_state( setup, SETUP_FLUSHED );

   pipe_surface_reference( &setup->fb.cbuf, color );
   pipe_surface_reference( &setup->fb.zsbuf, zstencil );
}

void
lp_setup_clear( struct setup_context *setup,
                const float *color,
                double depth,
                unsigned stencil,
                unsigned flags )
{
   unsigned i;

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
   setup->ccw_is_frontface = ccw_is_frontface;
   setup->cullmode = cull_mode;
   setup->triangle = first_triangle;
}



void
lp_setup_set_fs_inputs( struct setup_context *setup,
                        const struct lp_shader_input *input,
                        unsigned nr )
{
   memcpy( setup->fs.input, input, nr * sizeof input[0] );
   setup->fs.nr_inputs = nr;
}

void
lp_setup_set_fs( struct setup_context *setup,
                 struct lp_fragment_shader *fs )
{
   /* FIXME: reference count */

   setup->fs.jit_function = fs->current->jit_function;
}

void
lp_setup_set_fs_constants(struct setup_context *setup,
                          struct pipe_buffer *buffer)
{
   const void *data = buffer ? llvmpipe_buffer(buffer)->data : NULL;
   struct pipe_buffer *dummy;

   /* FIXME: hold on to the reference */
   dummy = NULL;
   pipe_buffer_reference(&dummy, buffer);

   setup->fs.jit_context.constants = data;

   setup->fs.jit_context_dirty = TRUE;
}


void
lp_setup_set_alpha_ref_value( struct setup_context *setup,
                              float alpha_ref_value )
{
   if(setup->fs.jit_context.alpha_ref_value != alpha_ref_value) {
      setup->fs.jit_context.alpha_ref_value = alpha_ref_value;
      setup->fs.jit_context_dirty = TRUE;
   }
}

void
lp_setup_set_blend_color( struct setup_context *setup,
                          const struct pipe_blend_color *blend_color )
{
   unsigned i, j;

   if(!setup->fs.jit_context.blend_color)
      setup->fs.jit_context.blend_color = align_malloc(4 * 16, 16);

   for (i = 0; i < 4; ++i) {
      uint8_t c = float_to_ubyte(blend_color->color[i]);
      for (j = 0; j < 16; ++j)
         setup->fs.jit_context.blend_color[i*4 + j] = c;
   }

   setup->fs.jit_context_dirty = TRUE;
}

void
lp_setup_set_sampler_textures( struct setup_context *setup,
                               unsigned num, struct pipe_texture **texture)
{
   struct pipe_texture *dummy;
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_texture *tex = i < num ? texture[i] : NULL;

      /* FIXME: hold on to the reference */
      dummy = NULL;
      pipe_texture_reference(&dummy, tex);

      if(tex) {
         struct llvmpipe_texture *lp_tex = llvmpipe_texture(tex);
         struct lp_jit_texture *jit_tex = &setup->fs.jit_context.textures[i];
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

   setup->fs.jit_context_dirty = TRUE;
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

   if(setup->fs.jit_context_dirty) {
      if(!setup->fs.last_jc ||
         memcmp(setup->fs.last_jc, &setup->fs.jit_context, sizeof *setup->fs.last_jc)) {
         struct lp_jit_context *jc;

         jc = get_data(&setup->data, sizeof *jc);
         if(jc) {
            memcpy(jc, &setup->fs.jit_context, sizeof *jc);
            setup->fs.last_jc = jc;
         }
      }

      setup->fs.jit_context_dirty = FALSE;
   }

   assert(setup->fs.last_jc);
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
   lp_setup_update_shader_state(setup);
   setup->triangle( setup, v0, v1, v2 );
}


void 
lp_setup_destroy( struct setup_context *setup )
{
   unsigned i, j;

   reset_context( setup );

   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++)
         FREE(setup->tile[i][j].head);

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
   unsigned i, j;

   setup->rast = lp_rast_create( screen );
   if (!setup->rast) 
      goto fail;

   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++)
         setup->tile[i][j].head = 
            setup->tile[i][j].tail = CALLOC_STRUCT(cmd_block);

   setup->data.head =
      setup->data.tail = CALLOC_STRUCT(data_block);

   setup->triangle = first_triangle;
   setup->line     = first_line;
   setup->point    = first_point;
   
   return setup;

fail:
   FREE(setup);
   return NULL;
}

