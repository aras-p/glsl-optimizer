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

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "tnl/t_vp_build.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/exec/tgsi_core.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_program.h"


#define TGSI_DEBUG 1




/* translate shader to TGSI format 
*/
static void compile_vs( struct st_context *st )
{
   struct st_vertex_program *vp = st->vp;

   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_vp_program( &vp->Base, vp->tokens, ST_FP_MAX_TOKENS );

   vp->vs.inputs_read
      = tgsi_mesa_translate_vertex_input_mask(vp->Base.Base.InputsRead);
   vp->vs.outputs_written
      = tgsi_mesa_translate_vertex_output_mask(vp->Base.Base.OutputsWritten);
   vp->vs.tokens = &vp->tokens[0];

   if (TGSI_DEBUG)
      tgsi_dump( vp->tokens, 0 );

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   tgsi_emit_sse2(
      vp->vs.tokens,
      &vp->sse2_program );
#endif

   vp->dirty = 0;
}



static void update_vs( struct st_context *st )
{
   struct st_vertex_program *vp;

   /* find active shader and params -- Should be covered by
    * ST_NEW_VERTEX_PROGRAM
    */
   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->VertexProgram) {
      struct gl_vertex_program *f
         = st->ctx->Shader.CurrentProgram->VertexProgram;
      vp = st_vertex_program(f);
   }
   else {
      assert(st->ctx->VertexProgram._Current);
      vp = st_vertex_program(st->ctx->VertexProgram._Current);
   }

   if (st->vp != vp || vp->dirty) {
      st->vp = vp;

      if (vp->dirty) 
	 compile_vs( st );

#if defined(USE_X86_ASM) || defined(SLANG_X86)
      st->vp->vs.executable = (void *) x86_get_func( &vp->sse2_program );
#endif

      st->state.vs = st->vp->vs;
      st->pipe->set_vs_state(st->pipe, &st->state.vs);
   }
}


const struct st_tracked_state st_update_vs = {
   .name = "st_update_vs",
   .dirty = {
      .mesa  = 0, 
      .st   = ST_NEW_VERTEX_PROGRAM,
   },
   .update = update_vs
};




