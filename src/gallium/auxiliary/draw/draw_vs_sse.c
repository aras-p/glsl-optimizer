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

#include "rtasm/rtasm_cpu.h"
#include "rtasm/rtasm_x86sse.h"
#include "tgsi/exec/tgsi_sse2.h"
#include "tgsi/util/tgsi_parse.h"

#define SSE_MAX_VERTICES 4
#define SSE_SWIZZLES 1

#if SSE_SWIZZLES
typedef void (XSTDCALL *codegen_function) (
   const struct tgsi_exec_vector *input, /* 1 */
   struct tgsi_exec_vector *output, /* 2 */
   float (*constant)[4],        /* 3 */
   struct tgsi_exec_vector *temporary, /* 4 */
   float (*immediates)[4],      /* 5 */
   const float (*aos_input)[4], /* 6 */
   uint num_inputs,             /* 7 */
   uint input_stride,           /* 8 */
   float (*aos_output)[4],      /* 9 */
   uint num_outputs,            /* 10 */
   uint output_stride );        /* 11 */
#else
typedef void (XSTDCALL *codegen_function) (
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary,
   float (*immediates)[4] );
#endif

struct draw_sse_vertex_shader {
   struct draw_vertex_shader base;
   struct x86_function sse2_program;

   codegen_function func;
   
   struct tgsi_exec_machine *machine;

   float immediates[TGSI_EXEC_NUM_IMMEDIATES][4];
};


static void
vs_sse_prepare( struct draw_vertex_shader *base,
		struct draw_context *draw )
{
}



/* Simplified vertex shader interface for the pt paths.  Given the
 * complexity of code-generating all the above operations together,
 * it's time to try doing all the other stuff separately.
 */
static void
vs_sse_run_linear( struct draw_vertex_shader *base,
		   const float (*input)[4],
		   float (*output)[4],
		   const float (*constants)[4],
		   unsigned count,
		   unsigned input_stride,
		   unsigned output_stride )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned int i;

   for (i = 0; i < count; i += MAX_TGSI_VERTICES) {
      unsigned int max_vertices = MIN2(MAX_TGSI_VERTICES, count - i);

#if SSE_SWIZZLES
      /* run compiled shader
       */
      shader->func(machine->Inputs,
		   machine->Outputs,
		   (float (*)[4])constants,
		   machine->Temps,
		   shader->immediates,
                   input,
                   base->info.num_inputs,
                   input_stride,
                   output,
                   base->info.num_outputs,
                   output_stride );

      input = (const float (*)[4])((const char *)input + input_stride * max_vertices);
      output = (float (*)[4])((char *)output + output_stride * max_vertices);
#else
      unsigned int j, slot;

      /* Swizzle inputs.  
       */
      for (j = 0; j < max_vertices; j++) {
         for (slot = 0; slot < base->info.num_inputs; slot++) {
            machine->Inputs[slot].xyzw[0].f[j] = input[slot][0];
            machine->Inputs[slot].xyzw[1].f[j] = input[slot][1];
            machine->Inputs[slot].xyzw[2].f[j] = input[slot][2];
            machine->Inputs[slot].xyzw[3].f[j] = input[slot][3];
         } 

	 input = (const float (*)[4])((const char *)input + input_stride);
      }

      /* run compiled shader
       */
      shader->func(machine->Inputs,
		   machine->Outputs,
		   (float (*)[4])constants,
		   machine->Temps,
		   shader->immediates);

      /* Unswizzle all output results.  
       */
      for (j = 0; j < max_vertices; j++) {
         for (slot = 0; slot < base->info.num_outputs; slot++) {
            output[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
            output[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
            output[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
            output[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
         } 

	 output = (float (*)[4])((char *)output + output_stride);
      }
#endif
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

   if (!rtasm_cpu_has_sse2())
      return NULL;

   vs = CALLOC_STRUCT( draw_sse_vertex_shader );
   if (vs == NULL) 
      return NULL;

   /* we make a private copy of the tokens */
   vs->base.state.tokens = tgsi_dup_tokens(templ->tokens);
   if (!vs->base.state.tokens)
      goto fail;

   tgsi_scan_shader(templ->tokens, &vs->base.info);

   vs->base.prepare = vs_sse_prepare;
   vs->base.run_linear = vs_sse_run_linear;
   vs->base.delete = vs_sse_delete;
   vs->machine = &draw->machine;
   
   x86_init_func( &vs->sse2_program );

   if (!tgsi_emit_sse2( (struct tgsi_token *) vs->base.state.tokens,
			&vs->sse2_program, vs->immediates, SSE_SWIZZLES )) 
      goto fail;
      
   vs->func = (codegen_function) x86_get_func( &vs->sse2_program );
   if (!vs->func) {
      goto fail;
   }
   
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

