/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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

#ifndef ILO_GPE_GEN6_H
#define ILO_GPE_GEN6_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_cp.h"
#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_gpe.h"

/**
 * Translate winsys tiling to hardware tiling.
 */
static inline int
ilo_gpe_gen6_translate_winsys_tiling(enum intel_tiling_mode tiling)
{
   switch (tiling) {
   case INTEL_TILING_NONE:
      return GEN6_TILING_NONE;
   case INTEL_TILING_X:
      return GEN6_TILING_X;
   case INTEL_TILING_Y:
      return GEN6_TILING_Y;
   default:
      assert(!"unknown tiling");
      return GEN6_TILING_NONE;
   }
}

/**
 * Translate a pipe primitive type to the matching hardware primitive type.
 */
static inline int
ilo_gpe_gen6_translate_pipe_prim(unsigned prim)
{
   static const int prim_mapping[PIPE_PRIM_MAX] = {
      [PIPE_PRIM_POINTS]                     = GEN6_3DPRIM_POINTLIST,
      [PIPE_PRIM_LINES]                      = GEN6_3DPRIM_LINELIST,
      [PIPE_PRIM_LINE_LOOP]                  = GEN6_3DPRIM_LINELOOP,
      [PIPE_PRIM_LINE_STRIP]                 = GEN6_3DPRIM_LINESTRIP,
      [PIPE_PRIM_TRIANGLES]                  = GEN6_3DPRIM_TRILIST,
      [PIPE_PRIM_TRIANGLE_STRIP]             = GEN6_3DPRIM_TRISTRIP,
      [PIPE_PRIM_TRIANGLE_FAN]               = GEN6_3DPRIM_TRIFAN,
      [PIPE_PRIM_QUADS]                      = GEN6_3DPRIM_QUADLIST,
      [PIPE_PRIM_QUAD_STRIP]                 = GEN6_3DPRIM_QUADSTRIP,
      [PIPE_PRIM_POLYGON]                    = GEN6_3DPRIM_POLYGON,
      [PIPE_PRIM_LINES_ADJACENCY]            = GEN6_3DPRIM_LINELIST_ADJ,
      [PIPE_PRIM_LINE_STRIP_ADJACENCY]       = GEN6_3DPRIM_LINESTRIP_ADJ,
      [PIPE_PRIM_TRIANGLES_ADJACENCY]        = GEN6_3DPRIM_TRILIST_ADJ,
      [PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]   = GEN6_3DPRIM_TRISTRIP_ADJ,
   };

   assert(prim_mapping[prim]);

   return prim_mapping[prim];
}

/**
 * Translate a pipe texture target to the matching hardware surface type.
 */
static inline int
ilo_gpe_gen6_translate_texture(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_BUFFER:
      return GEN6_SURFTYPE_BUFFER;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return GEN6_SURFTYPE_1D;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
      return GEN6_SURFTYPE_2D;
   case PIPE_TEXTURE_3D:
      return GEN6_SURFTYPE_3D;
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return GEN6_SURFTYPE_CUBE;
   default:
      assert(!"unknown texture target");
      return GEN6_SURFTYPE_BUFFER;
   }
}

/**
 * Fill in DW2 to DW7 of 3DSTATE_SF.
 */
static inline void
ilo_gpe_gen6_fill_3dstate_sf_raster(const struct ilo_dev_info *dev,
                                    const struct ilo_rasterizer_state *rasterizer,
                                    int num_samples,
                                    enum pipe_format depth_format,
                                    uint32_t *payload, unsigned payload_len)
{
   assert(payload_len == Elements(rasterizer->sf.payload));

   if (rasterizer) {
      const struct ilo_rasterizer_sf *sf = &rasterizer->sf;

      memcpy(payload, sf->payload, sizeof(sf->payload));
      if (num_samples > 1)
         payload[1] |= sf->dw_msaa;
   }
   else {
      payload[0] = 0;
      payload[1] = (num_samples > 1) ? GEN7_SF_DW2_MSRASTMODE_ON_PATTERN : 0;
      payload[2] = 0;
      payload[3] = 0;
      payload[4] = 0;
      payload[5] = 0;
   }

   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      int format;

      /* separate stencil */
      switch (depth_format) {
      case PIPE_FORMAT_Z16_UNORM:
         format = GEN6_ZFORMAT_D16_UNORM;
         break;
      case PIPE_FORMAT_Z32_FLOAT:
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         format = GEN6_ZFORMAT_D32_FLOAT;
         break;
      case PIPE_FORMAT_Z24X8_UNORM:
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         format = GEN6_ZFORMAT_D24_UNORM_X8_UINT;
         break;
      default:
         /* FLOAT surface is assumed when there is no depth buffer */
         format = GEN6_ZFORMAT_D32_FLOAT;
         break;
      }

      payload[0] |= format << GEN7_SF_DW1_DEPTH_FORMAT__SHIFT;
   }
}

