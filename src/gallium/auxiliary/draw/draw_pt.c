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


/* XXX: Shouldn't those two functions below use the '>' operator???
 */
static boolean too_many_elts( struct draw_context *draw,
                              unsigned elts )
{
   return elts > (8 * 1024);
}

static INLINE unsigned reduced_prim(unsigned prim)
{
   /*FIXME*/
   return prim;
}

static INLINE boolean good_prim(unsigned prim)
{
   /*FIXME*/
   return FALSE;
}

boolean
draw_pt_arrays(struct draw_context *draw, 
               unsigned prim,
               unsigned start, 
               unsigned count)
{
   const boolean pipeline = draw_need_pipeline(draw, prim);
   const boolean cliptest = !draw->rasterizer->bypass_clipping;
   const boolean shading  = !draw->rasterizer->bypass_vs;
   struct draw_pt_front_end *frontend = NULL;
   struct draw_pt_middle_end *middle = NULL;

   if (!draw->render)
      return FALSE;
   /*debug_printf("XXXXXXXXXX needs_pipeline = %d\n", pipeline);*/

   /* Overall we do:
    *     - frontend -- prepare fetch_elts, draw_elts - eg vcache
    *     - middle   -- fetch, shade, cliptest, viewport
    *     - pipeline -- the prim pipeline: clipping, wide lines, etc 
    *     - backend  -- the vbuf_render provided by the driver.
    */

   if (shading && !draw->use_pt_shaders)
      return FALSE;


   if (!cliptest && !pipeline && !shading) {
      /* This is the 'passthrough' path:
       */
      /* Fetch user verts, emit hw verts:
       */
      middle = draw->pt.middle.fetch_emit;
   }
   else if (!cliptest && !shading) {
      /* This is the 'passthrough' path targetting the pipeline backend.
       */
      /* Fetch user verts, emit pipeline verts, run pipeline:
       */
      middle = draw->pt.middle.fetch_pipeline;
   }
   else if (!cliptest && !pipeline) {
      /* Fetch user verts, run vertex shader, emit hw verts:
       */
      middle = draw->pt.middle.fetch_shade_emit;
   }
   else if (!pipeline) {
      /* Even though !pipeline, we have to run it to get clipping.  We
       * do know that the pipeline is just the clipping operation, but
       * that probably doesn't help much.
       *
       * This is going to be the most important path for a lot of
       * swtnl cards.
       */
      /* Fetch user verts, 
       *    run vertex shader, 
       *    cliptest and viewport trasform
       *    if no clipped vertices,
       *        emit hw verts
       *    else
       *        run pipline
       */
      middle = draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit;
   }
   else {
      /* This is what we're currently always doing:
       */
      /* Fetch user verts, run vertex shader, cliptest, run pipeline
       * or
       * Fetch user verts, run vertex shader, run pipeline
       */
      middle = draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit;
   }


   /* If !pipeline, need to make sure we respect the driver's limited
    * capabilites to receive blocks of vertex data and elements.
    */
#if 0
   if (!pipeline) {
      unsigned vertex_mode = passthrough;
      unsigned nr_verts = count_vertices( draw, start, count );
      unsigned hw_prim = prim;

      if (is_elts(draw)) {
         frontend = draw->pt.front.vcache;
         hw_prim = reduced_prim(prim);
      }
#if 0
      if (too_many_verts(nr_verts)) {
         /* if (is_verts(draw) && can_split(prim)) {
            draw = draw_arrays_split;
         }
         else */ {
            frontend = draw->pt.front.vcache;
            hw_prim = reduced_prim(prim);
         }
      }
#endif

      if (too_many_elts(count)) {

         /* if (is_elts(draw) && can_split(prim)) {
            draw = draw_elts_split;
         }
         else */ {
            frontend = draw->pt.front.vcache;
            hw_prim = reduced_prim(prim);
         }
      }

      if (!good_prim(hw_prim)) {
         frontend = draw->pt.front.vcache;
      }
   }
#else
   frontend = draw->pt.front.vcache;
#endif   

   frontend->prepare( frontend, prim, middle );

   frontend->run( frontend,
                  draw_pt_elt_func( draw ),
                  draw_pt_elt_ptr( draw, start ),
                  count );

   frontend->finish( frontend );

   return TRUE;
}


boolean draw_pt_init( struct draw_context *draw )
{
   draw->pt.middle.fetch_emit = draw_pt_fetch_emit( draw );
   if (!draw->pt.middle.fetch_emit)
      return FALSE;

   draw->pt.middle.fetch_pipeline = draw_pt_fetch_pipeline( draw );
   if (!draw->pt.middle.fetch_pipeline)
      return FALSE;

   draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit =
      draw_pt_fetch_pipeline_or_emit( draw );
   if (!draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit)
      return FALSE;

   draw->pt.front.vcache = draw_pt_vcache( draw );
   if (!draw->pt.front.vcache)
      return FALSE;

   return TRUE;
}


void draw_pt_destroy( struct draw_context *draw )
{
   if (draw->pt.middle.fetch_emit) {
      draw->pt.middle.fetch_emit->destroy( draw->pt.middle.fetch_emit );
      draw->pt.middle.fetch_emit = NULL;
   }

   if (draw->pt.middle.fetch_pipeline) {
      draw->pt.middle.fetch_pipeline->destroy( draw->pt.middle.fetch_pipeline );
      draw->pt.middle.fetch_pipeline = NULL;
   }

   if (draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit) {
      draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit->destroy(
         draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit );
      draw->pt.middle.fetch_shade_cliptest_pipeline_or_emit = NULL;
   }

   if (draw->pt.front.vcache) {
      draw->pt.front.vcache->destroy( draw->pt.front.vcache );
      draw->pt.front.vcache = NULL;
   }
}
