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

#ifndef ILO_BUILDER_3D_TOP_H
#define ILO_BUILDER_3D_TOP_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_state_gen.h"
#include "ilo_builder.h"

static inline void
gen6_3DSTATE_URB(struct ilo_builder *builder,
                 int vs_total_size, int gs_total_size,
                 int vs_entry_size, int gs_entry_size)
{
   const uint8_t cmd_len = 3;
   const int row_size = 128; /* 1024 bits */
   int vs_alloc_size, gs_alloc_size;
   int vs_num_entries, gs_num_entries;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   /* in 1024-bit URB rows */
   vs_alloc_size = (vs_entry_size + row_size - 1) / row_size;
   gs_alloc_size = (gs_entry_size + row_size - 1) / row_size;

   /* the valid range is [1, 5] */
   if (!vs_alloc_size)
      vs_alloc_size = 1;
   if (!gs_alloc_size)
      gs_alloc_size = 1;
   assert(vs_alloc_size <= 5 && gs_alloc_size <= 5);

   /* the valid range is [24, 256] in multiples of 4 */
   vs_num_entries = (vs_total_size / row_size / vs_alloc_size) & ~3;
   if (vs_num_entries > 256)
      vs_num_entries = 256;
   assert(vs_num_entries >= 24);

   /* the valid range is [0, 256] in multiples of 4 */
   gs_num_entries = (gs_total_size / row_size / gs_alloc_size) & ~3;
   if (gs_num_entries > 256)
      gs_num_entries = 256;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_URB) | (cmd_len - 2);
   dw[1] = (vs_alloc_size - 1) << GEN6_URB_DW1_VS_ENTRY_SIZE__SHIFT |
           vs_num_entries << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
   dw[2] = gs_num_entries << GEN6_URB_DW2_GS_ENTRY_COUNT__SHIFT |
           (gs_alloc_size - 1) << GEN6_URB_DW2_GS_ENTRY_SIZE__SHIFT;
}

