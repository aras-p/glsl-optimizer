/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#include "util/u_resource.h"
#include "brw_defines.h"
#include "intel_reg.h"

#include "ilo_cp.h"
#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_gpe_gen7.h"

static void
gen7_emit_GPGPU_WALKER(const struct ilo_dev_info *dev,
                       struct ilo_cp *cp)
{
   assert(!"GPGPU_WALKER unsupported");
}

static void
gen7_emit_3DSTATE_CLEAR_PARAMS(const struct ilo_dev_info *dev,
                               uint32_t clear_val,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x04);
   const uint8_t cmd_len = 3;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, clear_val);
   ilo_cp_write(cp, 1);
   ilo_cp_end(cp);
}

static void
gen7_emit_3dstate_pointer(const struct ilo_dev_info *dev,
                          int subop, uint32_t pointer,
                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, subop);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, pointer);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_CC_STATE_POINTERS(const struct ilo_dev_info *dev,
                                    uint32_t color_calc_state,
                                    struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x0e, color_calc_state, cp);
}

void
ilo_gpe_init_gs_cso_gen7(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *gs,
                         struct ilo_shader_cso *cso)
{
   int start_grf, vue_read_len, max_threads;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   start_grf = ilo_shader_get_kernel_param(gs, ILO_KERNEL_URB_DATA_START_REG);
   vue_read_len = ilo_shader_get_kernel_param(gs, ILO_KERNEL_INPUT_COUNT);

   /* in pairs */
   vue_read_len = (vue_read_len + 1) / 2;

   switch (dev->gen) {
   case ILO_GEN(7):
      max_threads = (dev->gt == 2) ? 128 : 36;
      break;
   default:
      max_threads = 1;
      break;
   }

   dw2 = (true) ? 0 : GEN6_GS_FLOATING_POINT_MODE_ALT;

   dw4 = vue_read_len << GEN6_GS_URB_READ_LENGTH_SHIFT |
         GEN7_GS_INCLUDE_VERTEX_HANDLES |
         0 << GEN6_GS_URB_ENTRY_READ_OFFSET_SHIFT |
         start_grf << GEN6_GS_DISPATCH_START_GRF_SHIFT;

   dw5 = (max_threads - 1) << GEN6_GS_MAX_THREADS_SHIFT |
         GEN6_GS_STATISTICS_ENABLE |
         GEN6_GS_ENABLE;

   STATIC_ASSERT(Elements(cso->payload) >= 3);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
}

static void
gen7_emit_3DSTATE_GS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *gs,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x11);
   const uint8_t cmd_len = 7;
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   if (!gs) {
      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, GEN6_GS_STATISTICS_ENABLE);
      ilo_cp_write(cp, 0);
      ilo_cp_end(cp);
      return;
   }

   cso = ilo_shader_get_kernel_cso(gs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= ((num_samplers + 3) / 4) << GEN6_GS_SAMPLER_COUNT_SHIFT;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, ilo_shader_get_kernel_offset(gs));
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0); /* scratch */
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, dw5);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_SF(const struct ilo_dev_info *dev,
                     const struct ilo_rasterizer_state *rasterizer,
                     const struct pipe_surface *zs_surf,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x13);
   const uint8_t cmd_len = 7;
   const int num_samples = 1;
   uint32_t payload[6];

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_gpe_gen6_fill_3dstate_sf_raster(dev,
         rasterizer, num_samples,
         (zs_surf) ? zs_surf->format : PIPE_FORMAT_NONE,
         payload, Elements(payload));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write_multi(cp, payload, 6);
   ilo_cp_end(cp);
}

void
ilo_gpe_init_rasterizer_wm_gen7(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm)
{
   uint32_t dw1, dw2;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   dw1 = GEN7_WM_POSITION_ZW_PIXEL |
         GEN7_WM_LINE_AA_WIDTH_2_0 |
         GEN7_WM_MSRAST_OFF_PIXEL;

   /* same value as in 3DSTATE_SF */
   if (state->line_smooth)
      dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_1_0;

   if (state->poly_stipple_enable)
      dw1 |= GEN7_WM_POLYGON_STIPPLE_ENABLE;
   if (state->line_stipple_enable)
      dw1 |= GEN7_WM_LINE_STIPPLE_ENABLE;

   if (state->bottom_edge_rule)
      dw1 |= GEN7_WM_POINT_RASTRULE_UPPER_RIGHT;

   dw2 = GEN7_WM_MSDISPMODE_PERSAMPLE;

   /*
    * assertion that makes sure
    *
    *   dw1 |= wm->dw_msaa_rast;
    *   dw2 |= wm->dw_msaa_disp;
    *
    * is valid
    */
   STATIC_ASSERT(GEN7_WM_MSRAST_OFF_PIXEL == 0 &&
                 GEN7_WM_MSDISPMODE_PERSAMPLE == 0);

   wm->dw_msaa_rast =
      (state->multisample) ? GEN7_WM_MSRAST_ON_PATTERN : 0;
   wm->dw_msaa_disp = GEN7_WM_MSDISPMODE_PERPIXEL;

   STATIC_ASSERT(Elements(wm->payload) >= 2);
   wm->payload[0] = dw1;
   wm->payload[1] = dw2;
}

void
ilo_gpe_init_fs_cso_gen7(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *fs,
                         struct ilo_shader_cso *cso)
{
   int start_grf, max_threads;
   uint32_t dw2, dw4, dw5;
   uint32_t wm_interps, wm_dw1;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   start_grf = ilo_shader_get_kernel_param(fs, ILO_KERNEL_URB_DATA_START_REG);
   /* see brwCreateContext() */
   max_threads = (dev->gt == 2) ? 172 : 48;

   dw2 = (true) ? 0 : GEN7_PS_FLOATING_POINT_MODE_ALT;

   dw4 = (max_threads - 1) << IVB_PS_MAX_THREADS_SHIFT |
         GEN7_PS_POSOFFSET_NONE;