/**
 * Fill in DW1 and DW8 to DW19 of 3DSTATE_SF.
 */
static inline void
ilo_gpe_gen6_fill_3dstate_sf_sbe(const struct ilo_dev_info *dev,
                                 const struct ilo_rasterizer_state *rasterizer,
                                 const struct ilo_shader_state *fs,
                                 uint32_t *dw, int num_dwords)
{
   int output_count, vue_offset, vue_len;
   const struct ilo_kernel_routing *routing;

   ILO_DEV_ASSERT(dev, 6, 7.5);
   assert(num_dwords == 13);

   if (!fs) {
      memset(dw, 0, sizeof(dw[0]) * num_dwords);
      dw[0] = 1 << GEN7_SBE_DW1_URB_READ_LEN__SHIFT;
      return;
   }

   output_count = ilo_shader_get_kernel_param(fs, ILO_KERNEL_INPUT_COUNT);
   assert(output_count <= 32);

   routing = ilo_shader_get_kernel_routing(fs);

   vue_offset = routing->source_skip;
   assert(vue_offset % 2 == 0);
   vue_offset /= 2;

   vue_len = (routing->source_len + 1) / 2;
   if (!vue_len)
      vue_len = 1;

   dw[0] = output_count << GEN7_SBE_DW1_ATTR_COUNT__SHIFT |
           vue_len << GEN7_SBE_DW1_URB_READ_LEN__SHIFT |
           vue_offset << GEN7_SBE_DW1_URB_READ_OFFSET__SHIFT;
   if (routing->swizzle_enable)
      dw[0] |= GEN7_SBE_DW1_ATTR_SWIZZLE_ENABLE;

   switch (rasterizer->state.sprite_coord_mode) {
   case PIPE_SPRITE_COORD_UPPER_LEFT:
      dw[0] |= GEN7_SBE_DW1_POINT_SPRITE_TEXCOORD_UPPERLEFT;
      break;
   case PIPE_SPRITE_COORD_LOWER_LEFT:
      dw[0] |= GEN7_SBE_DW1_POINT_SPRITE_TEXCOORD_LOWERLEFT;
      break;
   }

   STATIC_ASSERT(Elements(routing->swizzles) >= 16);
   memcpy(&dw[1], routing->swizzles, 2 * 16);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 268:
    *
    *     "This field (Point Sprite Texture Coordinate Enable) must be
    *      programmed to 0 when non-point primitives are rendered."
    *
    * TODO We do not check that yet.
    */
   dw[9] = routing->point_sprite_enable;

   dw[10] = routing->const_interp_enable;

   /* WrapShortest enables */
   dw[11] = 0;
   dw[12] = 0;
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
gen6_3DSTATE_BINDING_TABLE_POINTERS(struct ilo_builder *builder,
                                    uint32_t vs_binding_table,
                                    uint32_t gs_binding_table,
                                    uint32_t ps_binding_table)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_BINDING_TABLE_POINTERS) |
                        GEN6_PTR_BINDING_TABLE_DW0_VS_CHANGED |
                        GEN6_PTR_BINDING_TABLE_DW0_GS_CHANGED |
                        GEN6_PTR_BINDING_TABLE_DW0_PS_CHANGED |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
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
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLER_STATE_POINTERS) |
                        GEN6_PTR_SAMPLER_DW0_VS_CHANGED |
                        GEN6_PTR_SAMPLER_DW0_GS_CHANGED |
                        GEN6_PTR_SAMPLER_DW0_PS_CHANGED |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = vs_sampler_state;
   dw[2] = gs_sampler_state;
   dw[3] = ps_sampler_state;
}

static inline void
gen6_3DSTATE_URB(struct ilo_builder *builder,
                 int vs_total_size, int gs_total_size,
                 int vs_entry_size, int gs_entry_size)
{
   const uint8_t cmd_len = 3;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_URB) | (cmd_len - 2);
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
   dw[0] = dw0;
   dw[1] = (vs_alloc_size - 1) << GEN6_URB_DW1_VS_ENTRY_SIZE__SHIFT |
           vs_num_entries << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
   dw[2] = gs_num_entries << GEN6_URB_DW2_GS_ENTRY_COUNT__SHIFT |
           (gs_alloc_size - 1) << GEN6_URB_DW2_GS_ENTRY_SIZE__SHIFT;
}

