/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */

/**
 * @file brw_sampler_state.c
 *
 * This file contains code for emitting SAMPLER_STATE structures, which
 * specifies filter modes, wrap modes, border color, and so on.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"

#include "main/macros.h"
#include "main/samplerobj.h"

/**
 * Emit a 3DSTATE_SAMPLER_STATE_POINTERS_{VS,HS,GS,DS,PS} packet.
 */
static void
gen7_emit_sampler_state_pointers_xs(struct brw_context *brw,
                                    struct brw_stage_state *stage_state)
{
   static const uint16_t packet_headers[] = {
      [MESA_SHADER_VERTEX] = _3DSTATE_SAMPLER_STATE_POINTERS_VS,
      [MESA_SHADER_GEOMETRY] = _3DSTATE_SAMPLER_STATE_POINTERS_GS,
      [MESA_SHADER_FRAGMENT] = _3DSTATE_SAMPLER_STATE_POINTERS_PS,
   };

   /* Ivybridge requires a workaround flush before VS packets. */
   if (brw->gen == 7 && !brw->is_haswell && !brw->is_baytrail &&
       stage_state->stage == MESA_SHADER_VERTEX) {
      gen7_emit_vs_workaround_flush(brw);
   }

   BEGIN_BATCH(2);
   OUT_BATCH(packet_headers[stage_state->stage] << 16 | (2 - 2));
   OUT_BATCH(stage_state->sampler_offset);
   ADVANCE_BATCH();
}

/**
 * Emit a SAMPLER_STATE structure, given all the fields.
 */
void
brw_emit_sampler_state(struct brw_context *brw,
                       uint32_t *ss,
                       uint32_t batch_offset_for_sampler_state,
                       unsigned min_filter,
                       unsigned mag_filter,
                       unsigned mip_filter,
                       unsigned max_anisotropy,
                       unsigned address_rounding,
                       unsigned wrap_s,
                       unsigned wrap_t,
                       unsigned wrap_r,
                       unsigned min_lod,
                       unsigned max_lod,
                       int lod_bias,
                       unsigned base_level,
                       unsigned shadow_function,
                       bool non_normalized_coordinates,
                       uint32_t border_color_offset)
{
   ss[0] = BRW_SAMPLER_LOD_PRECLAMP_ENABLE |
           SET_FIELD(base_level, BRW_SAMPLER_BASE_MIPLEVEL) |
           SET_FIELD(mip_filter, BRW_SAMPLER_MIP_FILTER) |
           SET_FIELD(mag_filter, BRW_SAMPLER_MAG_FILTER) |
           SET_FIELD(min_filter, BRW_SAMPLER_MIN_FILTER);

   ss[2] = border_color_offset;
   if (brw->gen < 6) {
      ss[2] += brw->batch.bo->offset64; /* reloc */
      drm_intel_bo_emit_reloc(brw->batch.bo,
                              batch_offset_for_sampler_state + 8,
                              brw->batch.bo, border_color_offset,
                              I915_GEM_DOMAIN_SAMPLER, 0);
   }

   ss[3] = SET_FIELD(max_anisotropy, BRW_SAMPLER_MAX_ANISOTROPY) |
           SET_FIELD(address_rounding, BRW_SAMPLER_ADDRESS_ROUNDING);

   if (brw->gen >= 7) {
      ss[0] |= SET_FIELD(lod_bias & 0x1fff, GEN7_SAMPLER_LOD_BIAS);

      if (min_filter == BRW_MAPFILTER_ANISOTROPIC)
         ss[0] |= GEN7_SAMPLER_EWA_ANISOTROPIC_ALGORITHM;

      ss[1] = SET_FIELD(min_lod, GEN7_SAMPLER_MIN_LOD) |
              SET_FIELD(max_lod, GEN7_SAMPLER_MAX_LOD) |
              SET_FIELD(shadow_function, GEN7_SAMPLER_SHADOW_FUNCTION);

      ss[3] |= SET_FIELD(wrap_s, BRW_SAMPLER_TCX_WRAP_MODE) |
               SET_FIELD(wrap_t, BRW_SAMPLER_TCY_WRAP_MODE) |
               SET_FIELD(wrap_r, BRW_SAMPLER_TCZ_WRAP_MODE);

      if (non_normalized_coordinates)
         ss[3] |= GEN7_SAMPLER_NON_NORMALIZED_COORDINATES;
   } else {
      ss[0] |= SET_FIELD(lod_bias & 0x7ff, GEN4_SAMPLER_LOD_BIAS) |
               SET_FIELD(shadow_function, GEN4_SAMPLER_SHADOW_FUNCTION);

      if (brw->gen == 6 && min_filter != mag_filter)
         ss[0] |= GEN6_SAMPLER_MIN_MAG_NOT_EQUAL;

      ss[1] = SET_FIELD(min_lod, GEN4_SAMPLER_MIN_LOD) |
              SET_FIELD(max_lod, GEN4_SAMPLER_MAX_LOD) |
              SET_FIELD(wrap_s, BRW_SAMPLER_TCX_WRAP_MODE) |
              SET_FIELD(wrap_t, BRW_SAMPLER_TCY_WRAP_MODE) |
              SET_FIELD(wrap_r, BRW_SAMPLER_TCZ_WRAP_MODE);

      if (brw->gen >= 6 && non_normalized_coordinates)
         ss[3] |= GEN6_SAMPLER_NON_NORMALIZED_COORDINATES;
   }
}

