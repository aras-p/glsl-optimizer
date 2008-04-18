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
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"




/* Overall we split things into:
 *     - frontend -- prepare fetch_elts, draw_elts - eg vcache
 *     - middle   -- fetch, shade, cliptest, viewport
 *     - pipeline -- the prim pipeline: clipping, wide lines, etc 
 *     - backend  -- the vbuf_render provided by the driver.
 */
boolean
draw_pt_arrays(struct draw_context *draw, 
               unsigned prim,
               unsigned start, 
               unsigned count)
{
   struct draw_pt_front_end *frontend = NULL;
   struct draw_pt_middle_end *middle = NULL;
   unsigned opt = 0;

   if (!draw->render) {
      opt |= PT_PIPELINE;
   }

   if (draw_need_pipeline(draw, prim)) {
      opt |= PT_PIPELINE;
   }

   if (!draw->rasterizer->bypass_clipping) {
      opt |= PT_CLIPTEST;
   }

   if (!draw->rasterizer->bypass_vs) {
      opt |= PT_SHADE;
   }


   middle = draw->pt.middle.opt[opt];
   if (middle == NULL) {
      middle = draw->pt.middle.opt[PT_PIPELINE | PT_CLIPTEST | PT_SHADE];
   }

   assert(middle);

   /* May create a short-circuited version of this for small primitives:
    */
   frontend = draw->pt.front.vcache;


   frontend->prepare( frontend, prim, middle, opt );

   frontend->run( frontend,
                  draw_pt_elt_func( draw ),
                  draw_pt_elt_ptr( draw, start ),
                  count );

   frontend->finish( frontend );

   return TRUE;
}


boolean draw_pt_init( struct draw_context *draw )
{
   draw->pt.front.vcache = draw_pt_vcache( draw );
   if (!draw->pt.front.vcache)
      return FALSE;

   draw->pt.middle.opt[0] = draw_pt_fetch_emit( draw );
   draw->pt.middle.opt[PT_SHADE | PT_CLIPTEST | PT_PIPELINE] = 
      draw_pt_fetch_pipeline_or_emit( draw );

   if (!draw->pt.middle.opt[PT_SHADE | PT_CLIPTEST | PT_PIPELINE])
      return FALSE;

   return TRUE;
}


void draw_pt_destroy( struct draw_context *draw )
{
   int i;

   for (i = 0; i < PT_MAX_MIDDLE; i++)
      if (draw->pt.middle.opt[i]) {
	 draw->pt.middle.opt[i]->destroy( draw->pt.middle.opt[i] );
	 draw->pt.middle.opt[i] = NULL;
      }

   if (draw->pt.front.vcache) {
      draw->pt.front.vcache->destroy( draw->pt.front.vcache );
      draw->pt.front.vcache = NULL;
   }
}



static unsigned reduced_prim[PIPE_PRIM_POLYGON + 1] = {
   PIPE_PRIM_POINTS,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES
};


/**
 * Draw vertex arrays
 * This is the main entrypoint into the drawing module.
 * \param prim  one of PIPE_PRIM_x
 * \param start  index of first vertex to draw
 * \param count  number of vertices to draw
 */
void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count)
{
   if (reduced_prim[prim] != draw->reduced_prim) {
      draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
      draw->reduced_prim = reduced_prim[prim];
   }

   /* drawing done here: */
   draw_pt_arrays(draw, prim, start, count);
}


/* Revamp me please:
 */
void draw_do_flush( struct draw_context *draw, unsigned flags )
{
   if (!draw->flushing) 
   {
      draw->flushing = TRUE;

      if (flags >= DRAW_FLUSH_STATE_CHANGE) {
	 draw->pipeline.first->flush( draw->pipeline.first, flags );
	 draw->pipeline.first = draw->pipeline.validate;
	 draw->reduced_prim = ~0;
      }
      
      draw->flushing = FALSE;
   }
}