   if (false)
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_INPUT_COUNT))
      dw4 |= GEN7_PS_ATTRIBUTE_ENABLE;

   assert(!ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_DISPATCH_16_OFFSET));
   dw4 |= GEN7_PS_8_DISPATCH_ENABLE;

   dw5 = start_grf << GEN7_PS_DISPATCH_START_GRF_SHIFT_0 |
         0 << GEN7_PS_DISPATCH_START_GRF_SHIFT_1 |
         0 << GEN7_PS_DISPATCH_START_GRF_SHIFT_2;

   /* FS affects 3DSTATE_WM too */
   wm_dw1 = 0;

   /*
    * TODO set this bit only when
    *
    *  a) fs writes colors and color is not masked, or
    *  b) fs writes depth, or
    *  c) fs or cc kills
    */
   wm_dw1 |= GEN7_WM_DISPATCH_ENABLE;

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 278:
    *
    *     "This bit (Pixel Shader Kill Pixel), if ENABLED, indicates that
    *      the PS kernel or color calculator has the ability to kill
    *      (discard) pixels or samples, other than due to depth or stencil
    *      testing. This bit is required to be ENABLED in the following
    *      situations:
    *
    *      - The API pixel shader program contains "killpix" or "discard"
    *        instructions, or other code in the pixel shader kernel that
    *        can cause the final pixel mask to differ from the pixel mask
    *        received on dispatch.
    *
    *      - A sampler with chroma key enabled with kill pixel mode is used
    *        by the pixel shader.
    *
    *      - Any render target has Alpha Test Enable or AlphaToCoverage
    *        Enable enabled.
    *
    *      - The pixel shader kernel generates and outputs oMask.
    *
    *      Note: As ClipDistance clipping is fully supported in hardware
    *      and therefore not via PS instructions, there should be no need
    *      to ENABLE this bit due to ClipDistance clipping."
    */
   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_USE_KILL))
      wm_dw1 |= GEN7_WM_KILL_ENABLE;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_OUTPUT_Z))
      wm_dw1 |= GEN7_WM_PSCDEPTH_ON;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_Z))
      wm_dw1 |= GEN7_WM_USES_SOURCE_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_W))
      wm_dw1 |= GEN7_WM_USES_SOURCE_W;

   wm_interps = ilo_shader_get_kernel_param(fs,
         ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS);

   wm_dw1 |= wm_interps << GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = wm_dw1;
}

static void
gen7_emit_3DSTATE_WM(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *fs,
                     const struct ilo_rasterizer_state *rasterizer,
                     bool cc_may_kill,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x14);
   const uint8_t cmd_len = 3;
   const int num_samples = 1;
   uint32_t dw1, dw2;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   /* see ilo_gpe_init_rasterizer_wm() */
   dw1 = rasterizer->wm.payload[0];
   dw2 = rasterizer->wm.payload[1];

   dw1 |= GEN7_WM_STATISTICS_ENABLE;

   if (false) {
      dw1 |= GEN7_WM_DEPTH_CLEAR;
      dw1 |= GEN7_WM_DEPTH_RESOLVE;
      dw1 |= GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE;
   }

   if (fs) {
      const struct ilo_shader_cso *fs_cso = ilo_shader_get_kernel_cso(fs);

      dw1 |= fs_cso->payload[3];
   }

   if (cc_may_kill) {
      dw1 |= GEN7_WM_DISPATCH_ENABLE |
             GEN7_WM_KILL_ENABLE;
   }

   if (num_samples > 1) {
      dw1 |= rasterizer->wm.dw_msaa_rast;
      dw2 |= rasterizer->wm.dw_msaa_disp;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, dw2);
   ilo_cp_end(cp);
}

static void
gen7_emit_3dstate_constant(const struct ilo_dev_info *dev,
                           int subop,
                           const uint32_t *bufs, const int *sizes,
                           int num_bufs,
                           struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, subop);
   const uint8_t cmd_len = 7;
   uint32_t dw[6];
   int total_read_length, i;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   /* VS, HS, DS, GS, and PS variants */
   assert(subop >= 0x15 && subop <= 0x1a && subop != 0x18);

   assert(num_bufs <= 4);

   dw[0] = 0;
   dw[1] = 0;

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
            dw[2 + i] = 0;
         }
         break;
      }

      /* read lengths are in 256-bit units */
      read_len = (sizes[i] + 31) / 32;
      /* the lower 5 bits are used for memory object control state */
      assert(bufs[i] % 32 == 0);

      dw[i / 2] |= read_len << ((i % 2) ? 16 : 0);
      dw[2 + i] = bufs[i];

      total_read_length += read_len;
   }

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 113:
    *
    *     "The sum of all four read length fields must be less than or equal
    *      to the size of 64"
    */
   assert(total_read_length <= 64);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write_multi(cp, dw, 6);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_CONSTANT_VS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x15, bufs, sizes, num_bufs, cp);
}

static void
gen7_emit_3DSTATE_CONSTANT_GS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x16, bufs, sizes, num_bufs, cp);
}

static void
gen7_emit_3DSTATE_CONSTANT_PS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x17, bufs, sizes, num_bufs, cp);
}

static void
gen7_emit_3DSTATE_SAMPLE_MASK(const struct ilo_dev_info *dev,
                              unsigned sample_mask,
                              int num_samples,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x18);
   const uint8_t cmd_len = 2;
   const unsigned valid_mask = ((1 << num_samples) - 1) | 0x1;

   ILO_GPE_VALID_GEN(dev, 7, 7);

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

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, sample_mask);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_CONSTANT_HS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x19, bufs, sizes, num_bufs, cp);
}

static void
gen7_emit_3DSTATE_CONSTANT_DS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x1a, bufs, sizes, num_bufs, cp);
}

static void
gen7_emit_3DSTATE_HS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *hs,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1b);
   const uint8_t cmd_len = 7;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   assert(!hs);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_TE(const struct ilo_dev_info *dev,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1c);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_DS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *ds,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1d);
   const uint8_t cmd_len = 6;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   assert(!ds);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);

}