static inline void
gen6_3DSTATE_VERTEX_BUFFERS(struct ilo_builder *builder,
                            const struct ilo_ve_state *ve,
                            const struct ilo_vb_state *vb)
{
   uint8_t cmd_len;
   uint32_t dw0, *dw;
   unsigned hw_idx, pos;

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
   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) |
         (cmd_len - 2);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;

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
   uint32_t dw0, *dw;
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
      dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) |
            (cmd_len - 2);

      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      memcpy(&dw[1], dummy.payload, sizeof(dummy.payload));

      return;
   }

   cmd_len = 2 * (ve->count + prepend_generated_ids) + 1;
   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) |
         (cmd_len - 2);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
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

   if (last_velement_edgeflag) {
      struct ilo_ve_cso edgeflag;

      for (i = 0; i < ve->count - 1; i++)
         memcpy(&dw[2 * i], ve->cso[i].payload, sizeof(ve->cso[i].payload));

      edgeflag = ve->cso[i];
      ve_set_cso_edgeflag(builder->dev, &edgeflag);
      memcpy(&dw[2 * i], edgeflag.payload, sizeof(edgeflag.payload));
   }
   else {
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
   unsigned pos;
   uint32_t dw0, *dw;

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

   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_INDEX_BUFFER) |
         format |
         (cmd_len - 2);
   if (enable_cut_index)
      dw0 |= GEN6_IB_DW0_CUT_INDEX_ENABLE;

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   ilo_builder_batch_reloc(builder, pos + 1, buf->bo, start_offset, 0);
   ilo_builder_batch_reloc(builder, pos + 2, buf->bo, end_offset, 0);
}

static inline void
gen6_3DSTATE_VIEWPORT_STATE_POINTERS(struct ilo_builder *builder,
                                     uint32_t clip_viewport,
                                     uint32_t sf_viewport,
                                     uint32_t cc_viewport)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VIEWPORT_STATE_POINTERS) |
                        GEN6_PTR_VP_DW0_CLIP_CHANGED |
                        GEN6_PTR_VP_DW0_SF_CHANGED |
                        GEN6_PTR_VP_DW0_CC_CHANGED |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = clip_viewport;
   dw[2] = sf_viewport;
   dw[3] = cc_viewport;
}

static inline void
gen6_3DSTATE_CC_STATE_POINTERS(struct ilo_builder *builder,
                               uint32_t blend_state,
                               uint32_t depth_stencil_state,
                               uint32_t color_calc_state)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CC_STATE_POINTERS) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = blend_state | 1;
   dw[2] = depth_stencil_state | 1;
   dw[3] = color_calc_state | 1;
}

static inline void
gen6_3DSTATE_SCISSOR_STATE_POINTERS(struct ilo_builder *builder,
                                    uint32_t scissor_rect)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SCISSOR_STATE_POINTERS) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = scissor_rect;
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
gen6_3DSTATE_GS(struct ilo_builder *builder,
                const struct ilo_shader_state *gs,
                const struct ilo_shader_state *vs,
                int verts_per_prim)
{
   const uint8_t cmd_len = 7;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);
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
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = dw2;
   dw[3] = 0;
   dw[4] = dw4;
   dw[5] = dw5;
   dw[6] = dw6;
}

static inline void
gen6_3DSTATE_CLIP(struct ilo_builder *builder,
                  const struct ilo_rasterizer_state *rasterizer,
                  const struct ilo_shader_state *fs,
                  bool enable_guardband,
                  int num_viewports)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) | (cmd_len - 2);
   uint32_t dw1, dw2, dw3, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   if (rasterizer) {
      int interps;

      dw1 = rasterizer->clip.payload[0];
      dw2 = rasterizer->clip.payload[1];
      dw3 = rasterizer->clip.payload[2];

      if (enable_guardband && rasterizer->clip.can_enable_guardband)
         dw2 |= GEN6_CLIP_DW2_GB_TEST_ENABLE;

      interps = (fs) ?  ilo_shader_get_kernel_param(fs,
            ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS) : 0;

      if (interps & (GEN6_INTERP_NONPERSPECTIVE_PIXEL |
                     GEN6_INTERP_NONPERSPECTIVE_CENTROID |
                     GEN6_INTERP_NONPERSPECTIVE_SAMPLE))
         dw2 |= GEN6_CLIP_DW2_NONPERSPECTIVE_BARYCENTRIC_ENABLE;

      dw3 |= GEN6_CLIP_DW3_RTAINDEX_FORCED_ZERO |
             (num_viewports - 1);
   }
   else {
      dw1 = 0;
      dw2 = 0;
      dw3 = 0;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = dw2;
   dw[3] = dw3;
}