static uint32_t
translate_wrap_mode(struct brw_context *brw, GLenum wrap, bool using_nearest)
{
   switch( wrap ) {
   case GL_REPEAT:
      return BRW_TEXCOORDMODE_WRAP;
   case GL_CLAMP:
      /* GL_CLAMP is the weird mode where coordinates are clamped to
       * [0.0, 1.0], so linear filtering of coordinates outside of
       * [0.0, 1.0] give you half edge texel value and half border
       * color.
       *
       * Gen8+ supports this natively.
       */
      if (brw->gen >= 8)
         return GEN8_TEXCOORDMODE_HALF_BORDER;

      /* On Gen4-7.5, we clamp the coordinates in the fragment shader
       * and set clamp_border here, which gets the result desired.
       * We just use clamp(_to_edge) for nearest, because for nearest
       * clamping to 1.0 gives border color instead of the desired
       * edge texels.
       */
      if (using_nearest)
	 return BRW_TEXCOORDMODE_CLAMP;
      else
	 return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case GL_CLAMP_TO_EDGE:
      return BRW_TEXCOORDMODE_CLAMP;
   case GL_CLAMP_TO_BORDER:
      return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT:
      return BRW_TEXCOORDMODE_MIRROR;
   case GL_MIRROR_CLAMP_TO_EDGE:
      return BRW_TEXCOORDMODE_MIRROR_ONCE;
   default:
      return BRW_TEXCOORDMODE_WRAP;
   }
}

/**
 * Return true if the given wrap mode requires the border color to exist.
 */
static bool
wrap_mode_needs_border_color(unsigned wrap_mode)
{
   return wrap_mode == BRW_TEXCOORDMODE_CLAMP_BORDER ||
          wrap_mode == GEN8_TEXCOORDMODE_HALF_BORDER;
}

/**
 * Upload SAMPLER_BORDER_COLOR_STATE.
 */
