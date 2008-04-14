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
  *   Zack Rusin
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

#include "tgsi/util/tgsi_parse.h"

#ifdef MESA_LLVM

#include "gallivm/gallivm.h"

struct draw_llvm_vertex_shader {
   struct draw_vertex_shader base;
   struct gallivm_prog *llvm_prog;
};


static void
vs_llvm_prepare( struct draw_vertex_shader *base,
		 struct draw_context *draw )
{
   draw_update_vertex_fetch( draw );
}



/**
 * Transform vertices with the current vertex program/shader
 * Up to four vertices can be shaded at a time.
 * \param vbuffer  the input vertex data
 * \param elts  indexes of four input vertices
 * \param count  number of vertices to shade [1..4]
 * \param vOut  array of pointers to four output vertices
 */
static boolean
vs_llvm_run( struct draw_vertex_shader *base,
	     struct draw_context *draw,
	     const unsigned *elts,
	     unsigned count,
	     void *vOut )
{
   struct draw_llvm_vertex_shader *shader =
      (struct draw_llvm_vertex_shader *)base;

   struct tgsi_exec_machine *machine = &draw->machine;
   unsigned int j;
   unsigned int clipped = 0;

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_MAX_ATTRIBS);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_MAX_ATTRIBS);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;


   assert(count <= 4);
   assert(draw->vertex_shader->state->output_semantic_name[0]
          == TGSI_SEMANTIC_POSITION);

   /* Consts does not require 16 byte alignment. */
   machine->Consts = (float (*)[4]) draw->user.constants;

   machine->Inputs = ALIGN16_ASSIGN(inputs);
   if (draw->rasterizer->bypass_vs) {
      /* outputs are just the inputs */
      machine->Outputs = machine->Inputs;
   }
   else {
      machine->Outputs = ALIGN16_ASSIGN(outputs);
   }


   draw->vertex_fetch.fetch_func( draw, machine, elts, count );

   if (!draw->rasterizer->bypass_vs) {
      /* run shader */
      gallivm_cpu_vs_exec(shader->llvm_prog,
                          machine->Inputs,
                          machine->Outputs,
                          machine->Consts,
                          machine->Temps);
   }

   /* store machine results */
   for (j = 0; j < count; j++) {
      unsigned slot;
      float x, y, z, w;

      x = vOut[j]->clip[0] = machine->Outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = machine->Outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = machine->Outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = machine->Outputs[0].xyzw[3].f[j];

      if (!draw->rasterizer->bypass_clipping) {
         vOut[j]->clipmask = compute_clipmask(vOut[j]->clip, draw->plane,
                                              draw->nr_planes);
         clipped += vOut[j]->clipmask;

         /* divide by w */
         w = 1.0f / w;
         x *= w;
         y *= w;
         z *= w;
      }
      else {
         vOut[j]->clipmask = 0;
      }
      vOut[j]->edgeflag = 1;
      vOut[j]->vertex_id = UNDEFINED_VERTEX_ID;

      if (!draw->identity_viewport) {
         /* Viewport mapping */
         vOut[j]->data[0][0] = x * scale[0] + trans[0];
         vOut[j]->data[0][1] = y * scale[1] + trans[1];
         vOut[j]->data[0][2] = z * scale[2] + trans[2];
         vOut[j]->data[0][3] = w;
      }
      else {
         vOut[j]->data[0][0] = x;
         vOut[j]->data[0][1] = y;
         vOut[j]->data[0][2] = z;
         vOut[j]->data[0][3] = w;
      }

      /* Remaining attributes are packed into sequential post-transform
       * vertex attrib slots.
       */
      for (slot = 1; slot < draw->num_vs_outputs; slot++) {
         vOut[j]->data[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
         vOut[j]->data[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
         vOut[j]->data[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
         vOut[j]->data[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
      }
   } /* loop over vertices */
   return clipped != 0;
}

static void
vs_llvm_delete( struct draw_vertex_shader *base )
{
   struct draw_llvm_vertex_shader *shader = 
      (struct draw_llvm_vertex_shader *)base;

   /* Do something to free compiled shader:
    */

   FREE( (void*) shader->base.state.tokens );
   FREE( shader );
}




struct draw_vertex_shader *
draw_create_vs_llvm(struct draw_context *draw,
		    const struct pipe_shader_state *templ)
{
   struct draw_llvm_vertex_shader *vs;
   uint nt = tgsi_num_tokens(templ->tokens);

   vs = CALLOC_STRUCT( draw_llvm_vertex_shader );
   if (vs == NULL) 
      return NULL;

   /* we make a private copy of the tokens */
   vs->base.state.tokens = mem_dup(templ->tokens, nt * sizeof(templ->tokens[0]));
   vs->base.prepare = vs_llvm_prepare;
   vs->base.run = vs_llvm_run;
   vs->base.delete = vs_llvm_delete;

   {
      struct gallivm_ir *ir = gallivm_ir_new(GALLIVM_VS);
      gallivm_ir_set_layout(ir, GALLIVM_SOA);
      gallivm_ir_set_components(ir, 4);
      gallivm_ir_fill_from_tgsi(ir, vs->base.state->tokens);
      vs->llvm_prog = gallivm_ir_compile(ir);
      gallivm_ir_delete(ir);
   }

   draw->engine = gallivm_global_cpu_engine();

   /* XXX: Why are there two versions of this?  Shouldn't creating the
    *      engine be a separate operation to compiling a shader?
    */
   if (!draw->engine) {
      draw->engine = gallivm_cpu_engine_create(vs->llvm_prog);
   }
   else {
      gallivm_cpu_jit_compile(draw->engine, vs->llvm_prog);
   }

   return &vs->base;
}





#else

struct draw_vertex_shader *
draw_create_vs_llvm(struct draw_context *draw,
                          const struct pipe_shader_state *shader)
{
   return NULL;
}

#endif