static inline void
gen6_3DSTATE_SF(struct ilo_builder *builder,
                const struct ilo_rasterizer_state *rasterizer,
                const struct ilo_shader_state *fs)
{
   const uint8_t cmd_len = 20;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SF) | (cmd_len - 2);
   uint32_t payload_raster[6], payload_sbe[13], *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_gpe_gen6_fill_3dstate_sf_raster(builder->dev, rasterizer,
         1, PIPE_FORMAT_NONE, payload_raster, Elements(payload_raster));
   ilo_gpe_gen6_fill_3dstate_sf_sbe(builder->dev, rasterizer,
         fs, payload_sbe, Elements(payload_sbe));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = payload_sbe[0];
   memcpy(&dw[2], payload_raster, sizeof(payload_raster));
   memcpy(&dw[8], &payload_sbe[1], sizeof(payload_sbe) - 4);
}

static inline void
gen6_3DSTATE_WM(struct ilo_builder *builder,
                const struct ilo_shader_state *fs,
                int num_samplers,
                const struct ilo_rasterizer_state *rasterizer,
                bool dual_blend, bool cc_may_kill,
                uint32_t hiz_op)
{
   const uint8_t cmd_len = 9;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (cmd_len - 2);
   const int num_samples = 1;
   const struct ilo_shader_cso *fs_cso;
   uint32_t dw2, dw4, dw5, dw6, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   if (!fs) {
      /* see brwCreateContext() */
      const int max_threads = (builder->dev->gt == 2) ? 80 : 40;

      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = 0;
      dw[2] = 0;
      dw[3] = 0;
      dw[4] = hiz_op;
      /* honor the valid range even if dispatching is disabled */
      dw[5] = (max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT;
      dw[6] = 0;
      dw[7] = 0;
      dw[8] = 0;

      return;
   }

   fs_cso = ilo_shader_get_kernel_cso(fs);
   dw2 = fs_cso->payload[0];
   dw4 = fs_cso->payload[1];
   dw5 = fs_cso->payload[2];
   dw6 = fs_cso->payload[3];

   dw2 |= (num_samplers + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 248:
    *
    *     "This bit (Statistics Enable) must be disabled if either of these
    *      bits is set: Depth Buffer Clear , Hierarchical Depth Buffer Resolve
    *      Enable or Depth Buffer Resolve Enable."
    */
   assert(!hiz_op);
   dw4 |= GEN6_WM_DW4_STATISTICS;

   if (cc_may_kill)
      dw5 |= GEN6_WM_DW5_PS_KILL | GEN6_WM_DW5_PS_ENABLE;

   if (dual_blend)
      dw5 |= GEN6_WM_DW5_DUAL_SOURCE_BLEND;

   dw5 |= rasterizer->wm.payload[0];

   dw6 |= rasterizer->wm.payload[1];

   if (num_samples > 1) {
      dw6 |= rasterizer->wm.dw_msaa_rast |
             rasterizer->wm.dw_msaa_disp;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = ilo_shader_get_kernel_offset(fs);
   dw[2] = dw2;
   dw[3] = 0; /* scratch */
   dw[4] = dw4;
   dw[5] = dw5;
   dw[6] = dw6;
   dw[7] = 0; /* kernel 1 */
   dw[8] = 0; /* kernel 2 */
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
   uint32_t buf_dw[4], buf_enabled;
   uint32_t dw0, *dw;

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

   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) |
         buf_enabled << 12 |
         (cmd_len - 2);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen6_3DSTATE_CONSTANT_GS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;
   uint32_t dw0, *dw;

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

   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_GS) |
         buf_enabled << 12 |
         (cmd_len - 2);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen6_3DSTATE_CONSTANT_PS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;
   uint32_t dw0, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);
   assert(num_bufs <= 4);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 287:
    *
    *     "The sum of all four read length fields (each incremented to
    *      represent the actual read length) must be less than or equal to 64"
    */
   buf_enabled = gen6_fill_3dstate_constant(builder->dev,
         bufs, sizes, num_bufs, 64, buf_dw, Elements(buf_dw));

   dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) |
         buf_enabled << 12 |
         (cmd_len - 2);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen6_3DSTATE_SAMPLE_MASK(struct ilo_builder *builder,
                         unsigned sample_mask)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK) |
                        (cmd_len - 2);
   const unsigned valid_mask = 0xf;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   sample_mask &= valid_mask;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = sample_mask;
}

