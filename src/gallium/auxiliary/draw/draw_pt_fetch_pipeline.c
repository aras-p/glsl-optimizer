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


struct fetch_pipeline_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct {
      const ubyte *ptr;
      unsigned pitch;
      void (*fetch)( const void *from, float *attrib);
      void (*emit)( const float *attrib, float **out );
   } fetch[PIPE_MAX_ATTRIBS];
   
   unsigned nr_fetch;
   unsigned pipeline_vertex_size;
   unsigned prim;
};


static void fetch_NULL( const void *from,
                        float *attrib )
{
}



static void emit_R32_FLOAT( const float *attrib,
                            float **out )
{
   (*out)[0] = attrib[0];
   (*out) += 1;
}

static void emit_R32G32_FLOAT( const float *attrib,
                               float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out) += 2;
}

static void emit_R32G32B32_FLOAT( const float *attrib,
                                  float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out)[2] = attrib[2];
   (*out) += 3;
}

static void emit_R32G32B32A32_FLOAT( const float *attrib,
                                     float **out )
{
   (*out)[0] = attrib[0];
   (*out)[1] = attrib[1];
   (*out)[2] = attrib[2];
   (*out)[3] = attrib[3];
   (*out) += 4;
}

static void emit_header( const float *attrib,
                         float **out )
{
   (*out)[0] = 0;
   (*out)[1] = 0;
   (*out)[2] = 0;
   (*out)[3] = 0;
   (*out)[3] = 1;
   (*out) += 5;
}

/**
 * General-purpose fetch from user's vertex arrays, emit to driver's
 * vertex buffer.
 *
 * XXX this is totally temporary.
 */
static void
fetch_store_general( struct fetch_pipeline_middle_end *fpme,
                     void *out_ptr,
                     const unsigned *fetch_elts,
                     unsigned count )
{
   float *out = (float *)out_ptr;
   uint i, j;

   for (i = 0; i < count; i++) {
      unsigned elt = fetch_elts[i];
      
      for (j = 0; j < fpme->nr_fetch; j++) {
         float attrib[4];
         const ubyte *from = (fpme->fetch[j].ptr +
                              fpme->fetch[j].pitch * elt);
         
         fpme->fetch[j].fetch( from, attrib );
         fpme->fetch[j].emit( attrib, &out );
      }
   }
}


/* We aren't running a vertex shader, but are running the pipeline.
 * That means the vertices we need to build look like:
 *
 * dw0: vertex header (zero?)
 * dw1: clip coord 0
 * dw2: clip coord 1
 * dw3: clip coord 2
 * dw4: clip coord 4
 * dw5: screen coord 0
 * dw6: screen coord 0
 * dw7: screen coord 0
 * dw8: screen coord 0
 * dw9: other attribs...
 *
 */
static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim )
{
   static const float zero = 0;
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   unsigned i, nr = 0;

   fpme->prim = prim;

   /* Emit the vertex header and empty clipspace coord field:
    */
   {
      fpme->fetch[nr].ptr = NULL;
      fpme->fetch[nr].pitch = 0;
      fpme->fetch[nr].fetch = fetch_NULL;
      fpme->fetch[nr].emit = emit_header;
      nr++;
   }
   

   /* Need to look at vertex shader inputs (we know it is a
    * passthrough shader, so these define the outputs too).  If we
    * were running a shader, we'd still be looking at the inputs at
    * this point.
    */
   for (i = 0; i < draw->vertex_shader->info.num_inputs; i++) {
      unsigned buf = draw->vertex_element[i].vertex_buffer_index;
      enum pipe_format format  = draw->vertex_element[i].src_format;

      fpme->fetch[nr].ptr = ((const ubyte *) draw->user.vbuffer[buf] + 
                            draw->vertex_buffer[buf].buffer_offset + 
                            draw->vertex_element[i].src_offset);

      fpme->fetch[nr].pitch = draw->vertex_buffer[buf].pitch;
      fpme->fetch[nr].fetch = draw_get_fetch_func( format );

      /* Always do this -- somewhat redundant...
       */
      fpme->fetch[nr].emit = emit_R32G32B32A32_FLOAT;
      nr++;
   }

   fpme->nr_fetch = nr;
   fpme->pipeline_vertex_size = (5 + (nr-1) * 4) * sizeof(float);
}



