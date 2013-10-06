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

#ifndef ILO_GPE_GEN7_H
#define ILO_GPE_GEN7_H

#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_cp.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_gpe_gen6.h"

/**
 * Commands that GEN7 GPE could emit.
 */
enum ilo_gpe_gen7_command {
   ILO_GPE_GEN7_STATE_BASE_ADDRESS,                  /* (0x0, 0x1, 0x01) */
   ILO_GPE_GEN7_STATE_SIP,                           /* (0x0, 0x1, 0x02) */
   ILO_GPE_GEN7_3DSTATE_VF_STATISTICS,               /* (0x1, 0x0, 0x0b) */
   ILO_GPE_GEN7_PIPELINE_SELECT,                     /* (0x1, 0x1, 0x04) */
   ILO_GPE_GEN7_MEDIA_VFE_STATE,                     /* (0x2, 0x0, 0x00) */
   ILO_GPE_GEN7_MEDIA_CURBE_LOAD,                    /* (0x2, 0x0, 0x01) */
   ILO_GPE_GEN7_MEDIA_INTERFACE_DESCRIPTOR_LOAD,     /* (0x2, 0x0, 0x02) */
   ILO_GPE_GEN7_MEDIA_STATE_FLUSH,                   /* (0x2, 0x0, 0x04) */
   ILO_GPE_GEN7_GPGPU_WALKER,                        /* (0x2, 0x1, 0x05) */
   ILO_GPE_GEN7_3DSTATE_CLEAR_PARAMS,                /* (0x3, 0x0, 0x04) */
   ILO_GPE_GEN7_3DSTATE_DEPTH_BUFFER,                /* (0x3, 0x0, 0x05) */
   ILO_GPE_GEN7_3DSTATE_STENCIL_BUFFER,              /* (0x3, 0x0, 0x06) */
   ILO_GPE_GEN7_3DSTATE_HIER_DEPTH_BUFFER,           /* (0x3, 0x0, 0x07) */
   ILO_GPE_GEN7_3DSTATE_VERTEX_BUFFERS,              /* (0x3, 0x0, 0x08) */
   ILO_GPE_GEN7_3DSTATE_VERTEX_ELEMENTS,             /* (0x3, 0x0, 0x09) */
   ILO_GPE_GEN7_3DSTATE_INDEX_BUFFER,                /* (0x3, 0x0, 0x0a) */
   ILO_GPE_GEN7_3DSTATE_VF,                          /* (0x3, 0x0, 0x0c) */
   ILO_GPE_GEN7_3DSTATE_CC_STATE_POINTERS,           /* (0x3, 0x0, 0x0e) */
   ILO_GPE_GEN7_3DSTATE_SCISSOR_STATE_POINTERS,      /* (0x3, 0x0, 0x0f) */
   ILO_GPE_GEN7_3DSTATE_VS,                          /* (0x3, 0x0, 0x10) */
   ILO_GPE_GEN7_3DSTATE_GS,                          /* (0x3, 0x0, 0x11) */
   ILO_GPE_GEN7_3DSTATE_CLIP,                        /* (0x3, 0x0, 0x12) */
   ILO_GPE_GEN7_3DSTATE_SF,                          /* (0x3, 0x0, 0x13) */
   ILO_GPE_GEN7_3DSTATE_WM,                          /* (0x3, 0x0, 0x14) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_VS,                 /* (0x3, 0x0, 0x15) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_GS,                 /* (0x3, 0x0, 0x16) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_PS,                 /* (0x3, 0x0, 0x17) */
   ILO_GPE_GEN7_3DSTATE_SAMPLE_MASK,                 /* (0x3, 0x0, 0x18) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_HS,                 /* (0x3, 0x0, 0x19) */
   ILO_GPE_GEN7_3DSTATE_CONSTANT_DS,                 /* (0x3, 0x0, 0x1a) */
   ILO_GPE_GEN7_3DSTATE_HS,                          /* (0x3, 0x0, 0x1b) */
   ILO_GPE_GEN7_3DSTATE_TE,                          /* (0x3, 0x0, 0x1c) */
   ILO_GPE_GEN7_3DSTATE_DS,                          /* (0x3, 0x0, 0x1d) */
   ILO_GPE_GEN7_3DSTATE_STREAMOUT,                   /* (0x3, 0x0, 0x1e) */
   ILO_GPE_GEN7_3DSTATE_SBE,                         /* (0x3, 0x0, 0x1f) */
   ILO_GPE_GEN7_3DSTATE_PS,                          /* (0x3, 0x0, 0x20) */
   ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP, /* (0x3, 0x0, 0x21) */
   ILO_GPE_GEN7_3DSTATE_VIEWPORT_STATE_POINTERS_CC,  /* (0x3, 0x0, 0x23) */
   ILO_GPE_GEN7_3DSTATE_BLEND_STATE_POINTERS,        /* (0x3, 0x0, 0x24) */
   ILO_GPE_GEN7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS, /* (0x3, 0x0, 0x25) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_VS,   /* (0x3, 0x0, 0x26) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_HS,   /* (0x3, 0x0, 0x27) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_DS,   /* (0x3, 0x0, 0x28) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_GS,   /* (0x3, 0x0, 0x29) */
   ILO_GPE_GEN7_3DSTATE_BINDING_TABLE_POINTERS_PS,   /* (0x3, 0x0, 0x2a) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_VS,   /* (0x3, 0x0, 0x2b) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_HS,   /* (0x3, 0x0, 0x2c) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_DS,   /* (0x3, 0x0, 0x2d) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_GS,   /* (0x3, 0x0, 0x2e) */
   ILO_GPE_GEN7_3DSTATE_SAMPLER_STATE_POINTERS_PS,   /* (0x3, 0x0, 0x2f) */
   ILO_GPE_GEN7_3DSTATE_URB_VS,                      /* (0x3, 0x0, 0x30) */
   ILO_GPE_GEN7_3DSTATE_URB_HS,                      /* (0x3, 0x0, 0x31) */
   ILO_GPE_GEN7_3DSTATE_URB_DS,                      /* (0x3, 0x0, 0x32) */
   ILO_GPE_GEN7_3DSTATE_URB_GS,                      /* (0x3, 0x0, 0x33) */
   ILO_GPE_GEN7_3DSTATE_DRAWING_RECTANGLE,           /* (0x3, 0x1, 0x00) */
   ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_OFFSET,         /* (0x3, 0x1, 0x06) */
   ILO_GPE_GEN7_3DSTATE_POLY_STIPPLE_PATTERN,        /* (0x3, 0x1, 0x07) */
   ILO_GPE_GEN7_3DSTATE_LINE_STIPPLE,                /* (0x3, 0x1, 0x08) */
   ILO_GPE_GEN7_3DSTATE_AA_LINE_PARAMETERS,          /* (0x3, 0x1, 0x0a) */
   ILO_GPE_GEN7_3DSTATE_MULTISAMPLE,                 /* (0x3, 0x1, 0x0d) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_VS,      /* (0x3, 0x1, 0x12) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_HS,      /* (0x3, 0x1, 0x13) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_DS,      /* (0x3, 0x1, 0x14) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_GS,      /* (0x3, 0x1, 0x15) */
   ILO_GPE_GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_PS,      /* (0x3, 0x1, 0x16) */
   ILO_GPE_GEN7_3DSTATE_SO_DECL_LIST,                /* (0x3, 0x1, 0x17) */
   ILO_GPE_GEN7_3DSTATE_SO_BUFFER,                   /* (0x3, 0x1, 0x18) */
   ILO_GPE_GEN7_PIPE_CONTROL,                        /* (0x3, 0x2, 0x00) */
   ILO_GPE_GEN7_3DPRIMITIVE,                         /* (0x3, 0x3, 0x00) */

