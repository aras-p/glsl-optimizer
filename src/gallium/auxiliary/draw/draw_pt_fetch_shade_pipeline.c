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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"
#include "draw/draw_gs.h"


struct fetch_pipeline_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct pt_emit *emit;
   struct pt_fetch *fetch;
   struct pt_post_vs *post_vs;

   unsigned vertex_data_offset;
   unsigned vertex_size;
   unsigned prim;
   unsigned opt;
};


static void fetch_pipeline_prepare( struct draw_pt_middle_end *middle,
                                    unsigned prim,
				    unsigned opt,
                                    unsigned *max_vertices )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *vs = draw->vs.vertex_shader;
   unsigned i;
   unsigned instance_id_index = ~0;

   /* Add one to num_outputs because the pipeline occasionally tags on
    * an additional texcoord, eg for AA lines.
    */
   unsigned nr = MAX2( vs->info.num_inputs,
		       vs->info.num_outputs + 1 );

   /* Scan for instanceID system value.
    */
   for (i = 0; i < vs->info.num_inputs; i++) {
      if (vs->info.input_semantic_name[i] == TGSI_SEMANTIC_INSTANCEID) {
         instance_id_index = i;
         break;
      }
   }

   fpme->prim = prim;
   fpme->opt = opt;

   /* Always leave room for the vertex header whether we need it or
    * not.  It's hard to get rid of it in particular because of the
    * viewport code in draw_pt_post_vs.c.  
    */
   fpme->vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);

   

   draw_pt_fetch_prepare( fpme->fetch, 
                          vs->info.num_inputs,
                          fpme->vertex_size,
                          instance_id_index );
   /* XXX: it's not really gl rasterization rules we care about here,
    * but gl vs dx9 clip spaces.
    */
   draw_pt_post_vs_prepare( fpme->post_vs,
			    (boolean)draw->bypass_clipping,
			    (boolean)draw->identity_viewport,
			    (boolean)draw->rasterizer->gl_rasterization_rules,
			    (draw->vs.edgeflag_output ? true : false) );    

   if (!(opt & PT_PIPELINE)) {
      draw_pt_emit_prepare( fpme->emit, 
			    prim,
                            max_vertices );

      *max_vertices = MAX2( *max_vertices,
                            DRAW_PIPE_MAX_VERTICES );
   }
   else {
      *max_vertices = DRAW_PIPE_MAX_VERTICES; 
   }

   /* return even number */
   *max_vertices = *max_vertices & ~1;

   /* No need to prepare the shader.
    */
   vs->prepare(vs, draw);
}



static void fetch_pipeline_run( struct draw_pt_middle_end *middle,
                                const unsigned *fetch_elts,
                                unsigned fetch_count,
                                const ushort *draw_elts,
                                unsigned draw_count )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *vshader = draw->vs.vertex_shader;
   struct draw_geometry_shader *gshader = draw->gs.geometry_shader;
   unsigned opt = fpme->opt;
   unsigned alloc_count = align( fetch_count, 4 );

   struct vertex_header *pipeline_verts = 
      (struct vertex_header *)MALLOC(fpme->vertex_size * alloc_count);

   if (!pipeline_verts) {
      /* Not much we can do here - just skip the rendering.
       */
      assert(0);
      return;
   }

   /* Fetch into our vertex buffer
    */
   draw_pt_fetch_run( fpme->fetch,
		      fetch_elts, 
		      fetch_count,
		      (char *)pipeline_verts );

   /* Run the shader, note that this overwrites the data[] parts of
    * the pipeline verts.
    */
   if (opt & PT_SHADE)
   {
      vshader->run_linear(vshader,
                          (const float (*)[4])pipeline_verts->data,
                          (      float (*)[4])pipeline_verts->data,
                          draw->pt.user.vs_constants,
                          fetch_count,
                          fpme->vertex_size,
                          fpme->vertex_size);
      if (gshader)
         draw_geometry_shader_run(gshader,
                                  (const float (*)[4])pipeline_verts->data,
                                  (      float (*)[4])pipeline_verts->data,
                                  draw->pt.user.gs_constants,
                                  fetch_count,
                                  fpme->vertex_size,
                                  fpme->vertex_size);
   }

   if (draw_pt_post_vs_run( fpme->post_vs,
			    pipeline_verts,
			    fetch_count,
			    fpme->vertex_size ))
   {
      opt |= PT_PIPELINE;
   }

   /* Do we need to run the pipeline?
    */
   if (opt & PT_PIPELINE) {
      draw_pipeline_run( fpme->draw,
                         fpme->prim,
                         pipeline_verts,
                         fetch_count,
                         fpme->vertex_size,
                         draw_elts,
                         draw_count );
   }
   else {
      draw_pt_emit( fpme->emit,
		    (const float (*)[4])pipeline_verts->data,
		    fetch_count,
		    fpme->vertex_size,
		    draw_elts,
		    draw_count );
   }


   FREE(pipeline_verts);
}


