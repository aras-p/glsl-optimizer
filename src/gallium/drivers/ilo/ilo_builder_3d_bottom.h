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

#ifndef ILO_BUILDER_3D_BOTTOM_H
#define ILO_BUILDER_3D_BOTTOM_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_format.h"
#include "ilo_shader.h"
#include "ilo_builder.h"
#include "ilo_builder_3d_top.h"

static inline void
gen6_3DSTATE_CLIP(struct ilo_builder *builder,
                  const struct ilo_rasterizer_state *rasterizer,
                  const struct ilo_shader_state *fs,
                  bool enable_guardband,
                  int num_viewports)
{
   const uint8_t cmd_len = 4;
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

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) | (cmd_len - 2);
   dw[1] = dw1;
   dw[2] = dw2;
   dw[3] = dw3;
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
gen6_3DSTATE_SF(struct ilo_builder *builder,
                const struct ilo_rasterizer_state *rasterizer,
                const struct ilo_shader_state *fs)
{
   const uint8_t cmd_len = 20;
   uint32_t payload_raster[6], payload_sbe[13], *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_gpe_gen6_fill_3dstate_sf_raster(builder->dev, rasterizer,
         1, PIPE_FORMAT_NONE, payload_raster, Elements(payload_raster));
   ilo_gpe_gen6_fill_3dstate_sf_sbe(builder->dev, rasterizer,
         fs, payload_sbe, Elements(payload_sbe));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) | (cmd_len - 2);
   dw[1] = payload_sbe[0];
   memcpy(&dw[2], payload_raster, sizeof(payload_raster));
   memcpy(&dw[8], &payload_sbe[1], sizeof(payload_sbe) - 4);
}

static inline void
gen7_3DSTATE_SF(struct ilo_builder *builder,
                const struct ilo_rasterizer_state *rasterizer,
                enum pipe_format zs_format)
{
   const uint8_t cmd_len = 7;
   const int num_samples = 1;
   uint32_t payload[6], *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_gpe_gen6_fill_3dstate_sf_raster(builder->dev,
         rasterizer, num_samples, zs_format,
         payload, Elements(payload));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) | (cmd_len - 2);
   memcpy(&dw[1], payload, sizeof(payload));
}

static inline void
gen7_3DSTATE_SBE(struct ilo_builder *builder,
                 const struct ilo_rasterizer_state *rasterizer,
                 const struct ilo_shader_state *fs)
{
   const uint8_t cmd_len = 14;
   uint32_t payload[13], *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_gpe_gen6_fill_3dstate_sf_sbe(builder->dev,
         rasterizer, fs, payload, Elements(payload));

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SBE) | (cmd_len - 2);
   memcpy(&dw[1], payload, sizeof(payload));
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

static inline void
gen7_3DSTATE_WM(struct ilo_builder *builder,
                const struct ilo_shader_state *fs,
                const struct ilo_rasterizer_state *rasterizer,
                bool cc_may_kill, uint32_t hiz_op)
{
   const uint8_t cmd_len = 3;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (cmd_len - 2);
   const int num_samples = 1;
   uint32_t dw1, dw2, *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /* see ilo_gpe_init_rasterizer_wm() */
   if (rasterizer) {
      dw1 = rasterizer->wm.payload[0];
      dw2 = rasterizer->wm.payload[1];

      assert(!hiz_op);
      dw1 |= GEN7_WM_DW1_STATISTICS;
   }
   else {
      dw1 = hiz_op;
      dw2 = 0;
   }

   if (fs) {
      const struct ilo_shader_cso *fs_cso = ilo_shader_get_kernel_cso(fs);

      dw1 |= fs_cso->payload[3];
   }

   if (cc_may_kill)
      dw1 |= GEN7_WM_DW1_PS_ENABLE | GEN7_WM_DW1_PS_KILL;

   if (num_samples > 1) {
      dw1 |= rasterizer->wm.dw_msaa_rast;
      dw2 |= rasterizer->wm.dw_msaa_disp;
   }

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = dw2;
}