static inline void
gen7_3dstate_push_constant_alloc(struct ilo_builder *builder,
                                 int subop, int offset, int size)
{
   const uint32_t cmd = GEN6_RENDER_TYPE_RENDER |
                        GEN6_RENDER_SUBTYPE_3D |
                        subop;
   const uint8_t cmd_len = 2;
   uint32_t *dw;
   int end;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /* VS, HS, DS, GS, and PS variants */
   assert(subop >= GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_VS &&
          subop <= GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_PS);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 68:
    *
    *     "(A table that says the maximum size of each constant buffer is
    *      16KB")
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 115:
    *
    *     "The sum of the Constant Buffer Offset and the Constant Buffer Size
    *      may not exceed the maximum value of the Constant Buffer Size."
    *
    * Thus, the valid range of buffer end is [0KB, 16KB].
    */
   end = (offset + size) / 1024;
   if (end > 16) {
      assert(!"invalid constant buffer end");
      end = 16;
   }

   /* the valid range of buffer offset is [0KB, 15KB] */
   offset = (offset + 1023) / 1024;
   if (offset > 15) {
      assert(!"invalid constant buffer offset");
      offset = 15;
   }

   if (offset > end) {
      assert(!size);
      offset = end;
   }

   /* the valid range of buffer size is [0KB, 15KB] */
   size = end - offset;
   if (size > 15) {
      assert(!"invalid constant buffer size");
      size = 15;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
   dw[1] = offset << GEN7_PCB_ALLOC_ANY_DW1_OFFSET__SHIFT |
           size;
}

static inline void
gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS(struct ilo_builder *builder,
                                    int offset, int size)
{
   gen7_3dstate_push_constant_alloc(builder,
         GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_VS, offset, size);
}

static inline void
gen7_3DSTATE_PUSH_CONSTANT_ALLOC_HS(struct ilo_builder *builder,
                                    int offset, int size)
{
   gen7_3dstate_push_constant_alloc(builder,
         GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_HS, offset, size);
}

static inline void
gen7_3DSTATE_PUSH_CONSTANT_ALLOC_DS(struct ilo_builder *builder,
                                    int offset, int size)
{
   gen7_3dstate_push_constant_alloc(builder,
         GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_DS, offset, size);
}

static inline void
gen7_3DSTATE_PUSH_CONSTANT_ALLOC_GS(struct ilo_builder *builder,
                                    int offset, int size)
{
   gen7_3dstate_push_constant_alloc(builder,
         GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_GS, offset, size);
}

static inline void
gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS(struct ilo_builder *builder,
                                    int offset, int size)
{
   gen7_3dstate_push_constant_alloc(builder,
         GEN7_RENDER_OPCODE_3DSTATE_PUSH_CONSTANT_ALLOC_PS, offset, size);
}

static inline void
gen7_3dstate_urb(struct ilo_builder *builder,
                 int subop, int offset, int size,
                 int entry_size)
{
   const uint32_t cmd = GEN6_RENDER_TYPE_RENDER |
                        GEN6_RENDER_SUBTYPE_3D |
                        subop;
   const uint8_t cmd_len = 2;
   const int row_size = 64; /* 512 bits */
   int alloc_size, num_entries, min_entries, max_entries;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /* VS, HS, DS, and GS variants */
   assert(subop >= GEN7_RENDER_OPCODE_3DSTATE_URB_VS &&
          subop <= GEN7_RENDER_OPCODE_3DSTATE_URB_GS);

   /* in multiples of 8KB */
   assert(offset % 8192 == 0);
   offset /= 8192;

   /* in multiple of 512-bit rows */
   alloc_size = (entry_size + row_size - 1) / row_size;
   if (!alloc_size)
      alloc_size = 1;

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 34:
    *
    *     "VS URB Entry Allocation Size equal to 4(5 512-bit URB rows) may
    *      cause performance to decrease due to banking in the URB. Element
    *      sizes of 16 to 20 should be programmed with six 512-bit URB rows."
    */
   if (subop == GEN7_RENDER_OPCODE_3DSTATE_URB_VS && alloc_size == 5)
      alloc_size = 6;

   /* in multiples of 8 */
   num_entries = (size / row_size / alloc_size) & ~7;

   switch (subop) {
   case GEN7_RENDER_OPCODE_3DSTATE_URB_VS:
      switch (ilo_dev_gen(builder->dev)) {
      case ILO_GEN(7.5):
         max_entries = (builder->dev->gt >= 2) ? 1664 : 640;
         min_entries = (builder->dev->gt >= 2) ? 64 : 32;
         break;
      case ILO_GEN(7):
      default:
         max_entries = (builder->dev->gt == 2) ? 704 : 512;
         min_entries = 32;
         break;
      }

      assert(num_entries >= min_entries);
      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   case GEN7_RENDER_OPCODE_3DSTATE_URB_HS:
      max_entries = (builder->dev->gt == 2) ? 64 : 32;
      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   case GEN7_RENDER_OPCODE_3DSTATE_URB_DS:
      if (num_entries)
         assert(num_entries >= 138);
      break;
   case GEN7_RENDER_OPCODE_3DSTATE_URB_GS:
      switch (ilo_dev_gen(builder->dev)) {
      case ILO_GEN(7.5):
         max_entries = (builder->dev->gt >= 2) ? 640 : 256;
         break;
      case ILO_GEN(7):
      default:
         max_entries = (builder->dev->gt == 2) ? 320 : 192;
         break;
      }

      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   default:
      break;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
   dw[1] = offset << GEN7_URB_ANY_DW1_OFFSET__SHIFT |
           (alloc_size - 1) << GEN7_URB_ANY_DW1_ENTRY_SIZE__SHIFT |
           num_entries;
}

static inline void
gen7_3DSTATE_URB_VS(struct ilo_builder *builder,
                    int offset, int size, int entry_size)
{
   gen7_3dstate_urb(builder, GEN7_RENDER_OPCODE_3DSTATE_URB_VS,
         offset, size, entry_size);
}

static inline void
gen7_3DSTATE_URB_HS(struct ilo_builder *builder,
                    int offset, int size, int entry_size)
{
   gen7_3dstate_urb(builder, GEN7_RENDER_OPCODE_3DSTATE_URB_HS,
         offset, size, entry_size);
}

static inline void
gen7_3DSTATE_URB_DS(struct ilo_builder *builder,
                    int offset, int size, int entry_size)
{
   gen7_3dstate_urb(builder, GEN7_RENDER_OPCODE_3DSTATE_URB_DS,
         offset, size, entry_size);
}

static inline void
gen7_3DSTATE_URB_GS(struct ilo_builder *builder,
                    int offset, int size, int entry_size)
{
   gen7_3dstate_urb(builder, GEN7_RENDER_OPCODE_3DSTATE_URB_GS,
         offset, size, entry_size);
}

static inline void
gen7_3DSTATE_VF(struct ilo_builder *builder,
                bool enable_cut_index,
                uint32_t cut_index)
{
   const uint8_t cmd_len = 2;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7.5, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN75_RENDER_CMD(3D, 3DSTATE_VF) | (cmd_len - 2);
   if (enable_cut_index)
      dw[0] |= GEN75_VF_DW0_CUT_INDEX_ENABLE;

   dw[1] = cut_index;
}

static inline void
gen6_3DSTATE_VF_STATISTICS(struct ilo_builder *builder,
                           bool enable)
{
   const uint8_t cmd_len = 1;
   const uint32_t dw0 = GEN6_RENDER_CMD(SINGLE_DW, 3DSTATE_VF_STATISTICS) |
                        enable;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_write(builder, cmd_len, &dw0);
}

static inline void
gen6_3DSTATE_VERTEX_BUFFERS(struct ilo_builder *builder,
                            const struct ilo_ve_state *ve,
                            const struct ilo_vb_state *vb)
{
   uint8_t cmd_len;
   uint32_t *dw;
   unsigned pos, hw_idx;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 82:
    *
    *     "From 1 to 33 VBs can be specified..."
    */
   assert(ve->vb_count <= 33);

   if (!ve->vb_count)
      return;

   cmd_len = 1 + 4 * ve->vb_count;
   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (cmd_len - 2);
   dw++;
   pos++;

   for (hw_idx = 0; hw_idx < ve->vb_count; hw_idx++) {
      const unsigned instance_divisor = ve->instance_divisors[hw_idx];
      const unsigned pipe_idx = ve->vb_mapping[hw_idx];
      const struct pipe_vertex_buffer *cso = &vb->states[pipe_idx];

      dw[0] = hw_idx << GEN6_VB_STATE_DW0_INDEX__SHIFT;

      if (instance_divisor)
         dw[0] |= GEN6_VB_STATE_DW0_ACCESS_INSTANCEDATA;
      else
         dw[0] |= GEN6_VB_STATE_DW0_ACCESS_VERTEXDATA;

      if (ilo_dev_gen(builder->dev) >= ILO_GEN(7))
         dw[0] |= GEN7_VB_STATE_DW0_ADDR_MODIFIED;

      /* use null vb if there is no buffer or the stride is out of range */
      if (cso->buffer && cso->stride <= 2048) {
         const struct ilo_buffer *buf = ilo_buffer(cso->buffer);
         const uint32_t start_offset = cso->buffer_offset;
         const uint32_t end_offset = buf->bo_size - 1;

         dw[0] |= cso->stride << GEN6_VB_STATE_DW0_PITCH__SHIFT;
         ilo_builder_batch_reloc(builder, pos + 1, buf->bo, start_offset, 0);
         ilo_builder_batch_reloc(builder, pos + 2, buf->bo, end_offset, 0);
      }
      else {
         dw[0] |= 1 << 13;
         dw[1] = 0;
         dw[2] = 0;
      }

      dw[3] = instance_divisor;

      dw += 4;
      pos += 4;
   }
}

/* the user vertex buffer must be uploaded with gen6_user_vertex_buffer() */
static inline void
gen6_user_3DSTATE_VERTEX_BUFFERS(struct ilo_builder *builder,
                                 uint32_t vb_begin, uint32_t vb_end,
                                 uint32_t stride)
{
   const struct ilo_builder_writer *bat =
      &builder->writers[ILO_BUILDER_WRITER_BATCH];
   const uint8_t cmd_len = 1 + 4;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (cmd_len - 2);
   dw++;
   pos++;

   /* VERTEX_BUFFER_STATE */
   dw[0] = 0 << GEN6_VB_STATE_DW0_INDEX__SHIFT |
           GEN6_VB_STATE_DW0_ACCESS_VERTEXDATA |
           stride << GEN6_VB_STATE_DW0_PITCH__SHIFT;
   dw[3] = 0;

   ilo_builder_batch_reloc(builder, pos + 1, bat->bo, vb_begin, 0);
   ilo_builder_batch_reloc(builder, pos + 2, bat->bo, vb_end, 0);
}

static inline void
ve_init_cso_with_components(const struct ilo_dev_info *dev,
                            int comp0, int comp1, int comp2, int comp3,
                            struct ilo_ve_cso *cso)
{
   ILO_DEV_ASSERT(dev, 6, 7.5);

   STATIC_ASSERT(Elements(cso->payload) >= 2);
   cso->payload[0] = GEN6_VE_STATE_DW0_VALID;
   cso->payload[1] =
         comp0 << GEN6_VE_STATE_DW1_COMP0__SHIFT |
         comp1 << GEN6_VE_STATE_DW1_COMP1__SHIFT |
         comp2 << GEN6_VE_STATE_DW1_COMP2__SHIFT |
         comp3 << GEN6_VE_STATE_DW1_COMP3__SHIFT;
}

static inline void
ve_set_cso_edgeflag(const struct ilo_dev_info *dev,
                    struct ilo_ve_cso *cso)
{
   int format;

   ILO_DEV_ASSERT(dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 94:
    *
    *     "- This bit (Edge Flag Enable) must only be ENABLED on the last
    *        valid VERTEX_ELEMENT structure.
    *
    *      - When set, Component 0 Control must be set to VFCOMP_STORE_SRC,
    *        and Component 1-3 Control must be set to VFCOMP_NOSTORE.
    *
    *      - The Source Element Format must be set to the UINT format.
    *
    *      - [DevSNB]: Edge Flags are not supported for QUADLIST
    *        primitives.  Software may elect to convert QUADLIST primitives
    *        to some set of corresponding edge-flag-supported primitive
    *        types (e.g., POLYGONs) prior to submission to the 3D pipeline."
    */

   cso->payload[0] |= GEN6_VE_STATE_DW0_EDGE_FLAG_ENABLE;
   cso->payload[1] =
         GEN6_VFCOMP_STORE_SRC << GEN6_VE_STATE_DW1_COMP0__SHIFT |
         GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP1__SHIFT |
         GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP2__SHIFT |
         GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP3__SHIFT;

   /*
    * Edge flags have format GEN6_FORMAT_R8_UINT when defined via
    * glEdgeFlagPointer(), and format GEN6_FORMAT_R32_FLOAT when defined
    * via glEdgeFlag(), as can be seen in vbo_attrib_tmp.h.
    *
    * Since all the hardware cares about is whether the flags are zero or not,
    * we can treat them as GEN6_FORMAT_R32_UINT in the latter case.
    */
   format = (cso->payload[0] >> GEN6_VE_STATE_DW0_FORMAT__SHIFT) & 0x1ff;
   if (format == GEN6_FORMAT_R32_FLOAT) {
      STATIC_ASSERT(GEN6_FORMAT_R32_UINT == GEN6_FORMAT_R32_FLOAT - 1);
      cso->payload[0] -= (1 << GEN6_VE_STATE_DW0_FORMAT__SHIFT);
   }
   else {
      assert(format == GEN6_FORMAT_R8_UINT);
   }
}

static inline void
gen6_3DSTATE_VERTEX_ELEMENTS(struct ilo_builder *builder,
                             const struct ilo_ve_state *ve,
                             bool last_velement_edgeflag,
                             bool prepend_generated_ids)
{
   uint8_t cmd_len;
   uint32_t *dw;
   unsigned i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 93:
    *
    *     "Up to 34 (DevSNB+) vertex elements are supported."
    */
   assert(ve->count + prepend_generated_ids <= 34);

   STATIC_ASSERT(Elements(ve->cso[0].payload) == 2);

   if (!ve->count && !prepend_generated_ids) {
      struct ilo_ve_cso dummy;

      ve_init_cso_with_components(builder->dev,
            GEN6_VFCOMP_STORE_0,
            GEN6_VFCOMP_STORE_0,
            GEN6_VFCOMP_STORE_0,
            GEN6_VFCOMP_STORE_1_FP,
            &dummy);

      cmd_len = 3;

      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) | (cmd_len - 2);
      memcpy(&dw[1], dummy.payload, sizeof(dummy.payload));

      return;
   }

   cmd_len = 2 * (ve->count + prepend_generated_ids) + 1;
   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) | (cmd_len - 2);
   dw++;

   if (prepend_generated_ids) {
      struct ilo_ve_cso gen_ids;

      ve_init_cso_with_components(builder->dev,
            GEN6_VFCOMP_STORE_VID,
            GEN6_VFCOMP_STORE_IID,
            GEN6_VFCOMP_NOSTORE,
            GEN6_VFCOMP_NOSTORE,
            &gen_ids);

      memcpy(dw, gen_ids.payload, sizeof(gen_ids.payload));
      dw += 2;
   }

   if (last_velement_edgeflag && ve->count) {
      struct ilo_ve_cso edgeflag;

      for (i = 0; i < ve->count - 1; i++)
         memcpy(&dw[2 * i], ve->cso[i].payload, sizeof(ve->cso[i].payload));

      edgeflag = ve->cso[i];
      ve_set_cso_edgeflag(builder->dev, &edgeflag);
      memcpy(&dw[2 * i], edgeflag.payload, sizeof(edgeflag.payload));
   } else {
      for (i = 0; i < ve->count; i++)
         memcpy(&dw[2 * i], ve->cso[i].payload, sizeof(ve->cso[i].payload));
   }
}

static inline void
gen6_3DSTATE_INDEX_BUFFER(struct ilo_builder *builder,
                          const struct ilo_ib_state *ib,
                          bool enable_cut_index)
{
   const uint8_t cmd_len = 3;
   struct ilo_buffer *buf = ilo_buffer(ib->hw_resource);
   uint32_t start_offset, end_offset;
   int format;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   if (!buf)
      return;

   /* this is moved to the new 3DSTATE_VF */
   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7.5))
      assert(!enable_cut_index);

   switch (ib->hw_index_size) {
   case 4:
      format = GEN6_IB_DW0_FORMAT_DWORD;
      break;
   case 2:
      format = GEN6_IB_DW0_FORMAT_WORD;
      break;
   case 1:
      format = GEN6_IB_DW0_FORMAT_BYTE;
      break;
   default:
      assert(!"unknown index size");
      format = GEN6_IB_DW0_FORMAT_BYTE;
      break;
   }

   /*
    * set start_offset to 0 here and adjust pipe_draw_info::start with
    * ib->draw_start_offset in 3DPRIMITIVE
    */
   start_offset = 0;
   end_offset = buf->bo_size;

   /* end_offset must also be aligned and is inclusive */
   end_offset -= (end_offset % ib->hw_index_size);
   end_offset--;

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_INDEX_BUFFER) |
           format |
           (cmd_len - 2);
   if (enable_cut_index)
      dw[0] |= GEN6_IB_DW0_CUT_INDEX_ENABLE;

   ilo_builder_batch_reloc(builder, pos + 1, buf->bo, start_offset, 0);
   ilo_builder_batch_reloc(builder, pos + 2, buf->bo, end_offset, 0);
}

