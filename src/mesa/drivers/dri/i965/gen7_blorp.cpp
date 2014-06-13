/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>

#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"

#include "brw_blorp.h"
#include "gen7_blorp.h"


/* 3DSTATE_URB_VS
 * 3DSTATE_URB_HS
 * 3DSTATE_URB_DS
 * 3DSTATE_URB_GS
 *
 * If the 3DSTATE_URB_VS is emitted, than the others must be also.
 * From the Ivybridge PRM, Volume 2 Part 1, section 1.7.1 3DSTATE_URB_VS:
 *
 *     3DSTATE_URB_HS, 3DSTATE_URB_DS, and 3DSTATE_URB_GS must also be
 *     programmed in order for the programming of this state to be
 *     valid.
 */
static void
gen7_blorp_emit_urb_config(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   unsigned urb_size = (brw->is_haswell && brw->gt == 3) ? 32 : 16;
   gen7_emit_push_constant_state(brw,
                                 urb_size / 2 /* vs_size */,
                                 0 /* gs_size */,
                                 urb_size / 2 /* fs_size */);

   /* The minimum valid number of VS entries is 32. See 3DSTATE_URB_VS, Dword
    * 1.15:0 "VS Number of URB Entries".
    */
   gen7_emit_urb_state(brw,
                       32 /* num_vs_entries */,
                       2 /* vs_size */,
                       2 /* vs_start */,
                       0 /* num_gs_entries */,
                       1 /* gs_size */,
                       2 /* gs_start */);
}


/* 3DSTATE_BLEND_STATE_POINTERS */
static void
gen7_blorp_emit_blend_state_pointer(struct brw_context *brw,
                                    const brw_blorp_params *params,
                                    uint32_t cc_blend_state_offset)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BLEND_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(cc_blend_state_offset | 1);
   ADVANCE_BATCH();
}


/* 3DSTATE_CC_STATE_POINTERS */
static void
gen7_blorp_emit_cc_state_pointer(struct brw_context *brw,
                                 const brw_blorp_params *params,
                                 uint32_t cc_state_offset)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(cc_state_offset | 1);
   ADVANCE_BATCH();
}

static void
gen7_blorp_emit_cc_viewport(struct brw_context *brw,
			    const brw_blorp_params *params)
{
   struct brw_cc_viewport *ccv;
   uint32_t cc_vp_offset;

   ccv = (struct brw_cc_viewport *)brw_state_batch(brw, AUB_TRACE_CC_VP_STATE,
						   sizeof(*ccv), 32,
						   &cc_vp_offset);
   ccv->min_depth = 0.0;
   ccv->max_depth = 1.0;

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VIEWPORT_STATE_POINTERS_CC << 16 | (2 - 2));
   OUT_BATCH(cc_vp_offset);
   ADVANCE_BATCH();
}


/* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS
 *
 * The offset is relative to CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
 */
static void
gen7_blorp_emit_depth_stencil_state_pointers(struct brw_context *brw,
                                             const brw_blorp_params *params,
                                             uint32_t depthstencil_offset)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_DEPTH_STENCIL_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(depthstencil_offset | 1);
   ADVANCE_BATCH();
}


/* SURFACE_STATE for renderbuffer or texture surface (see
 * brw_update_renderbuffer_surface and brw_update_texture_surface)
 */