   ILO_GPE_GEN7_COMMAND_COUNT,
};

/**
 * Indirect states that GEN7 GPE could emit.
 */
enum ilo_gpe_gen7_state {
   ILO_GPE_GEN7_INTERFACE_DESCRIPTOR_DATA,
   ILO_GPE_GEN7_SF_CLIP_VIEWPORT,
   ILO_GPE_GEN7_CC_VIEWPORT,
   ILO_GPE_GEN7_COLOR_CALC_STATE,
   ILO_GPE_GEN7_BLEND_STATE,
   ILO_GPE_GEN7_DEPTH_STENCIL_STATE,
   ILO_GPE_GEN7_SCISSOR_RECT,
   ILO_GPE_GEN7_BINDING_TABLE_STATE,
   ILO_GPE_GEN7_SURFACE_STATE,
   ILO_GPE_GEN7_SAMPLER_STATE,
   ILO_GPE_GEN7_SAMPLER_BORDER_COLOR_STATE,
   ILO_GPE_GEN7_PUSH_CONSTANT_BUFFER,

   ILO_GPE_GEN7_STATE_COUNT,
};

int
ilo_gpe_gen7_estimate_command_size(const struct ilo_dev_info *dev,
                                   enum ilo_gpe_gen7_command cmd,
                                   int arg);

