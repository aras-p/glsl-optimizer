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
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"

/* The simplest 'middle end' in the new vertex code.  
 * 
 * The responsibilities of a middle end are to:
 *  - perform vertex fetch using
 *       - draw vertex element/buffer state
 *       - a list of fetch indices we received as an input
 *  - run the vertex shader
 *  - cliptest, 
 *  - clip coord calculation 
 *  - viewport transformation
 *  - if necessary, run the primitive pipeline, passing it:
 *       - a linear array of vertex_header vertices constructed here
 *       - a set of draw indices we received as an input
 *  - otherwise, drive the hw backend,
 *       - allocate space for hardware format vertices
 *       - translate the vertex-shader output vertices to hw format
 *       - calling the backend draw functions.
 *
 * For convenience, we provide a helper function to drive the hardware
 * backend given similar inputs to those required to run the pipeline.
 *
 * In the case of passthrough mode, many of these actions are disabled
 * or noops, so we end up doing:
 *
 *  - perform vertex fetch
 *  - drive the hw backend
 *
 * IE, basically just vertex fetch to post-vs-format vertices,
 * followed by a call to the backend helper function.
 */


struct fetch_emit_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct {
      const ubyte *ptr;
      unsigned pitch;
      void (*fetch)( const void *from, float *attrib);
      void (*emit)( const float *attrib, float **out );
   } fetch[PIPE_ATTRIB_MAX];
   
   unsigned nr_fetch;
   unsigned hw_vertex_size;
};


static void fetch_B8G8R8A8_UNORM( const void *from,
                                  float *attrib )
{
   ubyte *ub = (ubyte *) from;
   attrib[2] = UBYTE_TO_FLOAT(ub[0]);
   attrib[1] = UBYTE_TO_FLOAT(ub[1]);
   attrib[0] = UBYTE_TO_FLOAT(ub[2]);
   attrib[3] = UBYTE_TO_FLOAT(ub[3]);
}

static void fetch_R32G32B32A32_FLOAT( const void *from,
                                  float *attrib )
{
   float *f = (float *) from;
   attrib[0] = f[0];
   attrib[1] = f[1];
   attrib[2] = f[2];
   attrib[3] = f[3];
}

static void fetch_R32G32B32_FLOAT( const void *from,
                                   float *attrib )
{
   float *f = (float *) from;
   attrib[0] = f[0];
   attrib[1] = f[1];
   attrib[2] = f[2];
   attrib[3] = 1.0;
}

static void fetch_R32G32_FLOAT( const void *from,
                                float *attrib )
{
   float *f = (float *) from;
   attrib[0] = f[0];
   attrib[1] = f[1];
   attrib[2] = 0.0;
   attrib[3] = 1.0;
}

static void fetch_R32_FLOAT( const void *from,
                             float *attrib )
{
   float *f = (float *) from;
   attrib[0] = f[0];
   attrib[1] = 0.0;
   attrib[2] = 0.0;
   attrib[3] = 1.0;
}

static void fetch_HEADER( const void *from,
                          float *attrib )
{
   attrib[0] = 0;
}


static void emit_1F( const float *attrib,
                     float **out )
{
   (*out)[0] = attrib[0];
   (*out) += 1;
}

static void emit_2F( const float *attrib,
                     float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out) += 2;
}

static void emit_3F( const float *attrib,
                     float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out)[2] = attrib[2];
   (*out) += 3;
}

static void emit_4F( const float *attrib,
                     float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out)[2] = attrib[2];
   (*out)[3] = attrib[3];
   (*out) += 4;
}


/**
 * General-purpose fetch from user's vertex arrays, emit to driver's
 * vertex buffer.
 *
 * XXX this is totally temporary.
 */
static void
fetch_store_general( struct fetch_emit_middle_end *feme,
                     void *out_ptr,
                     const unsigned *fetch_elts,
                     unsigned count )
{
   float *out = (float *)out_ptr;
   struct vbuf_render *render = feme->draw->render;
   uint i, j;

   for (i = 0; i < count; i++) {
      unsigned elt = fetch_elts[i];
      
      for (j = 0; j < feme->nr_fetch; j++) {
         float attrib[4];
         const ubyte *from = (feme->fetch[j].ptr +
                              feme->fetch[j].pitch * elt);
         
         feme->fetch[j].fetch( from, attrib );
         feme->fetch[j].emit( attrib, &out );
      }
   }
}