static inline void
gen7_3DSTATE_PS(struct ilo_builder *builder,
                const struct ilo_shader_state *fs,
                int num_samplers, bool dual_blend)
{
   const uint8_t cmd_len = 8;
   const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_PS) | (cmd_len - 2);
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5, *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   if (!fs) {
      int max_threads;

      /* GPU hangs if none of the dispatch enable bits is set */
      dw4 = GEN7_PS_DW4_8_PIXEL_DISPATCH;

      /* see brwCreateContext() */
      switch (ilo_dev_gen(builder->dev)) {
      case ILO_GEN(7.5):
         max_threads = (builder->dev->gt == 3) ? 408 :
                       (builder->dev->gt == 2) ? 204 : 102;
         dw4 |= (max_threads - 1) << GEN75_PS_DW4_MAX_THREADS__SHIFT;
         break;
      case ILO_GEN(7):
      default:
         max_threads = (builder->dev->gt == 2) ? 172 : 48;
         dw4 |= (max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
         break;
      }

      ilo_builder_batch_pointer(builder, cmd_len, &dw);
      dw[0] = dw0;
      dw[1] = 0;
      dw[2] = 0;
      dw[3] = 0;
      dw[4] = dw4;
      dw[5] = 0;
      dw[6] = 0;
      dw[7] = 0;

      return;
   }

   cso = ilo_shader_get_kernel_cso(fs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= (num_samplers + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT;

   if (dual_blend)
      dw4 |= GEN7_PS_DW4_DUAL_SOURCE_BLEND;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = ilo_shader_get_kernel_offset(fs);
   dw[2] = dw2;
   dw[3] = 0; /* scratch */
   dw[4] = dw4;
   dw[5] = dw5;
   dw[6] = 0; /* kernel 1 */
   dw[7] = 0; /* kernel 2 */
}

static inline void
gen6_3DSTATE_CONSTANT_PS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   const uint8_t cmd_len = 5;
   uint32_t buf_dw[4], buf_enabled;
   uint32_t *dw;

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

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) |
           buf_enabled << 12 |
           (cmd_len - 2);
   memcpy(&dw[1], buf_dw, sizeof(buf_dw));
}

static inline void
gen7_3DSTATE_CONSTANT_PS(struct ilo_builder *builder,
                         const uint32_t *bufs, const int *sizes,
                         int num_bufs)
{
   gen7_3dstate_constant(builder, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_PS,
         bufs, sizes, num_bufs);
}

static inline void
gen7_3DSTATE_BINDING_TABLE_POINTERS_PS(struct ilo_builder *builder,
                                       uint32_t binding_table)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_PS,
         binding_table);
}

static inline void
gen7_3DSTATE_SAMPLER_STATE_POINTERS_PS(struct ilo_builder *builder,
                                       uint32_t sampler_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_PS,
         sampler_state);
}

static inline void
gen6_3DSTATE_MULTISAMPLE(struct ilo_builder *builder,
                         int num_samples,
                         const uint32_t *packed_sample_pos,
                         bool pixel_location_center)
{
   const uint8_t cmd_len = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ? 4 : 3;
   uint32_t dw1, dw2, dw3, *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   dw1 = (pixel_location_center) ? GEN6_MULTISAMPLE_DW1_PIXLOC_CENTER :
      GEN6_MULTISAMPLE_DW1_PIXLOC_UL_CORNER;

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

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_MULTISAMPLE) | (cmd_len - 2);
   dw[1] = dw1;
   dw[2] = dw2;
   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7))
      dw[3] = dw3;
}

static inline void
gen6_3DSTATE_SAMPLE_MASK(struct ilo_builder *builder,
                         unsigned sample_mask)
{
   const uint8_t cmd_len = 2;
   const unsigned valid_mask = 0xf;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   sample_mask &= valid_mask;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK) | (cmd_len - 2);
   dw[1] = sample_mask;
}

static inline void
gen7_3DSTATE_SAMPLE_MASK(struct ilo_builder *builder,
                         unsigned sample_mask,
                         int num_samples)
{
   const uint8_t cmd_len = 2;
   const unsigned valid_mask = ((1 << num_samples) - 1) | 0x1;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 294:
    *
    *     "If Number of Multisamples is NUMSAMPLES_1, bits 7:1 of this field
    *      (Sample Mask) must be zero.
    *
    *      If Number of Multisamples is NUMSAMPLES_4, bits 7:4 of this field
    *      must be zero."
    */
   sample_mask &= valid_mask;

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK) | (cmd_len - 2);
   dw[1] = sample_mask;
}

