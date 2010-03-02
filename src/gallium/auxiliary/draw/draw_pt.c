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
#include "tgsi/tgsi_dump.h"
#include "util/u_math.h"
#include "util/u_prim.h"

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

   if (!draw->force_passthrough) {
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
 * Debug- print the first 'count' vertices.
 */
static void
draw_print_arrays(struct draw_context *draw, uint prim, int start, uint count)
{
   uint i;

   debug_printf("Draw arrays(prim = %u, start = %u, count = %u)\n",
                prim, start, count);

   for (i = 0; i < count; i++) {
      uint ii = 0;
      uint j;

      if (draw->pt.user.elts) {
         /* indexed arrays */
         switch (draw->pt.user.eltSize) {
         case 1:
            {
               const ubyte *elem = (const ubyte *) draw->pt.user.elts;
               ii = elem[start + i];
            }
            break;
         case 2:
            {
               const ushort *elem = (const ushort *) draw->pt.user.elts;
               ii = elem[start + i];
            }
            break;
         case 4:
            {
               const uint *elem = (const uint *) draw->pt.user.elts;
               ii = elem[start + i];
            }
            break;
         default:
            assert(0);
         }
         debug_printf("Element[%u + %u] -> Vertex %u:\n", start, i, ii);
      }
      else {
         /* non-indexed arrays */
         ii = start + i;
         debug_printf("Vertex %u:\n", ii);
      }

      for (j = 0; j < draw->pt.nr_vertex_elements; j++) {
         uint buf = draw->pt.vertex_element[j].vertex_buffer_index;
         ubyte *ptr = (ubyte *) draw->pt.user.vbuffer[buf];
         ptr += draw->pt.vertex_buffer[buf].stride * ii;
         ptr += draw->pt.vertex_element[j].src_offset;

         debug_printf("  Attr %u: ", j);
         switch (draw->pt.vertex_element[j].src_format) {
         case PIPE_FORMAT_R32_FLOAT:
            {
               float *v = (float *) ptr;
               debug_printf("%f  @ %p\n", v[0], (void *) v);
            }
            break;
         case PIPE_FORMAT_R32G32_FLOAT:
            {
               float *v = (float *) ptr;
               debug_printf("%f %f  @ %p\n", v[0], v[1], (void *) v);
            }
            break;
         case PIPE_FORMAT_R32G32B32_FLOAT:
            {
               float *v = (float *) ptr;
               debug_printf("%f %f %f  @ %p\n", v[0], v[1], v[2], (void *) v);
            }
            break;
         case PIPE_FORMAT_R32G32B32A32_FLOAT:
            {
               float *v = (float *) ptr;
               debug_printf("%f %f %f %f  @ %p\n", v[0], v[1], v[2], v[3],
                            (void *) v);
            }
            break;
         default:
            debug_printf("other format (fix me)\n");
            ;
         }
      }
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
   draw_arrays_instanced(draw, prim, start, count, 0, 1);
}

void
draw_arrays_instanced(struct draw_context *draw,
                      unsigned mode,
                      unsigned start,
                      unsigned count,
                      unsigned startInstance,
                      unsigned instanceCount)
{
   unsigned reduced_prim = u_reduced_prim(mode);
   unsigned instance;

   if (reduced_prim != draw->reduced_prim) {
      draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);
      draw->reduced_prim = reduced_prim;
   }

   if (0)
      draw_print_arrays(draw, mode, start, MIN2(count, 20));

#if 0
   {
      int i;
      debug_printf("draw_arrays(mode=%u start=%u count=%u):\n",
                   mode, start, count);
      tgsi_dump(draw->vs.vertex_shader->state.tokens, 0);
      debug_printf("Elements:\n");
      for (i = 0; i < draw->pt.nr_vertex_elements; i++) {
         debug_printf("  format=%s comps=%u\n",
                      util_format_name(draw->pt.vertex_element[i].src_format),
                      draw->pt.vertex_element[i].nr_components);
      }
      debug_printf("Buffers:\n");
      for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
         debug_printf("  stride=%u offset=%u ptr=%p\n",
                      draw->pt.vertex_buffer[i].stride,
                      draw->pt.vertex_buffer[i].buffer_offset,
                      draw->pt.user.vbuffer[i]);
      }
   }
#endif

   for (instance = 0; instance < instanceCount; instance++) {
      draw->instance_id = instance + startInstance;
      draw_pt_arrays(draw, mode, start, count);
   }
}
