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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "imports.h"
#include "macros.h"

#include "draw_context.h"
#include "draw_private.h"


struct draw_context *draw_create( void )
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );

   /* create pipeline stages */
   draw->pipeline.unfilled  = prim_unfilled( draw );
   draw->pipeline.twoside   = prim_twoside( draw );
   draw->pipeline.offset    = prim_offset( draw );
   draw->pipeline.clip      = prim_clip( draw );
   draw->pipeline.flatshade = prim_flatshade( draw );
   draw->pipeline.cull      = prim_cull( draw );

   ASSIGN_4V( draw->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( draw->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( draw->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( draw->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   draw->nr_planes = 6;

   draw->vf = vf_create( GL_TRUE );

   return draw;
}


void draw_destroy( struct draw_context *draw )
{
   if (draw->header.storage)
      ALIGN_FREE( draw->header.storage );

   vf_destroy( draw->vf );

   FREE( draw );
}


/**
 * Rebuild the rendering pipeline.
 */
static void validate_pipeline( struct draw_context *draw )
{
   struct prim_stage *next = draw->pipeline.setup;

   /*
    * NOTE: we build up the pipeline in end-to-start order.
    *
    * TODO: make the current primitive part of the state and build
    * shorter pipelines for lines & points.
    */

   if (draw->setup.fill_cw != PIPE_POLYGON_MODE_FILL ||
       draw->setup.fill_ccw != PIPE_POLYGON_MODE_FILL) {
      draw->pipeline.unfilled->next = next;
      next = draw->pipeline.unfilled;
   }
	 
   if (draw->setup.offset_cw ||
       draw->setup.offset_ccw) {
      draw->pipeline.offset->next = next;
      next = draw->pipeline.offset;
   }

   if (draw->setup.light_twoside) {
      draw->pipeline.twoside->next = next;
      next = draw->pipeline.twoside;
   }

   /* Always run the cull stage as we calculate determinant there
    * also.  Fix this..
    */
   {
      draw->pipeline.cull->next = next;
      next = draw->pipeline.cull;
   }

   /* Clip stage
    */
   {
      draw->pipeline.clip->next = next;
      next = draw->pipeline.clip;
   }

   /* Do software flatshading prior to clipping.  XXX: should only do
    * this for clipped primitives, ie it is a part of the clip
    * routine.
    */
   if (draw->setup.flatshade) {
      draw->pipeline.flatshade->next = next;
      next = draw->pipeline.flatshade;
   }
   
   draw->pipeline.first = next;
}


/**
 * Register new primitive setup/rendering state.
 * This causes the drawing pipeline to be rebuilt.
 */
void draw_set_setup_state( struct draw_context *draw,
                           const struct pipe_setup_state *setup )
{
   memcpy( &draw->setup, setup, sizeof(*setup) );
   validate_pipeline( draw );
}


/** 
 * Plug in the primitive rendering/rasterization stage.
 * This is provided by the device driver.
 */
void draw_set_setup_stage( struct draw_context *draw,
                           struct prim_stage *stage )
{
   draw->pipeline.setup = stage;
}


/**
 * Set the draw module's clipping state.
 */
void draw_set_clip_state( struct draw_context *draw,
                          const struct pipe_clip_state *clip )
{
   memcpy(&draw->plane[6], clip->ucp, clip->nr * sizeof(clip->ucp[0]));
   draw->nr_planes = 6 + clip->nr;
}


/**
 * Set the draw module's viewport state.
 */
void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport )
{
   draw->viewport = *viewport; /* struct copy */

   vf_set_vp_scale_translate( draw->vf, viewport->scale, viewport->translate );

   /* Using tnl/ and vf/ modules is temporary while getting started.
    * Full pipe will have vertex shader, vertex fetch of its own.
    */
}
