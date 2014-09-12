/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_BUILDER_RENDER_H
#define ILO_BUILDER_RENDER_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_builder.h"

static inline void
gen6_STATE_BASE_ADDRESS(struct ilo_builder *builder,
                        struct intel_bo *general_state_bo,
                        struct intel_bo *surface_state_bo,
                        struct intel_bo *dynamic_state_bo,
                        struct intel_bo *indirect_object_bo,
                        struct intel_bo *instruction_bo,
                        uint32_t general_state_size,
                        uint32_t dynamic_state_size,
                        uint32_t indirect_object_size,
                        uint32_t instruction_size)
{
   const uint8_t cmd_len = 10;
   const uint32_t dw0 = GEN6_RENDER_CMD(COMMON, STATE_BASE_ADDRESS) |
                        (cmd_len - 2);
   unsigned pos;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /* 4K-page aligned */
   assert(((general_state_size | dynamic_state_size |
            indirect_object_size | instruction_size) & 0xfff) == 0);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;

   dw[1] = 1;
   dw[2] = 1;
   dw[3] = 1;
   dw[4] = 1;
   dw[5] = 1;

   /* skip range checks */
   dw[6] = 1;
   dw[7] = 0xfffff000 + 1;
   dw[8] = 0xfffff000 + 1;
   dw[9] = 1;

   if (general_state_bo) {
      ilo_builder_batch_reloc(builder, pos + 1, general_state_bo, 1, 0);

      if (general_state_size) {
         ilo_builder_batch_reloc(builder, pos + 6, general_state_bo,
               general_state_size | 1, 0);
      }
   }

   if (surface_state_bo)
      ilo_builder_batch_reloc(builder, pos + 2, surface_state_bo, 1, 0);

   if (dynamic_state_bo) {
      ilo_builder_batch_reloc(builder, pos + 3, dynamic_state_bo, 1, 0);

      if (dynamic_state_size) {
         ilo_builder_batch_reloc(builder, pos + 7, dynamic_state_bo,
               dynamic_state_size | 1, 0);
      }
   }

   if (indirect_object_bo) {
      ilo_builder_batch_reloc(builder, pos + 4, indirect_object_bo, 1, 0);

      if (indirect_object_size) {
         ilo_builder_batch_reloc(builder, pos + 8, indirect_object_bo,
               indirect_object_size | 1, 0);
      }
   }

   if (instruction_bo) {
      ilo_builder_batch_reloc(builder, pos + 5, instruction_bo, 1, 0);

      if (instruction_size) {
         ilo_builder_batch_reloc(builder, pos + 9, instruction_bo,
               instruction_size | 1, 0);
      }
   }
}

static inline void
gen6_STATE_SIP(struct ilo_builder *builder,
               uint32_t sip)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(COMMON, STATE_SIP) | (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = sip;
}

static inline void
gen6_PIPELINE_SELECT(struct ilo_builder *builder,
                     int pipeline)
{
   const uint8_t cmd_len = 1;
   const uint32_t dw0 = GEN6_RENDER_CMD(SINGLE_DW, PIPELINE_SELECT) |
                        pipeline;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /* 3D or media */
   assert(pipeline == 0x0 || pipeline == 0x1);

   ilo_builder_batch_write(builder, cmd_len, &dw0);
}

