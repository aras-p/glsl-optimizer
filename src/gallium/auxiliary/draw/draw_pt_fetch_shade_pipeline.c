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
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"
#include "translate/translate.h"


struct fetch_pipeline_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct pt_emit *emit;

   unsigned pipeline_vertex_size;
   unsigned prim;
   unsigned opt;
};


static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim,
				    unsigned opt )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;

   fpme->prim = prim;
   fpme->opt = opt;

   if (!(opt & PT_PIPELINE)) 
      draw_pt_emit_prepare( fpme->emit, prim, opt );

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
   unsigned pipeline = PT_PIPELINE;

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
		   fpme->pipeline_vertex_size))
   {
      pipeline |= PT_CLIPTEST;
   }


   /* Do we need to run the pipeline?
    */
   if (fpme->opt & pipeline) {
      draw_pt_run_pipeline( fpme->draw,
                            fpme->prim,
                            pipeline_verts,
                            fpme->pipeline_vertex_size,
                            fetch_count,
                            draw_elts,
                            draw_count );
   } else {
      draw_pt_emit( fpme->emit,
		    pipeline_verts,
		    fpme->pipeline_vertex_size,
		    fetch_count,
		    draw_elts,
		    draw_count );
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
   struct fetch_pipeline_middle_end *fpme = CALLOC_STRUCT( fetch_pipeline_middle_end );
   if (!fpme)
      goto fail;

   fpme->base.prepare = fetch_pipeline_prepare;
   fpme->base.run     = fetch_pipeline_run;
   fpme->base.finish  = fetch_pipeline_finish;
   fpme->base.destroy = fetch_pipeline_destroy;

   fpme->draw = draw;

   fpme->emit = draw_pt_emit_create( draw );
   if (!fpme->emit) 
      goto fail;

   return &fpme->base;

 fail:
   if (fpme)
      fetch_pipeline_destroy( &fpme->base );

   return NULL;
}