static inline void
gen6_3DSTATE_VS(struct ilo_builder *builder,
                const struct ilo_shader_state *vs,
                int num_samplers)
{
   const uint8_t cmd_len = 6;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (cmd_len - 2);
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   if (!vs) {
      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = 0;
      dw[2] = 0;
      dw[3] = 0;
      dw[4] = 0;
      dw[5] = 0;

      return;
   }

   cso = ilo_shader_get_kernel_cso(vs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= ((num_samplers + 3) / 4) << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = ilo_shader_get_kernel_offset(vs);
   dw[2] = dw2;
   dw[3] = 0; /* scratch */
   dw[4] = dw4;
   dw[5] = dw5;
}

static inline void
gen7_3DSTATE_HS(struct ilo_builder *builder,
                const struct ilo_shader_state *hs,
                int num_samplers)
{
   const uint8_t cmd_len = 7;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   assert(!hs);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_HS) | (cmd_len - 2);
   dw[1] = 0;
   dw[2] = 0;
   dw[3] = 0;
   dw[4] = 0;
   dw[5] = 0;
   dw[6] = 0;
}

static inline void
gen7_3DSTATE_TE(struct ilo_builder *builder)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_TE) | (cmd_len - 2);
   dw[1] = 0;
   dw[2] = 0;
   dw[3] = 0;
}