static void
upload_default_color(struct brw_context *brw,
                     const struct gl_sampler_object *sampler,
                     int unit,
                     uint32_t *sdc_offset)
{
   struct gl_context *ctx = &brw->ctx;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj = texUnit->_Current;
   struct gl_texture_image *firstImage = texObj->Image[0][texObj->BaseLevel];
   float color[4];

   switch (firstImage->_BaseFormat) {
   case GL_DEPTH_COMPONENT:
      /* GL specs that border color for depth textures is taken from the
       * R channel, while the hardware uses A.  Spam R into all the
       * channels for safety.
       */
      color[0] = sampler->BorderColor.f[0];
      color[1] = sampler->BorderColor.f[0];
      color[2] = sampler->BorderColor.f[0];
      color[3] = sampler->BorderColor.f[0];
      break;
   case GL_ALPHA:
      color[0] = 0.0;
      color[1] = 0.0;
      color[2] = 0.0;
      color[3] = sampler->BorderColor.f[3];
      break;
   case GL_INTENSITY:
      color[0] = sampler->BorderColor.f[0];
      color[1] = sampler->BorderColor.f[0];
      color[2] = sampler->BorderColor.f[0];
      color[3] = sampler->BorderColor.f[0];
      break;
   case GL_LUMINANCE:
      color[0] = sampler->BorderColor.f[0];
      color[1] = sampler->BorderColor.f[0];
      color[2] = sampler->BorderColor.f[0];
      color[3] = 1.0;
      break;
   case GL_LUMINANCE_ALPHA:
      color[0] = sampler->BorderColor.f[0];
      color[1] = sampler->BorderColor.f[0];
      color[2] = sampler->BorderColor.f[0];
      color[3] = sampler->BorderColor.f[3];
      break;
   default:
      color[0] = sampler->BorderColor.f[0];
      color[1] = sampler->BorderColor.f[1];
      color[2] = sampler->BorderColor.f[2];
      color[3] = sampler->BorderColor.f[3];
      break;
   }

   /* In some cases we use an RGBA surface format for GL RGB textures,
    * where we've initialized the A channel to 1.0.  We also have to set
    * the border color alpha to 1.0 in that case.
    */
   if (firstImage->_BaseFormat == GL_RGB)
      color[3] = 1.0;

   if (brw->gen >= 8) {
      /* On Broadwell, the border color is represented as four 32-bit floats,
       * integers, or unsigned values, interpreted according to the surface
       * format.  This matches the sampler->BorderColor union exactly.  Since
       * we use floats both here and in the above reswizzling code, we preserve
       * the original bit pattern.  So we actually handle all three formats.
       */
      float *sdc = brw_state_batch(brw, AUB_TRACE_SAMPLER_DEFAULT_COLOR,
                                   4 * 4, 64, sdc_offset);
      COPY_4FV(sdc, color);
   } else if (brw->gen == 5 || brw->gen == 6) {
      struct gen5_sampler_default_color *sdc;

      sdc = brw_state_batch(brw, AUB_TRACE_SAMPLER_DEFAULT_COLOR,
			    sizeof(*sdc), 32, sdc_offset);

      memset(sdc, 0, sizeof(*sdc));

      UNCLAMPED_FLOAT_TO_UBYTE(sdc->ub[0], color[0]);
      UNCLAMPED_FLOAT_TO_UBYTE(sdc->ub[1], color[1]);
      UNCLAMPED_FLOAT_TO_UBYTE(sdc->ub[2], color[2]);
      UNCLAMPED_FLOAT_TO_UBYTE(sdc->ub[3], color[3]);

      UNCLAMPED_FLOAT_TO_USHORT(sdc->us[0], color[0]);
      UNCLAMPED_FLOAT_TO_USHORT(sdc->us[1], color[1]);
      UNCLAMPED_FLOAT_TO_USHORT(sdc->us[2], color[2]);
      UNCLAMPED_FLOAT_TO_USHORT(sdc->us[3], color[3]);

      UNCLAMPED_FLOAT_TO_SHORT(sdc->s[0], color[0]);
      UNCLAMPED_FLOAT_TO_SHORT(sdc->s[1], color[1]);
      UNCLAMPED_FLOAT_TO_SHORT(sdc->s[2], color[2]);
      UNCLAMPED_FLOAT_TO_SHORT(sdc->s[3], color[3]);

      sdc->hf[0] = _mesa_float_to_half(color[0]);
      sdc->hf[1] = _mesa_float_to_half(color[1]);
      sdc->hf[2] = _mesa_float_to_half(color[2]);
      sdc->hf[3] = _mesa_float_to_half(color[3]);

      sdc->b[0] = sdc->s[0] >> 8;
      sdc->b[1] = sdc->s[1] >> 8;
      sdc->b[2] = sdc->s[2] >> 8;
      sdc->b[3] = sdc->s[3] >> 8;

      sdc->f[0] = color[0];
      sdc->f[1] = color[1];
      sdc->f[2] = color[2];
      sdc->f[3] = color[3];
   } else {
      float *sdc = brw_state_batch(brw, AUB_TRACE_SAMPLER_DEFAULT_COLOR,
			           4 * 4, 32, sdc_offset);
      memcpy(sdc, color, 4 * 4);
   }
}

/**
 * Sets the sampler state for a single unit based off of the sampler key
 * entry.
 */
