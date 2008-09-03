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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_fs.h"

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"


void *
softpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_fragment_shader *state;

   /* debug */
   if (softpipe->dump_fs) 
      tgsi_dump(templ->tokens, 0);

   /* codegen */
   state = softpipe_create_fs_llvm( softpipe, templ );
   if (!state) {
      state = softpipe_create_fs_sse( softpipe, templ );
      if (!state) {
         state = softpipe_create_fs_exec( softpipe, templ );
      }
   }

   assert(state);

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &state->info);

   return state;
}


void
softpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->fs = (struct sp_fragment_shader *) fs;

   softpipe->dirty |= SP_NEW_FS;
}


void
softpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct sp_fragment_shader *state = fs;

   assert(fs != softpipe_context(pipe)->fs);
   
   state->delete( state );
}


void *
softpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_vertex_shader *state;

   state = CALLOC_STRUCT(sp_vertex_shader);
   if (state == NULL ) {
      return NULL;
   }

   state->draw_data = draw_create_vertex_shader(softpipe->draw, templ);
   if (state->draw_data == NULL) {
      FREE( state );
      return NULL;
   }

   return state;
}


void
softpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->vs = (const struct sp_vertex_shader *)vs;

   draw_bind_vertex_shader(softpipe->draw,
                           (softpipe->vs ? softpipe->vs->draw_data : NULL));

   softpipe->dirty |= SP_NEW_VS;
}


void
softpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   struct sp_vertex_shader *state =
      (struct sp_vertex_shader *)vs;

   draw_delete_vertex_shader(softpipe->draw, state->draw_data);
   FREE( state );
}



void
softpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             const struct pipe_constant_buffer *buf)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct pipe_winsys *ws = pipe->winsys;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   /* note: reference counting */
   winsys_buffer_reference(ws,
			 &softpipe->constants[shader].buffer,
			 buf ? buf->buffer : NULL);
   softpipe->constants[shader].size = buf ? buf->size : 0;

   softpipe->dirty |= SP_NEW_CONSTANTS;
}