int
ilo_gpe_gen7_estimate_state_size(const struct ilo_dev_info *dev,
                                 enum ilo_gpe_gen7_state state,
                                 int arg);

static inline void
gen7_emit_GPGPU_WALKER(const struct ilo_dev_info *dev,
                       struct ilo_cp *cp)
{
   assert(!"GPGPU_WALKER unsupported");
}

static inline void
gen7_emit_3DSTATE_CLEAR_PARAMS(const struct ilo_dev_info *dev,
                               uint32_t clear_val,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x04);
   const uint8_t cmd_len = 3;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, clear_val);
   ilo_cp_write(cp, 1);
   ilo_cp_end(cp);
}

static inline void
gen7_emit_3DSTATE_VF(const struct ilo_dev_info *dev,
                     bool enable_cut_index,
                     uint32_t cut_index,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x0c);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 7.5, 7.5);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2) |
                    ((enable_cut_index) ? HSW_CUT_INDEX_ENABLE : 0));
   ilo_cp_write(cp, cut_index);
   ilo_cp_end(cp);
}

static inline void
gen7_emit_3dstate_pointer(const struct ilo_dev_info *dev,
                          int subop, uint32_t pointer,
                          struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, subop);
   const uint8_t cmd_len = 2;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, pointer);
   ilo_cp_end(cp);
}

static inline void
gen7_emit_3DSTATE_CC_STATE_POINTERS(const struct ilo_dev_info *dev,
                                    uint32_t color_calc_state,
                                    struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x0e, color_calc_state, cp);
}

static inline void
gen7_emit_3DSTATE_GS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *gs,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x11);
   const uint8_t cmd_len = 7;
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_SF(const struct ilo_dev_info *dev,
                     const struct ilo_rasterizer_state *rasterizer,
                     const struct pipe_surface *zs_surf,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x13);
   const uint8_t cmd_len = 7;
   const int num_samples = 1;
   uint32_t payload[6];

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   ilo_gpe_gen6_fill_3dstate_sf_raster(dev,
         rasterizer, num_samples,
         (zs_surf) ? zs_surf->format : PIPE_FORMAT_NONE,
         payload, Elements(payload));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write_multi(cp, payload, 6);
   ilo_cp_end(cp);
}

static inline void
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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_CONSTANT_VS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x15, bufs, sizes, num_bufs, cp);
}

static inline void
gen7_emit_3DSTATE_CONSTANT_GS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x16, bufs, sizes, num_bufs, cp);
}

