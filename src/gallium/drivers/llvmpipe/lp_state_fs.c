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

#include "lp_context.h"
#include "lp_state.h"
#include "lp_fs.h"

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"


void *
llvmpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *state;

   /* debug */
   if (llvmpipe->dump_fs) 
      tgsi_dump(templ->tokens, 0);

   /* codegen */
   state = llvmpipe_create_fs_llvm( llvmpipe, templ );
   if (!state) {
      state = llvmpipe_create_fs_sse( llvmpipe, templ );
      if (!state) {
         state = llvmpipe_create_fs_exec( llvmpipe, templ );
      }
   }

   assert(state);

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &state->info);

   return state;
}


void
llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->fs = (struct lp_fragment_shader *) fs;

   llvmpipe->dirty |= LP_NEW_FS;
}


void
llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct lp_fragment_shader *state = fs;

   assert(fs != llvmpipe_context(pipe)->fs);
   
   state->delete( state );
}


void *
llvmpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_vertex_shader *state;

   state = CALLOC_STRUCT(lp_vertex_shader);
   if (state == NULL ) 
      goto fail;

   /* copy shader tokens, the ones passed in will go away.
    */
   state->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (state->shader.tokens == NULL)
      goto fail;

   state->draw_data = draw_create_vertex_shader(llvmpipe->draw, templ);
   if (state->draw_data == NULL) 
      goto fail;

   return state;

fail:
   if (state) {
      FREE( (void *)state->shader.tokens );
      FREE( state->draw_data );
      FREE( state );
   }
   return NULL;
}


void
llvmpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->vs = (const struct lp_vertex_shader *)vs;

   draw_bind_vertex_shader(llvmpipe->draw,
                           (llvmpipe->vs ? llvmpipe->vs->draw_data : NULL));

   llvmpipe->dirty |= LP_NEW_VS;
}


void
llvmpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   struct lp_vertex_shader *state =
      (struct lp_vertex_shader *)vs;

   draw_delete_vertex_shader(llvmpipe->draw, state->draw_data);
   FREE( state );
}



void
llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             const struct pipe_constant_buffer *buf)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   /* note: reference counting */
   pipe_buffer_reference(&llvmpipe->constants[shader].buffer,
			 buf ? buf->buffer : NULL);

   llvmpipe->dirty |= LP_NEW_CONSTANTS;
}