static inline void
gen6_3DSTATE_DRAWING_RECTANGLE(struct ilo_builder *builder,
                               unsigned x, unsigned y,
                               unsigned width, unsigned height)
{
   const uint8_t cmd_len = 4;
   unsigned xmax = x + width - 1;
   unsigned ymax = y + height - 1;
   unsigned rect_limit;
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

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_DRAWING_RECTANGLE) | (cmd_len - 2);
   dw[1] = y << 16 | x;
   dw[2] = ymax << 16 | xmax;
   /*
    * There is no need to set the origin.  It is intended to support front
    * buffer rendering.
    */
   dw[3] = 0;
}

static inline void
gen6_3DSTATE_POLY_STIPPLE_OFFSET(struct ilo_builder *builder,
                                 int x_offset, int y_offset)
{
   const uint8_t cmd_len = 2;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(x_offset >= 0 && x_offset <= 31);
   assert(y_offset >= 0 && y_offset <= 31);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_POLY_STIPPLE_OFFSET) | (cmd_len - 2);
   dw[1] = x_offset << 8 | y_offset;
}

static inline void
gen6_3DSTATE_POLY_STIPPLE_PATTERN(struct ilo_builder *builder,
                                  const struct pipe_poly_stipple *pattern)
{
   const uint8_t cmd_len = 33;
   uint32_t *dw;
   int i;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_POLY_STIPPLE_PATTERN) | (cmd_len - 2);
   dw++;

   STATIC_ASSERT(Elements(pattern->stipple) == 32);
   for (i = 0; i < 32; i++)
      dw[i] = pattern->stipple[i];
}

static inline void
gen6_3DSTATE_LINE_STIPPLE(struct ilo_builder *builder,
                          unsigned pattern, unsigned factor)
{
   const uint8_t cmd_len = 3;
   unsigned inverse;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert((pattern & 0xffff) == pattern);
   assert(factor >= 1 && factor <= 256);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_LINE_STIPPLE) | (cmd_len - 2);
   dw[1] = pattern;

   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) {
      /* in U1.16 */
      inverse = 65536 / factor;

      dw[2] = inverse << GEN7_LINE_STIPPLE_DW2_INVERSE_REPEAT_COUNT__SHIFT |
              factor;
   }
   else {
      /* in U1.13 */
      inverse = 8192 / factor;

      dw[2] = inverse << GEN6_LINE_STIPPLE_DW2_INVERSE_REPEAT_COUNT__SHIFT |
              factor;
   }
}

static inline void
gen6_3DSTATE_AA_LINE_PARAMETERS(struct ilo_builder *builder)
{
   const uint8_t cmd_len = 3;
   const uint32_t dw[3] = {
      GEN6_RENDER_CMD(3D, 3DSTATE_AA_LINE_PARAMETERS) | (cmd_len - 2),
      0 << GEN6_AA_LINE_DW1_BIAS__SHIFT | 0,
      0 << GEN6_AA_LINE_DW2_CAP_BIAS__SHIFT | 0,
   };

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_write(builder, cmd_len, dw);
}

static inline void
gen6_3DSTATE_DEPTH_BUFFER(struct ilo_builder *builder,
                          const struct ilo_zs_surface *zs)
{
   const uint32_t cmd = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER);
   const uint8_t cmd_len = 7;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
   dw[1] = zs->payload[0];
   dw[3] = zs->payload[2];
   dw[4] = zs->payload[3];
   dw[5] = zs->payload[4];
   dw[6] = zs->payload[5];

   if (zs->bo) {
      ilo_builder_batch_reloc(builder, pos + 2,
            zs->bo, zs->payload[1], INTEL_RELOC_WRITE);
   } else {
      dw[2] = 0;
   }
}

static inline void
gen6_3DSTATE_STENCIL_BUFFER(struct ilo_builder *builder,
                            const struct ilo_zs_surface *zs)
{
   const uint32_t cmd = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER);
   const uint8_t cmd_len = 3;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
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
   const uint32_t cmd = (ilo_dev_gen(builder->dev) >= ILO_GEN(7)) ?
      GEN7_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER) :
      GEN6_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER);
   const uint8_t cmd_len = 3;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = cmd | (cmd_len - 2);
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
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
           GEN6_CLEAR_PARAMS_DW0_VALID |
           (cmd_len - 2);
   dw[1] = clear_val;
}

