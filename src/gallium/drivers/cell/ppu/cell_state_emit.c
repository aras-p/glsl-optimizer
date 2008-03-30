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

#include "pipe/p_util.h"
#include "cell_context.h"
#include "cell_state.h"
#include "cell_state_emit.h"
#include "cell_state_per_fragment.h"
#include "cell_batch.h"
#include "cell_texture.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"


static void
emit_state_cmd(struct cell_context *cell, uint cmd,
               const void *state, uint state_size)
{
   uint64_t *dst = (uint64_t *) 
       cell_batch_alloc(cell, ROUNDUP8(sizeof(uint64_t) + state_size));
   *dst = cmd;
   memcpy(dst + 1, state, state_size);
}



void
cell_emit_state(struct cell_context *cell)
{
   if (cell->dirty & (CELL_NEW_FRAMEBUFFER | CELL_NEW_BLEND)) {
      struct cell_command_logicop logicop;

      if (cell->logic_op.store != NULL) {
	 spe_release_func(& cell->logic_op);
      }

      cell_generate_logic_op(& cell->logic_op,
			     & cell->blend->base,
			     cell->framebuffer.cbufs[0]);

      logicop.base = (intptr_t) cell->logic_op.store;
      logicop.size = 64 * 4;
      emit_state_cmd(cell, CELL_CMD_STATE_LOGICOP, &logicop,
		     sizeof(logicop));
   }

   if (cell->dirty & CELL_NEW_FRAMEBUFFER) {
      struct pipe_surface *cbuf = cell->framebuffer.cbufs[0];
      struct pipe_surface *zbuf = cell->framebuffer.zsbuf;
      struct cell_command_framebuffer *fb
         = cell_batch_alloc(cell, sizeof(*fb));
      fb->opcode = CELL_CMD_STATE_FRAMEBUFFER;
      fb->color_start = cell->cbuf_map[0];
      fb->color_format = cbuf->format;
      fb->depth_start = cell->zsbuf_map;
      fb->depth_format = zbuf ? zbuf->format : PIPE_FORMAT_NONE;
      fb->width = cell->framebuffer.width;
      fb->height = cell->framebuffer.height;
   }

   if (cell->dirty & CELL_NEW_BLEND) {
      struct cell_command_blend blend;

      if (cell->blend != NULL) {
         blend.base = (intptr_t) cell->blend->code.store;
         blend.size = (char *) cell->blend->code.csr
             - (char *) cell->blend->code.store;
         blend.read_fb = TRUE;
      } else {
         blend.base = 0;
         blend.size = 0;
         blend.read_fb = FALSE;
      }

      emit_state_cmd(cell, CELL_CMD_STATE_BLEND, &blend, sizeof(blend));
   }

   if (cell->dirty & CELL_NEW_DEPTH_STENCIL) {
      struct cell_command_depth_stencil_alpha_test dsat;
      

      if (cell->depth_stencil != NULL) {
	 dsat.base = (intptr_t) cell->depth_stencil->code.store;
	 dsat.size = (char *) cell->depth_stencil->code.csr
	     - (char *) cell->depth_stencil->code.store;
	 dsat.read_depth = TRUE;
	 dsat.read_stencil = FALSE;
      } else {
	 dsat.base = 0;
	 dsat.size = 0;
	 dsat.read_depth = FALSE;
	 dsat.read_stencil = FALSE;
      }

      emit_state_cmd(cell, CELL_CMD_STATE_DEPTH_STENCIL, &dsat,
		     sizeof(dsat));
   }

   if (cell->dirty & CELL_NEW_SAMPLER) {
      if (cell->sampler[0]) {
         emit_state_cmd(cell, CELL_CMD_STATE_SAMPLER,
                        cell->sampler[0], sizeof(struct pipe_sampler_state));
      }
   }

   if (cell->dirty & CELL_NEW_TEXTURE) {
      struct cell_command_texture texture;
      if (cell->texture[0]) {
         texture.start = cell->texture[0]->tiled_data;
         texture.width = cell->texture[0]->base.width[0];
         texture.height = cell->texture[0]->base.height[0];
      }
      else {
         texture.start = NULL;
         texture.width = 0;
         texture.height = 0;
      }

      emit_state_cmd(cell, CELL_CMD_STATE_TEXTURE,
                     &texture, sizeof(struct cell_command_texture));
   }

   if (cell->dirty & CELL_NEW_VERTEX_INFO) {
      emit_state_cmd(cell, CELL_CMD_STATE_VERTEX_INFO,
                     &cell->vertex_info, sizeof(struct vertex_info));
   }
   
   if (cell->dirty & CELL_NEW_VS) {
      const struct draw_context *const draw = cell->draw;
      struct cell_shader_info info;

      info.num_outputs = draw->num_vs_outputs;
      info.declarations = (uintptr_t) draw->machine.Declarations;
      info.num_declarations = draw->machine.NumDeclarations;
      info.instructions = (uintptr_t) draw->machine.Instructions;
      info.num_instructions = draw->machine.NumInstructions;
      info.immediates = (uintptr_t) draw->machine.Imms;
      info.num_immediates = draw->machine.ImmLimit / 4;

      emit_state_cmd(cell, CELL_CMD_STATE_BIND_VS,
		     & info, sizeof(info));
   }
}