static void
gen7_emit_3DSTATE_STREAMOUT(const struct ilo_dev_info *dev,
                            unsigned buffer_mask,
                            int vertex_attrib_count,
                            bool rasterizer_discard,
                            struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1e);
   const uint8_t cmd_len = 3;
   const bool enable = (buffer_mask != 0);
   uint32_t dw1, dw2;
   int read_len;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   if (!enable) {
      dw1 = 0 << SO_RENDER_STREAM_SELECT_SHIFT;
      if (rasterizer_discard)
         dw1 |= SO_RENDERING_DISABLE;

      dw2 = 0;

      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, dw1);
      ilo_cp_write(cp, dw2);
      ilo_cp_end(cp);
      return;
   }

   read_len = (vertex_attrib_count + 1) / 2;
   if (!read_len)
      read_len = 1;

   dw1 = SO_FUNCTION_ENABLE |
         0 << SO_RENDER_STREAM_SELECT_SHIFT |
         SO_STATISTICS_ENABLE |
         buffer_mask << 8;

   if (rasterizer_discard)
      dw1 |= SO_RENDERING_DISABLE;

   /* API_OPENGL */
   if (true)
      dw1 |= SO_REORDER_TRAILING;

   dw2 = 0 << SO_STREAM_3_VERTEX_READ_OFFSET_SHIFT |
         0 << SO_STREAM_3_VERTEX_READ_LENGTH_SHIFT |
         0 << SO_STREAM_2_VERTEX_READ_OFFSET_SHIFT |
         0 << SO_STREAM_2_VERTEX_READ_LENGTH_SHIFT |
         0 << SO_STREAM_1_VERTEX_READ_OFFSET_SHIFT |
         0 << SO_STREAM_1_VERTEX_READ_LENGTH_SHIFT |
         0 << SO_STREAM_0_VERTEX_READ_OFFSET_SHIFT |
         (read_len - 1) << SO_STREAM_0_VERTEX_READ_LENGTH_SHIFT;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, dw2);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_SBE(const struct ilo_dev_info *dev,
                      const struct ilo_rasterizer_state *rasterizer,
                      const struct ilo_shader_state *fs,
                      const struct ilo_shader_state *last_sh,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1f);
   const uint8_t cmd_len = 14;
   uint32_t dw[13];

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_gpe_gen6_fill_3dstate_sf_sbe(dev, rasterizer,
         fs, last_sh, dw, Elements(dw));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write_multi(cp, dw, 13);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_PS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *fs,
                     int num_samplers, bool dual_blend,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x20);
   const uint8_t cmd_len = 8;
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   if (!fs) {
      /* see brwCreateContext() */
      const int max_threads = (dev->gt == 2) ? 172 : 48;

      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      /* GPU hangs if none of the dispatch enable bits is set */
      ilo_cp_write(cp, (max_threads - 1) << IVB_PS_MAX_THREADS_SHIFT |
                       GEN7_PS_8_DISPATCH_ENABLE);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_end(cp);

      return;
   }

   cso = ilo_shader_get_kernel_cso(fs);
   dw2 = cso->payload[0];
   dw4 = cso->payload[1];
   dw5 = cso->payload[2];

   dw2 |= (num_samplers + 3) / 4 << GEN7_PS_SAMPLER_COUNT_SHIFT;

   if (dual_blend)
      dw4 |= GEN7_PS_DUAL_SOURCE_BLEND_ENABLE;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, ilo_shader_get_kernel_offset(fs));
   ilo_cp_write(cp, dw2);
   ilo_cp_write(cp, 0); /* scratch */
   ilo_cp_write(cp, dw4);
   ilo_cp_write(cp, dw5);
   ilo_cp_write(cp, 0); /* kernel 1 */
   ilo_cp_write(cp, 0); /* kernel 2 */
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(const struct ilo_dev_info *dev,
                                                  uint32_t sf_clip_viewport,
                                                  struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x21, sf_clip_viewport, cp);
}

static void
gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_CC(const struct ilo_dev_info *dev,
                                             uint32_t cc_viewport,
                                             struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x23, cc_viewport, cp);
}

static void
gen7_emit_3DSTATE_BLEND_STATE_POINTERS(const struct ilo_dev_info *dev,
                                       uint32_t blend_state,
                                       struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x24, blend_state, cp);
}

static void
gen7_emit_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(const struct ilo_dev_info *dev,
                                               uint32_t depth_stencil_state,
                                               struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x25, depth_stencil_state, cp);
}

static void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_VS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x26, binding_table, cp);
}

static void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_HS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x27, binding_table, cp);
}

static void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_DS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x28, binding_table, cp);
}

static void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_GS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x29, binding_table, cp);
}

static void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_PS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2a, binding_table, cp);
}

static void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_VS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2b, sampler_state, cp);
}

static void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_HS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2c, sampler_state, cp);
}

static void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_DS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2d, sampler_state, cp);
}

static void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_GS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2e, sampler_state, cp);
}

static void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_PS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2f, sampler_state, cp);
}

static void
gen7_emit_3dstate_urb(const struct ilo_dev_info *dev,
                      int subop, int offset, int size,
                      int entry_size,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, subop);
   const uint8_t cmd_len = 2;
   const int row_size = 64; /* 512 bits */
   int alloc_size, num_entries, min_entries, max_entries;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   /* VS, HS, DS, and GS variants */
   assert(subop >= 0x30 && subop <= 0x33);

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
   if (subop == 0x30 && alloc_size == 5)
      alloc_size = 6;

   /* in multiples of 8 */
   num_entries = (size / row_size / alloc_size) & ~7;

   switch (subop) {
   case 0x30: /* 3DSTATE_URB_VS */
      min_entries = 32;
      max_entries = (dev->gt == 2) ? 704 : 512;

      assert(num_entries >= min_entries);
      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   case 0x31: /* 3DSTATE_URB_HS */
      max_entries = (dev->gt == 2) ? 64 : 32;
      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   case 0x32: /* 3DSTATE_URB_DS */
      if (num_entries)
         assert(num_entries >= 138);
      break;
   case 0x33: /* 3DSTATE_URB_GS */
      max_entries = (dev->gt == 2) ? 320 : 192;
      if (num_entries > max_entries)
         num_entries = max_entries;
      break;
   default:
      break;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, offset << GEN7_URB_STARTING_ADDRESS_SHIFT |
                    (alloc_size - 1) << GEN7_URB_ENTRY_SIZE_SHIFT |
                    num_entries);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_URB_VS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x30, offset, size, entry_size, cp);
}

static void
gen7_emit_3DSTATE_URB_HS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x31, offset, size, entry_size, cp);
}

static void
gen7_emit_3DSTATE_URB_DS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x32, offset, size, entry_size, cp);
}

static void
gen7_emit_3DSTATE_URB_GS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x33, offset, size, entry_size, cp);
}

static void
gen7_emit_3dstate_push_constant_alloc(const struct ilo_dev_info *dev,
                                      int subop, int offset, int size,
                                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, subop);
   const uint8_t cmd_len = 2;
   int end;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   /* VS, HS, DS, GS, and PS variants */
   assert(subop >= 0x12 && subop <= 0x16);

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

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, offset << GEN7_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT |
                    size);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_VS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x12, offset, size, cp);
}

static void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_HS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x13, offset, size, cp);
}

static void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_DS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x14, offset, size, cp);
}

static void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_GS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x15, offset, size, cp);
}

static void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_PS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x16, offset, size, cp);
}