static uint32_t
gen7_blorp_emit_surface_state(struct brw_context *brw,
                              const brw_blorp_params *params,
                              const brw_blorp_surface_info *surface,
                              uint32_t read_domains, uint32_t write_domain,
                              bool is_render_target)
{
   uint32_t wm_surf_offset;
   uint32_t width = surface->width;
   uint32_t height = surface->height;
   /* Note: since gen7 uses INTEL_MSAA_LAYOUT_CMS or INTEL_MSAA_LAYOUT_UMS for
    * color surfaces, width and height are measured in pixels; we don't need
    * to divide them by 2 as we do for Gen6 (see
    * gen6_blorp_emit_surface_state).
    */
   struct intel_mipmap_tree *mt = surface->mt;
   uint32_t tile_x, tile_y;
   const uint8_t mocs = GEN7_MOCS_L3;

   uint32_t tiling = surface->map_stencil_as_y_tiled
      ? I915_TILING_Y : mt->tiling;

   uint32_t *surf = (uint32_t *)
      brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, 8 * 4, 32, &wm_surf_offset);
   memset(surf, 0, 8 * 4);

   surf[0] = BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
             surface->brw_surfaceformat << BRW_SURFACE_FORMAT_SHIFT |
             gen7_surface_tiling_mode(tiling);

   if (surface->mt->align_h == 4)
      surf[0] |= GEN7_SURFACE_VALIGN_4;
   if (surface->mt->align_w == 8)
      surf[0] |= GEN7_SURFACE_HALIGN_8;

   if (surface->array_layout == ALL_SLICES_AT_EACH_LOD)
      surf[0] |= GEN7_SURFACE_ARYSPC_LOD0;
   else
      surf[0] |= GEN7_SURFACE_ARYSPC_FULL;

   /* reloc */
   surf[1] =
      surface->compute_tile_offsets(&tile_x, &tile_y) + mt->bo->offset64;

   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = SET_FIELD(tile_x / 4, BRW_SURFACE_X_OFFSET) |
             SET_FIELD(tile_y / 2, BRW_SURFACE_Y_OFFSET) |
             SET_FIELD(mocs, GEN7_SURFACE_MOCS);

   surf[2] = SET_FIELD(width - 1, GEN7_SURFACE_WIDTH) |
             SET_FIELD(height - 1, GEN7_SURFACE_HEIGHT);

   uint32_t pitch_bytes = mt->pitch;
   if (surface->map_stencil_as_y_tiled)
      pitch_bytes *= 2;
   surf[3] = pitch_bytes - 1;

   surf[4] = gen7_surface_msaa_bits(surface->num_samples, surface->msaa_layout);
   if (surface->mt->mcs_mt) {
      gen7_set_surface_mcs_info(brw, surf, wm_surf_offset, surface->mt->mcs_mt,
                                is_render_target);
   }

   surf[7] = surface->mt->fast_clear_color_value;

   if (brw->is_haswell) {
      surf[7] |= (SET_FIELD(HSW_SCS_RED,   GEN7_SURFACE_SCS_R) |
                  SET_FIELD(HSW_SCS_GREEN, GEN7_SURFACE_SCS_G) |
                  SET_FIELD(HSW_SCS_BLUE,  GEN7_SURFACE_SCS_B) |
                  SET_FIELD(HSW_SCS_ALPHA, GEN7_SURFACE_SCS_A));
   }

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->batch.bo,
                           wm_surf_offset + 4,
                           mt->bo,
                           surf[1] - mt->bo->offset64,
                           read_domains, write_domain);

   gen7_check_surface_setup(surf, is_render_target);

   return wm_surf_offset;
}


/* 3DSTATE_VS
 *
 * Disable vertex shader.
 */
static void
gen7_blorp_emit_vs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_VS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_HS
 *
 * Disable the hull shader.
 */
static void
gen7_blorp_emit_hs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_HS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_HS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_TE
 *
 * Disable the tesselation engine.
 */
static void
gen7_blorp_emit_te_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_TE << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_DS
 *
 * Disable the domain shader.
 */