static inline void
gen7_3DSTATE_DS(struct ilo_builder *builder,
                const struct ilo_shader_state *ds,
                int num_samplers)
{
   const uint8_t cmd_len = 6;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   assert(!ds);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_DS) | (cmd_len - 2);
   dw[1] = 0;
   dw[2] = 0;
   dw[3] = 0;
   dw[4] = 0;
   dw[5] = 0;
}

static inline void
gen6_3DSTATE_GS(struct ilo_builder *builder,
                const struct ilo_shader_state *gs,
                const struct ilo_shader_state *vs,
                int verts_per_prim)
{
   const uint8_t cmd_len = 7;
   uint32_t dw1, dw2, dw4, dw5, dw6, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   if (gs) {
      const struct ilo_shader_cso *cso;

      dw1 = ilo_shader_get_kernel_offset(gs);

      cso = ilo_shader_get_kernel_cso(gs);
      dw2 = cso->payload[0];
      dw4 = cso->payload[1];
      dw5 = cso->payload[2];
      dw6 = cso->payload[3];
   }
   else if (vs && ilo_shader_get_kernel_param(vs, ILO_KERNEL_VS_GEN6_SO)) {
      struct ilo_shader_cso cso;
      enum ilo_kernel_param param;

      switch (verts_per_prim) {
      case 1:
         param = ILO_KERNEL_VS_GEN6_SO_POINT_OFFSET;
         break;
      case 2:
         param = ILO_KERNEL_VS_GEN6_SO_LINE_OFFSET;
         break;
      default:
         param = ILO_KERNEL_VS_GEN6_SO_TRI_OFFSET;
         break;
      }

      dw1 = ilo_shader_get_kernel_offset(vs) +
         ilo_shader_get_kernel_param(vs, param);

      /* cannot use VS's CSO */
      ilo_gpe_init_gs_cso_gen6(builder->dev, vs, &cso);
      dw2 = cso.payload[0];
      dw4 = cso.payload[1];
      dw5 = cso.payload[2];
      dw6 = cso.payload[3];
   }
   else {
      dw1 = 0;
      dw2 = 0;
      dw4 = 1 << GEN6_GS_DW4_URB_READ_LEN__SHIFT;
      dw5 = GEN6_GS_DW5_STATISTICS;
      dw6 = 0;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);
   dw[1] = dw1;
   dw[2] = dw2;
   dw[3] = 0;
   dw[4] = dw4;
   dw[5] = dw5;
   dw[6] = dw6;
}