static void
gen7_emit_3DSTATE_SO_DECL_LIST(const struct ilo_dev_info *dev,
                               const struct pipe_stream_output_info *so_info,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x17);
   uint16_t cmd_len;
   int buffer_selects, num_entries, i;
   uint16_t so_decls[128];

   ILO_GPE_VALID_GEN(dev, 7, 7);

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

            decl = buf << SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT |
                   SO_DECL_HOLE_FLAG |
                   ((1 << num_dwords) - 1) << SO_DECL_COMPONENT_MASK_SHIFT;

            so_decls[num_entries++] = decl;
            buffer_offsets[buf] += num_dwords;
         }

         reg = so_info->output[i].register_index;
         mask = ((1 << so_info->output[i].num_components) - 1) <<
            so_info->output[i].start_component;

         decl = buf << SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT |
                reg << SO_DECL_REGISTER_INDEX_SHIFT |
                mask << SO_DECL_COMPONENT_MASK_SHIFT;

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

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0 << SO_STREAM_TO_BUFFER_SELECTS_3_SHIFT |
                    0 << SO_STREAM_TO_BUFFER_SELECTS_2_SHIFT |
                    0 << SO_STREAM_TO_BUFFER_SELECTS_1_SHIFT |
                    buffer_selects << SO_STREAM_TO_BUFFER_SELECTS_0_SHIFT);
   ilo_cp_write(cp, 0 << SO_NUM_ENTRIES_3_SHIFT |
                    0 << SO_NUM_ENTRIES_2_SHIFT |
                    0 << SO_NUM_ENTRIES_1_SHIFT |
                    num_entries << SO_NUM_ENTRIES_0_SHIFT);

   for (i = 0; i < num_entries; i++) {
      ilo_cp_write(cp, so_decls[i]);
      ilo_cp_write(cp, 0);
   }
   for (; i < 128; i++) {
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
   }

   ilo_cp_end(cp);
}

static void
gen7_emit_3DSTATE_SO_BUFFER(const struct ilo_dev_info *dev,
                            int index, int base, int stride,
                            const struct pipe_stream_output_target *so_target,
                            struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x18);
   const uint8_t cmd_len = 4;
   struct ilo_buffer *buf;
   int end;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   if (!so_target || !so_target->buffer) {
      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, index << SO_BUFFER_INDEX_SHIFT);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_end(cp);
      return;
   }

   buf = ilo_buffer(so_target->buffer);

   /* DWord-aligned */
   assert(stride % 4 == 0 && base % 4 == 0);
   assert(so_target->buffer_offset % 4 == 0);

   stride &= ~3;
   base = (base + so_target->buffer_offset) & ~3;
   end = (base + so_target->buffer_size) & ~3;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, index << SO_BUFFER_INDEX_SHIFT |
                    stride);
   ilo_cp_write_bo(cp, base, buf->bo, INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_write_bo(cp, end, buf->bo, INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_end(cp);
}

static void
gen7_emit_3DPRIMITIVE(const struct ilo_dev_info *dev,
                      const struct pipe_draw_info *info,
                      const struct ilo_ib_state *ib,
                      bool rectlist,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x3, 0x00);
   const uint8_t cmd_len = 7;
   const int prim = (rectlist) ?
      _3DPRIM_RECTLIST : ilo_gpe_gen6_translate_pipe_prim(info->mode);
   const int vb_access = (info->indexed) ?
      GEN7_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM :
      GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL;
   const uint32_t vb_start = info->start +
      ((info->indexed) ? ib->draw_start_offset : 0);

   ILO_GPE_VALID_GEN(dev, 7, 7);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, vb_access | prim);
   ilo_cp_write(cp, info->count);
   ilo_cp_write(cp, vb_start);
   ilo_cp_write(cp, info->instance_count);
   ilo_cp_write(cp, info->start_instance);
   ilo_cp_write(cp, info->index_bias);
   ilo_cp_end(cp);
}

static uint32_t
gen7_emit_SF_CLIP_VIEWPORT(const struct ilo_dev_info *dev,
                           const struct ilo_viewport_cso *viewports,
                           unsigned num_viewports,
                           struct ilo_cp *cp)
{
   const int state_align = 64 / 4;
   const int state_len = 16 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 7, 7);

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

   dw = ilo_cp_steal_ptr(cp, "SF_CLIP_VIEWPORT",
         state_len, state_align, &state_offset);

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

void
ilo_gpe_init_view_surface_null_gen7(const struct ilo_dev_info *dev,
                                    unsigned width, unsigned height,
                                    unsigned depth, unsigned level,
                                    struct ilo_view_surface *surf)
{
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 62:
    *
    *     "A null surface is used in instances where an actual surface is not
    *      bound. When a write message is generated to a null surface, no
    *      actual surface is written to. When a read message (including any
    *      sampling engine message) is generated to a null surface, the result
    *      is all zeros.  Note that a null surface type is allowed to be used
    *      with all messages, even if it is not specificially indicated as
    *      supported. All of the remaining fields in surface state are ignored
    *      for null surfaces, with the following exceptions:
    *
    *      * Width, Height, Depth, LOD, and Render Target View Extent fields
    *        must match the depth buffer's corresponding state for all render
    *        target surfaces, including null.
    *      * All sampling engine and data port messages support null surfaces
    *        with the above behavior, even if not mentioned as specifically
    *        supported, except for the following:
    *        * Data Port Media Block Read/Write messages.
    *      * The Surface Type of a surface used as a render target (accessed
    *        via the Data Port's Render Target Write message) must be the same
    *        as the Surface Type of all other render targets and of the depth
    *        buffer (defined in 3DSTATE_DEPTH_BUFFER), unless either the depth
    *        buffer or render targets are SURFTYPE_NULL."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 65:
    *
    *     "If Surface Type is SURFTYPE_NULL, this field (Tiled Surface) must be
    *      true"
    */

   STATIC_ASSERT(Elements(surf->payload) >= 8);
   dw = surf->payload;

   dw[0] = BRW_SURFACE_NULL << BRW_SURFACE_TYPE_SHIFT |
           BRW_SURFACEFORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT |
           BRW_SURFACE_TILED << 13;

   dw[1] = 0;

   dw[2] = SET_FIELD(height - 1, GEN7_SURFACE_HEIGHT) |
           SET_FIELD(width  - 1, GEN7_SURFACE_WIDTH);

   dw[3] = SET_FIELD(depth - 1, BRW_SURFACE_DEPTH);

   dw[4] = 0;
   dw[5] = level;

   dw[6] = 0;
   dw[7] = 0;

   surf->bo = NULL;
}

void
ilo_gpe_init_view_surface_for_buffer_gen7(const struct ilo_dev_info *dev,
                                          const struct ilo_buffer *buf,
                                          unsigned offset, unsigned size,
                                          unsigned struct_size,
                                          enum pipe_format elem_format,
                                          bool is_rt, bool render_cache_rw,
                                          struct ilo_view_surface *surf)
{
   const bool typed = (elem_format != PIPE_FORMAT_NONE);
   const bool structured = (!typed && struct_size > 1);
   const int elem_size = (typed) ?
      util_format_get_blocksize(elem_format) : 1;
   int width, height, depth, pitch;
   int surface_type, surface_format, num_entries;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   surface_type = (structured) ? 5 : BRW_SURFACE_BUFFER;