static inline void
gen6_3DSTATE_DRAWING_RECTANGLE(struct ilo_builder *builder,
                               unsigned x, unsigned y,
                               unsigned width, unsigned height)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_DRAWING_RECTANGLE) |
                        (cmd_len - 2);
   unsigned xmax = x + width - 1;
   unsigned ymax = y + height - 1;
   int rect_limit;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) {
      rect_limit = 16383;
   }
   else {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 230:
       *
       *     "[DevSNB] Errata: This field (Clipped Drawing Rectangle Y Min)
       *      must be an even number"
       */
      assert(y % 2 == 0);

      rect_limit = 8191;
   }

   if (x > rect_limit) x = rect_limit;
   if (y > rect_limit) y = rect_limit;
   if (xmax > rect_limit) xmax = rect_limit;
   if (ymax > rect_limit) ymax = rect_limit;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = dw0;
   dw[1] = y << 16 | x;
   dw[2] = ymax << 16 | xmax;

   /*
    * There is no need to set the origin.  It is intended to support front
    * buffer rendering.
    */
   dw[3] = 0;
}

static inline void
zs_align_surface(const struct ilo_dev_info *dev,
                 unsigned align_w, unsigned align_h,
                 struct ilo_zs_surface *zs)
{
   unsigned mask, shift_w, shift_h;
   unsigned width, height;
   uint32_t dw3;

   ILO_DEV_ASSERT(dev, 6, 7.5);

   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      shift_w = 4;
      shift_h = 18;
      mask = 0x3fff;
   }
   else {
      shift_w = 6;
      shift_h = 19;
      mask = 0x1fff;
   }

   dw3 = zs->payload[2];

   /* aligned width and height */
   width = align(((dw3 >> shift_w) & mask) + 1, align_w);
   height = align(((dw3 >> shift_h) & mask) + 1, align_h);

   dw3 = (dw3 & ~((mask << shift_w) | (mask << shift_h))) |
      (width - 1) << shift_w |
      (height - 1) << shift_h;

   zs->payload[2] = dw3;
}

static inline void
gen6_3DSTATE_DEPTH_BUFFER(struct ilo_builder *builder,
                          const struct ilo_zs_surface *zs)
{
   const uint8_t cmd_len = 7;
   unsigned pos;
   uint32_t dw0, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   dw0 = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER);
   dw0 |= (cmd_len - 2);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = zs->payload[0];

   if (zs->bo) {
      ilo_builder_batch_reloc(builder, pos + 2,
            zs->bo, zs->payload[1], INTEL_RELOC_WRITE);
   } else {
      dw[2] = 0;
   }

   dw[3] = zs->payload[2];
   dw[4] = zs->payload[3];
   dw[5] = zs->payload[4];
   dw[6] = zs->payload[5];
}

static inline void
gen6_3DSTATE_POLY_STIPPLE_OFFSET(struct ilo_builder *builder,
                                 int x_offset, int y_offset)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_POLY_STIPPLE_OFFSET) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);
   assert(x_offset >= 0 && x_offset <= 31);
   assert(y_offset >= 0 && y_offset <= 31);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = x_offset << 8 | y_offset;
}

static inline void
gen6_3DSTATE_POLY_STIPPLE_PATTERN(struct ilo_builder *builder,
                                  const struct pipe_poly_stipple *pattern)
{
   const uint8_t cmd_len = 33;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_POLY_STIPPLE_PATTERN) |
                        (cmd_len - 2);
   uint32_t *dw;
   int i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);
   STATIC_ASSERT(Elements(pattern->stipple) == 32);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw++;

   for (i = 0; i < 32; i++)
      dw[i] = pattern->stipple[i];
}

static inline void
gen6_3DSTATE_LINE_STIPPLE(struct ilo_builder *builder,
                          unsigned pattern, unsigned factor)
{
   const uint8_t cmd_len = 3;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_LINE_STIPPLE) |
                        (cmd_len - 2);
   uint32_t *dw;
   unsigned inverse;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);
   assert((pattern & 0xffff) == pattern);
   assert(factor >= 1 && factor <= 256);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = pattern;

   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) {
      /* in U1.16 */
      inverse = (unsigned) (65536.0f / factor);
      dw[2] = inverse << 15 | factor;
   }
   else {
      /* in U1.13 */
      inverse = (unsigned) (8192.0f / factor);
      dw[2] = inverse << 16 | factor;
   }
}