static inline void
gen6_PIPE_CONTROL(struct ilo_builder *builder,
                  uint32_t dw1,
                  struct intel_bo *bo, uint32_t bo_offset,
                  bool write_qword)
{
   const uint8_t cmd_len = (write_qword) ? 5 : 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, PIPE_CONTROL) | (cmd_len - 2);
   uint32_t reloc_flags = INTEL_RELOC_WRITE;
   unsigned pos;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(bo_offset % ((write_qword) ? 8 : 4) == 0);

   if (dw1 & GEN6_PIPE_CONTROL_CS_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "1 of the following must also be set (when CS stall is set):
       *
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Notify Enable ([8] of DW1)"
       *
       * From the Ivy Bridge PRM, volume 2 part 1, page 61:
       *
       *     "One of the following must also be set (when CS stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)"
       */
      uint32_t bit_test = GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                          GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                          GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL |
                          GEN6_PIPE_CONTROL_DEPTH_STALL;

      /* post-sync op */
      bit_test |= GEN6_PIPE_CONTROL_WRITE_IMM |
                  GEN6_PIPE_CONTROL_WRITE_PS_DEPTH_COUNT |
                  GEN6_PIPE_CONTROL_WRITE_TIMESTAMP;

      if (ilo_dev_gen(builder->dev) == ILO_GEN(6))
         bit_test |= GEN6_PIPE_CONTROL_NOTIFY_ENABLE;

      assert(dw1 & bit_test);
   }

   if (dw1 & GEN6_PIPE_CONTROL_DEPTH_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "Following bits must be clear (when Depth Stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)"
       */
      assert(!(dw1 & (GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                      GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH)));
   }

   /*
    * From the Sandy Bridge PRM, volume 1 part 3, page 19:
    *
    *     "[DevSNB] PPGTT memory writes by MI_* (such as MI_STORE_DATA_IMM)
    *      and PIPE_CONTROL are not supported."
    *
    * The kernel will add the mapping automatically (when write domain is
    * INTEL_DOMAIN_INSTRUCTION).
    */
   if (ilo_dev_gen(builder->dev) == ILO_GEN(6) && bo) {
      bo_offset |= GEN6_PIPE_CONTROL_DW2_USE_GGTT;
      reloc_flags |= INTEL_RELOC_GGTT;
   }

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;

   if (bo)
      ilo_builder_batch_reloc(builder, pos + 2, bo, bo_offset, reloc_flags);
   else
      dw[2] = 0;

   dw[3] = 0;
   if (write_qword)
      dw[4] = 0;
}

static inline void
ilo_builder_batch_patch_sba(struct ilo_builder *builder)
{
   const struct ilo_builder_writer *inst =
      &builder->writers[ILO_BUILDER_WRITER_INSTRUCTION];

   if (!builder->sba_instruction_pos)
      return;

   ilo_builder_batch_reloc(builder, builder->sba_instruction_pos,
         inst->bo, 1, 0);
}

/**
 * Add a STATE_BASE_ADDRESS to the batch buffer.
 */
static inline void
ilo_builder_batch_state_base_address(struct ilo_builder *builder,
                                     bool init_all)
{
   const uint8_t cmd_len = 10;
   const struct ilo_builder_writer *bat =
      &builder->writers[ILO_BUILDER_WRITER_BATCH];
   unsigned pos;
   uint32_t *dw;

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(COMMON, STATE_BASE_ADDRESS) | (cmd_len - 2);
   dw[1] = init_all;

   ilo_builder_batch_reloc(builder, pos + 2, bat->bo, 1, 0);
   ilo_builder_batch_reloc(builder, pos + 3, bat->bo, 1, 0);

   dw[4] = init_all;

   /*
    * Since the instruction writer has WRITER_FLAG_APPEND set, it is tempting
    * not to set Instruction Base Address.  The problem is that we do not know
    * if the bo has been or will be moved by the kernel.  We need a relocation
    * entry because of that.
    *
    * And since we also set WRITER_FLAG_GROW, we have to wait until
    * ilo_builder_end(), when the final bo is known, to add the relocation
    * entry.
    */
   ilo_builder_batch_patch_sba(builder);
   builder->sba_instruction_pos = pos + 5;

   /* skip range checks */
   dw[6] = init_all;
   dw[7] = 0xfffff000 + init_all;
   dw[8] = 0xfffff000 + init_all;
   dw[9] = init_all;
}

#endif /* ILO_BUILDER_RENDER_H */
