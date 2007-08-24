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


#define TGSI_DEBUG 0

static void compile_vs( struct st_context *st,
			struct st_vertex_program *vs )
{
   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_vp_program( &vs->Base, vs->tokens, ST_FP_MAX_TOKENS );

   if (TGSI_DEBUG)
      tgsi_dump( vs->tokens, TGSI_DUMP_VERBOSE );

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   tgsi_emit_sse2(
      vs->tokens,
      &vs->sse2_program );
#endif
}


static void
update_vs_constants(struct st_context *st,
                    struct gl_program_parameter_list *params)

{
   const uint paramBytes = params->NumParameters * sizeof(GLfloat) * 4;
   struct pipe_winsys *ws = st->pipe->winsys;
   struct pipe_constant_buffer *cbuf
      = &st->state.constants[PIPE_SHADER_VERTEX];

   if (!cbuf->buffer)   
      cbuf->buffer = ws->buffer_create(ws, 1);

   /* load Mesa constants into the constant buffer */
   if (paramBytes)
      ws->buffer_data(ws, cbuf->buffer, paramBytes, params->ParameterValues);

   cbuf->size = paramBytes;

   st->pipe->set_constant_buffer(st->pipe, PIPE_SHADER_VERTEX, 0, cbuf);
}


static void update_vs( struct st_context *st )
{
   struct pipe_shader_state vs;
   struct st_vertex_program *vp = NULL;
   struct gl_program_parameter_list *params = NULL;

   /* find active shader and params */
   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->VertexProgram) {
      struct gl_vertex_program *f
         = st->ctx->Shader.CurrentProgram->VertexProgram;
      vp = st_vertex_program(f);
      params = f->Base.Parameters;
   }
   else if (st->ctx->VertexProgram._Current) {
      vp = st_vertex_program(st->ctx->VertexProgram._Current);
      params = st->ctx->VertexProgram._Current->Base.Parameters;
   }

   /* update constants */
   if (vp && params) {
      _mesa_load_state_parameters(st->ctx, params);
      /*_mesa_print_parameter_list(params);*/
      update_vs_constants(st, params);
   }

   /* translate shader to TGSI format */
   if (vp->dirty)
      compile_vs( st, vp );

   /* update pipe state */
   memset( &vs, 0, sizeof(vs) );
   vs.inputs_read
      = tgsi_mesa_translate_vertex_input_mask(vp->Base.Base.InputsRead);
   vs.outputs_written
      = tgsi_mesa_translate_vertex_output_mask(vp->Base.Base.OutputsWritten);
   vs.tokens = &vp->tokens[0];

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   vs.executable = (void *) x86_get_func( &vp->sse2_program );
#endif

   if (memcmp(&vs, &st->state.vs, sizeof(vs)) != 0 ||
       vp->dirty) 
   {
      vp->dirty = 0;
      st->state.vs = vs;
      st->pipe->set_vs_state(st->pipe, &vs);
   }
}


const struct st_tracked_state st_update_vs = {
   .dirty = {
      .mesa  = (_NEW_PROGRAM |
                _NEW_MODELVIEW |
                _NEW_PROJECTION |
                _NEW_LIGHT), /*XXX MORE?*/
      .st   = ST_NEW_VERTEX_PROGRAM,
   },
   .update = update_vs
};





/**
 * When TnL state has changed, need to generate new vertex program.
 * This should be done before updating the vertes shader (vs) state.
 */
static void update_tnl( struct st_context *st )
{
   uint before = st->ctx->NewState;
   if (st->ctx->VertexProgram._MaintainTnlProgram)
      _tnl_UpdateFixedFunctionProgram( st->ctx );
   assert(before == st->ctx->NewState);
}


const struct st_tracked_state st_update_tnl = {
   .dirty = {
      .mesa  = (_NEW_PROGRAM |
                _NEW_LIGHT |
                _NEW_TEXTURE |
                _NEW_TRANSFORM |
                _NEW_LIGHT), /* XXX more? */
      .st   = ST_NEW_MESA,  /* XXX correct? */
   },
   .update = update_tnl
};