static void
brw_update_sampler_state(struct brw_context *brw,
                         int unit,
                         uint32_t *sampler_state,
                         uint32_t batch_offset_for_sampler_state)
{
   struct gl_context *ctx = &brw->ctx;
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *texObj = texUnit->_Current;
   const struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);

   /* These don't use samplers at all. */
   if (texObj->Target == GL_TEXTURE_BUFFER)
      return;

   unsigned min_filter, mag_filter, mip_filter;

   /* Select min and mip filters. */
   switch (sampler->MinFilter) {
   case GL_NEAREST:
      min_filter = BRW_MAPFILTER_NEAREST;
      mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_LINEAR:
      min_filter = BRW_MAPFILTER_LINEAR;
      mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      min_filter = BRW_MAPFILTER_NEAREST;
      mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      min_filter = BRW_MAPFILTER_LINEAR;
      mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      min_filter = BRW_MAPFILTER_NEAREST;
      mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      min_filter = BRW_MAPFILTER_LINEAR;
      mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   default:
      unreachable("not reached");
   }

   /* Select mag filter. */
   if (sampler->MagFilter == GL_LINEAR)
      mag_filter = BRW_MAPFILTER_LINEAR;
   else
      mag_filter = BRW_MAPFILTER_NEAREST;

   /* Enable anisotropic filtering if desired. */
   unsigned max_anisotropy = BRW_ANISORATIO_2;
   if (sampler->MaxAnisotropy > 1.0) {
      min_filter = BRW_MAPFILTER_ANISOTROPIC;
      mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      if (sampler->MaxAnisotropy > 2.0) {
	 max_anisotropy =
            MIN2((sampler->MaxAnisotropy - 2) / 2, BRW_ANISORATIO_16);
      }
   }

   /* Set address rounding bits if not using nearest filtering. */
   unsigned address_rounding = 0;
   if (min_filter != BRW_MAPFILTER_NEAREST) {
      address_rounding |= BRW_ADDRESS_ROUNDING_ENABLE_U_MIN |
                          BRW_ADDRESS_ROUNDING_ENABLE_V_MIN |
                          BRW_ADDRESS_ROUNDING_ENABLE_R_MIN;
   }
   if (mag_filter != BRW_MAPFILTER_NEAREST) {
      address_rounding |= BRW_ADDRESS_ROUNDING_ENABLE_U_MAG |
                          BRW_ADDRESS_ROUNDING_ENABLE_V_MAG |
                          BRW_ADDRESS_ROUNDING_ENABLE_R_MAG;
   }

   bool either_nearest =
      sampler->MinFilter == GL_NEAREST || sampler->MagFilter == GL_NEAREST;
   unsigned wrap_s = translate_wrap_mode(brw, sampler->WrapS, either_nearest);
   unsigned wrap_t = translate_wrap_mode(brw, sampler->WrapT, either_nearest);
   unsigned wrap_r = translate_wrap_mode(brw, sampler->WrapR, either_nearest);

   if (texObj->Target == GL_TEXTURE_CUBE_MAP ||
       texObj->Target == GL_TEXTURE_CUBE_MAP_ARRAY) {
      /* Cube maps must use the same wrap mode for all three coordinate
       * dimensions.  Prior to Haswell, only CUBE and CLAMP are valid.
       */
      if ((ctx->Texture.CubeMapSeamless || sampler->CubeMapSeamless) &&
         (sampler->MinFilter != GL_NEAREST ||
          sampler->MagFilter != GL_NEAREST)) {
	 wrap_s = BRW_TEXCOORDMODE_CUBE;
	 wrap_t = BRW_TEXCOORDMODE_CUBE;
	 wrap_r = BRW_TEXCOORDMODE_CUBE;
      } else {
	 wrap_s = BRW_TEXCOORDMODE_CLAMP;
	 wrap_t = BRW_TEXCOORDMODE_CLAMP;
	 wrap_r = BRW_TEXCOORDMODE_CLAMP;
      }
   } else if (texObj->Target == GL_TEXTURE_1D) {
      /* There's a bug in 1D texture sampling - it actually pays
       * attention to the wrap_t value, though it should not.
       * Override the wrap_t value here to GL_REPEAT to keep
       * any nonexistent border pixels from floating in.
       */
      wrap_t = BRW_TEXCOORDMODE_WRAP;
   }

   /* Set shadow function. */
   unsigned shadow_function = 0;
   if (sampler->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      shadow_function =
	 intel_translate_shadow_compare_func(sampler->CompareFunc);
   }

   const int lod_bits = brw->gen >= 7 ? 8 : 6;
   const unsigned min_lod = U_FIXED(CLAMP(sampler->MinLod, 0, 13), lod_bits);
   const unsigned max_lod = U_FIXED(CLAMP(sampler->MaxLod, 0, 13), lod_bits);
   const int lod_bias =
      S_FIXED(CLAMP(texUnit->LodBias + sampler->LodBias, -16, 15), lod_bits);
   const unsigned base_level = U_FIXED(0, 1);

   /* Upload the border color if necessary.  If not, just point it at
    * offset 0 (the start of the batch) - the color should be ignored,
    * but that address won't fault in case something reads it anyway.
    */
   uint32_t border_color_offset = 0;
   if (wrap_mode_needs_border_color(wrap_s) ||
       wrap_mode_needs_border_color(wrap_t) ||
       wrap_mode_needs_border_color(wrap_r)) {
      upload_default_color(brw, sampler, unit, &border_color_offset);
   }

   const bool non_normalized_coords = texObj->Target == GL_TEXTURE_RECTANGLE;

   brw_emit_sampler_state(brw,
                          sampler_state,
                          batch_offset_for_sampler_state,
                          min_filter, mag_filter, mip_filter,
                          max_anisotropy,
                          address_rounding,
                          wrap_s, wrap_t, wrap_r,
                          min_lod, max_lod, lod_bias, base_level,
                          shadow_function,
                          non_normalized_coords,
                          border_color_offset);
}


