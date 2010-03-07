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

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "cell_context.h"
#include "cell_gen_fragment.h"
#include "cell_state.h"
#include "cell_state_emit.h"
#include "cell_batch.h"
#include "cell_texture.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"


/**
 * Find/create a cell_command_fragment_ops object corresponding to the
 * current blend/stencil/z/colormask/etc. state.
 */
static struct cell_command_fragment_ops *
lookup_fragment_ops(struct cell_context *cell)
{
   struct cell_fragment_ops_key key;
   struct cell_command_fragment_ops *ops;

   /*
    * Build key
    */
   memset(&key, 0, sizeof(key));
   key.blend = *cell->blend;
   key.blend_color = cell->blend_color;
   key.dsa = *cell->depth_stencil;

   if (cell->framebuffer.cbufs[0])
      key.color_format = cell->framebuffer.cbufs[0]->format;
   else
      key.color_format = PIPE_FORMAT_NONE;

   if (cell->framebuffer.zsbuf)
      key.zs_format = cell->framebuffer.zsbuf->format;
   else
      key.zs_format = PIPE_FORMAT_NONE;

   /*
    * Look up key in cache.
    */
   ops = (struct cell_command_fragment_ops *)
      util_keymap_lookup(cell->fragment_ops_cache, &key);

   /*
    * If not found, create/save new fragment ops command.
    */
   if (!ops) {
      struct spe_function spe_code_front, spe_code_back;
      unsigned int facing_dependent, total_code_size;

      if (0)
         debug_printf("**** Create New Fragment Ops\n");

      /* Prepare the buffer that will hold the generated code.  The
       * "0" passed in for the size means that the SPE code will
       * use a default size.
       */
      spe_init_func(&spe_code_front, 0);
      spe_init_func(&spe_code_back, 0);

      /* Generate new code.  Always generate new code for both front-facing
       * and back-facing fragments, even if it's the same code in both
       * cases.
       */
      cell_gen_fragment_function(cell, CELL_FACING_FRONT, &spe_code_front);
      cell_gen_fragment_function(cell, CELL_FACING_BACK, &spe_code_back);

      /* Make sure the code is a multiple of 8 bytes long; this is
       * required to ensure that the dual pipe instruction alignment
       * is correct.  It's also important for the SPU unpacking,
       * which assumes 8-byte boundaries.
       */
      unsigned int front_code_size = spe_code_size(&spe_code_front);
      while (front_code_size % 8 != 0) {
         spe_lnop(&spe_code_front);
         front_code_size = spe_code_size(&spe_code_front);
      }
      unsigned int back_code_size = spe_code_size(&spe_code_back);
      while (back_code_size % 8 != 0) {
         spe_lnop(&spe_code_back);
         back_code_size = spe_code_size(&spe_code_back);
      }

      /* Determine whether the code we generated is facing-dependent, by
       * determining whether the generated code is different for the front-
       * and back-facing fragments.
       */
      if (front_code_size == back_code_size && memcmp(spe_code_front.store, spe_code_back.store, front_code_size) == 0) {
         /* Code is identical; only need one copy. */
         facing_dependent = 0;
         total_code_size = front_code_size;
      }
      else {
         /* Code is different for front-facing and back-facing fragments.
          * Need to send both copies.
          */
         facing_dependent = 1;
         total_code_size = front_code_size + back_code_size;
      }

      /* alloc new fragment ops command.  Note that this structure
       * has variant length based on the total code size required.
       */
      ops = CALLOC_VARIANT_LENGTH_STRUCT(cell_command_fragment_ops, total_code_size);
      /* populate the new cell_command_fragment_ops object */
      ops->opcode[0] = CELL_CMD_STATE_FRAGMENT_OPS;
      ops->total_code_size = total_code_size;
      ops->front_code_index = 0;
      memcpy(ops->code, spe_code_front.store, front_code_size);
      if (facing_dependent) {
        /* We have separate front- and back-facing code.  Append the
         * back-facing code to the buffer.  Be careful because the code
         * size is in bytes, but the buffer is of unsigned elements.
         */
        ops->back_code_index = front_code_size / sizeof(spe_code_front.store[0]);
        memcpy(ops->code + ops->back_code_index, spe_code_back.store, back_code_size);
      }
      else {
        /* Use the same code for front- and back-facing fragments */
        ops->back_code_index = ops->front_code_index;
      }

      /* Set the fields for the fallback case.  Note that these fields
       * (and the whole fallback case) will eventually go away.
       */
      ops->dsa = *cell->depth_stencil;
      ops->blend = *cell->blend;
      ops->blend_color = cell->blend_color;

      /* insert cell_command_fragment_ops object into keymap/cache */
      util_keymap_insert(cell->fragment_ops_cache, &key, ops, NULL);

      /* release rtasm buffer */
      spe_release_func(&spe_code_front);
      spe_release_func(&spe_code_back);
   }
   else {
      if (0)
         debug_printf("**** Re-use Fragment Ops\n");
   }

   return ops;
}