static inline void
gen6_3DSTATE_GS_SVB_INDEX(struct ilo_builder *builder,
                          int index, unsigned svbi,
                          unsigned max_svbi,
                          bool load_vertex_count)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);
   assert(index >= 0 && index < 4);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_GS_SVB_INDEX) | (cmd_len - 2);

   dw[1] = index << GEN6_SVBI_DW1_INDEX__SHIFT;
   if (load_vertex_count)
      dw[1] |= GEN6_SVBI_DW1_LOAD_INTERNAL_VERTEX_COUNT;

   dw[2] = svbi;
   dw[3] = max_svbi;
}

static inline void
gen7_3DSTATE_GS(struct ilo_builder *builder,
                const struct ilo_shader_state *gs,
                int num_samplers)
{
   const uint8_t cmd_len = 7;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5, *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   if (!gs) {
      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = 0;
      dw[2] = 0;
      dw[3] = 0;
      dw[4] = 0;
      dw[5] = GEN7_GS_DW5_STATISTICS;
      dw[6] = 0;
      return;
   }

   cso = ilo_shader_get_kernel_cso(gs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= ((num_samplers + 3) / 4) << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = dw0;
   dw[1] = ilo_shader_get_kernel_offset(gs);
   dw[2] = dw2;
   dw[3] = 0; /* scratch */
   dw[4] = dw4;
   dw[5] = dw5;
   dw[6] = 0;
}

static inline void
gen7_3DSTATE_STREAMOUT(struct ilo_builder *builder,
                       unsigned buffer_mask,
                       int vertex_attrib_count,
                       bool rasterizer_discard)
{
   const uint8_t cmd_len = 3;
   const bool enable = (buffer_mask != 0);
   const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_STREAMOUT) |
                        (cmd_len - 2);
   uint32_t dw1, dw2, *dw;
   int read_len;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   if (!enable) {
      dw1 = 0 << GEN7_SO_DW1_RENDER_STREAM_SELECT__SHIFT;
      if (rasterizer_discard)
         dw1 |= GEN7_SO_DW1_RENDER_DISABLE;

      dw2 = 0;

      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = dw1;
      dw[2] = dw2;
      return;
   }

   read_len = (vertex_attrib_count + 1) / 2;
   if (!read_len)
      read_len = 1;

   dw1 = GEN7_SO_DW1_SO_ENABLE |
         0 << GEN7_SO_DW1_RENDER_STREAM_SELECT__SHIFT |
         GEN7_SO_DW1_STATISTICS |
         buffer_mask << 8;

   if (rasterizer_discard)
      dw1 |= GEN7_SO_DW1_RENDER_DISABLE;

   /* API_OPENGL */
   if (true)
      dw1 |= GEN7_SO_DW1_REORDER_TRAILING;

   dw2 = 0 << GEN7_SO_DW2_STREAM3_READ_OFFSET__SHIFT |
         0 << GEN7_SO_DW2_STREAM3_READ_LEN__SHIFT |
         0 << GEN7_SO_DW2_STREAM2_READ_OFFSET__SHIFT |
         0 << GEN7_SO_DW2_STREAM2_READ_LEN__SHIFT |
         0 << GEN7_SO_DW2_STREAM1_READ_OFFSET__SHIFT |
         0 << GEN7_SO_DW2_STREAM1_READ_LEN__SHIFT |
         0 << GEN7_SO_DW2_STREAM0_READ_OFFSET__SHIFT |
         (read_len - 1) << GEN7_SO_DW2_STREAM0_READ_LEN__SHIFT;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = dw2;
}

static inline void
gen7_3DSTATE_SO_DECL_LIST(struct ilo_builder *builder,
                          const struct pipe_stream_output_info *so_info)
{
   uint16_t cmd_len;
   int buffer_selects, num_entries, i;
   uint16_t so_decls[128];
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   buffer_selects = 0;
   num_entries = 0;

   if (so_info) {
      int buffer_offsets[PIPE_MAX_SO_BUFFERS];

      memset(buffer_offsets, 0, sizeof(buffer_offsets));

      for (i = 0; i < so_info->num_outputs; i++) {
         unsigned decl, buf, reg, mask;

         buf = so_info->output[i].output_buffer;

         /* pad with holes */
         assert(buffer_offsets[buf] <= so_info->output[i].dst_offset);
         while (buffer_offsets[buf] < so_info->output[i].dst_offset) {
            int num_dwords;

            num_dwords = so_info->output[i].dst_offset - buffer_offsets[buf];
            if (num_dwords > 4)
               num_dwords = 4;

            decl = buf << GEN7_SO_DECL_OUTPUT_SLOT__SHIFT |
                   GEN7_SO_DECL_HOLE_FLAG |
                   ((1 << num_dwords) - 1) << GEN7_SO_DECL_COMPONENT_MASK__SHIFT;

            so_decls[num_entries++] = decl;
            buffer_offsets[buf] += num_dwords;
         }

         reg = so_info->output[i].register_index;
         mask = ((1 << so_info->output[i].num_components) - 1) <<
            so_info->output[i].start_component;

         decl = buf << GEN7_SO_DECL_OUTPUT_SLOT__SHIFT |
                reg << GEN7_SO_DECL_REG_INDEX__SHIFT |
                mask << GEN7_SO_DECL_COMPONENT_MASK__SHIFT;

         so_decls[num_entries++] = decl;
         buffer_selects |= 1 << buf;
         buffer_offsets[buf] += so_info->output[i].num_components;
      }
   }

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 201:
    *
    *     "Errata: All 128 decls for all four streams must be included
    *      whenever this command is issued. The "Num Entries [n]" fields still
    *      contain the actual numbers of valid decls."
    *
    * Also note that "DWord Length" has 9 bits for this command, and the type
    * of cmd_len is thus uint16_t.
    */
   cmd_len = 2 * 128 + 3;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SO_DECL_LIST) | (cmd_len - 2);
   dw[1] = 0 << GEN7_SO_DECL_DW1_STREAM3_BUFFER_SELECTS__SHIFT |
           0 << GEN7_SO_DECL_DW1_STREAM2_BUFFER_SELECTS__SHIFT |
           0 << GEN7_SO_DECL_DW1_STREAM1_BUFFER_SELECTS__SHIFT |
           buffer_selects << GEN7_SO_DECL_DW1_STREAM0_BUFFER_SELECTS__SHIFT;
   dw[2] = 0 << GEN7_SO_DECL_DW2_STREAM3_ENTRY_COUNT__SHIFT |
           0 << GEN7_SO_DECL_DW2_STREAM2_ENTRY_COUNT__SHIFT |
           0 << GEN7_SO_DECL_DW2_STREAM1_ENTRY_COUNT__SHIFT |
           num_entries << GEN7_SO_DECL_DW2_STREAM0_ENTRY_COUNT__SHIFT;
   dw += 3;

   for (i = 0; i < num_entries; i++) {
      dw[0] = so_decls[i];
      dw[1] = 0;
      dw += 2;
   }
   for (; i < 128; i++) {
      dw[0] = 0;
      dw[1] = 0;
      dw += 2;
   }
}