static inline void
gen6_3DSTATE_AA_LINE_PARAMETERS(struct ilo_builder *builder)
{
   const uint8_t cmd_len = 3;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_AA_LINE_PARAMETERS) |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = 0 << 16 | 0;
   dw[2] = 0 << 16 | 0;
}

static inline void
gen6_3DSTATE_GS_SVB_INDEX(struct ilo_builder *builder,
                          int index, unsigned svbi,
                          unsigned max_svbi,
                          bool load_vertex_count)
{
   const uint8_t cmd_len = 4;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS_SVB_INDEX) |
                        (cmd_len - 2);
   uint32_t dw1, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);
   assert(index >= 0 && index < 4);

   dw1 = index << GEN6_SVBI_DW1_INDEX__SHIFT;
   if (load_vertex_count)
      dw1 |= GEN6_SVBI_DW1_LOAD_INTERNAL_VERTEX_COUNT;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = svbi;
   dw[3] = max_svbi;
}

static inline void
gen6_3DSTATE_MULTISAMPLE(struct ilo_builder *builder,
                         int num_samples,
                         const uint32_t *packed_sample_pos,
                         bool pixel_location_center)
{
   const uint8_t cmd_len = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ? 4 : 3;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_MULTISAMPLE) |
                        (cmd_len - 2);
   uint32_t dw1, dw2, dw3, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   dw1 = (pixel_location_center) ?
      GEN6_MULTISAMPLE_DW1_PIXLOC_CENTER : GEN6_MULTISAMPLE_DW1_PIXLOC_UL_CORNER;

   switch (num_samples) {
   case 0:
   case 1:
      dw1 |= GEN6_MULTISAMPLE_DW1_NUMSAMPLES_1;
      dw2 = 0;
      dw3 = 0;
      break;
   case 4:
      dw1 |= GEN6_MULTISAMPLE_DW1_NUMSAMPLES_4;
      dw2 = packed_sample_pos[0];
      dw3 = 0;
      break;
   case 8:
      assert(ilo_dev_gen(builder->dev) >= ILO_GEN(7));
      dw1 |= GEN7_MULTISAMPLE_DW1_NUMSAMPLES_8;
      dw2 = packed_sample_pos[0];
      dw3 = packed_sample_pos[1];
      break;
   default:
      assert(!"unsupported sample count");
      dw1 |= GEN6_MULTISAMPLE_DW1_NUMSAMPLES_1;
      dw2 = 0;
      dw3 = 0;
      break;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = dw2;
   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7))
      dw[2] = dw3;
}

static inline void
gen6_3DSTATE_STENCIL_BUFFER(struct ilo_builder *builder,
                            const struct ilo_zs_surface *zs)
{
   const uint8_t cmd_len = 3;
   uint32_t dw0, *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   dw0 = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER);
   dw0 |= (cmd_len - 2);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   /* see ilo_gpe_init_zs_surface() */
   dw[1] = zs->payload[6];

   if (zs->separate_s8_bo) {
      ilo_builder_batch_reloc(builder, pos + 2,
            zs->separate_s8_bo, zs->payload[7], INTEL_RELOC_WRITE);
   } else {
      dw[2] = 0;
   }
}

static inline void
gen6_3DSTATE_HIER_DEPTH_BUFFER(struct ilo_builder *builder,
                               const struct ilo_zs_surface *zs)
{
   const uint8_t cmd_len = 3;
   uint32_t dw0, *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   dw0 = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER);
   dw0 |= (cmd_len - 2);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   /* see ilo_gpe_init_zs_surface() */
   dw[1] = zs->payload[8];

   if (zs->hiz_bo) {
      ilo_builder_batch_reloc(builder, pos + 2,
            zs->hiz_bo, zs->payload[9], INTEL_RELOC_WRITE);
   } else {
      dw[2] = 0;
   }
}

static inline void
gen6_3DSTATE_CLEAR_PARAMS(struct ilo_builder *builder,
                          uint32_t clear_val)
{
   const uint8_t cmd_len = 2;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
                        GEN6_CLEAR_PARAMS_DW0_VALID |
                        (cmd_len - 2);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = clear_val;
}