static inline void
gen7_3DSTATE_CLEAR_PARAMS(struct ilo_builder *builder,
                          uint32_t clear_val)
{
   const uint8_t cmd_len = 3;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) | (cmd_len - 2);
   dw[1] = clear_val;
   dw[2] = GEN7_CLEAR_PARAMS_DW2_VALID;
}

static inline void
gen6_3DSTATE_VIEWPORT_STATE_POINTERS(struct ilo_builder *builder,
                                     uint32_t clip_viewport,
                                     uint32_t sf_viewport,
                                     uint32_t cc_viewport)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VIEWPORT_STATE_POINTERS) |
           GEN6_PTR_VP_DW0_CLIP_CHANGED |
           GEN6_PTR_VP_DW0_SF_CHANGED |
           GEN6_PTR_VP_DW0_CC_CHANGED |
           (cmd_len - 2);
   dw[1] = clip_viewport;
   dw[2] = sf_viewport;
   dw[3] = cc_viewport;
}

static inline void
gen6_3DSTATE_SCISSOR_STATE_POINTERS(struct ilo_builder *builder,
                                    uint32_t scissor_rect)
{
   const uint8_t cmd_len = 2;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SCISSOR_STATE_POINTERS) |
           (cmd_len - 2);
   dw[1] = scissor_rect;
}

static inline void
gen6_3DSTATE_CC_STATE_POINTERS(struct ilo_builder *builder,
                               uint32_t blend_state,
                               uint32_t depth_stencil_state,
                               uint32_t color_calc_state)
{
   const uint8_t cmd_len = 4;
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CC_STATE_POINTERS) | (cmd_len - 2);
   dw[1] = blend_state | GEN6_PTR_CC_DW1_BLEND_CHANGED;
   dw[2] = depth_stencil_state | GEN6_PTR_CC_DW2_ZS_CHANGED;
   dw[3] = color_calc_state | GEN6_PTR_CC_DW3_CC_CHANGED;
}

static inline void
gen7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(struct ilo_builder *builder,
                                             uint32_t sf_clip_viewport)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP,
         sf_clip_viewport);
}

static inline void
gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC(struct ilo_builder *builder,
                                        uint32_t cc_viewport)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_CC,
         cc_viewport);
}

static inline void
gen7_3DSTATE_CC_STATE_POINTERS(struct ilo_builder *builder,
                               uint32_t color_calc_state)
{
   gen7_3dstate_pointer(builder,
         GEN6_RENDER_OPCODE_3DSTATE_CC_STATE_POINTERS, color_calc_state);
}

static inline void
gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(struct ilo_builder *builder,
                                          uint32_t depth_stencil_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_DEPTH_STENCIL_STATE_POINTERS,
         depth_stencil_state);
}

static inline void
gen7_3DSTATE_BLEND_STATE_POINTERS(struct ilo_builder *builder,
                                  uint32_t blend_state)
{
   gen7_3dstate_pointer(builder,
         GEN7_RENDER_OPCODE_3DSTATE_BLEND_STATE_POINTERS,
         blend_state);
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
gen7_SF_CLIP_VIEWPORT(struct ilo_builder *builder,
                      const struct ilo_viewport_cso *viewports,
                      unsigned num_viewports)
{
   const int state_align = 64;
   const int state_len = 16 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 270:
    *
    *     "The viewport-specific state used by both the SF and CL units
    *      (SF_CLIP_VIEWPORT) is stored as an array of up to 16 elements, each
    *      of which contains the DWords described below. The start of each
    *      element is spaced 16 DWords apart. The location of first element of
    *      the array, as specified by both Pointer to SF_VIEWPORT and Pointer
    *      to CLIP_VIEWPORT, is aligned to a 64-byte boundary."
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
      dw[8] = fui(vp->min_gbx);
      dw[9] = fui(vp->max_gbx);
      dw[10] = fui(vp->min_gby);
      dw[11] = fui(vp->max_gby);
      dw[12] = 0;
      dw[13] = 0;
      dw[14] = 0;
      dw[15] = 0;

      dw += 16;
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

#endif /* ILO_BUILDER_3D_BOTTOM_H */
