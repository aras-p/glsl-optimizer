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

#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"

static unsigned trim( unsigned count, unsigned first, unsigned incr )
{
   if (count < first)
      return 0;
   return count - (count - first) % incr; 
}



/* Overall we split things into:
 *     - frontend -- prepare fetch_elts, draw_elts - eg vcache
 *     - middle   -- fetch, shade, cliptest, viewport
 *     - pipeline -- the prim pipeline: clipping, wide lines, etc 
 *     - backend  -- the vbuf_render provided by the driver.
 */
static boolean
draw_pt_arrays(struct draw_context *draw, 
               unsigned prim,
               unsigned start, 
               unsigned count)
{
   struct draw_pt_front_end *frontend = NULL;
   struct draw_pt_middle_end *middle = NULL;
   unsigned opt = 0;

   /* Sanitize primitive length:
    */
   {
      unsigned first, incr;
      draw_pt_split_prim(prim, &first, &incr);
      count = trim(count, first, incr); 
      if (count < first)
         return TRUE;
   }


   if (!draw->render) {
      opt |= PT_PIPELINE;
   }

   if (draw_need_pipeline(draw,
                          draw->rasterizer,
                          prim)) {
      opt |= PT_PIPELINE;
   }

   if (!draw->bypass_clipping && !draw->pt.test_fse) {
      opt |= PT_CLIPTEST;
   }

   if (!draw->rasterizer->bypass_vs) {
      opt |= PT_SHADE;
   }


   if (opt == 0) 
      middle = draw->pt.middle.fetch_emit;
   else if (opt == PT_SHADE && !draw->pt.no_fse)
      middle = draw->pt.middle.fetch_shade_emit;
   else
      middle = draw->pt.middle.general;


   /* Pick the right frontend
    */
   if (draw->pt.user.elts || (opt & PT_PIPELINE)) {
      frontend = draw->pt.front.vcache;
   } else {
      frontend = draw->pt.front.varray;
   }

   frontend->prepare( frontend, prim, middle, opt );

   frontend->run(frontend, 
                 draw_pt_elt_func(draw),
                 draw_pt_elt_ptr(draw, start),
                 count);

   frontend->finish( frontend );

   return TRUE;
}


boolean draw_pt_init( struct draw_context *draw )
{
   draw->pt.test_fse = debug_get_bool_option("DRAW_FSE", FALSE);
   draw->pt.no_fse = debug_get_bool_option("DRAW_NO_FSE", FALSE);

   draw->pt.front.vcache = draw_pt_vcache( draw );
   if (!draw->pt.front.vcache)
      return FALSE;

   draw->pt.front.varray = draw_pt_varray(draw);
   if (!draw->pt.front.varray)
      return FALSE;

   draw->pt.middle.fetch_emit = draw_pt_fetch_emit( draw );
   if (!draw->pt.middle.fetch_emit)
      return FALSE;

   draw->pt.middle.fetch_shade_emit = draw_pt_middle_fse( draw );
   if (!draw->pt.middle.fetch_shade_emit)
      return FALSE;

   draw->pt.middle.general = draw_pt_fetch_pipeline_or_emit( draw );
   if (!draw->pt.middle.general)
      return FALSE;

   return TRUE;
}


void draw_pt_destroy( struct draw_context *draw )
{
   if (draw->pt.middle.general) {
      draw->pt.middle.general->destroy( draw->pt.middle.general );
      draw->pt.middle.general = NULL;
   }

   if (draw->pt.middle.fetch_emit) {
      draw->pt.middle.fetch_emit->destroy( draw->pt.middle.fetch_emit );
      draw->pt.middle.fetch_emit = NULL;
   }

   if (draw->pt.middle.fetch_shade_emit) {
      draw->pt.middle.fetch_shade_emit->destroy( draw->pt.middle.fetch_shade_emit );
      draw->pt.middle.fetch_shade_emit = NULL;
   }

   if (draw->pt.front.vcache) {
      draw->pt.front.vcache->destroy( draw->pt.front.vcache );
      draw->pt.front.vcache = NULL;
   }

   if (draw->pt.front.varray) {
      draw->pt.front.varray->destroy( draw->pt.front.varray );
      draw->pt.front.varray = NULL;
   }
}




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
   unsigned reduced_prim = draw_pt_reduced_prim(prim);
   if (reduced_prim != draw->reduced_prim) {
      draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
      draw->reduced_prim = reduced_prim;
   }

   /* drawing done here: */
   draw_pt_arrays(draw, prim, start, count);
}

boolean draw_pt_get_edgeflag( struct draw_context *draw,
                              unsigned idx )
{
   if (draw->pt.user.edgeflag)
      return (draw->pt.user.edgeflag[idx/32] & (1 << (idx%32))) != 0;
   else
      return 1;
}