static void fetch_emit_prepare( struct draw_pt_middle_end *middle )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   const struct vertex_info *vinfo = draw->render->get_vertex_info(draw->render);
   unsigned nr_attrs = vinfo->num_attribs;
   unsigned i;

   for (i = 0; i < nr_attrs; i++) {
      unsigned src_element = vinfo->src_index[i];
      unsigned src_buffer = draw->vertex_element[src_element].vertex_buffer_index;
         
      feme->fetch[i].ptr = ((const ubyte *)draw->user.vbuffer[src_buffer] + 
                            draw->vertex_buffer[src_buffer].buffer_offset + 
                            draw->vertex_element[src_element].src_offset);

      feme->fetch[i].pitch = draw->vertex_buffer[src_buffer].pitch;
         
      switch (draw->vertex_element[src_element].src_format) {
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         feme->fetch[i].fetch = fetch_B8G8R8A8_UNORM;
         break;
      case PIPE_FORMAT_R32G32B32A32_FLOAT:
         feme->fetch[i].fetch = fetch_R32G32B32A32_FLOAT;
         break;
      case PIPE_FORMAT_R32G32B32_FLOAT:
         feme->fetch[i].fetch = fetch_R32G32B32_FLOAT;
         break;
      case PIPE_FORMAT_R32G32_FLOAT:
         feme->fetch[i].fetch = fetch_R32G32_FLOAT;
         break;
      case PIPE_FORMAT_R32_FLOAT:
         feme->fetch[i].fetch = fetch_R32_FLOAT;
         break;
      default:
         assert(0);
         feme->fetch[i].fetch = NULL;
         break;
      }
         
      switch (vinfo->emit[i]) {
      case EMIT_4F:
         feme->fetch[i].emit = emit_4F;
         break;
      case EMIT_3F:
         feme->fetch[i].emit = emit_3F;
         break;
      case EMIT_2F:
         feme->fetch[i].emit = emit_2F;
         break;
      case EMIT_1F:
         feme->fetch[i].emit = emit_1F;
         break;
      case EMIT_HEADER:
         feme->fetch[i].ptr = NULL;
         feme->fetch[i].pitch = 0;
         feme->fetch[i].fetch = fetch_HEADER;
         feme->fetch[i].emit = emit_1F;
         break;
      case EMIT_1F_PSIZE:
         feme->fetch[i].ptr = (const ubyte *)&feme->draw->rasterizer->point_size;
         feme->fetch[i].pitch = 0;
         feme->fetch[i].fetch = fetch_R32_FLOAT;
         feme->fetch[i].emit = emit_1F;
      default:
         assert(0);
         feme->fetch[i].emit = NULL;
         break;
      }
   }

   feme->nr_fetch = nr_attrs;
   feme->hw_vertex_size = vinfo->size * 4;
}





static void fetch_emit_run( struct draw_pt_middle_end *middle,
                            unsigned prim,
                            const unsigned *fetch_elts,
                            unsigned fetch_count,
                            const ushort *draw_elts,
                            unsigned draw_count )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   void *hw_verts;
   boolean ok;
   
   ok = draw->render->set_primitive( draw->render, 
                                     prim );
   if (!ok) {
      assert(0);
      return;
   }
   

   hw_verts = draw->render->allocate_vertices( draw->render,
                                               (ushort)feme->hw_vertex_size,
                                               (ushort)fetch_count );
   if (!hw_verts) {
      assert(0);
      return;
   }
         
					
   /* Single routine to fetch vertices and emit HW verts.
    */
   fetch_store_general( feme, 
                        hw_verts,
                        fetch_elts,
                        fetch_count );

   /* XXX: Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw( draw->render, 
                       draw_elts, 
                       draw_count );

   /* Done -- that was easy, wasn't it: 
    */
   draw->render->release_vertices( draw->render, 
                                   hw_verts, 
                                   feme->hw_vertex_size, 
                                   fetch_count );

}



static void fetch_emit_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}

static void fetch_emit_destroy( struct draw_pt_middle_end *middle )
{
   FREE(middle);
}


struct draw_pt_middle_end *draw_pt_fetch_emit( struct draw_context *draw )
{
   struct fetch_emit_middle_end *fetch_emit = CALLOC_STRUCT( fetch_emit_middle_end );
 
   fetch_emit->base.prepare = fetch_emit_prepare;
   fetch_emit->base.run     = fetch_emit_run;
   fetch_emit->base.finish  = fetch_emit_finish;
   fetch_emit->base.destroy = fetch_emit_destroy;

   fetch_emit->draw = draw;
     
   return &fetch_emit->base;
}

