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


/**
 * General-purpose fetch from user's vertex arrays, emit to driver's
 * vertex buffer.
 *
 * XXX this is totally temporary.
 */
static void
fetch_store_general( struct draw_context *draw,
                     float *out,
                     unsigned start,
                     unsigned count )
{
   const struct vertex_info *vinfo = draw->render->get_vertex_info(draw->render);
   const unsigned nr_attrs = vinfo->num_attribs;
   uint i, j;

   const unsigned *pitch   = draw->vertex_fetch.pitch;
   const ubyte **src       = draw->vertex_fetch.src_ptr;

   for (i = start; i < start + count; i++) {
      for (j = 0; j < nr_attrs; j++) {
         /* vinfo->src_index is the output of the vertex shader
          * matching this hw-vertex component. 
          *
          * In passthrough, we require a 1:1 mapping between vertex
          * shader outputs and inputs, which in turn correspond to
          * vertex elements in the state.  So, this is the vertex
          * element we're interested in...
          */
         const uint jj = vinfo->src_index[j];
         const enum pipe_format srcFormat  = draw->vertex_element[jj].src_format;
         const ubyte *from = src[jj] + i * pitch[jj];
         float attrib[4];

         /* Except...  When we're not.  Two cases EMIT_HEADER &
          * EMIT_1F_PSIZE don't consume an input.  Should have some
          * method for indicating this, or change the logic here
          * somewhat so it doesn't matter.
          *
          * Just hack this up now, do something better about it later.
          */
         if (vinfo->emit[j] == EMIT_HEADER) {
            memset(out, 0, sizeof(struct vertex_header));
            out += sizeof(struct vertex_header) / 4;
            continue;
         }
         else if (vinfo->emit[j] == EMIT_1F_PSIZE) {
            out[0] = 1.0;       /* xxx */
            out += 1;
            continue;
         }
         

         /* The normal fetch/emit code:
          */
         switch (srcFormat) {
         case PIPE_FORMAT_B8G8R8A8_UNORM:
            {
               ubyte *ub = (ubyte *) from;
               attrib[0] = UBYTE_TO_FLOAT(ub[0]);
               attrib[1] = UBYTE_TO_FLOAT(ub[1]);
               attrib[2] = UBYTE_TO_FLOAT(ub[2]);
               attrib[3] = UBYTE_TO_FLOAT(ub[3]);
            }
            break;
         case PIPE_FORMAT_R32G32B32A32_FLOAT:
            {
               float *f = (float *) from;
               attrib[0] = f[0];
               attrib[1] = f[1];
               attrib[2] = f[2];
               attrib[3] = f[3];
            }
            break;
         case PIPE_FORMAT_R32G32B32_FLOAT:
            {
               float *f = (float *) from;
               attrib[0] = f[0];
               attrib[1] = f[1];
               attrib[2] = f[2];
               attrib[3] = 1.0;
            }
            break;
         case PIPE_FORMAT_R32G32_FLOAT:
            {
               float *f = (float *) from;
               attrib[0] = f[0];
               attrib[1] = f[1];
               attrib[2] = 0.0;
               attrib[3] = 1.0;
            }
            break;
         case PIPE_FORMAT_R32_FLOAT:
            {
               float *f = (float *) from;
               attrib[0] = f[0];
               attrib[1] = 0.0;
               attrib[2] = 0.0;
               attrib[3] = 1.0;
            }
            break;
         default:
            assert(0);
         }

         debug_printf("attrib %d: %f %f %f %f\n", j, 
                      attrib[0], attrib[1], attrib[2], attrib[3]);

         switch (vinfo->emit[j]) {
         case EMIT_1F:
            out[0] = attrib[0];
            out += 1;
            break;
         case EMIT_2F:
            out[0] = attrib[0];
            out[1] = attrib[1];
            out += 2;
            break;
         case EMIT_4F:
            out[0] = attrib[0];
            out[1] = attrib[1];
            out[2] = attrib[2];
            out[3] = attrib[3];
            out += 4;
            break;
         default:
            assert(0);
         }
      }
      debug_printf("\n");
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

   draw->vertex_fetch.pt_fetch = fetch_store_general;
   return TRUE;
}




static boolean split_prim_inplace(unsigned prim, unsigned *first, unsigned *incr)
{
   switch (prim) {
   case PIPE_PRIM_POINTS:
      *first = 1;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_LINES:
      *first = 2;
      *incr = 2;
      return TRUE;
   case PIPE_PRIM_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_TRIANGLES:
      *first = 3;
      *incr = 3;
      return TRUE;
   case PIPE_PRIM_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_QUADS:
      *first = 4;
      *incr = 4;
      return TRUE;
   case PIPE_PRIM_QUAD_STRIP:
      *first = 4;
      *incr = 2;
      return TRUE;
   default:
      *first = 0;
      *incr = 1;		/* set to one so that count % incr works */
      return FALSE;
   }
}



static boolean set_prim( struct draw_context *draw,
                         unsigned prim,
                         unsigned count )
{
   assert(!draw->user.elts);   

   switch (prim) { 
   case PIPE_PRIM_LINE_LOOP:
      if (count > 1024)
         return FALSE;
      return draw->render->set_primitive( draw->render, PIPE_PRIM_LINE_STRIP );

   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      if (count > 1024)
         return FALSE;
      return draw->render->set_primitive( draw->render, prim );

   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_QUAD_STRIP:
      return draw->render->set_primitive( draw->render, PIPE_PRIM_TRIANGLES );

   default:
      return draw->render->set_primitive( draw->render, prim );
      break;
   }

   return TRUE;
}



#define INDEX(i) (start + (i))
static void pt_draw_arrays( struct draw_context *draw,
			    unsigned start,
			    unsigned length )
{
   ushort *tmp = NULL;
   unsigned i, j;

   switch (draw->pt.prim) {
   case PIPE_PRIM_LINE_LOOP:
      tmp = MALLOC( sizeof(ushort) * (length + 1) );

      for (i = 0; i < length; i++)
	 tmp[i] = INDEX(i);
      tmp[length] = 0;

      draw->render->draw( draw->render,
			  tmp,
			  length+1 );
      break;


   case PIPE_PRIM_QUAD_STRIP:
      tmp = MALLOC( sizeof(ushort) * (length / 2 * 6) );

      for (j = i = 0; i + 3 < length; i += 2, j += 6) {
	 tmp[j+0] = INDEX(i+0);
	 tmp[j+1] = INDEX(i+1);
	 tmp[j+2] = INDEX(i+3);

	 tmp[j+3] = INDEX(i+2);
	 tmp[j+4] = INDEX(i+0);
	 tmp[j+5] = INDEX(i+3);
      }

      if (j)
	 draw->render->draw( draw->render, tmp, j );
      break;

   case PIPE_PRIM_QUADS:
      tmp = MALLOC( sizeof(int) * (length / 4 * 6) );

      for (j = i = 0; i + 3 < length; i += 4, j += 6) {
	 tmp[j+0] = INDEX(i+0);
	 tmp[j+1] = INDEX(i+1);
	 tmp[j+2] = INDEX(i+3);

	 tmp[j+3] = INDEX(i+1);
	 tmp[j+4] = INDEX(i+2);
	 tmp[j+5] = INDEX(i+3);
      }

      if (j)
	 draw->render->draw( draw->render, tmp, j );
      break;

   default:
      draw->render->draw_arrays( draw->render,
                                 start,
                                 length );
      break;
   }

   if (tmp)
      FREE(tmp);
}



static boolean do_draw( struct draw_context *draw, 
                        unsigned start, unsigned count )
{
   float *hw_verts = 
      draw->render->allocate_vertices( draw->render,
                                       (ushort)draw->pt.hw_vertex_size,
                                       (ushort)count );

   if (!hw_verts)
      return FALSE;
					
   /* Single routine to fetch vertices and emit HW verts.
    */
   draw->vertex_fetch.pt_fetch( draw, 
				hw_verts,
				start, count );

   /* Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   pt_draw_arrays( draw,
                   0,
                   count );
   

   draw->render->release_vertices( draw->render, 
				   hw_verts, 
				   draw->pt.hw_vertex_size, 
				   count );

   return TRUE;
}


boolean
draw_passthrough_arrays(struct draw_context *draw, 
                        unsigned prim,
                        unsigned start, 
                        unsigned count)
{
   unsigned i = 0;
   unsigned first, incr;
   
   //debug_printf("%s prim %d start %d count %d\n", __FUNCTION__, prim, start, count);
   
   split_prim_inplace(prim, &first, &incr);

   count -= (count - first) % incr; 

   debug_printf("%s %d %d %d\n", __FUNCTION__, prim, start, count);

   if (draw_need_pipeline(draw))
      return FALSE;

   debug_printf("%s AAA\n", __FUNCTION__);

   if (!set_prim(draw, prim, count))
      return FALSE;

   /* XXX: need a single value that reflects the most recent call to
    * driver->set_primitive:
    */
   draw->pt.prim = prim;

   debug_printf("%s BBB\n", __FUNCTION__);

   if (!update_shader(draw))
      return FALSE;

   debug_printf("%s CCC\n", __FUNCTION__);

   /* Chop this up into bite-sized pieces that a driver should be able
    * to devour -- problem is we don't have a quick way to query the
    * driver on the maximum size for this chunk in the current state.
    */
   while (i + first <= count) {
      int nr = MIN2( count - i, 1024 );

      /* snap to prim boundary 
       */
      nr -= (nr - first) % incr; 

      if (!do_draw( draw, start + i, nr )) {
         assert(0);
         return FALSE;
      }

      /* increment allowing for repeated vertices
       */
      i += nr - (first - incr);
   }


   debug_printf("%s DONE\n", __FUNCTION__);
   return TRUE;
}