static inline void
gen6_3DPRIMITIVE(struct ilo_builder *builder,
                 const struct pipe_draw_info *info,
                 const struct ilo_ib_state *ib,
                 bool rectlist)
{
   const uint8_t cmd_len = 6;
   const int prim = (rectlist) ?
      GEN6_3DPRIM_RECTLIST : ilo_gpe_gen6_translate_pipe_prim(info->mode);
   const int vb_access = (info->indexed) ?
      GEN6_3DPRIM_DW0_ACCESS_RANDOM : GEN6_3DPRIM_DW0_ACCESS_SEQUENTIAL;
   const uint32_t vb_start = info->start +
      ((info->indexed) ? ib->draw_start_offset : 0);
   uint32_t dw0, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   dw0 = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) |
         vb_access |
         prim << GEN6_3DPRIM_DW0_TYPE__SHIFT |
         (cmd_len - 2);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = info->count;
   dw[2] = vb_start;
   dw[3] = info->instance_count;
   dw[4] = info->start_instance;
   dw[5] = info->index_bias;
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

static inline uint32_t
gen6_SF_VIEWPORT(struct ilo_builder *builder,
                 const struct ilo_viewport_cso *viewports,
                 unsigned num_viewports)
{
   const int state_align = 32;
   const int state_len = 8 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 262:
    *
    *     "The viewport-specific state used by the SF unit (SF_VIEWPORT) is
    *      stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_SF_VIEWPORT, state_align, state_len, &dw);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->m00);
      dw[1] = fui(vp->m11);
      dw[2] = fui(vp->m22);
      dw[3] = fui(vp->m30);
      dw[4] = fui(vp->m31);
      dw[5] = fui(vp->m32);
      dw[6] = 0;
      dw[7] = 0;

      dw += 8;
   }

   return state_offset;
}

static inline uint32_t
gen6_CLIP_VIEWPORT(struct ilo_builder *builder,
                   const struct ilo_viewport_cso *viewports,
                   unsigned num_viewports)
{
   const int state_align = 32;
   const int state_len = 4 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 193:
    *
    *     "The viewport-related state is stored as an array of up to 16
    *      elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_CLIP_VIEWPORT, state_align, state_len, &dw);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->min_gbx);
      dw[1] = fui(vp->max_gbx);
      dw[2] = fui(vp->min_gby);
      dw[3] = fui(vp->max_gby);

      dw += 4;
   }

   return state_offset;
}

static inline uint32_t
gen6_CC_VIEWPORT(struct ilo_builder *builder,
                 const struct ilo_viewport_cso *viewports,
                 unsigned num_viewports)
{
   const int state_align = 32;
   const int state_len = 2 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 385:
    *
    *     "The viewport state is stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_CC_VIEWPORT, state_align, state_len, &dw);

   for (i = 0; i < num_viewports; i++) {
      const struct ilo_viewport_cso *vp = &viewports[i];

      dw[0] = fui(vp->min_z);
      dw[1] = fui(vp->max_z);

      dw += 2;
   }

   return state_offset;
}

static inline uint32_t
gen6_COLOR_CALC_STATE(struct ilo_builder *builder,
                      const struct pipe_stencil_ref *stencil_ref,
                      ubyte alpha_ref,
                      const struct pipe_blend_color *blend_color)
{
   const int state_align = 64;
   const int state_len = 6;
   uint32_t state_offset, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_COLOR_CALC, state_align, state_len, &dw);

   dw[0] = stencil_ref->ref_value[0] << 24 |
           stencil_ref->ref_value[1] << 16 |
           GEN6_CC_DW0_ALPHATEST_UNORM8;
   dw[1] = alpha_ref;
   dw[2] = fui(blend_color->color[0]);
   dw[3] = fui(blend_color->color[1]);
   dw[4] = fui(blend_color->color[2]);
   dw[5] = fui(blend_color->color[3]);

   return state_offset;
}