static inline void
gen7_3DSTATE_SO_BUFFER(struct ilo_builder *builder,
                       int index, int base, int stride,
                       const struct pipe_stream_output_target *so_target)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_SO_BUFFER) |
                        (cmd_len - 2);
   struct ilo_buffer *buf;
   int end;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   if (!so_target || !so_target->buffer) {
      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = index << GEN7_SO_BUF_DW1_INDEX__SHIFT;
      dw[2] = 0;
      dw[3] = 0;

      return;
   }

   buf = ilo_buffer(so_target->buffer);

   /* DWord-aligned */
   assert(stride % 4 == 0 && base % 4 == 0);
   assert(so_target->buffer_offset % 4 == 0);

   stride &= ~3;
   base = (base + so_target->buffer_offset) & ~3;
   end = (base + so_target->buffer_size) & ~3;

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = dw0;
   dw[1] = index << GEN7_SO_BUF_DW1_INDEX__SHIFT |
           stride;

   ilo_builder_batch_reloc(builder, pos + 2,
         buf->bo, base, INTEL_RELOC_WRITE);
   ilo_builder_batch_reloc(builder, pos + 3,
         buf->bo, end, INTEL_RELOC_WRITE);
}

static inline void
gen6_3DSTATE_BINDING_TABLE_POINTERS(struct ilo_builder *builder,
                                    uint32_t vs_binding_table,
                                    uint32_t gs_binding_table,
                                    uint32_t ps_binding_table)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_BINDING_TABLE_POINTERS) |
           GEN6_PTR_BINDING_TABLE_DW0_VS_CHANGED |
           GEN6_PTR_BINDING_TABLE_DW0_GS_CHANGED |
           GEN6_PTR_BINDING_TABLE_DW0_PS_CHANGED |
           (cmd_len - 2);
   dw[1] = vs_binding_table;
   dw[2] = gs_binding_table;
   dw[3] = ps_binding_table;
}

static inline void
gen6_3DSTATE_SAMPLER_STATE_POINTERS(struct ilo_builder *builder,
                                    uint32_t vs_sampler_state,
                                    uint32_t gs_sampler_state,
                                    uint32_t ps_sampler_state)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLER_STATE_POINTERS) |
           GEN6_PTR_SAMPLER_DW0_VS_CHANGED |
           GEN6_PTR_SAMPLER_DW0_GS_CHANGED |
           GEN6_PTR_SAMPLER_DW0_PS_CHANGED |
           (cmd_len - 2);
   dw[1] = vs_sampler_state;
   dw[2] = gs_sampler_state;
   dw[3] = ps_sampler_state;
}

static inline void
gen7_3dstate_pointer(struct ilo_builder *builder,
                     int subop, uint32_t pointer)
{
   const uint32_t cmd = GEN6_RENDER_TYPE_RENDER |
                        GEN6_RENDER_SUBTYPE_3D |
                        subop;
   const uint8_t cmd_len = 2;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
   dw[1] = pointer;
}

static inline void
gen7_3DSTATE_BINDING_TABLE_POINTERS_VS(struct ilo_builder *builder,
                                       uint32_t binding_table)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_VS,
         binding_table);
}

static inline void
gen7_3DSTATE_BINDING_TABLE_POINTERS_HS(struct ilo_builder *builder,
                                       uint32_t binding_table)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_HS,
         binding_table);
}

static inline void
gen7_3DSTATE_BINDING_TABLE_POINTERS_DS(struct ilo_builder *builder,
                                       uint32_t binding_table)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_DS,
         binding_table);
}

static inline void
gen7_3DSTATE_BINDING_TABLE_POINTERS_GS(struct ilo_builder *builder,
                                       uint32_t binding_table)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_GS,
         binding_table);
}

static inline void
gen7_3DSTATE_SAMPLER_STATE_POINTERS_VS(struct ilo_builder *builder,
                                       uint32_t sampler_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_VS,
         sampler_state);
}

static inline void
gen7_3DSTATE_SAMPLER_STATE_POINTERS_HS(struct ilo_builder *builder,
                                       uint32_t sampler_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_HS,
         sampler_state);
}

static inline void
gen7_3DSTATE_SAMPLER_STATE_POINTERS_DS(struct ilo_builder *builder,
                                       uint32_t sampler_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_DS,
         sampler_state);
}