static void
gen7_blorp_emit_ds_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_DS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_DS << 16 | (6 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

/* 3DSTATE_GS
 *
 * Disable the geometry shader.
 */
static void
gen7_blorp_emit_gs_disable(struct brw_context *brw,
                           const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_GS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   /**
    * From Graphics BSpec: 3D-Media-GPGPU Engine > 3D Pipeline Stages >
    * Geometry > Geometry Shader > State:
    *
    *     "Note: Because of corruption in IVB:GT2, software needs to flush the
    *     whole fixed function pipeline when the GS enable changes value in
    *     the 3DSTATE_GS."
    *
    * The hardware architects have clarified that in this context "flush the
    * whole fixed function pipeline" means to emit a PIPE_CONTROL with the "CS
    * Stall" bit set.
    */
   if (!brw->is_haswell && brw->gt == 2 && brw->gs.enabled)
      gen7_emit_cs_stall_flush(brw);

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
   brw->gs.enabled = false;
}

/* 3DSTATE_STREAMOUT
 *
 * Disable streamout.
 */
static void
gen7_blorp_emit_streamout_disable(struct brw_context *brw,
                                  const brw_blorp_params *params)
{
   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_sf_config(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   /* 3DSTATE_SF
    *
    * Disable ViewportTransformEnable (dw1.1)
    *
    * From the SandyBridge PRM, Volume 2, Part 1, Section 1.3, "3D
    * Primitives Overview":
    *     RECTLIST: Viewport Mapping must be DISABLED (as is typical with the
    *     use of screen- space coordinates).
    *
    * A solid rectangle must be rendered, so set FrontFaceFillMode (dw1.6:5)
    * and BackFaceFillMode (dw1.4:3) to SOLID(0).
    *
    * From the Sandy Bridge PRM, Volume 2, Part 1, Section
    * 6.4.1.1 3DSTATE_SF, Field FrontFaceFillMode:
    *     SOLID: Any triangle or rectangle object found to be front-facing
    *     is rendered as a solid object. This setting is required when
    *     (rendering rectangle (RECTLIST) objects.
    */
   {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_SF << 16 | (7 - 2));
      OUT_BATCH(params->depth_format <<
                GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT);
      OUT_BATCH(params->dst.num_samples > 1 ? GEN6_SF_MSRAST_ON_PATTERN : 0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_SBE */
   {
      BEGIN_BATCH(14);
      OUT_BATCH(_3DSTATE_SBE << 16 | (14 - 2));
      OUT_BATCH((1 - 1) << GEN7_SBE_NUM_OUTPUTS_SHIFT | /* only position */
                1 << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT |
                0 << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT);
      for (int i = 0; i < 12; ++i)
         OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}


/**
 * Disable thread dispatch (dw5.19) and enable the HiZ op.
 */
static void
gen7_blorp_emit_wm_config(struct brw_context *brw,
                          const brw_blorp_params *params,
                          brw_blorp_prog_data *prog_data)
{
   uint32_t dw1 = 0, dw2 = 0;

   switch (params->hiz_op) {
   case GEN6_HIZ_OP_DEPTH_CLEAR:
      dw1 |= GEN7_WM_DEPTH_CLEAR;
      break;
   case GEN6_HIZ_OP_DEPTH_RESOLVE:
      dw1 |= GEN7_WM_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_HIZ_RESOLVE:
      dw1 |= GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE;
      break;
   case GEN6_HIZ_OP_NONE:
      break;
   default:
      unreachable("not reached");
   }
   dw1 |= GEN7_WM_LINE_AA_WIDTH_1_0;
   dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5;
   dw1 |= 0 << GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT; /* No interp */
   if (params->use_wm_prog) {
      dw1 |= GEN7_WM_KILL_ENABLE; /* TODO: temporarily smash on */
      dw1 |= GEN7_WM_DISPATCH_ENABLE; /* We are rendering */
   }

      if (params->dst.num_samples > 1) {
         dw1 |= GEN7_WM_MSRAST_ON_PATTERN;
         if (prog_data && prog_data->persample_msaa_dispatch)
            dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
         else
            dw2 |= GEN7_WM_MSDISPMODE_PERPIXEL;
      } else {
         dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;
         dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
      }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}


/**
 * 3DSTATE_PS
 *
 * Pixel shader dispatch is disabled above in 3DSTATE_WM, dw1.29. Despite
 * that, thread dispatch info must still be specified.
 *     - Maximum Number of Threads (dw4.24:31) must be nonzero, as the
 *       valid range for this field is [0x3, 0x2f].
 *     - A dispatch mode must be given; that is, at least one of the
 *       "N Pixel Dispatch Enable" (N=8,16,32) fields must be set. This was
 *       discovered through simulator error messages.
 */
static void
gen7_blorp_emit_ps_config(struct brw_context *brw,
                          const brw_blorp_params *params,
                          uint32_t prog_offset,
                          brw_blorp_prog_data *prog_data)
{
   uint32_t dw2, dw4, dw5;
   const int max_threads_shift = brw->is_haswell ?
      HSW_PS_MAX_THREADS_SHIFT : IVB_PS_MAX_THREADS_SHIFT;

   dw2 = dw4 = dw5 = 0;
   dw4 |= (brw->max_wm_threads - 1) << max_threads_shift;

   /* If there's a WM program, we need to do 16-pixel dispatch since that's
    * what the program is compiled for.  If there isn't, then it shouldn't
    * matter because no program is actually being run.  However, the hardware
    * gets angry if we don't enable at least one dispatch mode, so just enable
    * 16-pixel dispatch unconditionally.
    */
   dw4 |= GEN7_PS_16_DISPATCH_ENABLE;

   if (brw->is_haswell)
      dw4 |= SET_FIELD(1, HSW_PS_SAMPLE_MASK); /* 1 sample for now */
   if (params->use_wm_prog) {
      dw2 |= 1 << GEN7_PS_SAMPLER_COUNT_SHIFT; /* Up to 4 samplers */
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;
      dw5 |= prog_data->first_curbe_grf << GEN7_PS_DISPATCH_START_GRF_SHIFT_0;
   }

   switch (params->fast_clear_op) {
   case GEN7_FAST_CLEAR_OP_FAST_CLEAR:
      dw4 |= GEN7_PS_RENDER_TARGET_FAST_CLEAR_ENABLE;
      break;
   case GEN7_FAST_CLEAR_OP_RESOLVE:
      dw4 |= GEN7_PS_RENDER_TARGET_RESOLVE_ENABLE;
      break;
   default:
      break;
   }

   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
   OUT_BATCH(params->use_wm_prog ? prog_offset : 0);
   OUT_BATCH(dw2);
   OUT_BATCH(0);
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_binding_table_pointers_ps(struct brw_context *brw,
                                          const brw_blorp_params *params,
                                          uint32_t wm_bind_bo_offset)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(wm_bind_bo_offset);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_sampler_state_pointers_ps(struct brw_context *brw,
                                          const brw_blorp_params *params,
                                          uint32_t sampler_offset)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SAMPLER_STATE_POINTERS_PS << 16 | (2 - 2));
   OUT_BATCH(sampler_offset);
   ADVANCE_BATCH();
}


static void
gen7_blorp_emit_constant_ps(struct brw_context *brw,
                            const brw_blorp_params *params,
                            uint32_t wm_push_const_offset)
{
   const uint8_t mocs = GEN7_MOCS_L3;

   /* Make sure the push constants fill an exact integer number of
    * registers.
    */
   assert(sizeof(brw_blorp_wm_push_constants) % 32 == 0);

   /* There must be at least one register worth of push constant data. */
   assert(BRW_BLORP_NUM_PUSH_CONST_REGS > 0);

   /* Enable push constant buffer 0. */
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 |
             (7 - 2));
   OUT_BATCH(BRW_BLORP_NUM_PUSH_CONST_REGS);
   OUT_BATCH(0);
   OUT_BATCH(wm_push_const_offset | mocs);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

static void
gen7_blorp_emit_constant_ps_disable(struct brw_context *brw,
                                    const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_CONSTANT_PS << 16 | (7 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

static void
gen7_blorp_emit_depth_stencil_config(struct brw_context *brw,
                                     const brw_blorp_params *params)
{
   const uint8_t mocs = GEN7_MOCS_L3;
   uint32_t surfwidth, surfheight;
   uint32_t surftype;
   unsigned int depth = MAX2(params->depth.mt->logical_depth0, 1);
   unsigned int min_array_element;
   GLenum gl_target = params->depth.mt->target;
   unsigned int lod;

   switch (gl_target) {
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
      /* The PRM claims that we should use BRW_SURFACE_CUBE for this
       * situation, but experiments show that gl_Layer doesn't work when we do
       * this.  So we use BRW_SURFACE_2D, since for rendering purposes this is
       * equivalent.
       */
      surftype = BRW_SURFACE_2D;
      depth *= 6;
      break;
   default:
      surftype = translate_tex_target(gl_target);
      break;
   }

   min_array_element = params->depth.layer;
   if (params->depth.mt->num_samples > 1) {
      /* Convert physical layer to logical layer. */
      min_array_element /= params->depth.mt->num_samples;
   }

   lod = params->depth.level - params->depth.mt->first_level;

   if (params->hiz_op != GEN6_HIZ_OP_NONE && lod == 0) {
      /* HIZ ops for lod 0 may set the width & height a little
       * larger to allow the fast depth clear to fit the hardware
       * alignment requirements. (8x4)
       */
      surfwidth = params->depth.width;
      surfheight = params->depth.height;
   } else {
      surfwidth = params->depth.mt->logical_width0;
      surfheight = params->depth.mt->logical_height0;
   }

   /* 3DSTATE_DEPTH_BUFFER */
   {
      intel_emit_depth_stall_flushes(brw);

      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH((params->depth.mt->pitch - 1) |
                params->depth_format << 18 |
                1 << 22 | /* hiz enable */
                1 << 28 | /* depth write */
                surftype << 29);
      OUT_RELOC(params->depth.mt->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                0);
      OUT_BATCH((surfwidth - 1) << 4 |
                (surfheight - 1) << 18 |
                lod);
      OUT_BATCH(((depth - 1) << 21) |
                (min_array_element << 10) |
                mocs);
      OUT_BATCH(0);
      OUT_BATCH((depth - 1) << 21);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_HIER_DEPTH_BUFFER */
   {
      struct intel_mipmap_tree *hiz_mt = params->depth.mt->hiz_mt;

      BEGIN_BATCH(3);
      OUT_BATCH((GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
      OUT_BATCH((mocs << 25) |
                (hiz_mt->pitch - 1));
      OUT_RELOC(hiz_mt->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_STENCIL_BUFFER */
   {
      BEGIN_BATCH(3);
      OUT_BATCH((GEN7_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}


static void
gen7_blorp_emit_depth_disable(struct brw_context *brw,
                              const brw_blorp_params *params)
{
   intel_emit_depth_stall_flushes(brw);

   BEGIN_BATCH(7);
   OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
   OUT_BATCH(BRW_DEPTHFORMAT_D32_FLOAT << 18 | (BRW_SURFACE_NULL << 29));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/* 3DSTATE_CLEAR_PARAMS
 *
 * From the Ivybridge PRM, Volume 2 Part 1, Section 11.5.5.4
 * 3DSTATE_CLEAR_PARAMS:
 *    3DSTATE_CLEAR_PARAMS must always be programmed in the along
 *    with the other Depth/Stencil state commands(i.e.  3DSTATE_DEPTH_BUFFER,
 *    3DSTATE_STENCIL_BUFFER, or 3DSTATE_HIER_DEPTH_BUFFER).
 */
static void
gen7_blorp_emit_clear_params(struct brw_context *brw,
                             const brw_blorp_params *params)
{
   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
   OUT_BATCH(params->depth.mt ? params->depth.mt->depth_clear_value : 0);
   OUT_BATCH(GEN7_DEPTH_CLEAR_VALID);
   ADVANCE_BATCH();
}


/* 3DPRIMITIVE */
static void
gen7_blorp_emit_primitive(struct brw_context *brw,
                          const brw_blorp_params *params)
{
   BEGIN_BATCH(7);
   OUT_BATCH(CMD_3D_PRIM << 16 | (7 - 2));
   OUT_BATCH(GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL |
             _3DPRIM_RECTLIST);
   OUT_BATCH(3); /* vertex count per instance */
   OUT_BATCH(0);
   OUT_BATCH(1); /* instance count */
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}


/**
 * \copydoc gen6_blorp_exec()
 */
void
gen7_blorp_exec(struct brw_context *brw,
                const brw_blorp_params *params)
{
   if (brw->gen >= 8)
      return;

   brw_blorp_prog_data *prog_data = NULL;
   uint32_t cc_blend_state_offset = 0;
   uint32_t cc_state_offset = 0;
   uint32_t depthstencil_offset;
   uint32_t wm_push_const_offset = 0;
   uint32_t wm_bind_bo_offset = 0;
   uint32_t sampler_offset = 0;

   uint32_t prog_offset = params->get_wm_prog(brw, &prog_data);
   gen6_emit_3dstate_multisample(brw, params->dst.num_samples);
   gen6_emit_3dstate_sample_mask(brw,
                                 params->dst.num_samples > 1 ?
                                 (1 << params->dst.num_samples) - 1 : 1);
   gen6_blorp_emit_state_base_address(brw, params);
   gen6_blorp_emit_vertices(brw, params);
   gen7_blorp_emit_urb_config(brw, params);
   if (params->use_wm_prog) {
      cc_blend_state_offset = gen6_blorp_emit_blend_state(brw, params);
      cc_state_offset = gen6_blorp_emit_cc_state(brw, params);
      gen7_blorp_emit_blend_state_pointer(brw, params, cc_blend_state_offset);
      gen7_blorp_emit_cc_state_pointer(brw, params, cc_state_offset);
   }
   depthstencil_offset = gen6_blorp_emit_depth_stencil_state(brw, params);
   gen7_blorp_emit_depth_stencil_state_pointers(brw, params,
                                                depthstencil_offset);
   if (params->use_wm_prog) {
      uint32_t wm_surf_offset_renderbuffer;
      uint32_t wm_surf_offset_texture = 0;
      wm_push_const_offset = gen6_blorp_emit_wm_constants(brw, params);
      intel_miptree_used_for_rendering(params->dst.mt);
      wm_surf_offset_renderbuffer =
         gen7_blorp_emit_surface_state(brw, params, &params->dst,
                                       I915_GEM_DOMAIN_RENDER,
                                       I915_GEM_DOMAIN_RENDER,
                                       true /* is_render_target */);
      if (params->src.mt) {
         wm_surf_offset_texture =
            gen7_blorp_emit_surface_state(brw, params, &params->src,
                                          I915_GEM_DOMAIN_SAMPLER, 0,
                                          false /* is_render_target */);
      }
      wm_bind_bo_offset =
         gen6_blorp_emit_binding_table(brw, params,
                                       wm_surf_offset_renderbuffer,
                                       wm_surf_offset_texture);
      sampler_offset = gen6_blorp_emit_sampler_state(brw, params);
   }
   gen7_blorp_emit_vs_disable(brw, params);
   gen7_blorp_emit_hs_disable(brw, params);
   gen7_blorp_emit_te_disable(brw, params);
   gen7_blorp_emit_ds_disable(brw, params);
   gen7_blorp_emit_gs_disable(brw, params);
   gen7_blorp_emit_streamout_disable(brw, params);
   gen6_blorp_emit_clip_disable(brw, params);
   gen7_blorp_emit_sf_config(brw, params);
   gen7_blorp_emit_wm_config(brw, params, prog_data);
   if (params->use_wm_prog) {
      gen7_blorp_emit_binding_table_pointers_ps(brw, params,
                                                wm_bind_bo_offset);
      gen7_blorp_emit_sampler_state_pointers_ps(brw, params, sampler_offset);
      gen7_blorp_emit_constant_ps(brw, params, wm_push_const_offset);
   } else {
      gen7_blorp_emit_constant_ps_disable(brw, params);
   }
   gen7_blorp_emit_ps_config(brw, params, prog_offset, prog_data);
   gen7_blorp_emit_cc_viewport(brw, params);

   if (params->depth.mt)
      gen7_blorp_emit_depth_stencil_config(brw, params);
   else
      gen7_blorp_emit_depth_disable(brw, params);
   gen7_blorp_emit_clear_params(brw, params);
   gen6_blorp_emit_drawing_rectangle(brw, params);
   gen7_blorp_emit_primitive(brw, params);
}
