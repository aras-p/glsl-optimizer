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
#include "draw/draw_vs.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"

/* A first pass at incorporating vertex fetch/emit functionality into 
 */
struct draw_vs_varient_generic {
   struct draw_vs_varient base;

   struct pipe_viewport_state viewport;

   struct draw_vertex_shader *shader;
   struct draw_context *draw;
   
   /* Basic plan is to run these two translate functions before/after
    * the vertex shader's existing run_linear() routine to simulate
    * the inclusion of this functionality into the shader...  
    * 
    * Next will look at actually including it.
    */
   struct translate *fetch;
   struct translate *emit;

   const float (*constants)[4];
};




static void vsvg_set_constants( struct draw_vs_varient *varient,
                                const float (*constants)[4] )
{
   struct draw_vs_varient_generic *vsvg = (struct draw_vs_varient_generic *)varient;

   vsvg->constants = constants;
}


static void vsvg_set_input( struct draw_vs_varient *varient,
                            unsigned buffer,
                            const void *ptr,
                            unsigned stride )
{
   struct draw_vs_varient_generic *vsvg = (struct draw_vs_varient_generic *)varient;

   vsvg->fetch->set_buffer(vsvg->fetch, 
                           buffer, 
                           ptr, 
                           stride);
}


/* Mainly for debug at this stage:
 */
static void do_rhw_viewport( struct draw_vs_varient_generic *vsvg,
                             unsigned count,
                             void *output_buffer )
{
   char *ptr = (char *)output_buffer;
   const float *scale = vsvg->viewport.scale;
   const float *trans = vsvg->viewport.translate;
   unsigned stride = vsvg->base.key.output_stride;
   unsigned j;

   for (j = 0; j < count; j++, ptr += stride) {
      float *data = (float *)ptr;
      float w = 1.0f / data[3];

      data[0] = data[0] * w * scale[0] + trans[0];
      data[1] = data[1] * w * scale[1] + trans[1];
      data[2] = data[2] * w * scale[2] + trans[2];
      data[3] = w;
   }
}

static void do_viewport( struct draw_vs_varient_generic *vsvg,
                             unsigned count,
                             void *output_buffer )
{
   char *ptr = (char *)output_buffer;
   const float *scale = vsvg->viewport.scale;
   const float *trans = vsvg->viewport.translate;
   unsigned stride = vsvg->base.key.output_stride;
   unsigned j;

   for (j = 0; j < count; j++, ptr += stride) {
      float *data = (float *)ptr;

      data[0] = data[0] * scale[0] + trans[0];
      data[1] = data[1] * scale[1] + trans[1];
      data[2] = data[2] * scale[2] + trans[2];
   }
}
                         

static void vsvg_run_elts( struct draw_vs_varient *varient,
                           const unsigned *elts,
                           unsigned count,
                           void *output_buffer)
{
   struct draw_vs_varient_generic *vsvg = (struct draw_vs_varient_generic *)varient;
			
   /* Want to do this in small batches for cache locality?
    */
   
   vsvg->fetch->run_elts( vsvg->fetch, 
                          elts,
                          count,
                          output_buffer );

   //if (!vsvg->base.vs->is_passthrough) 
   {
      vsvg->base.vs->run_linear( vsvg->base.vs, 
                                 output_buffer,
                                 output_buffer,
                                 vsvg->constants,
                                 count,
                                 vsvg->base.key.output_stride, 
                                 vsvg->base.key.output_stride);


      if (vsvg->base.key.clip) {
         /* not really handling clipping, just do the rhw so we can
          * see the results...
          */
         do_rhw_viewport( vsvg,
                          count,
                          output_buffer );
      }
      else if (vsvg->base.key.viewport) {
         do_viewport( vsvg,
                      count,
                      output_buffer );
      }


      //if (!vsvg->already_in_emit_format)

      vsvg->emit->set_buffer( vsvg->emit,
                              0, 
                              output_buffer,
                              vsvg->base.key.output_stride );


      vsvg->emit->run( vsvg->emit,
                       0, count,
                       output_buffer );
   }
}


