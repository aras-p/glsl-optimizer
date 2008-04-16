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

   if (!draw->render)
      return FALSE;
   /*debug_printf("XXXXXXXXXX needs_pipeline = %d\n", pipeline);*/


   if (draw_need_pipeline(draw, prim)) {
      opt |= PT_PIPELINE;
   }

   if (!draw->rasterizer->bypass_clipping) {
      opt |= PT_CLIPTEST;
   }

   if (!draw->rasterizer->bypass_vs) {
      opt |= PT_SHADE;

      if (!draw->use_pt_shaders)
	 return FALSE;
   }


   if (draw->pt.middle.opt[opt] == NULL) {
      opt = PT_PIPELINE | PT_CLIPTEST | PT_SHADE;
   }

   middle = draw->pt.middle.opt[opt];
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
   draw->pt.middle.opt[PT_PIPELINE] = draw_pt_fetch_pipeline( draw );
//   draw->pt.middle.opt[PT_SHADE] = draw_pt_shade_emit( draw );
//   draw->pt.middle.opt[PT_SHADE | PT_PIPELINE] = draw_pt_shade_pipeline( draw );
//   draw->pt.middle.opt[PT_SHADE | PT_CLIPTEST] = draw_pt_shade_clip_either( draw );
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
