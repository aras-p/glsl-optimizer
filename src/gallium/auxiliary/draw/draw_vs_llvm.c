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

#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

#include "tgsi/tgsi_parse.h"

#ifdef MESA_LLVM

struct draw_llvm_vertex_shader {
   struct draw_vertex_shader base;
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
                   const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
		   unsigned count,
		   unsigned input_stride,
		   unsigned output_stride )
{
   struct draw_llvm_vertex_shader *shader =
      (struct draw_llvm_vertex_shader *)base;
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
   vs->machine = draw->vs.machine;

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