static void vsvg_run_linear( struct draw_vs_varient *varient,
                                   unsigned start,
                                   unsigned count,
                                   void *output_buffer )
{
   struct draw_vs_varient_generic *vsvg = (struct draw_vs_varient_generic *)varient;
	
   //debug_printf("%s %d %d\n", __FUNCTION__, start, count);
   
				
   vsvg->fetch->run( vsvg->fetch, 
                     start,
                     count,
                     output_buffer );

   //if (!vsvg->base.vs->is_passthrough) 
   {
      vsvg->base.vs->run_linear( vsvg->base.vs, 
                                 output_buffer,
                                 output_buffer,
                                 vsvg->constants,
                                 count,
                                 vsvg->base.key.output_stride, 
                                 vsvg->base.key.output_stride);

      if (vsvg->base.key.clip) {
         /* not really handling clipping, just do the rhw so we can
          * see the results...
          */
         do_rhw_viewport( vsvg,
                          count,
                          output_buffer );
      }
      else if (vsvg->base.key.viewport) {
         do_viewport( vsvg,
                      count,
                      output_buffer );
      }

      //if (!vsvg->already_in_emit_format)
      vsvg->emit->set_buffer( vsvg->emit,
                              0, 
                              output_buffer,
                              vsvg->base.key.output_stride );


      vsvg->emit->run( vsvg->emit,
                       0, count,
                       output_buffer );
   }
}




static void vsvg_set_viewport( struct draw_vs_varient *varient,
                               const struct pipe_viewport_state *viewport )
{
   struct draw_vs_varient_generic *vsvg = (struct draw_vs_varient_generic *)varient;

   vsvg->viewport = *viewport;
}

static void vsvg_destroy( struct draw_vs_varient *varient )
{
   FREE(varient);
}


struct draw_vs_varient *draw_vs_varient_generic( struct draw_vertex_shader *vs,
                                                 const struct draw_vs_varient_key *key )
{
   unsigned i;
   struct translate_key fetch, emit;

   struct draw_vs_varient_generic *vsvg = CALLOC_STRUCT( draw_vs_varient_generic );
   if (vsvg == NULL)
      return NULL;

   vsvg->base.key = *key;
   vsvg->base.vs = vs;
   vsvg->base.set_input     = vsvg_set_input;
   vsvg->base.set_constants = vsvg_set_constants;
   vsvg->base.set_viewport  = vsvg_set_viewport;
   vsvg->base.run_elts      = vsvg_run_elts;
   vsvg->base.run_linear    = vsvg_run_linear;
   vsvg->base.destroy       = vsvg_destroy;



   /* Build free-standing fetch and emit functions:
    */
   fetch.nr_elements = key->nr_inputs;
   fetch.output_stride = 0;
   for (i = 0; i < key->nr_inputs; i++) {
      fetch.element[i].input_format = key->element[i].in.format;
      fetch.element[i].input_buffer = key->element[i].in.buffer;
      fetch.element[i].input_offset = key->element[i].in.offset;
      fetch.element[i].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      fetch.element[i].output_offset = fetch.output_stride;
      fetch.output_stride += 4 * sizeof(float);
   }


   emit.nr_elements = key->nr_outputs;
   emit.output_stride = key->output_stride;
   for (i = 0; i < key->nr_outputs; i++) {
      emit.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      emit.element[i].input_buffer = 0;
      emit.element[i].input_offset = i * 4 * sizeof(float);
      emit.element[i].output_format = key->element[i].out.format;
      emit.element[i].output_offset = key->element[i].out.offset;
   }

   vsvg->fetch = draw_vs_get_fetch( vs->draw, &fetch );
   vsvg->emit = draw_vs_get_emit( vs->draw, &emit );

   return &vsvg->base;
}





