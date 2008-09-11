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

#include "pipe/p_shader_tokens.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

#include "tgsi/tgsi_parse.h"

#ifdef MESA_LLVM

#include "gallivm/gallivm.h"

struct draw_llvm_vertex_shader {
   struct draw_vertex_shader base;
   struct gallivm_prog *llvm_prog;
   struct tgsi_exec_machine *machine;
};


static void
vs_llvm_prepare( struct draw_vertex_shader *base,
		 struct draw_context *draw )
{
}




static void
vs_llvm_run_linear( struct draw_vertex_shader *base,
		   const float (*input)[4],
		   float (*output)[4],
		   const float (*constants)[4],
		   unsigned count,
		   unsigned input_stride,
		   unsigned output_stride )
{
   struct draw_llvm_vertex_shader *shader =
      (struct draw_llvm_vertex_shader *)base;

   gallivm_cpu_vs_exec(shader->llvm_prog, shader->machine,
                       input, base->info.num_inputs, output, base->info.num_outputs,
                       constants, count, input_stride, output_stride);
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

   vs = CALLOC_STRUCT( draw_llvm_vertex_shader );
   if (vs == NULL)
      return NULL;

   /* we make a private copy of the tokens */
   vs->base.state.tokens = tgsi_dup_tokens(templ->tokens);
   if (!vs->base.state.tokens) {
      FREE(vs);
      return NULL;
   }

   tgsi_scan_shader(vs->base.state.tokens, &vs->base.info);

   vs->base.draw = draw;
   vs->base.prepare = vs_llvm_prepare;
   vs->base.create_varient = draw_vs_varient_generic;
   vs->base.run_linear = vs_llvm_run_linear;
   vs->base.delete = vs_llvm_delete;
   vs->machine = &draw->vs.machine;

   {
      struct gallivm_ir *ir = gallivm_ir_new(GALLIVM_VS);
      gallivm_ir_set_layout(ir, GALLIVM_SOA);
      gallivm_ir_set_components(ir, 4);
      gallivm_ir_fill_from_tgsi(ir, vs->base.state.tokens);
      vs->llvm_prog = gallivm_ir_compile(ir);
      gallivm_ir_delete(ir);
   }

   draw->vs.engine = gallivm_global_cpu_engine();

   /* XXX: Why are there two versions of this?  Shouldn't creating the
    *      engine be a separate operation to compiling a shader?
    */
   if (!draw->vs.engine) {
      draw->vs.engine = gallivm_cpu_engine_create(vs->llvm_prog);
   }
   else {
      gallivm_cpu_jit_compile(draw->vs.engine, vs->llvm_prog);
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