static void fetch_pipeline_linear_run( struct draw_pt_middle_end *middle,
                                       unsigned start,
                                       unsigned count)
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *shader = draw->vs.vertex_shader;
   struct draw_geometry_shader *geometry_shader = draw->gs.geometry_shader;
   unsigned opt = fpme->opt;
   unsigned alloc_count = align( count, 4 );

   struct vertex_header *pipeline_verts =
      (struct vertex_header *)MALLOC(fpme->vertex_size * alloc_count);

   if (!pipeline_verts) {
      /* Not much we can do here - just skip the rendering.
       */
      assert(0);
      return;
   }

   /* Fetch into our vertex buffer
    */
   draw_pt_fetch_run_linear( fpme->fetch,
                             start,
                             count,
                             (char *)pipeline_verts );

   /* Run the shader, note that this overwrites the data[] parts of
    * the pipeline verts.
    */
   if (opt & PT_SHADE)
   {
      shader->run_linear(shader,
			 (const float (*)[4])pipeline_verts->data,
			 (      float (*)[4])pipeline_verts->data,
                         draw->pt.user.vs_constants,
			 count,
			 fpme->vertex_size,
			 fpme->vertex_size);

      if (geometry_shader)
         draw_geometry_shader_run(geometry_shader,
                                  (const float (*)[4])pipeline_verts->data,
                                  (      float (*)[4])pipeline_verts->data,
                                  draw->pt.user.gs_constants,
                                  count,
                                  fpme->vertex_size,
                                  fpme->vertex_size);
   }

   if (draw_pt_post_vs_run( fpme->post_vs,
			    pipeline_verts,
			    count,
			    fpme->vertex_size ))
   {
      opt |= PT_PIPELINE;
   }

   /* Do we need to run the pipeline?
    */
   if (opt & PT_PIPELINE) {
      draw_pipeline_run_linear( fpme->draw,
                                fpme->prim,
                                pipeline_verts,
                                count,
                                fpme->vertex_size);
   }
   else {
      draw_pt_emit_linear( fpme->emit,
                           (const float (*)[4])pipeline_verts->data,
                           fpme->vertex_size,
                           count );
   }

   FREE(pipeline_verts);
}



static boolean fetch_pipeline_linear_run_elts( struct draw_pt_middle_end *middle,
                                            unsigned start,
                                            unsigned count,
                                            const ushort *draw_elts,
                                            unsigned draw_count )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *shader = draw->vs.vertex_shader;
   struct draw_geometry_shader *geometry_shader = draw->gs.geometry_shader;
   unsigned opt = fpme->opt;
   unsigned alloc_count = align( count, 4 );

   struct vertex_header *pipeline_verts =
      (struct vertex_header *)MALLOC(fpme->vertex_size * alloc_count);

   if (!pipeline_verts) 
      return FALSE;

   /* Fetch into our vertex buffer
    */
   draw_pt_fetch_run_linear( fpme->fetch,
                             start,
                             count,
                             (char *)pipeline_verts );

   /* Run the shader, note that this overwrites the data[] parts of
    * the pipeline verts.
    */
   if (opt & PT_SHADE)
   {
      shader->run_linear(shader,
			 (const float (*)[4])pipeline_verts->data,
			 (      float (*)[4])pipeline_verts->data,
                         draw->pt.user.vs_constants,
			 count,
			 fpme->vertex_size,
			 fpme->vertex_size);

      if (geometry_shader)
         draw_geometry_shader_run(geometry_shader,
                                  (const float (*)[4])pipeline_verts->data,
                                  (      float (*)[4])pipeline_verts->data,
                                  draw->pt.user.gs_constants,
                                  count,
                                  fpme->vertex_size,
                                  fpme->vertex_size);
   }

   if (draw_pt_post_vs_run( fpme->post_vs,
			    pipeline_verts,
			    count,
			    fpme->vertex_size ))
   {
      opt |= PT_PIPELINE;
   }

   /* Do we need to run the pipeline?
    */
   if (opt & PT_PIPELINE) {
      draw_pipeline_run( fpme->draw,
                         fpme->prim,
                         pipeline_verts,
                         count,
                         fpme->vertex_size,
                         draw_elts,
                         draw_count );
   }
   else {
      draw_pt_emit( fpme->emit,
		    (const float (*)[4])pipeline_verts->data,
		    count,
		    fpme->vertex_size,
		    draw_elts,
		    draw_count );
   }

   FREE(pipeline_verts);
   return TRUE;
}



static void fetch_pipeline_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}

static void fetch_pipeline_destroy( struct draw_pt_middle_end *middle )
{
   struct fetch_pipeline_middle_end *fpme = (struct fetch_pipeline_middle_end *)middle;

   if (fpme->fetch)
      draw_pt_fetch_destroy( fpme->fetch );

   if (fpme->emit)
      draw_pt_emit_destroy( fpme->emit );

   if (fpme->post_vs)
      draw_pt_post_vs_destroy( fpme->post_vs );

   FREE(middle);
}


struct draw_pt_middle_end *draw_pt_fetch_pipeline_or_emit( struct draw_context *draw )
{
   struct fetch_pipeline_middle_end *fpme = CALLOC_STRUCT( fetch_pipeline_middle_end );
   if (!fpme)
      goto fail;

   fpme->base.prepare        = fetch_pipeline_prepare;
   fpme->base.run            = fetch_pipeline_run;
   fpme->base.run_linear     = fetch_pipeline_linear_run;
   fpme->base.run_linear_elts = fetch_pipeline_linear_run_elts;
   fpme->base.finish         = fetch_pipeline_finish;
   fpme->base.destroy        = fetch_pipeline_destroy;

   fpme->draw = draw;

   fpme->fetch = draw_pt_fetch_create( draw );
   if (!fpme->fetch)
      goto fail;

   fpme->post_vs = draw_pt_post_vs_create( draw );
   if (!fpme->post_vs)
      goto fail;

   fpme->emit = draw_pt_emit_create( draw );
   if (!fpme->emit) 
      goto fail;

   return &fpme->base;

 fail:
   if (fpme)
      fetch_pipeline_destroy( &fpme->base );

   return NULL;
}