/**
 * Add a point to the primitive queue.
 * \param i0  index into user's vertex arrays
 */
static void do_point( struct draw_context *draw,
		      const char *v0 )
{
   struct prim_header prim;
   
   prim.reset_line_stipple = 0;
   prim.edgeflags = 1;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;

   draw->pipeline.first->point( draw->pipeline.first, &prim );
}


/**
 * Add a line to the primitive queue.
 * \param i0  index into user's vertex arrays
 * \param i1  index into user's vertex arrays
 */
static void do_line( struct draw_context *draw,
		     const char *v0,
		     const char *v1 )
{
   struct prim_header prim;
   
   prim.reset_line_stipple = 1; /* fixme */
   prim.edgeflags = 1;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;

   draw->pipeline.first->line( draw->pipeline.first, &prim );
}

/**
 * Add a triangle to the primitive queue.
 */
static void do_triangle( struct draw_context *draw,
			 char *v0,
			 char *v1,
			 char *v2 )
{
   struct prim_header prim;
   
//   _mesa_printf("tri %d %d %d\n", i0, i1, i2);
   prim.reset_line_stipple = 1;
   prim.edgeflags = ~0;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;
   prim.v[2] = (struct vertex_header *)v2;

   draw->pipeline.first->tri( draw->pipeline.first, &prim );
}


static void run_pipeline( struct fetch_pipeline_middle_end *fpme,
                          char *verts,
                          const ushort *elts,
                          unsigned count )
{
   struct draw_context *draw = fpme->draw;
   unsigned stride = fpme->pipeline_vertex_size;
   unsigned i;
   
   switch (fpme->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) 
         do_point( draw, 
                   verts + stride * elts[i] );
      break;
   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) 
         do_line( draw, 
                  verts + stride * elts[i+0],
                  verts + stride * elts[i+1]);
      break;
   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3)
         do_triangle( draw, 
                      verts + stride * elts[i+0],
                      verts + stride * elts[i+1],
                      verts + stride * elts[i+2]);
      break;
   }
}




static void fetch_pipeline_run( struct draw_pt_middle_end *middle,
                            const unsigned *fetch_elts,
                            unsigned fetch_count,
                            const ushort *draw_elts,
                            unsigned draw_count )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   char *pipeline_verts;
   
   pipeline_verts = MALLOC( fpme->pipeline_vertex_size * 
                            fetch_count );
   if (!pipeline_verts) {
      assert(0);
      return;
   }
         
					
   /* Single routine to fetch vertices and emit pipeline verts.
    */
   fetch_store_general( fpme, 
                        pipeline_verts,
                        fetch_elts,
                        fetch_count );

   
   run_pipeline( fpme,
                 pipeline_verts,
                 draw_elts,
                 draw_count );
                 

   /* Done -- that was easy, wasn't it: 
    */
   FREE( pipeline_verts );
}



static void fetch_pipeline_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}

static void fetch_pipeline_destroy( struct draw_pt_middle_end *middle )
{
   FREE(middle);
}


struct draw_pt_middle_end *draw_pt_fetch_pipeline( struct draw_context *draw )
{
   struct fetch_pipeline_middle_end *fetch_pipeline = CALLOC_STRUCT( fetch_pipeline_middle_end );
 
   fetch_pipeline->base.prepare = fetch_pipeline_prepare;
   fetch_pipeline->base.run     = fetch_pipeline_run;
   fetch_pipeline->base.finish  = fetch_pipeline_finish;
   fetch_pipeline->base.destroy = fetch_pipeline_destroy;

   fetch_pipeline->draw = draw;
     
   return &fetch_pipeline->base;
}