static void
brw_upload_sampler_state_table(struct brw_context *brw,
                               struct gl_program *prog,
                               struct brw_stage_state *stage_state)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t sampler_count = stage_state->sampler_count;

   GLbitfield SamplersUsed = prog->SamplersUsed;

   if (sampler_count == 0)
      return;

   /* SAMPLER_STATE is 4 DWords on all platforms. */
   const int dwords = 4;
   const int size_in_bytes = dwords * sizeof(uint32_t);

   uint32_t *sampler_state = brw_state_batch(brw, AUB_TRACE_SAMPLER_STATE,
                                             sampler_count * size_in_bytes,
                                             32, &stage_state->sampler_offset);
   memset(sampler_state, 0, sampler_count * size_in_bytes);

   uint32_t batch_offset_for_sampler_state = stage_state->sampler_offset;

   for (unsigned s = 0; s < sampler_count; s++) {
      if (SamplersUsed & (1 << s)) {
         const unsigned unit = prog->SamplerUnits[s];
         if (ctx->Texture.Unit[unit]._Current) {
            brw_update_sampler_state(brw, unit, sampler_state,
                                     batch_offset_for_sampler_state);
         }
      }

      sampler_state += dwords;
      batch_offset_for_sampler_state += size_in_bytes;
   }

   if (brw->gen >= 7) {
      /* Emit a 3DSTATE_SAMPLER_STATE_POINTERS_XS packet. */
      gen7_emit_sampler_state_pointers_xs(brw, stage_state);
   } else {
      /* Flag that the sampler state table pointer has changed; later atoms
       * will handle it.
       */
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }
}

static void
brw_upload_fs_samplers(struct brw_context *brw)
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct gl_program *fs = (struct gl_program *) brw->fragment_program;
   brw_upload_sampler_state_table(brw, fs, &brw->wm.base);
}

const struct brw_tracked_state brw_fs_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_FRAGMENT_PROGRAM,
      .cache = 0
   },
   .emit = brw_upload_fs_samplers,
};

static void
brw_upload_vs_samplers(struct brw_context *brw)
{
   /* BRW_NEW_VERTEX_PROGRAM */
   struct gl_program *vs = (struct gl_program *) brw->vertex_program;
   brw_upload_sampler_state_table(brw, vs, &brw->vs.base);
}


const struct brw_tracked_state brw_vs_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_VERTEX_PROGRAM,
      .cache = 0
   },
   .emit = brw_upload_vs_samplers,
};


static void
brw_upload_gs_samplers(struct brw_context *brw)
{
   /* BRW_NEW_GEOMETRY_PROGRAM */
   struct gl_program *gs = (struct gl_program *) brw->geometry_program;
   if (!gs)
      return;

   brw_upload_sampler_state_table(brw, gs, &brw->gs.base);
}


const struct brw_tracked_state brw_gs_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_GEOMETRY_PROGRAM,
      .cache = 0
   },
   .emit = brw_upload_gs_samplers,
};
