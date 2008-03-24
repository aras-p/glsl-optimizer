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

#include "draw_vs.h"

#if defined(__i386__) || defined(__386__)

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"

#include "draw_private.h"
#include "draw_context.h"

#include "rtasm/rtasm_x86sse.h"
#include "tgsi/exec/tgsi_sse2.h"
#include "tgsi/util/tgsi_parse.h"


typedef void (XSTDCALL *codegen_function) (
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary );


struct draw_sse_vertex_shader {
   struct draw_vertex_shader base;
   struct x86_function sse2_program;
   codegen_function func;
};


/* Should be part of the generated shader:
 */
static INLINE unsigned
compute_clipmask(const float *clip, /*const*/ float plane[][4], unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   /* Do the hardwired planes first:
    */
   if (-clip[0] + clip[3] < 0) mask |= CLIP_RIGHT_BIT;
   if ( clip[0] + clip[3] < 0) mask |= CLIP_LEFT_BIT;
   if (-clip[1] + clip[3] < 0) mask |= CLIP_TOP_BIT;
   if ( clip[1] + clip[3] < 0) mask |= CLIP_BOTTOM_BIT;
   if (-clip[2] + clip[3] < 0) mask |= CLIP_FAR_BIT;
   if ( clip[2] + clip[3] < 0) mask |= CLIP_NEAR_BIT;

   /* Followed by any remaining ones:
    */
   for (i = 6; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0) 
         mask |= (1<<i);
   }

   return mask;
}


static void
vs_sse_prepare( struct draw_vertex_shader *base,
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
static void
vs_sse_run( struct draw_vertex_shader *base,
	    struct draw_context *draw, 
	    const unsigned *elts, 
	    unsigned count,
	    struct vertex_header *vOut[] )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   struct tgsi_exec_machine *machine = &draw->machine;
   unsigned int j;

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_ATTRIB_MAX);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_ATTRIB_MAX);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   assert(count <= 4);
   assert(draw->vertex_shader->info.output_semantic_name[0]
          == TGSI_SEMANTIC_POSITION);

   /* Consts does not require 16 byte alignment. */
   machine->Consts = (float (*)[4]) draw->user.constants;
   machine->Inputs = ALIGN16_ASSIGN(inputs);
   machine->Outputs = ALIGN16_ASSIGN(outputs);


   /* Fetch vertices.  This may at some point be integrated into the
    * compiled shader -- that would require a reorganization where
    * multiple versions of the compiled shader might exist,
    * specialized for each fetch state.
    */
   draw->vertex_fetch.fetch_func( draw, machine, elts, count );


   /* run compiled shader
    */   
   shader->func(
      machine->Inputs,
      machine->Outputs,
      machine->Consts,
      machine->Temps );


   /* XXX: Computing the clipmask and emitting results should be done
    *      in the vertex program as a set of instructions appended to
    *      the user-provided code.
    */
   for (j = 0; j < count; j++) {
      unsigned slot;
      float x, y, z, w;

      x = vOut[j]->clip[0] = machine->Outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = machine->Outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = machine->Outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = machine->Outputs[0].xyzw[3].f[j];

      if (!draw->rasterizer->bypass_clipping) {
         vOut[j]->clipmask = compute_clipmask(vOut[j]->clip, draw->plane, draw->nr_planes);
         vOut[j]->edgeflag = 1;

         /* divide by w */
         w = 1.0f / w;
         x *= w;
         y *= w;
         z *= w;
         
         /* Viewport mapping */
         vOut[j]->data[0][0] = x * scale[0] + trans[0];
         vOut[j]->data[0][1] = y * scale[1] + trans[1];
         vOut[j]->data[0][2] = z * scale[2] + trans[2];
         vOut[j]->data[0][3] = w;
      }
      else {
         vOut[j]->clipmask = 0;
         vOut[j]->edgeflag = 1;
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
   } 
}



static void
vs_sse_delete( struct draw_vertex_shader *base )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   
   x86_release_func( &shader->sse2_program );

   FREE( (void*) shader->base.state.tokens );
   FREE( shader );
}


struct draw_vertex_shader *
draw_create_vs_sse(struct draw_context *draw,
                          const struct pipe_shader_state *templ)
{
   struct draw_sse_vertex_shader *vs;
   uint nt = tgsi_num_tokens(templ->tokens);

   if (!draw->use_sse) 
      return NULL;

   vs = CALLOC_STRUCT( draw_sse_vertex_shader );
   if (vs == NULL) 
      return NULL;

   /* we make a private copy of the tokens */
   vs->base.state.tokens = mem_dup(templ->tokens, nt * sizeof(templ->tokens[0]));
   vs->base.prepare = vs_sse_prepare;
   vs->base.run = vs_sse_run;
   vs->base.delete = vs_sse_delete;
   
   x86_init_func( &vs->sse2_program );

   if (!tgsi_emit_sse2( (struct tgsi_token *) vs->base.state.tokens,
			&vs->sse2_program )) 
      goto fail;
      
   vs->func = (codegen_function) x86_get_func( &vs->sse2_program );
   
   return &vs->base;

fail:
   fprintf(stderr, "tgsi_emit_sse2() failed, falling back to interpreter\n");

   x86_release_func( &vs->sse2_program );
   
   FREE(vs);
   return NULL;
}



#else

struct draw_vertex_shader *
draw_create_vs_sse( struct draw_context *draw,
		    const struct pipe_shader_state *templ )
{
   return (void *) 0;
}


#endif