static inline void
gen7_emit_3DSTATE_CONSTANT_PS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x17, bufs, sizes, num_bufs, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLE_MASK(const struct ilo_dev_info *dev,
                              unsigned sample_mask,
                              int num_samples,
                              struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x18);
   const uint8_t cmd_len = 2;
   const unsigned valid_mask = ((1 << num_samples) - 1) | 0x1;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_CONSTANT_HS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x19, bufs, sizes, num_bufs, cp);
}

static inline void
gen7_emit_3DSTATE_CONSTANT_DS(const struct ilo_dev_info *dev,
                              const uint32_t *bufs, const int *sizes,
                              int num_bufs,
                              struct ilo_cp *cp)
{
   gen7_emit_3dstate_constant(dev, 0x1a, bufs, sizes, num_bufs, cp);
}

static inline void
gen7_emit_3DSTATE_HS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *hs,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1b);
   const uint8_t cmd_len = 7;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_TE(const struct ilo_dev_info *dev,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1c);
   const uint8_t cmd_len = 4;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static inline void
gen7_emit_3DSTATE_DS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *ds,
                     int num_samplers,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1d);
   const uint8_t cmd_len = 6;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_SBE(const struct ilo_dev_info *dev,
                      const struct ilo_rasterizer_state *rasterizer,
                      const struct ilo_shader_state *fs,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x1f);
   const uint8_t cmd_len = 14;
   uint32_t dw[13];

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   ilo_gpe_gen6_fill_3dstate_sf_sbe(dev, rasterizer, fs, dw, Elements(dw));

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, cmd | (cmd_len - 2));
   ilo_cp_write_multi(cp, dw, 13);
   ilo_cp_end(cp);
}

static inline void
gen7_emit_3DSTATE_PS(const struct ilo_dev_info *dev,
                     const struct ilo_shader_state *fs,
                     int num_samplers, bool dual_blend,
                     struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, 0x20);
   const uint8_t cmd_len = 8;
   const struct ilo_shader_cso *cso;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   if (!fs) {
      int max_threads;

      /* GPU hangs if none of the dispatch enable bits is set */
      dw4 = GEN7_PS_8_DISPATCH_ENABLE;

      /* see brwCreateContext() */
      switch (dev->gen) {
      case ILO_GEN(7.5):
         max_threads = (dev->gt == 3) ? 408 : (dev->gt == 2) ? 204 : 102;
         dw4 |= max_threads << HSW_PS_MAX_THREADS_SHIFT;
         break;
      case ILO_GEN(7):
      default:
         max_threads = (dev->gt == 2) ? 172 : 48;
         dw4 |= max_threads << IVB_PS_MAX_THREADS_SHIFT;
         break;
      }

      ilo_cp_begin(cp, cmd_len);
      ilo_cp_write(cp, cmd | (cmd_len - 2));
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, 0);
      ilo_cp_write(cp, dw4);
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

static inline void
gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(const struct ilo_dev_info *dev,
                                                  uint32_t sf_clip_viewport,
                                                  struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x21, sf_clip_viewport, cp);
}

static inline void
gen7_emit_3DSTATE_VIEWPORT_STATE_POINTERS_CC(const struct ilo_dev_info *dev,
                                             uint32_t cc_viewport,
                                             struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x23, cc_viewport, cp);
}

static inline void
gen7_emit_3DSTATE_BLEND_STATE_POINTERS(const struct ilo_dev_info *dev,
                                       uint32_t blend_state,
                                       struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x24, blend_state, cp);
}

static inline void
gen7_emit_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(const struct ilo_dev_info *dev,
                                               uint32_t depth_stencil_state,
                                               struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x25, depth_stencil_state, cp);
}

static inline void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_VS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x26, binding_table, cp);
}

static inline void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_HS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x27, binding_table, cp);
}

static inline void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_DS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x28, binding_table, cp);
}

static inline void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_GS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x29, binding_table, cp);
}

