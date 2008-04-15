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

#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"

struct fetch_pipeline_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   const ubyte *input_buf[2];

   struct {
      const ubyte **input_buf;
      unsigned input_offset;
      unsigned output_offset;

      void (*emit)( const float *attrib, void *ptr );
   } translate[PIPE_MAX_ATTRIBS];
   unsigned nr_translate;

   unsigned pipeline_vertex_size;
   unsigned hw_vertex_size;
   unsigned prim;
};


static void emit_NULL( const float *attrib,
		       void *ptr )
{
}

static void emit_R32_FLOAT( const float *attrib,
                            void *ptr )
{
   float *out = (float *)ptr;
   out[0] = attrib[0];
}

static void emit_R32G32_FLOAT( const float *attrib,
                               void *ptr )
{
   float *out = (float *)ptr;
   out[0] = attrib[0];
   out[1] = attrib[1];
}

static void emit_R32G32B32_FLOAT( const float *attrib,
                                  void *ptr )
{
   float *out = (float *)ptr;
   out[0] = attrib[0];
   out[1] = attrib[1];
   out[2] = attrib[2];
}

static void emit_R32G32B32A32_FLOAT( const float *attrib,
                                     void *ptr )
{
   float *out = (float *)ptr;
   out[0] = attrib[0];
   out[1] = attrib[1];
   out[2] = attrib[2];
   out[3] = attrib[3];
}

static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   unsigned i;
   boolean ok;
   const struct vertex_info *vinfo;
   unsigned dst_offset;

   fpme->prim = prim;

   ok = draw->render->set_primitive(draw->render, prim);
   if (!ok) {
      assert(0);
      return;
   }

   /* Must do this after set_primitive() above:
    */
   vinfo = draw->render->get_vertex_info(draw->render);


   /* In passthrough mode, need to translate from vertex shader
    * outputs to hw vertices.
    */
   dst_offset = 0;
   for (i = 0; i < vinfo->num_attribs; i++) {
      unsigned emit_sz = 0;
      unsigned src_buffer = 0;
      unsigned src_offset = (sizeof(struct vertex_header) + 
			     vinfo->src_index[i] * 4 * sizeof(float) );


         
      switch (vinfo->emit[i]) {
      case EMIT_4F:
         fpme->translate[i].emit = emit_R32G32B32A32_FLOAT;
	 emit_sz = 4 * sizeof(float);
         break;
      case EMIT_3F:
         fpme->translate[i].emit = emit_R32G32B32_FLOAT;
	 emit_sz = 3 * sizeof(float);
         break;
      case EMIT_2F:
         fpme->translate[i].emit = emit_R32G32_FLOAT;
	 emit_sz = 2 * sizeof(float);
         break;
      case EMIT_1F:
         fpme->translate[i].emit = emit_R32_FLOAT;
	 emit_sz = 1 * sizeof(float);
         break;
      case EMIT_1F_PSIZE:
         fpme->translate[i].emit = emit_R32_FLOAT;
	 emit_sz = 1 * sizeof(float);
         src_buffer = 1;
	 src_offset = 0;
         break;
      default:
         assert(0);
         fpme->translate[i].emit = emit_NULL;
	 emit_sz = 0;
         break;
      }

      fpme->translate[i].input_buf = &fpme->input_buf[src_buffer];
      fpme->translate[i].input_offset = src_offset;
      fpme->translate[i].output_offset = dst_offset;
      dst_offset += emit_sz;
   }

   fpme->nr_translate = vinfo->num_attribs;
   fpme->hw_vertex_size = vinfo->size * 4;

   //fpme->pipeline_vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);
   fpme->pipeline_vertex_size = MAX_VERTEX_ALLOCATION;
   fpme->hw_vertex_size = vinfo->size * 4;
}




static void fetch_pipeline_run( struct draw_pt_middle_end *middle,
                                const unsigned *fetch_elts,
                                unsigned fetch_count,
                                const ushort *draw_elts,
                                unsigned draw_count )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *shader = draw->vertex_shader;
   char *pipeline_verts;

   pipeline_verts = MALLOC(fpme->pipeline_vertex_size *
			   fetch_count);

   if (!pipeline_verts) {
      assert(0);
      return;
   }


   /* Shade
    */
   shader->prepare(shader, draw);
   if (shader->run(shader, draw, fetch_elts, fetch_count, pipeline_verts,
                   fpme->pipeline_vertex_size)) {
      /* Run the pipeline */
      draw_pt_run_pipeline( fpme->draw,
                            fpme->prim,
                            pipeline_verts,
                            fpme->pipeline_vertex_size,
                            fetch_count,
                            draw_elts,
                            draw_count );
   } else {
      unsigned i, j;
      void *hw_verts;
      char *out_buf;

      /* XXX: need to flush to get prim_vbuf.c to release its allocation?? 
       */
      draw_do_flush( draw, DRAW_FLUSH_BACKEND );

      hw_verts = draw->render->allocate_vertices(draw->render,
                                                 (ushort)fpme->hw_vertex_size,
                                                 (ushort)fetch_count);
      if (!hw_verts) {
         assert(0);
         return;
      }

      out_buf = (char *)hw_verts;
      fpme->input_buf[0] = (const ubyte *)pipeline_verts;
      fpme->input_buf[1] = (const ubyte *)&fpme->draw->rasterizer->point_size;

      for (i = 0; i < fetch_count; i++) {

         for (j = 0; j < fpme->nr_translate; j++) {

            const float *attrib = (const float *)( (*fpme->translate[i].input_buf) + 
						   fpme->translate[i].input_offset );

	    char *dest = out_buf + fpme->translate[i].output_offset;

            /*debug_printf("emiting [%f, %f, %f, %f]\n",
                         attrib[0], attrib[1],
                         attrib[2], attrib[3]);*/

            fpme->translate[j].emit(attrib, dest);
         }

	 fpme->input_buf[0] += fpme->pipeline_vertex_size;
      }

      draw->render->draw(draw->render,
                         draw_elts,
                         draw_count);

      draw->render->release_vertices(draw->render,
                                     hw_verts,
                                     fpme->hw_vertex_size,
                                     fetch_count);
   }


   FREE(pipeline_verts);
}



static void fetch_pipeline_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}

static void fetch_pipeline_destroy( struct draw_pt_middle_end *middle )
{
   FREE(middle);
}


struct draw_pt_middle_end *draw_pt_fetch_pipeline_or_emit( struct draw_context *draw )
{
   struct fetch_pipeline_middle_end *fetch_pipeline = CALLOC_STRUCT( fetch_pipeline_middle_end );

   fetch_pipeline->base.prepare = fetch_pipeline_prepare;
   fetch_pipeline->base.run     = fetch_pipeline_run;
   fetch_pipeline->base.finish  = fetch_pipeline_finish;
   fetch_pipeline->base.destroy = fetch_pipeline_destroy;

   fetch_pipeline->draw = draw;

   return &fetch_pipeline->base;
}
