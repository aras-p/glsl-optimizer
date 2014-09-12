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

#ifndef ILO_BUILDER_MEDIA_H
#define ILO_BUILDER_MEDIA_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_builder.h"

static inline void
gen6_MEDIA_VFE_STATE(struct ilo_builder *builder,
                     int max_threads, int num_urb_entries,
                     int urb_entry_size)
{
   const uint8_t cmd_len = 8;
   const uint32_t dw0 = GEN6_RENDER_CMD(MEDIA, MEDIA_VFE_STATE) |
                        (cmd_len - 2);
   uint32_t dw2, dw4, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   dw2 = (max_threads - 1) << 16 |
         num_urb_entries << 8 |
         1 << 7 | /* Reset Gateway Timer */
         1 << 6;  /* Bypass Gateway Control */

   dw4 = urb_entry_size << 16 |  /* URB Entry Allocation Size */
         480;                    /* CURBE Allocation Size */

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = 0; /* scratch */
   dw[2] = dw2;
   dw[3] = 0; /* MBZ */
   dw[4] = dw4;
   dw[5] = 0; /* scoreboard */
   dw[6] = 0;
   dw[7] = 0;
}

static inline void
gen6_MEDIA_CURBE_LOAD(struct ilo_builder *builder,
                     uint32_t buf, int size)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(MEDIA, MEDIA_CURBE_LOAD) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   assert(buf % 32 == 0);
   /* gen6_push_constant_buffer() allocates buffers in 256-bit units */
   size = align(size, 32);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = 0; /* MBZ */
   dw[2] = size;
   dw[3] = buf;
}

static inline void
gen6_MEDIA_INTERFACE_DESCRIPTOR_LOAD(struct ilo_builder *builder,
                                     uint32_t offset, int num_ids)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 =
      GEN6_RENDER_CMD(MEDIA, MEDIA_INTERFACE_DESCRIPTOR_LOAD) | (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   assert(offset % 32 == 0);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = 0; /* MBZ */
   /* every ID has 8 DWords */
   dw[2] = num_ids * 8 * 4;
   dw[3] = offset;
}

static inline void
gen6_MEDIA_GATEWAY_STATE(struct ilo_builder *builder,
                         int id, int byte, int thread_count)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(MEDIA, MEDIA_GATEWAY_STATE) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = id << 16 |
           byte << 8 |
           thread_count;
}

static inline void
gen6_MEDIA_STATE_FLUSH(struct ilo_builder *builder,
                       int thread_count_water_mark,
                       int barrier_mask)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(MEDIA, MEDIA_STATE_FLUSH) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = thread_count_water_mark << 16 |
           barrier_mask;
}

static inline void
gen6_MEDIA_OBJECT_WALKER(struct ilo_builder *builder)
{
   assert(!"MEDIA_OBJECT_WALKER unsupported");
}

static inline void
gen7_GPGPU_WALKER(struct ilo_builder *builder)
{
   assert(!"GPGPU_WALKER unsupported");
}

static inline uint32_t
gen6_INTERFACE_DESCRIPTOR_DATA(struct ilo_builder *builder,
                               const struct ilo_shader_state **cs,
                               uint32_t *sampler_state,
                               int *num_samplers,
                               uint32_t *binding_table_state,
                               int *num_surfaces,
                               int num_ids)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 2, page 34:
    *
    *     "(Interface Descriptor Total Length) This field must have the same
    *      alignment as the Interface Descriptor Data Start Address.
    *
    *      It must be DQWord (32-byte) aligned..."
    *
    * From the Sandy Bridge PRM, volume 2 part 2, page 35:
    *
    *     "(Interface Descriptor Data Start Address) Specifies the 32-byte
    *      aligned address of the Interface Descriptor data."
    */
   const int state_align = 32;
   const int state_len = (32 / 4) * num_ids;
   uint32_t state_offset, *dw;
   int i;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_BLOB, state_align, state_len, &dw);

   for (i = 0; i < num_ids; i++) {
      dw[0] = ilo_shader_get_kernel_offset(cs[i]);
      dw[1] = 1 << 18; /* SPF */
      dw[2] = sampler_state[i] |
              (num_samplers[i] + 3) / 4 << 2;
      dw[3] = binding_table_state[i] |
              num_surfaces[i];
      dw[4] = 0 << 16 |  /* CURBE Read Length */
              0;         /* CURBE Read Offset */
      dw[5] = 0; /* Barrier ID */
      dw[6] = 0;
      dw[7] = 0;

      dw += 8;
   }

   return state_offset;
}

#endif /* ILO_BUILDER_MEDIA_H */
