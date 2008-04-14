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
  *   Brian Paul
  */

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"

#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

#include "tgsi/util/tgsi_parse.h"

#define MAX_TGSI_VERTICES 4

static void
vs_exec_prepare( struct draw_vertex_shader *shader,
		 struct draw_context *draw )
{
   /* specify the vertex program to interpret/execute */
   tgsi_exec_machine_bind_shader(&draw->machine,
				 shader->state.tokens,
				 PIPE_MAX_SAMPLERS,
				 NULL /*samplers*/ );

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
vs_exec_run( struct draw_vertex_shader *shader,
	     struct draw_context *draw,
	     const unsigned *elts, 
	     unsigned count,
	     void *vOut,
             unsigned vertex_size)
{
   struct tgsi_exec_machine *machine = &draw->machine;
   unsigned int i, j;
   unsigned int clipped = 0;

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_MAX_ATTRIBS);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_MAX_ATTRIBS);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   assert(draw->vertex_shader->info.output_semantic_name[0]
          == TGSI_SEMANTIC_POSITION);

   machine->Consts = (float (*)[4]) draw->user.constants;
   machine->Inputs = ALIGN16_ASSIGN(inputs);
   if (draw->rasterizer->bypass_vs) {
      /* outputs are just the inputs */
      machine->Outputs = machine->Inputs;
   }
   else {
      machine->Outputs = ALIGN16_ASSIGN(outputs);
   }

   for (i = 0; i < count; i += MAX_TGSI_VERTICES) {
      unsigned int max_vertices = MIN2(MAX_TGSI_VERTICES, count - i);
      draw->vertex_fetch.fetch_func( draw, machine, &elts[i], max_vertices );

      if (!draw->rasterizer->bypass_vs) {
         /* run interpreter */
         tgsi_exec_machine_run( machine );
      }

      /* store machine results */
      for (j = 0; j < max_vertices; j++) {
         unsigned slot;
         float x, y, z, w;
         struct vertex_header *out =
            draw_header_from_block(vOut, vertex_size, i + j);

         /* Handle attr[0] (position) specially:
          *
          * XXX: Computing the clipmask should be done in the vertex
          * program as a set of DP4 instructions appended to the
          * user-provided code.
          */
         x = out->clip[0] = machine->Outputs[0].xyzw[0].f[j];
         y = out->clip[1] = machine->Outputs[0].xyzw[1].f[j];
         z = out->clip[2] = machine->Outputs[0].xyzw[2].f[j];
         w = out->clip[3] = machine->Outputs[0].xyzw[3].f[j];

         if (!draw->rasterizer->bypass_clipping) {
            out->clipmask = compute_clipmask(out->clip, draw->plane,
                                             draw->nr_planes);
            clipped += out->clipmask;

            /* divide by w */
            w = 1.0f / w;
            x *= w;
            y *= w;
            z *= w;
         }
         else {
            out->clipmask = 0;
         }
         out->edgeflag = 1;
	 out->vertex_id = UNDEFINED_VERTEX_ID;

         if (!draw->identity_viewport) {
            /* Viewport mapping */
            out->data[0][0] = x * scale[0] + trans[0];
            out->data[0][1] = y * scale[1] + trans[1];
            out->data[0][2] = z * scale[2] + trans[2];
            out->data[0][3] = w;
         }
         else {
            out->data[0][0] = x;
            out->data[0][1] = y;
            out->data[0][2] = z;
            out->data[0][3] = w;
         }

         /* Remaining attributes are packed into sequential post-transform
          * vertex attrib slots.
          */
         for (slot = 1; slot < draw->num_vs_outputs; slot++) {
            out->data[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
            out->data[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
            out->data[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
            out->data[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
         }

#if 0 /*DEBUG*/
         printf("%d) Post xform vert:\n", i + j);
         for (slot = 0; slot < draw->num_vs_outputs; slot++) {
            printf("\t%d: %f %f %f %f\n", slot,
                   out->data[slot][0],
                   out->data[slot][1],
                   out->data[slot][2],
                   out->data[slot][3]);
         }
#endif
      } /* loop over vertices */
   }
   return clipped != 0;
}



static void
vs_exec_delete( struct draw_vertex_shader *dvs )
{
   FREE((void*) dvs->state.tokens);
   FREE( dvs );
}


struct draw_vertex_shader *
draw_create_vs_exec(struct draw_context *draw,
		    const struct pipe_shader_state *state)
{
   struct draw_vertex_shader *vs = CALLOC_STRUCT( draw_vertex_shader );
   uint nt = tgsi_num_tokens(state->tokens);

   if (vs == NULL) 
      return NULL;

   /* we make a private copy of the tokens */
   vs->state.tokens = mem_dup(state->tokens, nt * sizeof(state->tokens[0]));
   vs->prepare = vs_exec_prepare;
   vs->run = vs_exec_run;
   vs->delete = vs_exec_delete;

   return vs;
}
