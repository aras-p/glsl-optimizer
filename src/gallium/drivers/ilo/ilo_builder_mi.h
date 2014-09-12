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

#ifndef ILO_BUILDER_MI_H
#define ILO_BUILDER_MI_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_builder.h"

static inline void
gen6_MI_STORE_DATA_IMM(struct ilo_builder *builder,
                       struct intel_bo *bo, uint32_t bo_offset,
                       uint64_t val, bool store_qword)
{
   const uint8_t cmd_len = (store_qword) ? 5 : 4;
   uint32_t reloc_flags = INTEL_RELOC_WRITE;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_MI_CMD(MI_STORE_DATA_IMM) | (cmd_len - 2);
   /* must use GGTT on GEN6 as in PIPE_CONTROL */
   if (ilo_dev_gen(builder->dev) == ILO_GEN(6)) {
      dw[0] |= GEN6_MI_STORE_DATA_IMM_DW0_USE_GGTT;
      reloc_flags |= INTEL_RELOC_GGTT;
   }

   dw[1] = 0; /* MBZ */
   dw[3] = (uint32_t) val;

   if (store_qword) {
      assert(bo_offset % 8 == 0);
      dw[4] = (uint32_t) (val >> 32);
   } else {
      assert(bo_offset % 4 == 0);
      assert(val == (uint64_t) ((uint32_t) val));
   }

   ilo_builder_batch_reloc(builder, pos + 2, bo, bo_offset, reloc_flags);
}

static inline void
gen6_MI_LOAD_REGISTER_IMM(struct ilo_builder *builder,
                          uint32_t reg, uint32_t val)
{
   const uint8_t cmd_len = 3;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(reg % 4 == 0);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_MI_CMD(MI_LOAD_REGISTER_IMM) | (cmd_len - 2);
   dw[1] = reg;
   dw[2] = val;
}

static inline void
gen6_MI_STORE_REGISTER_MEM(struct ilo_builder *builder,
                           struct intel_bo *bo, uint32_t bo_offset,
                           uint32_t reg)
{
   const uint8_t cmd_len = 3;
   uint32_t reloc_flags = INTEL_RELOC_WRITE;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(reg % 4 == 0 && bo_offset % 4 == 0);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_MI_CMD(MI_STORE_REGISTER_MEM) | (cmd_len - 2);
   /* must use GGTT on GEN6 as in PIPE_CONTROL */
   if (ilo_dev_gen(builder->dev) == ILO_GEN(6)) {
      dw[0] |= GEN6_MI_STORE_REGISTER_MEM_DW0_USE_GGTT;
      reloc_flags |= INTEL_RELOC_GGTT;
   }

   dw[1] = reg;

   ilo_builder_batch_reloc(builder, pos + 2, bo, bo_offset, reloc_flags);
}

static inline void
gen6_MI_FLUSH_DW(struct ilo_builder *builder)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_MI_CMD(MI_FLUSH_DW) | (cmd_len - 2);
   dw[1] = 0;
   dw[2] = 0;
   dw[3] = 0;
}

static inline void
gen6_MI_REPORT_PERF_COUNT(struct ilo_builder *builder,
                          struct intel_bo *bo, uint32_t bo_offset,
                          uint32_t report_id)
{
   const uint8_t cmd_len = 3;
   uint32_t reloc_flags = INTEL_RELOC_WRITE;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(bo_offset % 64 == 0);

   /* must use GGTT on GEN6 as in PIPE_CONTROL */
   if (ilo_dev_gen(builder->dev) == ILO_GEN(6)) {
      bo_offset |= GEN6_MI_REPORT_PERF_COUNT_DW1_USE_GGTT;
      reloc_flags |= INTEL_RELOC_GGTT;
   }

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_MI_CMD(MI_REPORT_PERF_COUNT) | (cmd_len - 2);
   dw[2] = report_id;

   ilo_builder_batch_reloc(builder, pos + 1, bo, bo_offset, reloc_flags);
}

/**
 * Add a MI_BATCH_BUFFER_END to the batch buffer.  Pad with MI_NOOP if
 * necessary.
 */
static inline void
gen6_mi_batch_buffer_end(struct ilo_builder *builder)
{
   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 107:
    *
    *     "The batch buffer must be QWord aligned and a multiple of QWords in
    *      length."
    */
   const bool pad = !(builder->writers[ILO_BUILDER_WRITER_BATCH].used & 0x7);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   if (pad) {
      ilo_builder_batch_pointer(builder, 2, &dw);
      dw[0] = GEN6_MI_CMD(MI_BATCH_BUFFER_END);
      dw[1] = GEN6_MI_CMD(MI_NOOP);
   } else {
      ilo_builder_batch_pointer(builder, 1, &dw);
      dw[0] = GEN6_MI_CMD(MI_BATCH_BUFFER_END);
   }
}

#endif /* ILO_BUILDER_MI_H */