   surface_format = (typed) ?
      ilo_translate_color_format(elem_format) : BRW_SURFACEFORMAT_RAW;

   num_entries = size / struct_size;
   /* see if there is enough space to fit another element */
   if (size % struct_size >= elem_size && !structured)
      num_entries++;

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 67:
    *
    *     "For SURFTYPE_BUFFER render targets, this field (Surface Base
    *      Address) specifies the base address of first element of the
    *      surface. The surface is interpreted as a simple array of that
    *      single element type. The address must be naturally-aligned to the
    *      element size (e.g., a buffer containing R32G32B32A32_FLOAT elements
    *      must be 16-byte aligned)
    *
    *      For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    */
   if (is_rt)
      assert(offset % elem_size == 0);

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 68:
    *
    *     "For typed buffer and structured buffer surfaces, the number of
    *      entries in the buffer ranges from 1 to 2^27.  For raw buffer
    *      surfaces, the number of entries in the buffer is the number of
    *      bytes which can range from 1 to 2^30."
    */
   assert(num_entries >= 1 &&
          num_entries <= 1 << ((typed || structured) ? 27 : 30));

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 69:
    *
    *     "For SURFTYPE_BUFFER: The low two bits of this field (Width) must be
    *      11 if the Surface Format is RAW (the size of the buffer must be a
    *      multiple of 4 bytes)."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 70:
    *
    *     "For surfaces of type SURFTYPE_BUFFER and SURFTYPE_STRBUF, this
    *      field (Surface Pitch) indicates the size of the structure."
    *
    *     "For linear surfaces with Surface Type of SURFTYPE_STRBUF, the pitch
    *      must be a multiple of 4 bytes."
    */
   if (structured)
      assert(struct_size % 4 == 0);
   else if (!typed)
      assert(num_entries % 4 == 0);

   pitch = struct_size;

   pitch--;
   num_entries--;
   /* bits [6:0] */
   width  = (num_entries & 0x0000007f);
   /* bits [20:7] */
   height = (num_entries & 0x001fff80) >> 7;
   /* bits [30:21] */
   depth  = (num_entries & 0x7fe00000) >> 21;
   /* limit to [26:21] */
   if (typed || structured)
      depth &= 0x3f;

   STATIC_ASSERT(Elements(surf->payload) >= 8);
   dw = surf->payload;

   dw[0] = surface_type << BRW_SURFACE_TYPE_SHIFT |
           surface_format << BRW_SURFACE_FORMAT_SHIFT;
   if (render_cache_rw)
      dw[0] |= BRW_SURFACE_RC_READ_WRITE;

   dw[1] = offset;

   dw[2] = SET_FIELD(height, GEN7_SURFACE_HEIGHT) |
           SET_FIELD(width, GEN7_SURFACE_WIDTH);

   dw[3] = SET_FIELD(depth, BRW_SURFACE_DEPTH) |
           pitch;

   dw[4] = 0;
   dw[5] = 0;

   dw[6] = 0;
   dw[7] = 0;

   /* do not increment reference count */
   surf->bo = buf->bo;
}

