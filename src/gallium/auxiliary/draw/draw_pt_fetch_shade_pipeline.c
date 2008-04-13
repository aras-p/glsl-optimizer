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

   struct {
      const ubyte *ptr;
      unsigned pitch;
      void (*fetch)( const void *from, float *attrib);
      void (*emit)( const float *attrib, float **out );
   } fetch[PIPE_MAX_ATTRIBS];

   unsigned nr_fetch;
   unsigned pipeline_vertex_size;
   unsigned hw_vertex_size;
   unsigned prim;
};

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

static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   unsigned i, nr = 0;
   boolean ok;
   const struct vertex_info *vinfo;

   fpme->prim = prim;

   ok = draw->render->set_primitive(draw->render, prim);
   if (!ok) {
      assert(0);
      return;
   }
   /* Must do this after set_primitive() above:
    */
   vinfo = draw->render->get_vertex_info(draw->render);

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
   //fpme->pipeline_vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);
   fpme->pipeline_vertex_size = (MAX_VERTEX_SIZE + 0x0f) & ~0x0f;
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
   unsigned i;

   //debug_printf("fc = %d, VS = %d\n", fetch_count, VS_QUEUE_LENGTH);
   if (fetch_count < VS_QUEUE_LENGTH) {
      pipeline_verts = draw->vs.vertex_cache;
   } else {
      pipeline_verts = MALLOC(fpme->pipeline_vertex_size *
                              fetch_count);
   }

   if (!pipeline_verts) {
      assert(0);
      return;
   }

   /*FIXME: this init phase should go away */
   for (i = 0; i < fetch_count; ++i) {
      struct vertex_header *header =
         (struct vertex_header*)(pipeline_verts + (fpme->pipeline_vertex_size * i));
      header->clipmask = 0;
      header->edgeflag = draw_get_edgeflag(draw, i);
      header->pad = 0;
      header->vertex_id = UNDEFINED_VERTEX_ID;
   }

   /* Shade
    */
   shader->prepare(shader, draw);
   if (shader->run(shader, draw, fetch_elts, fetch_count, pipeline_verts)) {
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
      float *out;

      hw_verts = draw->render->allocate_vertices(draw->render,
                                                 (ushort)fpme->hw_vertex_size,
                                                 (ushort)fetch_count);
      if (!hw_verts) {
         assert(0);
         return;
      }

      out = (float *)hw_verts;
      for (i = 0; i < fetch_count; i++) {
         struct vertex_header *header =
            (struct vertex_header*)(pipeline_verts + (fpme->pipeline_vertex_size * i));

         for (j = 0; j < fpme->nr_fetch; j++) {
            float *attrib = header->data[j];
            /*debug_printf("emiting [%f, %f, %f, %f]\n",
                         attrib[0], attrib[1],
                         attrib[2], attrib[3]);*/
            fpme->fetch[j].emit(attrib, &out);
         }
      }
      /* XXX: Draw arrays path to avoid re-emitting index list again and
       * again.
       */
      draw->render->draw(draw->render,
                         draw_elts,
                         draw_count);

      draw->render->release_vertices(draw->render,
                                     hw_verts,
                                     fpme->hw_vertex_size,
                                     fetch_count);
   }


   if (pipeline_verts != draw->vs.vertex_cache)
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
