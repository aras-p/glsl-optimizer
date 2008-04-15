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
#include "translate/translate.h"


struct fetch_pipeline_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct translate *translate;

   unsigned pipeline_vertex_size;
   unsigned prim;
};


static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   unsigned i;
   boolean ok;
   const struct vertex_info *vinfo;
   unsigned dst_offset;
   struct translate_key hw_key;

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
      unsigned output_format;
      unsigned src_offset = (sizeof(struct vertex_header) + 
			     vinfo->src_index[i] * 4 * sizeof(float) );


         
      switch (vinfo->emit[i]) {
      case EMIT_4F:
         output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
	 emit_sz = 4 * sizeof(float);
         break;
      case EMIT_3F:
         output_format = PIPE_FORMAT_R32G32B32_FLOAT;
	 emit_sz = 3 * sizeof(float);
         break;
      case EMIT_2F:
         output_format = PIPE_FORMAT_R32G32_FLOAT;
	 emit_sz = 2 * sizeof(float);
         break;
      case EMIT_1F:
         output_format = PIPE_FORMAT_R32_FLOAT;
	 emit_sz = 1 * sizeof(float);
         break;
      case EMIT_1F_PSIZE:
         output_format = PIPE_FORMAT_R32_FLOAT;
	 emit_sz = 1 * sizeof(float);
         src_buffer = 1;
	 src_offset = 0;
         break;
      case EMIT_4UB:
         output_format = PIPE_FORMAT_B8G8R8A8_UNORM;
	 emit_sz = 4 * sizeof(ubyte);
      default:
         assert(0);
         output_format = PIPE_FORMAT_NONE;
	 emit_sz = 0;
         break;
      }
      
      hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      hw_key.element[i].input_buffer = src_buffer;
      hw_key.element[i].input_offset = src_offset;
      hw_key.element[i].output_format = output_format;
      hw_key.element[i].output_offset = dst_offset;

      dst_offset += emit_sz;
   }

   hw_key.nr_elements = vinfo->num_attribs;
   hw_key.output_stride = vinfo->size * 4;

   /* Don't bother with caching at this stage:
    */
   if (!fpme->translate ||
       memcmp(&fpme->translate->key, &hw_key, sizeof(hw_key)) != 0) 
   {
      if (fpme->translate)
	 fpme->translate->release(fpme->translate);

      fpme->translate = translate_generic_create( &hw_key );
   }



   //fpme->pipeline_vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);
   fpme->pipeline_vertex_size = MAX_VERTEX_ALLOCATION;
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
      struct translate *translate = fpme->translate;
      void *hw_verts;

      /* XXX: need to flush to get prim_vbuf.c to release its allocation?? 
       */
      draw_do_flush( draw, DRAW_FLUSH_BACKEND );

      hw_verts = draw->render->allocate_vertices(draw->render,
                                                 (ushort)fpme->translate->key.output_stride,
                                                 (ushort)fetch_count);
      if (!hw_verts) {
         assert(0);
         return;
      }

      translate->set_buffer(translate, 
			    0, 
			    pipeline_verts,
			    fpme->pipeline_vertex_size );

      translate->set_buffer(translate, 
			    1, 
			    &fpme->draw->rasterizer->point_size,
			    0);

      translate->run( translate,
		      0, fetch_count,
		      hw_verts );

      draw->render->draw(draw->render,
                         draw_elts,
                         draw_count);

      draw->render->release_vertices(draw->render,
                                     hw_verts,
                                     fpme->translate->key.output_stride,
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
