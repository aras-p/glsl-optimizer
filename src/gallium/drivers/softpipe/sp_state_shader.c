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
#include "sp_texture.h"

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "draw/draw_context.h"
#include "draw/draw_vs.h"
#include "draw/draw_gs.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"


static void *
softpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_fragment_shader *state;
   unsigned i;

   /* debug */
   if (softpipe->dump_fs) 
      tgsi_dump(templ->tokens, 0);

   /* codegen */
   state = softpipe_create_fs_sse( softpipe, templ );
   if (!state) {
      state = softpipe_create_fs_exec( softpipe, templ );
   }

   if (!state)
      return NULL;

   /* draw's fs state */
   state->draw_shader = draw_create_fragment_shader(softpipe->draw, templ);
   if (!state->draw_shader) {
      state->delete( state );
      return NULL;
   }

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &state->info);

   for (i = 0; i < state->info.num_properties; ++i) {
      if (state->info.properties[i].name == TGSI_PROPERTY_FS_COORD_ORIGIN)
         state->origin_lower_left = state->info.properties[i].data[0];
      else if (state->info.properties[i].name == TGSI_PROPERTY_FS_COORD_PIXEL_CENTER)
	 state->pixel_center_integer = state->info.properties[i].data[0];
      else if (state->info.properties[i].name == TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS)
	 state->color0_writes_all_cbufs = state->info.properties[i].data[0];
   }

   return state;
}


static void
softpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   if (softpipe->fs == fs)
      return;

   draw_flush(softpipe->draw);

   softpipe->fs = fs;

   draw_bind_fragment_shader(softpipe->draw,
                             (softpipe->fs ? softpipe->fs->draw_shader : NULL));

   softpipe->dirty |= SP_NEW_FS;
}


static void
softpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_fragment_shader *state = fs;

   assert(fs != softpipe_context(pipe)->fs);

   if (softpipe->fs_machine->Tokens == state->shader.tokens) {
      /* unbind the shader from the tgsi executor if we're
       * deleting it.
       */
      tgsi_exec_machine_bind_shader(softpipe->fs_machine, NULL, 0, NULL);
   }

   draw_delete_fragment_shader(softpipe->draw, state->draw_shader);

   state->delete( state );
}


static void *
softpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_vertex_shader *state;

   state = CALLOC_STRUCT(sp_vertex_shader);
   if (state == NULL ) 
      goto fail;

   /* copy shader tokens, the ones passed in will go away.
    */
   state->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (state->shader.tokens == NULL)
      goto fail;

   state->draw_data = draw_create_vertex_shader(softpipe->draw, templ);
   if (state->draw_data == NULL) 
      goto fail;

   state->max_sampler = state->draw_data->info.file_max[TGSI_FILE_SAMPLER];

   return state;

fail:
   if (state) {
      FREE( (void *)state->shader.tokens );
      FREE( state->draw_data );
      FREE( state );
   }
   return NULL;
}


static void
softpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->vs = (struct sp_vertex_shader *) vs;

   draw_bind_vertex_shader(softpipe->draw,
                           (softpipe->vs ? softpipe->vs->draw_data : NULL));

   softpipe->dirty |= SP_NEW_VS;
}


static void
softpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   struct sp_vertex_shader *state = (struct sp_vertex_shader *) vs;

   draw_delete_vertex_shader(softpipe->draw, state->draw_data);
   FREE( (void *)state->shader.tokens );
   FREE( state );
}


static void *
softpipe_create_gs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_geometry_shader *state;

   state = CALLOC_STRUCT(sp_geometry_shader);
   if (state == NULL )
      goto fail;

   /* debug */
   if (softpipe->dump_gs)
      tgsi_dump(templ->tokens, 0);

   /* copy shader tokens, the ones passed in will go away.
    */
   state->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (state->shader.tokens == NULL)
      goto fail;

   state->draw_data = draw_create_geometry_shader(softpipe->draw, templ);
   if (state->draw_data == NULL)
      goto fail;

   state->max_sampler = state->draw_data->info.file_max[TGSI_FILE_SAMPLER];

   return state;

fail:
   if (state) {
      FREE( (void *)state->shader.tokens );
      FREE( state->draw_data );
      FREE( state );
   }
   return NULL;
}


static void
softpipe_bind_gs_state(struct pipe_context *pipe, void *gs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->gs = (struct sp_geometry_shader *)gs;

   draw_bind_geometry_shader(softpipe->draw,
                             (softpipe->gs ? softpipe->gs->draw_data : NULL));

   softpipe->dirty |= SP_NEW_GS;
}


static void
softpipe_delete_gs_state(struct pipe_context *pipe, void *gs)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   struct sp_geometry_shader *state =
      (struct sp_geometry_shader *)gs;

   draw_delete_geometry_shader(softpipe->draw,
                               (state) ? state->draw_data : 0);
   FREE(state);
}


static void
softpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_resource *constants)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned size = constants ? constants->width0 : 0;
   const void *data = constants ? softpipe_resource(constants)->data : NULL;

   assert(shader < PIPE_SHADER_TYPES);

   draw_flush(softpipe->draw);

   /* note: reference counting */
   pipe_resource_reference(&softpipe->constants[shader][index], constants);

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_mapped_constant_buffer(softpipe->draw, shader, index, data, size);
   }

   softpipe->mapped_constants[shader][index] = data;
   softpipe->const_buffer_size[shader][index] = size;

   softpipe->dirty |= SP_NEW_CONSTANTS;
}


void
softpipe_init_shader_funcs(struct pipe_context *pipe)
{
   pipe->create_fs_state = softpipe_create_fs_state;
   pipe->bind_fs_state   = softpipe_bind_fs_state;
   pipe->delete_fs_state = softpipe_delete_fs_state;

   pipe->create_vs_state = softpipe_create_vs_state;
   pipe->bind_vs_state   = softpipe_bind_vs_state;
   pipe->delete_vs_state = softpipe_delete_vs_state;

   pipe->create_gs_state = softpipe_create_gs_state;
   pipe->bind_gs_state   = softpipe_bind_gs_state;
   pipe->delete_gs_state = softpipe_delete_gs_state;

   pipe->set_constant_buffer = softpipe_set_constant_buffer;
}
