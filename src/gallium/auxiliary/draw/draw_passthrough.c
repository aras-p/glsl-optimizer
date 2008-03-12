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


/* This code is a prototype of what a passhthrough vertex shader might
 * look like.
 *
 * Probably the best approach for us is to do:
 *    - vertex fetch
 *    - vertex shader
 *    - cliptest / viewport transform
 *
 * in one step, then examine the clipOrMask & choose between two paths:
 *
 * Either:
 *    - build primitive headers
 *    - clip and the primitive path
 *    - build clipped vertex buffers,
 *    - vertex-emit to vbuf buffers
 *
 * Or, if no clipping:
 *    - vertex-emit directly to vbuf buffers
 *
 * But when bypass clipping is enabled, we just take the latter
 * choice.  If (some new) passthrough-vertex-shader flag is also set,
 * the pipeline degenerates to:
 *
 *    - vertex fetch
 *    - vertex emit to vbuf buffers
 *
 * Which is what is prototyped here.
 */
#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"



/* Example of a fetch/emit passthrough shader which could be
 * generated when bypass_clipping is enabled on a passthrough vertex
 * shader.
 */
static void fetch_xyz_rgb_st( struct draw_context *draw,
			      float *out,
			      unsigned start,
			      unsigned count )
{
   const unsigned *pitch   = draw->vertex_fetch.pitch;
   const ubyte **src       = draw->vertex_fetch.src_ptr;
   unsigned i;

   const ubyte *xyzw = src[0] + start * pitch[0];
   const ubyte *rgba = src[1] + start * pitch[1];
   const ubyte *st = src[2] + start * pitch[2];

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (i = 0; i < count; i++) {
      {
	 const float *in = (const float *)xyzw; xyzw += pitch[0];
         /* decode input, encode output.  Assume both are float[4] */
	 out[0] = in[0];
	 out[1] = in[1];
	 out[2] = in[2];
	 out[3] = in[3];
      }

      {
	 const float *in = (const float *)rgba; rgba += pitch[1];
         /* decode input, encode output.  Assume both are float[4] */
	 out[4] = in[0];
	 out[5] = in[1];
	 out[6] = in[2];
 	 out[7] = in[3];
      }

      {
	 const float *in = (const float *)st; st += pitch[2];
         /* decode input, encode output.  Assume both are float[2] */
	 out[8] = in[0];
	 out[9] = in[1];
      }

      out += 10;
   }
}

			       
static boolean update_shader( struct draw_context *draw )
{
   const struct vertex_info *vinfo = draw->render->get_vertex_info(draw->render);

   unsigned nr_attrs = vinfo->num_attribs;
   unsigned i;

   for (i = 0; i < nr_attrs; i++) {
      unsigned buf = draw->vertex_element[i].vertex_buffer_index;

      draw->vertex_fetch.src_ptr[i] = (const ubyte *) draw->user.vbuffer[buf] + 
						       draw->vertex_buffer[buf].buffer_offset + 
						       draw->vertex_element[i].src_offset;

      draw->vertex_fetch.pitch[i] = draw->vertex_buffer[buf].pitch;
      draw->vertex_fetch.fetch[i] = NULL;
   }

   draw->vertex_fetch.nr_attrs = nr_attrs;
   draw->vertex_fetch.fetch_func = NULL;
   draw->vertex_fetch.pt_fetch = NULL;

   draw->pt.hw_vertex_size = vinfo->size * 4;

   /* Just trying to figure out how this would work:
    */
   if (nr_attrs == 3 &&
       0 /* some other tests */)
   {
      draw->vertex_fetch.pt_fetch = fetch_xyz_rgb_st;
      assert(vinfo->size == 10);
      return TRUE;
   }
   
   return FALSE;
}



static boolean set_prim( struct draw_context *draw,
		      unsigned prim )
{
   assert(!draw->user.elts);   

   draw->pt.prim = prim;

   switch (prim) { 
   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_QUAD_STRIP:
      return FALSE;
   default:
      draw->render->set_primitive( draw->render, prim );
      return TRUE;
   }
}



boolean
draw_passthrough_arrays(struct draw_context *draw, 
                        unsigned prim,
                        unsigned start, 
                        unsigned count)
{
   float *hw_verts;

   if (!set_prim(draw, prim))
      return FALSE;

   if (!update_shader( draw ))
      return FALSE;

   hw_verts = draw->render->allocate_vertices( draw->render,
                                               draw->pt.hw_vertex_size,
                                               count );

   if (!hw_verts)
      return FALSE;
					
   /* Single routine to fetch vertices, run shader and emit HW verts.
    * Clipping and viewport transformation are done on hardware.
    */
   draw->vertex_fetch.pt_fetch( draw, 
				hw_verts,
				start, count );

   /* Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw_arrays( draw->render,
                              start,
                              count );
   

   draw->render->release_vertices( draw->render, 
				   hw_verts, 
				   draw->pt.hw_vertex_size, 
				   count );

   return TRUE;
}

