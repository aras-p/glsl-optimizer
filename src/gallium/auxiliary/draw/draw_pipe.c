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

#include "pipe/p_util.h"
#include "draw/draw_private.h"
#include "draw/draw_pipe.h"


boolean draw_pipeline_init( struct draw_context *draw )
{
   /* create pipeline stages */
   draw->pipeline.wide_line  = draw_wide_line_stage( draw );
   draw->pipeline.wide_point = draw_wide_point_stage( draw );
   draw->pipeline.stipple   = draw_stipple_stage( draw );
   draw->pipeline.unfilled  = draw_unfilled_stage( draw );
   draw->pipeline.twoside   = draw_twoside_stage( draw );
   draw->pipeline.offset    = draw_offset_stage( draw );
   draw->pipeline.clip      = draw_clip_stage( draw );
   draw->pipeline.flatshade = draw_flatshade_stage( draw );
   draw->pipeline.cull      = draw_cull_stage( draw );
   draw->pipeline.validate  = draw_validate_stage( draw );
   draw->pipeline.first     = draw->pipeline.validate;

   if (!draw->pipeline.wide_line ||
       !draw->pipeline.wide_point ||
       !draw->pipeline.stipple ||
       !draw->pipeline.unfilled ||
       !draw->pipeline.twoside ||
       !draw->pipeline.offset ||
       !draw->pipeline.clip ||
       !draw->pipeline.flatshade ||
       !draw->pipeline.cull ||
       !draw->pipeline.validate)
      return FALSE;

   /* these defaults are oriented toward the needs of softpipe */
   draw->pipeline.wide_point_threshold = 1000000.0; /* infinity */
   draw->pipeline.wide_line_threshold = 1.0;
   draw->pipeline.line_stipple = TRUE;
   draw->pipeline.point_sprite = TRUE;

   return TRUE;
}


void draw_pipeline_destroy( struct draw_context *draw )
{
   if (draw->pipeline.wide_line)
      draw->pipeline.wide_line->destroy( draw->pipeline.wide_line );
   if (draw->pipeline.wide_point)
      draw->pipeline.wide_point->destroy( draw->pipeline.wide_point );
   if (draw->pipeline.stipple)
      draw->pipeline.stipple->destroy( draw->pipeline.stipple );
   if (draw->pipeline.unfilled)
      draw->pipeline.unfilled->destroy( draw->pipeline.unfilled );
   if (draw->pipeline.twoside)
      draw->pipeline.twoside->destroy( draw->pipeline.twoside );
   if (draw->pipeline.offset)
      draw->pipeline.offset->destroy( draw->pipeline.offset );
   if (draw->pipeline.clip)
      draw->pipeline.clip->destroy( draw->pipeline.clip );
   if (draw->pipeline.flatshade)
      draw->pipeline.flatshade->destroy( draw->pipeline.flatshade );
   if (draw->pipeline.cull)
      draw->pipeline.cull->destroy( draw->pipeline.cull );
   if (draw->pipeline.validate)
      draw->pipeline.validate->destroy( draw->pipeline.validate );
   if (draw->pipeline.aaline)
      draw->pipeline.aaline->destroy( draw->pipeline.aaline );
   if (draw->pipeline.aapoint)
      draw->pipeline.aapoint->destroy( draw->pipeline.aapoint );
   if (draw->pipeline.pstipple)
      draw->pipeline.pstipple->destroy( draw->pipeline.pstipple );
   if (draw->pipeline.rasterize)
      draw->pipeline.rasterize->destroy( draw->pipeline.rasterize );
}




/* This is only used for temporary verts.
 */
#define MAX_VERTEX_SIZE ((2 + PIPE_MAX_SHADER_OUTPUTS) * 4 * sizeof(float))


/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
void draw_alloc_temp_verts( struct draw_stage *stage, unsigned nr )
{
   assert(!stage->tmp);

   stage->nr_tmps = nr;

   if (nr) {
      ubyte *store = (ubyte *) MALLOC( MAX_VERTEX_SIZE * nr );
      unsigned i;

      stage->tmp = (struct vertex_header **) MALLOC( sizeof(struct vertex_header *) * nr );
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}


void draw_free_temp_verts( struct draw_stage *stage )
{
   if (stage->tmp) {
      FREE( stage->tmp[0] );
      FREE( stage->tmp );
      stage->tmp = NULL;
   }
}

void draw_reset_vertex_ids(struct draw_context *draw)
{
   struct draw_stage *stage = draw->pipeline.first;
   
   while (stage) {
      unsigned i;

      for (i = 0; i < stage->nr_tmps; i++)
	 stage->tmp[i]->vertex_id = UNDEFINED_VERTEX_ID;

      stage = stage->next;
   }

   draw_pt_reset_vertex_ids(draw);
}