static inline void
gen7_emit_3DSTATE_BINDING_TABLE_POINTERS_PS(const struct ilo_dev_info *dev,
                                            uint32_t binding_table,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2a, binding_table, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_VS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2b, sampler_state, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_HS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2c, sampler_state, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_DS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2d, sampler_state, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_GS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2e, sampler_state, cp);
}

static inline void
gen7_emit_3DSTATE_SAMPLER_STATE_POINTERS_PS(const struct ilo_dev_info *dev,
                                            uint32_t sampler_state,
                                            struct ilo_cp *cp)
{
   gen7_emit_3dstate_pointer(dev, 0x2f, sampler_state, cp);
}

static inline void
gen7_emit_3dstate_urb(const struct ilo_dev_info *dev,
                      int subop, int offset, int size,
                      int entry_size,
                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x0, subop);
   const uint8_t cmd_len = 2;
   const int row_size = 64; /* 512 bits */
   int alloc_size, num_entries, min_entries, max_entries;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

      switch (dev->gen) {
      case ILO_GEN(7.5):
         max_entries = (dev->gt >= 2) ? 1644 : 640;
         break;
      case ILO_GEN(7):
      default:
         max_entries = (dev->gt == 2) ? 704 : 512;
         break;
      }

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
      switch (dev->gen) {
      case ILO_GEN(7.5):
         max_entries = (dev->gt >= 2) ? 640 : 256;
         break;
      case ILO_GEN(7):
      default:
         max_entries = (dev->gt == 2) ? 320 : 192;
         break;
      }

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

static inline void
gen7_emit_3DSTATE_URB_VS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x30, offset, size, entry_size, cp);
}

static inline void
gen7_emit_3DSTATE_URB_HS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x31, offset, size, entry_size, cp);
}

static inline void
gen7_emit_3DSTATE_URB_DS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x32, offset, size, entry_size, cp);
}

static inline void
gen7_emit_3DSTATE_URB_GS(const struct ilo_dev_info *dev,
                         int offset, int size, int entry_size,
                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_urb(dev, 0x33, offset, size, entry_size, cp);
}

static inline void
gen7_emit_3dstate_push_constant_alloc(const struct ilo_dev_info *dev,
                                      int subop, int offset, int size,
                                      struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, subop);
   const uint8_t cmd_len = 2;
   int end;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_VS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x12, offset, size, cp);
}

static inline void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_HS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x13, offset, size, cp);
}

static inline void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_DS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x14, offset, size, cp);
}

static inline void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_GS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x15, offset, size, cp);
}

static inline void
gen7_emit_3DSTATE_PUSH_CONSTANT_ALLOC_PS(const struct ilo_dev_info *dev,
                                         int offset, int size,
                                         struct ilo_cp *cp)
{
   gen7_emit_3dstate_push_constant_alloc(dev, 0x16, offset, size, cp);
}

static inline void
gen7_emit_3DSTATE_SO_DECL_LIST(const struct ilo_dev_info *dev,
                               const struct pipe_stream_output_info *so_info,
                               struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x17);
   uint16_t cmd_len;
   int buffer_selects, num_entries, i;
   uint16_t so_decls[128];

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
gen7_emit_3DSTATE_SO_BUFFER(const struct ilo_dev_info *dev,
                            int index, int base, int stride,
                            const struct pipe_stream_output_target *so_target,
                            struct ilo_cp *cp)
{
   const uint32_t cmd = ILO_GPE_CMD(0x3, 0x1, 0x18);
   const uint8_t cmd_len = 4;
   struct ilo_buffer *buf;
   int end;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline void
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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

static inline uint32_t
gen7_emit_SF_CLIP_VIEWPORT(const struct ilo_dev_info *dev,
                           const struct ilo_viewport_cso *viewports,
                           unsigned num_viewports,
                           struct ilo_cp *cp)
{
   const int state_align = 64 / 4;
   const int state_len = 16 * num_viewports;
   uint32_t state_offset, *dw;
   unsigned i;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

#endif /* ILO_GPE_GEN7_H */