static inline void
gen7_3DSTATE_SAMPLER_STATE_POINTERS_GS(struct ilo_builder *builder,
                                       uint32_t sampler_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_GS,
         sampler_state);
}

static inline unsigned
gen6_fill_3dstate_constant(const struct ilo_dev_info *dev,
                           const uint32_t *bufs, const int *sizes,
                           int num_bufs, int max_read_length,
                           uint32_t *dw, int num_dwords)
{
   unsigned enabled = 0x0;
   int total_read_length, i;

   assert(num_dwords == 4);

   total_read_length = 0;
   for (i = 0; i < 4; i++) {
      if (i < num_bufs && sizes[i]) {
         /* in 256-bit units minus one */
         const int read_len = (sizes[i] + 31) / 32 - 1;

         assert(bufs[i] % 32 == 0);
         assert(read_len < 32);

         enabled |= 1 << i;
         dw[i] = bufs[i] | read_len;

         total_read_length += read_len + 1;
      }
      else {
         dw[i] = 0;
      }
   }

   assert(total_read_length <= max_read_length);

   return enabled;
}

static inline void
gen6_3DSTATE_CONSTANT_VS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 138:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 32"
    */
   buf_enabled = gen6_fill_3dstate_constant(builder->dev,
         bufs, sizes, num_bufs, 32, buf_dw, Elements(buf_dw));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) |
           buf_enabled << 12 |
           (cmd_len - 2);
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen6_3DSTATE_CONSTANT_GS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 161:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 64"
    */
   buf_enabled = gen6_fill_3dstate_constant(builder->dev,
         bufs, sizes, num_bufs, 64, buf_dw, Elements(buf_dw));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_GS) |
           buf_enabled << 12 |
           (cmd_len - 2);
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen7_3dstate_constant(struct ilo_builder *builder,
                      int subop,
                      const uint32_t *bufs, const int *sizes,
                      int num_bufs)
{
   const uint32_t cmd = GEN6_RENDER_TYPE_RENDER |
                        GEN6_RENDER_SUBTYPE_3D |
                        subop;
   const uint8_t cmd_len = 7;
   uint32_t payload[6], *dw;
   int total_read_length, i;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /* VS, HS, DS, GS, and PS variants */
   assert(subop >= GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_VS &&
          subop <= GEN7_RENDER_OPCODE_3DSTATE_CONSTANT_DS &&
          subop != GEN6_RENDER_OPCODE_3DSTATE_SAMPLE_MASK);

   assert(num_bufs <= 4);

   payload[0] = 0;
   payload[1] = 0;

   total_read_length = 0;
   for (i = 0; i < 4; i++) {
      int read_len;

      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 112:
       *
       *     "Constant buffers must be enabled in order from Constant Buffer 0
       *      to Constant Buffer 3 within this command.  For example, it is
       *      not allowed to enable Constant Buffer 1 by programming a
       *      non-zero value in the VS Constant Buffer 1 Read Length without a
       *      non-zero value in VS Constant Buffer 0 Read Length."
       */
      if (i >= num_bufs || !sizes[i]) {
         for (; i < 4; i++) {
            assert(i >= num_bufs || !sizes[i]);
            payload[2 + i] = 0;
         }
         break;
      }

      /* read lengths are in 256-bit units */
      read_len = (sizes[i] + 31) / 32;
      /* the lower 5 bits are used for memory object control state */
      assert(bufs[i] % 32 == 0);

      payload[i / 2] |= read_len << ((i % 2) ? 16 : 0);
      payload[2 + i] = bufs[i];

      total_read_length += read_len;
   }

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 113:
    *
    *     "The sum of all four read length fields must be less than or equal
    *      to the size of 64"
    */
   assert(total_read_length <= 64);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
   memcpy(&dw[1], payload, sizeof(payload));
}

static inline void
gen7_3DSTATE_CONSTANT_VS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   gen7_3dstate_constant(builder, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_VS,
         bufs, sizes, num_bufs);
}

static inline void
gen7_3DSTATE_CONSTANT_HS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   gen7_3dstate_constant(builder, GEN7_RENDER_OPCODE_3DSTATE_CONSTANT_HS,
         bufs, sizes, num_bufs);
}

static inline void
gen7_3DSTATE_CONSTANT_DS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   gen7_3dstate_constant(builder, GEN7_RENDER_OPCODE_3DSTATE_CONSTANT_DS,
         bufs, sizes, num_bufs);
}

static inline void
gen7_3DSTATE_CONSTANT_GS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   gen7_3dstate_constant(builder, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_GS,
         bufs, sizes, num_bufs);
}

static inline uint32_t
gen6_BINDING_TABLE_STATE(struct ilo_builder *builder,
                         uint32_t *surface_states,
                         int num_surface_states)
{
   const int state_align = 32;
   const int state_len = num_surface_states;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 69:
    *
    *     "It is stored as an array of up to 256 elements..."
    */
   assert(num_surface_states <= 256);

   if (!num_surface_states)
      return 0;

   return ilo_builder_surface_write(builder, ILO_BUILDER_ITEM_BINDING_TABLE,
         state_align, state_len, surface_states);
}

static inline uint32_t
gen6_SURFACE_STATE(struct ilo_builder *builder,
                   const struct ilo_view_surface *surf,
                   bool for_render)
{
   const int state_align = 32;
   const int state_len = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ? 8 : 6;
   uint32_t state_offset;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   state_offset = ilo_builder_surface_write(builder, ILO_BUILDER_ITEM_SURFACE,
         state_align, state_len, surf->payload);

   if (surf->bo) {
      ilo_builder_surface_reloc(builder, state_offset, 1, surf->bo,
            surf->payload[1], (for_render) ? INTEL_RELOC_WRITE : 0);
   }

   return state_offset;
}