static inline uint32_t
gen6_BLEND_STATE(struct ilo_builder *builder,
                 const struct ilo_blend_state *blend,
                 const struct ilo_fb_state *fb,
                 const struct ilo_dsa_state *dsa)
{
   const int state_align = 64;
   int state_len;
   uint32_t state_offset, *dw;
   unsigned num_targets, i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 376:
    *
    *     "The blend state is stored as an array of up to 8 elements..."
    */
   num_targets = fb->state.nr_cbufs;
   assert(num_targets <= 8);

   if (!num_targets) {
      if (!dsa->dw_alpha)
         return 0;
      /* to be able to reference alpha func */
      num_targets = 1;
   }

   state_len = 2 * num_targets;

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_BLEND, state_align, state_len, &dw);

   for (i = 0; i < num_targets; i++) {
      const unsigned idx = (blend->independent_blend_enable) ? i : 0;
      const struct ilo_blend_cso *cso = &blend->cso[idx];
      const int num_samples = fb->num_samples;
      const struct util_format_description *format_desc =
         (idx < fb->state.nr_cbufs && fb->state.cbufs[idx]) ?
         util_format_description(fb->state.cbufs[idx]->format) : NULL;
      bool rt_is_unorm, rt_is_pure_integer, rt_dst_alpha_forced_one;

      rt_is_unorm = true;
      rt_is_pure_integer = false;
      rt_dst_alpha_forced_one = false;

      if (format_desc) {
         int ch;

         switch (format_desc->format) {
         case PIPE_FORMAT_B8G8R8X8_UNORM:
            /* force alpha to one when the HW format has alpha */
            assert(ilo_translate_render_format(builder->dev,
                     PIPE_FORMAT_B8G8R8X8_UNORM) ==
                  GEN6_FORMAT_B8G8R8A8_UNORM);
            rt_dst_alpha_forced_one = true;
            break;
         default:
            break;
         }

         for (ch = 0; ch < 4; ch++) {
            if (format_desc->channel[ch].type == UTIL_FORMAT_TYPE_VOID)
               continue;

            if (format_desc->channel[ch].pure_integer) {
               rt_is_unorm = false;
               rt_is_pure_integer = true;
               break;
            }

            if (!format_desc->channel[ch].normalized ||
                format_desc->channel[ch].type != UTIL_FORMAT_TYPE_UNSIGNED)
               rt_is_unorm = false;
         }
      }

      dw[0] = cso->payload[0];
      dw[1] = cso->payload[1];

      if (!rt_is_pure_integer) {
         if (rt_dst_alpha_forced_one)
            dw[0] |= cso->dw_blend_dst_alpha_forced_one;
         else
            dw[0] |= cso->dw_blend;
      }

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 365:
       *
       *     "Logic Ops are only supported on *_UNORM surfaces (excluding
       *      _SRGB variants), otherwise Logic Ops must be DISABLED."
       *
       * Since logicop is ignored for non-UNORM color buffers, no special care
       * is needed.
       */
      if (rt_is_unorm)
         dw[1] |= cso->dw_logicop;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 356:
       *
       *     "When NumSamples = 1, AlphaToCoverage and AlphaToCoverage
       *      Dither both must be disabled."
       *
       * There is no such limitation on GEN7, or for AlphaToOne.  But GL
       * requires that anyway.
       */
      if (num_samples > 1)
         dw[1] |= cso->dw_alpha_mod;

      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 382:
       *
       *     "Alpha Test can only be enabled if Pixel Shader outputs a float
       *      alpha value."
       */
      if (!rt_is_pure_integer)
         dw[1] |= dsa->dw_alpha;

      dw += 2;
   }

   return state_offset;
}

static inline uint32_t
gen6_DEPTH_STENCIL_STATE(struct ilo_builder *builder,
                         const struct ilo_dsa_state *dsa)
{
   const int state_align = 64;
   const int state_len = 3;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   STATIC_ASSERT(Elements(dsa->payload) >= state_len);

   return ilo_builder_state_write(builder, ILO_BUILDER_ITEM_DEPTH_STENCIL,
         state_align, state_len, dsa->payload);
}

static inline uint32_t
gen6_SCISSOR_RECT(struct ilo_builder *builder,
                  const struct ilo_scissor_state *scissor,
                  unsigned num_viewports)
{
   const int state_align = 32;
   const int state_len = 2 * num_viewports;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 263:
    *
    *     "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
    *      stored as an array of up to 16 elements..."
    */
   assert(num_viewports && num_viewports <= 16);
   assert(Elements(scissor->payload) >= state_len);

   return ilo_builder_state_write(builder, ILO_BUILDER_ITEM_SCISSOR_RECT,
         state_align, state_len, scissor->payload);
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

   state_offset = ilo_builder_state_pointer(builder,
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
   return ilo_builder_state_write(builder, ILO_BUILDER_ITEM_BLOB,
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

   state_offset = ilo_builder_state_pointer(builder,
         ILO_BUILDER_ITEM_BLOB, state_align, state_len, (uint32_t **) &buf);

   /* zero out the unused range */
   if (size < state_len * 4)
      memset(&buf[size], 0, state_len * 4 - size);

   if (pcb)
      *pcb = buf;

   return state_offset;
}

#endif /* ILO_GPE_GEN6_H */
