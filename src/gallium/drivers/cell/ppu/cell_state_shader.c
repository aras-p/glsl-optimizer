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

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_simple_screen.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_parse.h"

#include "cell_context.h"
#include "cell_state.h"
#include "cell_gen_fp.h"


/** cast wrapper */
static INLINE struct cell_fragment_shader_state *
cell_fragment_shader_state(void *shader)
{
   return (struct cell_fragment_shader_state *) shader;
}


/** cast wrapper */
static INLINE struct cell_vertex_shader_state *
cell_vertex_shader_state(void *shader)
{
   return (struct cell_vertex_shader_state *) shader;
}


/**
 * Create fragment shader state.
 * Called via pipe->create_fs_state()
 */
static void *
cell_create_fs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct cell_context *cell = cell_context(pipe);
   struct cell_fragment_shader_state *cfs;

   cfs = CALLOC_STRUCT(cell_fragment_shader_state);
   if (!cfs)
      return NULL;

   cfs->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (!cfs->shader.tokens) {
      FREE(cfs);
      return NULL;
   }

   tgsi_scan_shader(templ->tokens, &cfs->info);

   cell_gen_fragment_program(cell, cfs->shader.tokens, &cfs->code);

   return cfs;
}


/**
 * Called via pipe->bind_fs_state()
 */
static void
cell_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct cell_context *cell = cell_context(pipe);

   cell->fs = cell_fragment_shader_state(fs);

   cell->dirty |= CELL_NEW_FS;
}


/**
 * Called via pipe->delete_fs_state()
 */
static void
cell_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct cell_fragment_shader_state *cfs = cell_fragment_shader_state(fs);

   spe_release_func(&cfs->code);

   FREE((void *) cfs->shader.tokens);
   FREE(cfs);
}


/**
 * Create vertex shader state.
 * Called via pipe->create_vs_state()
 */
static void *
cell_create_vs_state(struct pipe_context *pipe,
                     const struct pipe_shader_state *templ)
{
   struct cell_context *cell = cell_context(pipe);
   struct cell_vertex_shader_state *cvs;

   cvs = CALLOC_STRUCT(cell_vertex_shader_state);
   if (!cvs)
      return NULL;

   cvs->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (!cvs->shader.tokens) {
      FREE(cvs);
      return NULL;
   }

   tgsi_scan_shader(templ->tokens, &cvs->info);

   cvs->draw_data = draw_create_vertex_shader(cell->draw, &cvs->shader);
   if (cvs->draw_data == NULL) {
      FREE( (void *) cvs->shader.tokens );
      FREE( cvs );
      return NULL;
   }

   return cvs;
}


/**
 * Called via pipe->bind_vs_state()
 */
static void
cell_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct cell_context *cell = cell_context(pipe);

   cell->vs = cell_vertex_shader_state(vs);

   draw_bind_vertex_shader(cell->draw,
                           (cell->vs ? cell->vs->draw_data : NULL));

   cell->dirty |= CELL_NEW_VS;
}


/**
 * Called via pipe->delete_vs_state()
 */
static void
cell_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct cell_context *cell = cell_context(pipe);
   struct cell_vertex_shader_state *cvs = cell_vertex_shader_state(vs);

   draw_delete_vertex_shader(cell->draw, cvs->draw_data);
   FREE( (void *) cvs->shader.tokens );
   FREE( cvs );
}


/**
 * Called via pipe->set_constant_buffer()
 */
static void
cell_set_constant_buffer(struct pipe_context *pipe,
                         uint shader, uint index,
                         struct pipe_buffer *buf)
{
   struct cell_context *cell = cell_context(pipe);

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   draw_flush(cell->draw);

   /* note: reference counting */
   pipe_buffer_reference(&cell->constants[shader], buf);

   if (shader == PIPE_SHADER_VERTEX)
      cell->dirty |= CELL_NEW_VS_CONSTANTS;
   else if (shader == PIPE_SHADER_FRAGMENT)
      cell->dirty |= CELL_NEW_FS_CONSTANTS;
}


void
cell_init_shader_functions(struct cell_context *cell)
{
   cell->pipe.create_fs_state = cell_create_fs_state;
   cell->pipe.bind_fs_state   = cell_bind_fs_state;
   cell->pipe.delete_fs_state = cell_delete_fs_state;

   cell->pipe.create_vs_state = cell_create_vs_state;
   cell->pipe.bind_vs_state   = cell_bind_vs_state;
   cell->pipe.delete_vs_state = cell_delete_vs_state;

   cell->pipe.set_constant_buffer = cell_set_constant_buffer;
}