void
ilo_gpe_init_view_surface_for_texture_gen7(const struct ilo_dev_info *dev,
                                           const struct ilo_texture *tex,
                                           enum pipe_format format,
                                           unsigned first_level,
                                           unsigned num_levels,
                                           unsigned first_layer,
                                           unsigned num_layers,
                                           bool is_rt, bool render_cache_rw,
                                           struct ilo_view_surface *surf)
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;
   unsigned layer_offset, x_offset, y_offset;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 7, 7);

   surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);
   assert(surface_type != BRW_SURFACE_BUFFER);

   if (format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT && tex->separate_s8)
      format = PIPE_FORMAT_Z32_FLOAT;

   if (is_rt)
      surface_format = ilo_translate_render_format(format);
   else
      surface_format = ilo_translate_texture_format(format);
   assert(surface_format >= 0);

   width = tex->base.width0;
   height = tex->base.height0;
   depth = (tex->base.target == PIPE_TEXTURE_3D) ?
      tex->base.depth0 : num_layers;
   pitch = tex->bo_stride;

   if (surface_type == BRW_SURFACE_CUBE) {
      /*
       * From the Ivy Bridge PRM, volume 4 part 1, page 70:
       *
       *     "For SURFTYPE_CUBE:For Sampling Engine Surfaces, the range of
       *      this field is [0,340], indicating the number of cube array
       *      elements (equal to the number of underlying 2D array elements
       *      divided by 6). For other surfaces, this field must be zero."
       *
       * When is_rt is true, we treat the texture as a 2D one to avoid the
       * restriction.
       */
      if (is_rt) {
         surface_type = BRW_SURFACE_2D;
      }
      else {
         assert(num_layers % 6 == 0);
         depth = num_layers / 6;
      }
   }

   /* sanity check the size */
   assert(width >= 1 && height >= 1 && depth >= 1 && pitch >= 1);
   assert(first_layer < 2048 && num_layers <= 2048);
   switch (surface_type) {
   case BRW_SURFACE_1D:
      assert(width <= 16384 && height == 1 && depth <= 2048);
      break;
   case BRW_SURFACE_2D:
      assert(width <= 16384 && height <= 16384 && depth <= 2048);
      break;
   case BRW_SURFACE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case BRW_SURFACE_CUBE:
      assert(width <= 16384 && height <= 16384 && depth <= 86);
      assert(width == height);
      if (is_rt)
         assert(first_layer == 0);
      break;
   default:
      assert(!"unexpected surface type");
      break;
   }

   if (is_rt) {
      /*
       * Compute the offset to the layer manually.
       *
       * For rendering, the hardware requires LOD to be the same for all
       * render targets and the depth buffer.  We need to compute the offset
       * to the layer manually and always set LOD to 0.
       */
      if (true) {
         /* we lose the capability for layered rendering */
         assert(num_layers == 1);

         layer_offset = ilo_texture_get_slice_offset(tex,
               first_level, first_layer, &x_offset, &y_offset);

         assert(x_offset % 4 == 0);
         assert(y_offset % 2 == 0);
         x_offset /= 4;
         y_offset /= 2;

         /* derive the size for the LOD */
         width = u_minify(width, first_level);
         height = u_minify(height, first_level);
         if (surface_type == BRW_SURFACE_3D)
            depth = u_minify(depth, first_level);
         else
            depth = 1;

         first_level = 0;
         first_layer = 0;
         lod = 0;
      }
      else {
         layer_offset = 0;
         x_offset = 0;
         y_offset = 0;
      }

      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      layer_offset = 0;
      x_offset = 0;
      y_offset = 0;

      lod = num_levels - 1;
   }

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 68:
    *
    *     "The Base Address for linear render target surfaces and surfaces
    *      accessed with the typed surface read/write data port messages must
    *      be element-size aligned, for non-YUV surface formats, or a multiple
    *      of 2 element-sizes for YUV surface formats.  Other linear surfaces
    *      have no alignment requirements (byte alignment is sufficient)."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 70:
    *
    *     "For linear render target surfaces and surfaces accessed with the
    *      typed data port messages, the pitch must be a multiple of the
    *      element size for non-YUV surface formats. Pitch must be a multiple
    *      of 2 * element size for YUV surface formats. For linear surfaces
    *      with Surface Type of SURFTYPE_STRBUF, the pitch must be a multiple
    *      of 4 bytes.For other linear surfaces, the pitch can be any multiple
    *      of bytes."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 74:
    *
    *     "For linear surfaces, this field (X Offset) must be zero."
    */
   if (tex->tiling == INTEL_TILING_NONE) {
      if (is_rt) {
         const int elem_size = util_format_get_blocksize(format);
         assert(layer_offset % elem_size == 0);
         assert(pitch % elem_size == 0);
      }

      assert(!x_offset);
   }

   STATIC_ASSERT(Elements(surf->payload) >= 8);
   dw = surf->payload;

   dw[0] = surface_type << BRW_SURFACE_TYPE_SHIFT |
           surface_format << BRW_SURFACE_FORMAT_SHIFT |
           ilo_gpe_gen6_translate_winsys_tiling(tex->tiling) << 13;

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 63:
    *
    *     "If this field (Surface Array) is enabled, the Surface Type must be
    *      SURFTYPE_1D, SURFTYPE_2D, or SURFTYPE_CUBE. If this field is
    *      disabled and Surface Type is SURFTYPE_1D, SURFTYPE_2D, or
    *      SURFTYPE_CUBE, the Depth field must be set to zero."
    *
    * For non-3D sampler surfaces, resinfo (the sampler message) always
    * returns zero for the number of layers when this field is not set.
    */
   if (surface_type != BRW_SURFACE_3D) {
      if (util_resource_is_array_texture(&tex->base))
         dw[0] |= GEN7_SURFACE_IS_ARRAY;
      else
         assert(depth == 1);
   }

   if (tex->valign_4)
      dw[0] |= GEN7_SURFACE_VALIGN_4;

   if (tex->halign_8)
      dw[0] |= GEN7_SURFACE_HALIGN_8;

   if (tex->array_spacing_full)
      dw[0] |= GEN7_SURFACE_ARYSPC_FULL;
   else
      dw[0] |= GEN7_SURFACE_ARYSPC_LOD0;

   if (render_cache_rw)
      dw[0] |= BRW_SURFACE_RC_READ_WRITE;

   if (surface_type == BRW_SURFACE_CUBE && !is_rt)
      dw[0] |= BRW_SURFACE_CUBEFACE_ENABLES;

   dw[1] = layer_offset;

   dw[2] = SET_FIELD(height - 1, GEN7_SURFACE_HEIGHT) |
           SET_FIELD(width - 1, GEN7_SURFACE_WIDTH);

   dw[3] = SET_FIELD(depth - 1, BRW_SURFACE_DEPTH) |
           (pitch - 1);

   dw[4] = first_layer << 18 |
           (num_layers - 1) << 7;

   /*
    * MSFMT_MSS means the samples are not interleaved and MSFMT_DEPTH_STENCIL
    * means the samples are interleaved.  The layouts are the same when the
    * number of samples is 1.
    */
   if (tex->interleaved && tex->base.nr_samples > 1) {
      assert(!is_rt);
      dw[4] |= GEN7_SURFACE_MSFMT_DEPTH_STENCIL;
   }
   else {
      dw[4] |= GEN7_SURFACE_MSFMT_MSS;
   }

   if (tex->base.nr_samples > 4)
      dw[4] |= GEN7_SURFACE_MULTISAMPLECOUNT_8;
   else if (tex->base.nr_samples > 2)
      dw[4] |= GEN7_SURFACE_MULTISAMPLECOUNT_4;
   else
      dw[4] |= GEN7_SURFACE_MULTISAMPLECOUNT_1;

   dw[5] = x_offset << BRW_SURFACE_X_OFFSET_SHIFT |
           y_offset << BRW_SURFACE_Y_OFFSET_SHIFT |
           SET_FIELD(first_level, GEN7_SURFACE_MIN_LOD) |
           lod;

   dw[6] = 0;
   dw[7] = 0;

   /* do not increment reference count */
   surf->bo = tex->bo;
}