static void
emit_state_cmd(struct cell_context *cell, uint cmd,
               const void *state, uint state_size)
{
   uint32_t *dst = (uint32_t *) 
       cell_batch_alloc16(cell, ROUNDUP16(sizeof(opcode_t) + state_size));
   *dst = cmd;
   memcpy(dst + 4, state, state_size);
}


/**
 * For state marked as 'dirty', construct a state-update command block
 * and insert it into the current batch buffer.
 */
void
cell_emit_state(struct cell_context *cell)
{
   if (cell->dirty & CELL_NEW_FRAMEBUFFER) {
      struct pipe_surface *cbuf = cell->framebuffer.cbufs[0];
      struct pipe_surface *zbuf = cell->framebuffer.zsbuf;
      STATIC_ASSERT(sizeof(struct cell_command_framebuffer) % 16 == 0);
      struct cell_command_framebuffer *fb
         = cell_batch_alloc16(cell, sizeof(*fb));
      fb->opcode[0] = CELL_CMD_STATE_FRAMEBUFFER;
      fb->color_start = cell->cbuf_map[0];
      fb->color_format = cbuf->format;
      fb->depth_start = cell->zsbuf_map;
      fb->depth_format = zbuf ? zbuf->format : PIPE_FORMAT_NONE;
      fb->width = cell->framebuffer.width;
      fb->height = cell->framebuffer.height;
#if 0
      printf("EMIT color format %s\n", util_format_name(fb->color_format));
      printf("EMIT depth format %s\n", util_format_name(fb->depth_format));
#endif
   }

   if (cell->dirty & (CELL_NEW_RASTERIZER)) {
      STATIC_ASSERT(sizeof(struct cell_command_rasterizer) % 16 == 0);
      struct cell_command_rasterizer *rast =
         cell_batch_alloc16(cell, sizeof(*rast));
      rast->opcode[0] = CELL_CMD_STATE_RASTERIZER;
      rast->rasterizer = *cell->rasterizer;
   }

   if (cell->dirty & (CELL_NEW_FS)) {
      /* Send new fragment program to SPUs */
      STATIC_ASSERT(sizeof(struct cell_command_fragment_program) % 16 == 0);
      struct cell_command_fragment_program *fp
            = cell_batch_alloc16(cell, sizeof(*fp));
      fp->opcode[0] = CELL_CMD_STATE_FRAGMENT_PROGRAM;
      fp->num_inst = cell->fs->code.num_inst;
      memcpy(&fp->code, cell->fs->code.store,
             SPU_MAX_FRAGMENT_PROGRAM_INSTS * SPE_INST_SIZE);
      if (0) {
         int i;
         printf("PPU Emit CELL_CMD_STATE_FRAGMENT_PROGRAM:\n");
         for (i = 0; i < fp->num_inst; i++) {
            printf(" %3d: 0x%08x\n", i, fp->code[i]);
         }
      }
   }

   if (cell->dirty & (CELL_NEW_FS_CONSTANTS)) {
      const uint shader = PIPE_SHADER_FRAGMENT;
      const uint num_const = cell->constants[shader]->size / sizeof(float);
      uint i, j;
      float *buf = cell_batch_alloc16(cell, ROUNDUP16(32 + num_const * sizeof(float)));
      uint32_t *ibuf = (uint32_t *) buf;
      const float *constants = pipe_buffer_map(cell->pipe.screen,
                                               cell->constants[shader],
                                               PIPE_BUFFER_USAGE_CPU_READ);
      ibuf[0] = CELL_CMD_STATE_FS_CONSTANTS;
      ibuf[4] = num_const;
      j = 8;
      for (i = 0; i < num_const; i++) {
         buf[j++] = constants[i];
      }
      pipe_buffer_unmap(cell->pipe.screen, cell->constants[shader]);
   }

   if (cell->dirty & (CELL_NEW_FRAMEBUFFER |
                      CELL_NEW_DEPTH_STENCIL |
                      CELL_NEW_BLEND)) {
      struct cell_command_fragment_ops *fops, *fops_cmd;
      /* Note that cell_command_fragment_ops is a variant-sized record */
      fops = lookup_fragment_ops(cell);
      fops_cmd = cell_batch_alloc16(cell, ROUNDUP16(sizeof(*fops_cmd) + fops->total_code_size));
      memcpy(fops_cmd, fops, sizeof(*fops) + fops->total_code_size);
   }

   if (cell->dirty & CELL_NEW_SAMPLER) {
      uint i;
      for (i = 0; i < CELL_MAX_SAMPLERS; i++) {
         if (cell->dirty_samplers & (1 << i)) {
            if (cell->sampler[i]) {
               STATIC_ASSERT(sizeof(struct cell_command_sampler) % 16 == 0);
               struct cell_command_sampler *sampler
                  = cell_batch_alloc16(cell, sizeof(*sampler));
               sampler->opcode[0] = CELL_CMD_STATE_SAMPLER;
               sampler->unit = i;
               sampler->state = *cell->sampler[i];
            }
         }
      }
      cell->dirty_samplers = 0x0;
   }

   if (cell->dirty & CELL_NEW_TEXTURE) {
      uint i;
      for (i = 0;i < CELL_MAX_SAMPLERS; i++) {
         if (cell->dirty_textures & (1 << i)) {
            STATIC_ASSERT(sizeof(struct cell_command_texture) % 16 == 0);
            struct cell_command_texture *texture =
               (struct cell_command_texture *)
               cell_batch_alloc16(cell, sizeof(*texture));

            texture->opcode[0] = CELL_CMD_STATE_TEXTURE;
            texture->unit = i;
            if (cell->texture[i]) {
               struct cell_texture *ct = cell->texture[i];
               uint level;
               for (level = 0; level < CELL_MAX_TEXTURE_LEVELS; level++) {
                  texture->start[level] = (ct->mapped +
                                           ct->level_offset[level]);
                  texture->width[level] = u_minify(ct->base.width0, level);
                  texture->height[level] = u_minify(ct->base.height0, level);
                  texture->depth[level] = u_minify(ct->base.depth0, level);
               }
               texture->target = ct->base.target;
            }
            else {
               uint level;
               for (level = 0; level < CELL_MAX_TEXTURE_LEVELS; level++) {
                  texture->start[level] = NULL;
                  texture->width[level] = 0;
                  texture->height[level] = 0;
                  texture->depth[level] = 0;
               }
               texture->target = 0;
            }
         }
      }
      cell->dirty_textures = 0x0;
   }

   if (cell->dirty & CELL_NEW_VERTEX_INFO) {
      emit_state_cmd(cell, CELL_CMD_STATE_VERTEX_INFO,
                     &cell->vertex_info, sizeof(struct vertex_info));
   }

#if 0
   if (cell->dirty & CELL_NEW_VS) {
      const struct draw_context *const draw = cell->draw;
      struct cell_shader_info info;

      info.num_outputs = draw_num_shader_outputs(draw);
      info.declarations = (uintptr_t) draw->vs.machine.Declarations;
      info.num_declarations = draw->vs.machine.NumDeclarations;
      info.instructions = (uintptr_t) draw->vs.machine.Instructions;
      info.num_instructions = draw->vs.machine.NumInstructions;
      info.immediates = (uintptr_t) draw->vs.machine.Imms;
      info.num_immediates = draw->vs.machine.ImmLimit / 4;

      emit_state_cmd(cell, CELL_CMD_STATE_BIND_VS, &info, sizeof(info));
   }
#endif
}
