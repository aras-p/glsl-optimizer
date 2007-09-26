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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "pipe/p_util.h"
#include "pipe/p_defines.h"
#include "draw_private.h"





/**
 * Rebuild the rendering pipeline.
 */
static void validate_begin( struct draw_stage *stage )
{
   struct draw_context *draw = stage->draw;
   struct draw_stage *next = draw->pipeline.rasterize;

   /*
    * NOTE: we build up the pipeline in end-to-start order.
    *
    * TODO: make the current primitive part of the state and build
    * shorter pipelines for lines & points.
    */

   if (draw->rasterizer->fill_cw != PIPE_POLYGON_MODE_FILL ||
       draw->rasterizer->fill_ccw != PIPE_POLYGON_MODE_FILL) {
      draw->pipeline.unfilled->next = next;
      next = draw->pipeline.unfilled;
   }
	 
   if (draw->rasterizer->offset_cw ||
       draw->rasterizer->offset_ccw) {
      draw->pipeline.offset->next = next;
      next = draw->pipeline.offset;
   }

   if (draw->rasterizer->light_twoside) {
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
   if (draw->rasterizer->flatshade) {
      draw->pipeline.flatshade->next = next;
      next = draw->pipeline.flatshade;
   }

   if (draw->feedback.enabled || draw->feedback.discard) {
      draw->pipeline.feedback->next = next;
      next = draw->pipeline.feedback;
   }

   draw->pipeline.first = next;
   draw->pipeline.first->begin( draw->pipeline.first );
}




/**
 * Create validate pipeline stage.
 */
struct draw_stage *draw_validate_stage( struct draw_context *draw )
{
   struct draw_stage *stage = CALLOC_STRUCT(draw_stage);

   stage->draw = draw;
   stage->next = NULL;
   stage->begin = validate_begin;
   stage->point = NULL;
   stage->line = NULL;
   stage->tri = NULL;
   stage->end = NULL;
   stage->reset_stipple_counter = NULL;

   return stage;
}
