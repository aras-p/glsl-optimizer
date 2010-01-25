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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_config.h"

#include "draw_vs.h"

#if defined(PIPE_ARCH_X86)

#include "pipe/p_shader_tokens.h"

#include "draw_private.h"
#include "draw_context.h"

#include "rtasm/rtasm_cpu.h"
#include "rtasm/rtasm_x86sse.h"
#include "tgsi/tgsi_sse2.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"

#define SSE_MAX_VERTICES 4


struct draw_sse_vertex_shader {
   struct draw_vertex_shader base;
   struct x86_function sse2_program;

   tgsi_sse2_vs_func func;
   
   struct tgsi_exec_machine *machine;
};


static void
vs_sse_prepare( struct draw_vertex_shader *base,
		struct draw_context *draw )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   struct tgsi_exec_machine *machine = shader->machine;

   machine->Samplers = draw->vs.samplers;
}



/* Simplified vertex shader interface for the pt paths.  Given the
 * complexity of code-generating all the above operations together,
 * it's time to try doing all the other stuff separately.
 */
static void
vs_sse_run_linear( struct draw_vertex_shader *base,
		   const float (*input)[4],
		   float (*output)[4],
                  const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
		   unsigned count,
		   unsigned input_stride,
		   unsigned output_stride )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned int i;

   /* By default, execute all channels.  XXX move this inside the loop
    * below when we support shader conditionals/loops.
    */
   tgsi_set_exec_mask(machine, 1, 1, 1, 1);

   for (i = 0; i < count; i += MAX_TGSI_VERTICES) {
      unsigned int max_vertices = MIN2(MAX_TGSI_VERTICES, count - i);

      if (max_vertices < 4) {
         /* disable the unused execution channels */
         tgsi_set_exec_mask(machine,
                            1,
                            max_vertices > 1,
                            max_vertices > 2,
                            0);
      }

      /* run compiled shader
       */
      shader->func(machine,
                   (const float (*)[4])constants[0],
		   shader->base.immediates,
                   input,
                   base->info.num_inputs,
                   input_stride,
                   output,
                   base->info.num_outputs,
                   output_stride );

      input = (const float (*)[4])((const char *)input + input_stride * max_vertices);
      output = (float (*)[4])((char *)output + output_stride * max_vertices);
   }
}




static void
vs_sse_delete( struct draw_vertex_shader *base )
{
   struct draw_sse_vertex_shader *shader = (struct draw_sse_vertex_shader *)base;
   
   x86_release_func( &shader->sse2_program );

   align_free( (void *) shader->base.immediates );

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

   vs->base.draw = draw;
   if (1)
      vs->base.create_varient = draw_vs_varient_aos_sse;
   else
      vs->base.create_varient = draw_vs_varient_generic;
   vs->base.prepare = vs_sse_prepare;
   vs->base.run_linear = vs_sse_run_linear;
   vs->base.delete = vs_sse_delete;
   
   vs->base.immediates = align_malloc(TGSI_EXEC_NUM_IMMEDIATES * 4 *
                                      sizeof(float), 16);

   vs->machine = draw->vs.machine;
   
   x86_init_func( &vs->sse2_program );

   if (!tgsi_emit_sse2( (struct tgsi_token *) vs->base.state.tokens,
			&vs->sse2_program, 
                        (float (*)[4])vs->base.immediates, 
                        TRUE )) 
      goto fail;
      
   vs->func = (tgsi_sse2_vs_func) x86_get_func( &vs->sse2_program );
   if (!vs->func) {
      goto fail;
   }
   
   return &vs->base;

fail:
   debug_error("tgsi_emit_sse2() failed, falling back to interpreter\n");

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