static int
gen7_estimate_command_size(const struct ilo_dev_info *dev,
                           enum ilo_gpe_gen7_command cmd,
                           int arg)
{
   static const struct {
      int header;
      int body;
   } gen7_command_size_table[ILO_GPE_GEN7_COMMAND_COUNT] = {
      [ILO_GPE_GEN7_STATE_BASE_ADDRESS]                       = { 0,  10 },
      [ILO_GPE_GEN7_STATE_SIP]                                = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_VF_STATISTICS]                    = { 0,  1  },
      [ILO_GPE_GEN7_PIPELINE_SELECT]                          = { 0,  1  },
      [ILO_GPE_GEN7_MEDIA_VFE_STATE]                          = { 0,  8  },
      [ILO_GPE_GEN7_MEDIA_CURBE_LOAD]                         = { 0,  4  },
      [ILO_GPE_GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD]          = { 0,  4  },
      [ILO_GPE_GEN7_MEDIA_STATE_FLUSH]                        = { 0,  2  },
      [ILO_GPE_GEN7_GPGPU_WALKER]                             = { 0,  11 },
      [ILO_GPE_GEN7_3DSTATE_CLEAR_PARAMS]                     = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_DEPTH_BUFFER]                     = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_STENCIL_BUFFER]                   = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_HIER_DEPTH_BUFFER]                = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_VERTEX_BUFFERS]                   = { 1,  4  },
      [ILO_GPE_GEN7_3DSTATE_VERTEX_ELEMENTS]                  = { 1,  2  },
      [ILO_GPE_GEN7_3DSTATE_INDEX_BUFFER]                     = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_CC_STATE_POINTERS]                = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SCISSOR_STATE_POINTERS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_VS]                               = { 0,  6  },
      [ILO_GPE_GEN7_3DSTATE_GS]                               = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_CLIP]                             = { 0,  4  },
      [ILO_GPE_GEN7_3DSTATE_SF]                               = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_WM]                               = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_CONSTANT_VS]                      = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_CONSTANT_GS]                      = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_CONSTANT_PS]                      = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLE_MASK]                      = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_CONSTANT_HS]                      = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_CONSTANT_DS]                      = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_HS]                               = { 0,  7  },
      [ILO_GPE_GEN7_3DSTATE_TE]                               = { 0,  4  },
      [ILO_GPE_GEN7_3DSTATE_DS]                               = { 0,  6  },
      [ILO_GPE_GEN7_3DSTATE_STREAMOUT]                        = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_SBE]                              = { 0,  14 },
      [ILO_GPE_GEN7_3DSTATE_PS]                               = { 0,  8  },
      [ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP]  = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC]       = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BLEND_STATE_POINTERS]             = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS]     = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS]        = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_URB_VS]                           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_URB_HS]                           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_URB_DS]                           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_URB_GS]                           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_DRAWING_RECTANGLE]                = { 0,  4  },
      [ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_OFFSET]              = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_PATTERN]             = { 0,  33, },
      [ILO_GPE_GEN7_3DSTATE_LINE_STIPPLE]                     = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_AA_LINE_PARAMETERS]               = { 0,  3  },
      [ILO_GPE_GEN7_3DSTATE_MULTISAMPLE]                      = { 0,  4  },
      [ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS]           = { 0,  2  },
      [ILO_GPE_GEN7_3DSTATE_SO_DECL_LIST]                     = { 3,  2  },
      [ILO_GPE_GEN7_3DSTATE_SO_BUFFER]                        = { 0,  4  },
      [ILO_GPE_GEN7_PIPE_CONTROL]                             = { 0,  5  },
      [ILO_GPE_GEN7_3DPRIMITIVE]                              = { 0,  7  },
   };
   const int header = gen7_command_size_table[cmd].header;
   const int body = gen7_command_size_table[cmd].body;
   const int count = arg;

   ILO_GPE_VALID_GEN(dev, 7, 7);
   assert(cmd < ILO_GPE_GEN7_COMMAND_COUNT);

   return (likely(count)) ? header + body * count : 0;
}

static int
gen7_estimate_state_size(const struct ilo_dev_info *dev,
                         enum ilo_gpe_gen7_state state,
                         int arg)
{
   static const struct {
      int alignment;
      int body;
      bool is_array;
   } gen7_state_size_table[ILO_GPE_GEN7_STATE_COUNT] = {
      [ILO_GPE_GEN7_INTERFACE_DESCRIPTOR_DATA]          = { 8,  8,  true },
      [ILO_GPE_GEN7_SF_CLIP_VIEWPORT]                   = { 16, 16, true },
      [ILO_GPE_GEN7_CC_VIEWPORT]                        = { 8,  2,  true },
      [ILO_GPE_GEN7_COLOR_CALC_STATE]                   = { 16, 6,  false },
      [ILO_GPE_GEN7_BLEND_STATE]                        = { 16, 2,  true },
      [ILO_GPE_GEN7_DEPTH_STENCIL_STATE]                = { 16, 3,  false },
      [ILO_GPE_GEN7_SCISSOR_RECT]                       = { 8,  2,  true },
      [ILO_GPE_GEN7_BINDING_TABLE_STATE]                = { 8,  1,  true },
      [ILO_GPE_GEN7_SURFACE_STATE]                      = { 8,  8,  false },
      [ILO_GPE_GEN7_SAMPLER_STATE]                      = { 8,  4,  true },
      [ILO_GPE_GEN7_SAMPLER_BORDER_COLOR_STATE]         = { 8,  4,  false },
      [ILO_GPE_GEN7_PUSH_CONSTANT_BUFFER]               = { 8,  1,  true },
   };
   const int alignment = gen7_state_size_table[state].alignment;
   const int body = gen7_state_size_table[state].body;
   const bool is_array = gen7_state_size_table[state].is_array;
   const int count = arg;
   int estimate;

   ILO_GPE_VALID_GEN(dev, 7, 7);
   assert(state < ILO_GPE_GEN7_STATE_COUNT);

   if (likely(count)) {
      if (is_array) {
         estimate = (alignment - 1) + body * count;
      }
      else {
         estimate = (alignment - 1) + body;
         /* all states are aligned */
         if (count > 1)
            estimate += util_align_npot(body, alignment) * (count - 1);
      }
   }
   else {
      estimate = 0;
   }

   return estimate;
}

