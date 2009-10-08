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

#include "lp_setup_context.h"
#include "util/u_math.h"
#include "util/u_memory.h"

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
}




/* Add a command to all active bins.
 */
static void bin_everywhere( struct setup_context *setup,
                            lp_rast_cmd cmd,
                            const union lp_rast_cmd_arg *arg )
{
   unsigned i, j;
   for (i = 0; i < setup->tiles_x; i++)
      for (j = 0; j < setup->tiles_y; j++)
         bin_cmd( &setup->tile[i][j], cmd, arg );
}


static void
rasterize_bins( struct setup_context *setup,
                boolean write_depth )
{
   struct lp_rasterizer *rast = setup->rast;
   struct cmd_block *block;
   unsigned i,j,k;

   lp_rast_bind_color( rast, 
                       setup->fb.color, 
                       TRUE );                    /* WRITE */
                       
   lp_rast_bind_depth( rast,
                       setup->fb.zstencil,
                       write_depth );             /* WRITE */

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

   reset_context( setup );
}



static void
begin_binning( struct setup_context *setup )
{
   if (setup->fb.color) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR)
         bin_everywhere( setup, 
                         lp_rast_clear_color, 
                         &setup->clear.color );
      else
         bin_everywhere( setup, 
                         lp_rast_load_color, 
                         NULL );
   }

   if (setup->fb.zstencil) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL)
         bin_everywhere( setup, 
                         lp_rast_clear_zstencil, 
                         &setup->clear.zstencil );
      else
         bin_everywhere( setup, 
                         lp_rast_load_zstencil, 
                         NULL );
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
   unsigned width, height;

   set_state( setup, SETUP_FLUSHED );

   pipe_surface_reference( &setup->fb.color, color );
   pipe_surface_reference( &setup->fb.zstencil, zstencil );

   width = MAX2( color->width, zstencil->width );
   height = MAX2( color->height, zstencil->height );

   setup->tiles_x = align( width, TILESIZE ) / TILESIZE;
   setup->tiles_y = align( height, TILESIZE ) / TILESIZE;
}

void
lp_setup_clear( struct setup_context *setup,
                const float *clear_color,
                double clear_depth,
                unsigned clear_stencil,
                unsigned flags )
{
   if (setup->state == SETUP_ACTIVE) {
      struct lp_rast_clear_info *clear_info;

      clear_info = alloc_clear_info( setup );

      if (flags & PIPE_CLEAR_COLOR) {
         pack_color( setup, 
                     clear_info->color,
                     clear_color );
         bin_everywhere(setup, lp_rast_clear_color, clear_info );
      }

      if (flags & PIPE_CLEAR_DEPTH_STENCIL) {
         pack_depth_stencil( setup, 
                             clear_info->depth, 
                             clear_depth,
                             clear_stencil );
         
         bin_everywhere(setup, lp_rast_clear_zstencil, clear_info );
      }
   }
   else {
      set_state( setup, SETUP_CLEARED );

      setup->clear.flags |= flags;

      if (flags & PIPE_CLEAR_COLOR) {
         util_pack_color(rgba, 
                         setup->fb.cbuf->format, 
                         &setup->clear.color.clear_color );
      }

      if (flags & PIPE_CLEAR_DEPTH_STENCIL) {
         setup->clear.zstencil.clear_zstencil = 
            util_pack_z_stencil(setup->fb.zsbuf->format, 
                                depth,
                                stencil);
      }
   }
}


void
lp_setup_set_fs_inputs( struct setup_context *setup,
                        const enum lp_interp *interp,
                        unsigned nr )
{
   memcpy( setup->interp, interp, nr * sizeof interp[0] );
}

void
lp_setup_set_shader_state( struct setup_context *setup,
                           const struct jit_context *jc )
{
}


static void
first_triangle( struct setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4])
{
   set_state( setup, STATE_ACTIVE );
   setup_choose_triangle( setup, v0, v1, v2 );
}



/* Stubs for lines & points for now:
 */
void
lp_setup_point(struct setup_context *setup,
		     const float (*v0)[4])
{
   setup->point( setup, v0 );
}

void
lp_setup_line(struct setup_context *setup,
		    const float (*v0)[4],
		    const float (*v1)[4])
{
   setup->line( setup, v0, v1 );
}

void
lp_setup_tri(struct setup_context *setup,
             const float (*v0)[4],
             const float (*v1)[4],
             const float (*v2)[4])
{
   setup->triangle( setup, v0, v1, v2 );
}


void setup_destroy_context( struct setup_context *setup )
{
   lp_rast_destroy( setup->rast );
   FREE( setup );
}


/**
 * Create a new primitive tiling engine.  Currently also creates a
 * rasterizer to use with it.
 */
struct setup_context *setup_create_context( void )
{
   struct setup_context *setup = CALLOC_STRUCT(setup_context);

   setup->rast = lp_rast_create( void );
   if (!setup->rast) 
      goto fail;

   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++)
         setup->tile[i][j].first = 
            setup->tile[i][j].next = CALLOC_STRUCT(cmd_block);

   return setup;

fail:
   FREE(setup);
   return NULL;
}