static inline uint32_t
gen6_so_SURFACE_STATE(struct ilo_builder *builder,
                      const struct pipe_stream_output_target *so,
                      const struct pipe_stream_output_info *so_info,
                      int so_index)
{
   struct ilo_buffer *buf = ilo_buffer(so->buffer);
   unsigned bo_offset, struct_size;
   enum pipe_format elem_format;
   struct ilo_view_surface surf;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   bo_offset = so->buffer_offset + so_info->output[so_index].dst_offset * 4;
   struct_size = so_info->stride[so_info->output[so_index].output_buffer] * 4;

   switch (so_info->output[so_index].num_components) {
   case 1:
      elem_format = PIPE_FORMAT_R32_FLOAT;
      break;
   case 2:
      elem_format = PIPE_FORMAT_R32G32_FLOAT;
      break;
   case 3:
      elem_format = PIPE_FORMAT_R32G32B32_FLOAT;
      break;
   case 4:
      elem_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      break;
   default:
      assert(!"unexpected SO components length");
      elem_format = PIPE_FORMAT_R32_FLOAT;
      break;
   }

   ilo_gpe_init_view_surface_for_buffer_gen6(builder->dev, buf, bo_offset,
         so->buffer_size, struct_size, elem_format, false, true, &surf);

   return gen6_SURFACE_STATE(builder, &surf, false);
}

static inline uint32_t
gen6_SAMPLER_STATE(struct ilo_builder *builder,
                   const struct ilo_sampler_cso * const *samplers,
                   const struct pipe_sampler_view * const *views,
                   const uint32_t *sampler_border_colors,
                   int num_samplers)
{
   const int state_align = 32;
   const int state_len = 4 * num_samplers;
   uint32_t state_offset, *dw;
   int i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 101:
    *
    *     "The sampler state is stored as an array of up to 16 elements..."
    */
   assert(num_samplers <= 16);

   if (!num_samplers)
      return 0;

   state_offset = ilo_builder_dynamic_pointer(builder,
         ILO_BUILDER_ITEM_SAMPLER, state_align, state_len, &dw);

   for (i = 0; i < num_samplers; i++) {
      const struct ilo_sampler_cso *sampler = samplers[i];
      const struct pipe_sampler_view *view = views[i];
      const uint32_t border_color = sampler_border_colors[i];
      uint32_t dw_filter, dw_wrap;

      /* there may be holes */
      if (!sampler || !view) {
         /* disabled sampler */
         dw[0] = 1 << 31;
         dw[1] = 0;
         dw[2] = 0;
         dw[3] = 0;
         dw += 4;

         continue;
      }

      /* determine filter and wrap modes */
      switch (view->texture->target) {
      case PIPE_TEXTURE_1D:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap_1d;
         break;
      case PIPE_TEXTURE_3D:
         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 103:
          *
          *     "Only MAPFILTER_NEAREST and MAPFILTER_LINEAR are supported for
          *      surfaces of type SURFTYPE_3D."
          */
         dw_filter = sampler->dw_filter;
         dw_wrap = sampler->dw_wrap;
         break;
      case PIPE_TEXTURE_CUBE:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap_cube;
         break;
      default:
         dw_filter = (sampler->anisotropic) ?
            sampler->dw_filter_aniso : sampler->dw_filter;
         dw_wrap = sampler->dw_wrap;
         break;
      }

      dw[0] = sampler->payload[0];
      dw[1] = sampler->payload[1];
      assert(!(border_color & 0x1f));
      dw[2] = border_color;
      dw[3] = sampler->payload[2];

      dw[0] |= dw_filter;

      if (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) {
         dw[3] |= dw_wrap;
      }
      else {
         /*
          * From the Sandy Bridge PRM, volume 4 part 1, page 21:
          *
          *     "[DevSNB] Errata: Incorrect behavior is observed in cases
          *      where the min and mag mode filters are different and
          *      SurfMinLOD is nonzero. The determination of MagMode uses the
          *      following equation instead of the one in the above
          *      pseudocode: MagMode = (LOD + SurfMinLOD - Base <= 0)"
          *
          * As a way to work around that, we set Base to
          * view->u.tex.first_level.
          */
         dw[0] |= view->u.tex.first_level << 22;

         dw[1] |= dw_wrap;
      }

      dw += 4;
   }

   return state_offset;
}

static inline uint32_t
gen6_SAMPLER_BORDER_COLOR_STATE(struct ilo_builder *builder,
                                const struct ilo_sampler_cso *sampler)
{
   const int state_align = 32;
   const int state_len = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ? 4 : 12;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(Elements(sampler->payload) >= 3 + state_len);

   /* see ilo_gpe_init_sampler_cso() */
   return ilo_builder_dynamic_write(builder, ILO_BUILDER_ITEM_BLOB,
         state_align, state_len, &sampler->payload[3]);
}

static inline uint32_t
gen6_push_constant_buffer(struct ilo_builder *builder,
                          int size, void **pcb)
{
   /*
    * For all VS, GS, FS, and CS push constant buffers, they must be aligned
    * to 32 bytes, and their sizes are specified in 256-bit units.
    */
   const int state_align = 32;
   const int state_len = align(size, 32) / 4;
   uint32_t state_offset;
   char *buf;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   state_offset = ilo_builder_dynamic_pointer(builder,
         ILO_BUILDER_ITEM_BLOB, state_align, state_len, (uint32_t **) &buf);

   /* zero out the unused range */
   if (size < state_len * 4)
      memset(&buf[size], 0, state_len * 4 - size);

   if (pcb)
      *pcb = buf;

   return state_offset;
}

static inline uint32_t
gen6_user_vertex_buffer(struct ilo_builder *builder,
                        int size, const void *vertices)
{
   const int state_align = 8;
   const int state_len = size / 4;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(size % 4 == 0);

   return ilo_builder_dynamic_write(builder, ILO_BUILDER_ITEM_BLOB,
         state_align, state_len, vertices);
}

#endif /* ILO_BUILDER_3D_TOP_H */