static void
gen7_init(struct ilo_gpe_gen7 *gen7)
{
   const struct ilo_gpe_gen6 *gen6 = ilo_gpe_gen6_get();

   gen7->estimate_command_size = gen7_estimate_command_size;
   gen7->estimate_state_size = gen7_estimate_state_size;

#define GEN7_USE(gen7, name, from) gen7->emit_ ## name = from->emit_ ## name
#define GEN7_SET(gen7, name)       gen7->emit_ ## name = gen7_emit_ ## name
   GEN7_USE(gen7, STATE_BASE_ADDRESS, gen6);
   GEN7_USE(gen7, STATE_SIP, gen6);
   GEN7_USE(gen7, 3DSTATE_VF_STATISTICS, gen6);
   GEN7_USE(gen7, PIPELINE_SELECT, gen6);
   GEN7_USE(gen7, MEDIA_VFE_STATE, gen6);
   GEN7_USE(gen7, MEDIA_CURBE_LOAD, gen6);
   GEN7_USE(gen7, MEDIA_INTERFACE_DESCRIPTOR_LOAD, gen6);
   GEN7_USE(gen7, MEDIA_STATE_FLUSH, gen6);
   GEN7_SET(gen7, GPGPU_WALKER);
   GEN7_SET(gen7, 3DSTATE_CLEAR_PARAMS);
   GEN7_USE(gen7, 3DSTATE_DEPTH_BUFFER, gen6);
   GEN7_USE(gen7, 3DSTATE_STENCIL_BUFFER, gen6);
   GEN7_USE(gen7, 3DSTATE_HIER_DEPTH_BUFFER, gen6);
   GEN7_USE(gen7, 3DSTATE_VERTEX_BUFFERS, gen6);
   GEN7_USE(gen7, 3DSTATE_VERTEX_ELEMENTS, gen6);
   GEN7_USE(gen7, 3DSTATE_INDEX_BUFFER, gen6);
   GEN7_SET(gen7, 3DSTATE_CC_STATE_POINTERS);
   GEN7_USE(gen7, 3DSTATE_SCISSOR_STATE_POINTERS, gen6);
   GEN7_USE(gen7, 3DSTATE_VS, gen6);
   GEN7_SET(gen7, 3DSTATE_GS);
   GEN7_USE(gen7, 3DSTATE_CLIP, gen6);
   GEN7_SET(gen7, 3DSTATE_SF);
   GEN7_SET(gen7, 3DSTATE_WM);
   GEN7_SET(gen7, 3DSTATE_CONSTANT_VS);
   GEN7_SET(gen7, 3DSTATE_CONSTANT_GS);
   GEN7_SET(gen7, 3DSTATE_CONSTANT_PS);
   GEN7_SET(gen7, 3DSTATE_SAMPLE_MASK);
   GEN7_SET(gen7, 3DSTATE_CONSTANT_HS);
   GEN7_SET(gen7, 3DSTATE_CONSTANT_DS);
   GEN7_SET(gen7, 3DSTATE_HS);
   GEN7_SET(gen7, 3DSTATE_TE);
   GEN7_SET(gen7, 3DSTATE_DS);
   GEN7_SET(gen7, 3DSTATE_STREAMOUT);
   GEN7_SET(gen7, 3DSTATE_SBE);
   GEN7_SET(gen7, 3DSTATE_PS);
   GEN7_SET(gen7, 3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP);
   GEN7_SET(gen7, 3DSTATE_VIEWPORT_STATE_POINTERS_CC);
   GEN7_SET(gen7, 3DSTATE_BLEND_STATE_POINTERS);
   GEN7_SET(gen7, 3DSTATE_DEPTH_STENCIL_STATE_POINTERS);
   GEN7_SET(gen7, 3DSTATE_BINDING_TABLE_POINTERS_VS);
   GEN7_SET(gen7, 3DSTATE_BINDING_TABLE_POINTERS_HS);
   GEN7_SET(gen7, 3DSTATE_BINDING_TABLE_POINTERS_DS);
   GEN7_SET(gen7, 3DSTATE_BINDING_TABLE_POINTERS_GS);
   GEN7_SET(gen7, 3DSTATE_BINDING_TABLE_POINTERS_PS);
   GEN7_SET(gen7, 3DSTATE_SAMPLER_STATE_POINTERS_VS);
   GEN7_SET(gen7, 3DSTATE_SAMPLER_STATE_POINTERS_HS);
   GEN7_SET(gen7, 3DSTATE_SAMPLER_STATE_POINTERS_DS);
   GEN7_SET(gen7, 3DSTATE_SAMPLER_STATE_POINTERS_GS);
   GEN7_SET(gen7, 3DSTATE_SAMPLER_STATE_POINTERS_PS);
   GEN7_SET(gen7, 3DSTATE_URB_VS);
   GEN7_SET(gen7, 3DSTATE_URB_HS);
   GEN7_SET(gen7, 3DSTATE_URB_DS);
   GEN7_SET(gen7, 3DSTATE_URB_GS);
   GEN7_USE(gen7, 3DSTATE_DRAWING_RECTANGLE, gen6);
   GEN7_USE(gen7, 3DSTATE_POLY_STIPPLE_OFFSET, gen6);
   GEN7_USE(gen7, 3DSTATE_POLY_STIPPLE_PATTERN, gen6);
   GEN7_USE(gen7, 3DSTATE_LINE_STIPPLE, gen6);
   GEN7_USE(gen7, 3DSTATE_AA_LINE_PARAMETERS, gen6);
   GEN7_USE(gen7, 3DSTATE_MULTISAMPLE, gen6);
   GEN7_SET(gen7, 3DSTATE_PUSH_CONSTANT_ALLOC_VS);
   GEN7_SET(gen7, 3DSTATE_PUSH_CONSTANT_ALLOC_HS);
   GEN7_SET(gen7, 3DSTATE_PUSH_CONSTANT_ALLOC_DS);
   GEN7_SET(gen7, 3DSTATE_PUSH_CONSTANT_ALLOC_GS);
   GEN7_SET(gen7, 3DSTATE_PUSH_CONSTANT_ALLOC_PS);
   GEN7_SET(gen7, 3DSTATE_SO_DECL_LIST);
   GEN7_SET(gen7, 3DSTATE_SO_BUFFER);
   GEN7_USE(gen7, PIPE_CONTROL, gen6);
   GEN7_SET(gen7, 3DPRIMITIVE);
   GEN7_USE(gen7, INTERFACE_DESCRIPTOR_DATA, gen6);
   GEN7_SET(gen7, SF_CLIP_VIEWPORT);
   GEN7_USE(gen7, CC_VIEWPORT, gen6);
   GEN7_USE(gen7, COLOR_CALC_STATE, gen6);
   GEN7_USE(gen7, BLEND_STATE, gen6);
   GEN7_USE(gen7, DEPTH_STENCIL_STATE, gen6);
   GEN7_USE(gen7, SCISSOR_RECT, gen6);
   GEN7_USE(gen7, BINDING_TABLE_STATE, gen6);
   GEN7_USE(gen7, SURFACE_STATE, gen6);
   GEN7_USE(gen7, SAMPLER_STATE, gen6);
   GEN7_USE(gen7, SAMPLER_BORDER_COLOR_STATE, gen6);
   GEN7_USE(gen7, push_constant_buffer, gen6);
#undef GEN7_USE
#undef GEN7_SET
}

static struct ilo_gpe_gen7 gen7_gpe;

const struct ilo_gpe_gen7 *
ilo_gpe_gen7_get(void)
{
   if (!gen7_gpe.estimate_command_size)
      gen7_init(&gen7_gpe);

   return &gen7_gpe;
}
