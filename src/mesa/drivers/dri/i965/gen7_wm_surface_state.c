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
#include "main/mtypes.h"
#include "main/blend.h"
#include "main/samplerobj.h"
#include "main/texformat.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"
#include "intel_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/**
 * Convert an swizzle enumeration (i.e. SWIZZLE_X) to one of the Gen7.5+
 * "Shader Channel Select" enumerations (i.e. HSW_SCS_RED)
 */
static unsigned
swizzle_to_scs(GLenum swizzle)
{
   switch (swizzle) {
   case SWIZZLE_X:
      return HSW_SCS_RED;
   case SWIZZLE_Y:
      return HSW_SCS_GREEN;
   case SWIZZLE_Z:
      return HSW_SCS_BLUE;
   case SWIZZLE_W:
      return HSW_SCS_ALPHA;
   case SWIZZLE_ZERO:
      return HSW_SCS_ZERO;
   case SWIZZLE_ONE:
      return HSW_SCS_ONE;
   }

   assert(!"Should not get here: invalid swizzle mode");
   return HSW_SCS_ZERO;
}

uint32_t
gen7_surface_tiling_mode(uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_X:
      return GEN7_SURFACE_TILING_X;
   case I915_TILING_Y:
      return GEN7_SURFACE_TILING_Y;
   default:
      return GEN7_SURFACE_TILING_NONE;
   }
}


uint32_t
gen7_surface_msaa_bits(unsigned num_samples, enum intel_msaa_layout layout)
{
   uint32_t ss4 = 0;

   if (num_samples > 4)
      ss4 |= GEN7_SURFACE_MULTISAMPLECOUNT_8;
   else if (num_samples > 1)
      ss4 |= GEN7_SURFACE_MULTISAMPLECOUNT_4;
   else
      ss4 |= GEN7_SURFACE_MULTISAMPLECOUNT_1;

   if (layout == INTEL_MSAA_LAYOUT_IMS)
      ss4 |= GEN7_SURFACE_MSFMT_DEPTH_STENCIL;
   else
      ss4 |= GEN7_SURFACE_MSFMT_MSS;

   return ss4;
}


void
gen7_set_surface_mcs_info(struct brw_context *brw,
                          uint32_t *surf,
                          uint32_t surf_offset,
                          const struct intel_mipmap_tree *mcs_mt,
                          bool is_render_target)
{
   /* From the Ivy Bridge PRM, Vol4 Part1 p76, "MCS Base Address":
    *
    *     "The MCS surface must be stored as Tile Y."
    */
   assert(mcs_mt->region->tiling == I915_TILING_Y);

   /* Compute the pitch in units of tiles.  To do this we need to divide the
    * pitch in bytes by 128, since a single Y-tile is 128 bytes wide.
    */
   unsigned pitch_tiles = mcs_mt->region->pitch / 128;

   /* The upper 20 bits of surface state DWORD 6 are the upper 20 bits of the
    * GPU address of the MCS buffer; the lower 12 bits contain other control
    * information.  Since buffer addresses are always on 4k boundaries (and
    * thus have their lower 12 bits zero), we can use an ordinary reloc to do
    * the necessary address translation.
    */
   assert ((mcs_mt->region->bo->offset & 0xfff) == 0);

   surf[6] = GEN7_SURFACE_MCS_ENABLE |
             SET_FIELD(pitch_tiles - 1, GEN7_SURFACE_MCS_PITCH) |
             mcs_mt->region->bo->offset;

   drm_intel_bo_emit_reloc(brw->batch.bo,
                           surf_offset + 6 * 4,
                           mcs_mt->region->bo,
                           surf[6] & 0xfff,
                           is_render_target ? I915_GEM_DOMAIN_RENDER
                           : I915_GEM_DOMAIN_SAMPLER,
                           is_render_target ? I915_GEM_DOMAIN_RENDER : 0);
}


void
gen7_check_surface_setup(uint32_t *surf, bool is_render_target)
{
   unsigned num_multisamples = surf[4] & INTEL_MASK(5, 3);
   unsigned multisampled_surface_storage_format = surf[4] & (1 << 6);
   unsigned surface_array_spacing = surf[0] & (1 << 10);
   bool is_multisampled = num_multisamples != GEN7_SURFACE_MULTISAMPLECOUNT_1;

   (void) surface_array_spacing;

   /* From the Ivybridge PRM, Volume 4 Part 1, page 66 (RENDER_SURFACE_STATE
    * dword 0 bit 10 "Surface Array Spacing" Programming Notes):
    *
    *   If Multisampled Surface Storage Format is MSFMT_MSS and Number of
    *   Multisamples is not MULTISAMPLECOUNT_1, this field must be set to
    *   ARYSPC_LOD0.
    */
   if (multisampled_surface_storage_format == GEN7_SURFACE_MSFMT_MSS
       && is_multisampled)
      assert(surface_array_spacing == GEN7_SURFACE_ARYSPC_LOD0);

   /* From the Ivybridge PRM, Volume 4 Part 1, page 72 (RENDER_SURFACE_STATE
    * dword 4 bit 6 "Multisampled Surface Storage" Programming Notes):
    *
    *   All multisampled render target surfaces must have this field set to
    *   MSFMT_MSS.
    *
    * But also:
    *
    *   This field is ignored if Number of Multisamples is MULTISAMPLECOUNT_1.
    */
   if (is_render_target && is_multisampled) {
      assert(multisampled_surface_storage_format == GEN7_SURFACE_MSFMT_MSS);
   }

   /* From the Ivybridge PRM, Volume 4 Part 1, page 72 (RENDER_SURFACE_STATE
    * dword 4 bit 6 "Multisampled Surface Storage Format" Errata):
    *
    *   If the surface’s Number of Multisamples is MULTISAMPLECOUNT_8, Width
    *   is >= 8192 (meaning the actual surface width is >= 8193 pixels), this
    *   field must be set to MSFMT_MSS.
    */
   uint32_t width = GET_FIELD(surf[2], GEN7_SURFACE_WIDTH) + 1;
   if (num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_8 && width >= 8193) {
      assert(multisampled_surface_storage_format == GEN7_SURFACE_MSFMT_MSS);
   }

   /* From the Ivybridge PRM, Volume 4 Part 1, page 72 (RENDER_SURFACE_STATE
    * dword 4 bit 6 "Multisampled Surface Storage Format" Errata):
    *
    *   If the surface’s Number of Multisamples is MULTISAMPLECOUNT_8,
    *   ((Depth+1) * (Height+1)) is > 4,194,304, OR if the surface’s Number of
    *   Multisamples is MULTISAMPLECOUNT_4, ((Depth+1) * (Height+1)) is >
    *   8,388,608, this field must be set to MSFMT_DEPTH_STENCIL.This field
    *   must be set to MSFMT_DEPTH_STENCIL if Surface Format is one of the
    *   following: I24X8_UNORM, L24X8_UNORM, A24X8_UNORM, or
    *   R24_UNORM_X8_TYPELESS.
    *
    * But also (from the Programming Notes):
    *
    *   This field is ignored if Number of Multisamples is MULTISAMPLECOUNT_1.
    */
   uint32_t depth = GET_FIELD(surf[3], BRW_SURFACE_DEPTH) + 1;
   uint32_t height = GET_FIELD(surf[2], GEN7_SURFACE_HEIGHT) + 1;
   if (num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_8 &&
       depth * height > 4194304) {
      assert(multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
   }
   if (num_multisamples == GEN7_SURFACE_MULTISAMPLECOUNT_4 &&
       depth * height > 8388608) {
      assert(multisampled_surface_storage_format ==
             GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
   }
   if (is_multisampled) {
      switch (GET_FIELD(surf[0], BRW_SURFACE_FORMAT)) {
      case BRW_SURFACEFORMAT_I24X8_UNORM:
      case BRW_SURFACEFORMAT_L24X8_UNORM:
      case BRW_SURFACEFORMAT_A24X8_UNORM:
      case BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS:
         assert(multisampled_surface_storage_format ==
                GEN7_SURFACE_MSFMT_DEPTH_STENCIL);
      }
   }
}


static void
gen7_update_buffer_texture_surface(struct gl_context *ctx,
                                   unsigned unit,
                                   uint32_t *binding_table,
                                   unsigned surf_index)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_buffer_object *intel_obj =
      intel_buffer_object(tObj->BufferObject);
   drm_intel_bo *bo = intel_obj ? intel_obj->buffer : NULL;
   gl_format format = tObj->_BufferObjectFormat;

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
                                    8 * 4, 32, &binding_table[surf_index]);
   memset(surf, 0, 8 * 4);

   uint32_t surface_format = brw_format_for_mesa_format(format);
   if (surface_format == 0 && format != MESA_FORMAT_RGBA_FLOAT32) {
      _mesa_problem(NULL, "bad format %s for texture buffer\n",
                    _mesa_get_format_name(format));
   }

   surf[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
             surface_format << BRW_SURFACE_FORMAT_SHIFT |
             BRW_SURFACE_RC_READ_WRITE;

   if (bo) {
      surf[1] = bo->offset; /* reloc */

      drm_intel_bo_emit_reloc(brw->batch.bo,
			      binding_table[surf_index] + 4,
			      bo, 0,
			      I915_GEM_DOMAIN_SAMPLER, 0);

      int texel_size = _mesa_get_format_bytes(format);
      int w = intel_obj->Base.Size / texel_size;

      /* note that these differ from GEN6 */
      surf[2] = SET_FIELD(w & 0x7f, GEN7_SURFACE_WIDTH) | /* bits 6:0 of size */
                SET_FIELD((w >> 7) & 0x3fff, GEN7_SURFACE_HEIGHT); /* 20:7 */
      surf[3] = SET_FIELD((w >> 21) & 0x3f, BRW_SURFACE_DEPTH) | /* bits 26:21 */
                (texel_size - 1);
   }

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

static void
gen7_update_texture_surface(struct gl_context *ctx,
                            unsigned unit,
                            uint32_t *binding_table,
                            unsigned surf_index)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct intel_mipmap_tree *mt = intelObj->mt;
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   uint32_t tile_x, tile_y;
   uint8_t mocs = brw->is_haswell ? GEN7_MOCS_L3 : 0;

   if (tObj->Target == GL_TEXTURE_BUFFER) {
      gen7_update_buffer_texture_surface(ctx, unit, binding_table, surf_index);
      return;
   }

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
                                    8 * 4, 32, &binding_table[surf_index]);
   memset(surf, 0, 8 * 4);

   uint32_t tex_format = translate_tex_format(brw,
                                              mt->format,
                                              tObj->DepthMode,
                                              sampler->sRGBDecode);

   surf[0] = translate_tex_target(tObj->Target) << BRW_SURFACE_TYPE_SHIFT |
             tex_format << BRW_SURFACE_FORMAT_SHIFT |
             gen7_surface_tiling_mode(mt->region->tiling) |
             BRW_SURFACE_CUBEFACE_ENABLES;

   if (mt->align_h == 4)
      surf[0] |= GEN7_SURFACE_VALIGN_4;
   if (mt->align_w == 8)
      surf[0] |= GEN7_SURFACE_HALIGN_8;

   if (mt->logical_depth0 > 1 && tObj->Target != GL_TEXTURE_3D)
      surf[0] |= GEN7_SURFACE_IS_ARRAY;

   if (mt->array_spacing_lod0)
      surf[0] |= GEN7_SURFACE_ARYSPC_LOD0;

   surf[1] = mt->region->bo->offset + mt->offset; /* reloc */
   surf[1] += intel_miptree_get_tile_offsets(intelObj->mt, firstImage->Level, 0,
                                             &tile_x, &tile_y);

   surf[2] = SET_FIELD(mt->logical_width0 - 1, GEN7_SURFACE_WIDTH) |
             SET_FIELD(mt->logical_height0 - 1, GEN7_SURFACE_HEIGHT);
   surf[3] = SET_FIELD(mt->logical_depth0 - 1, BRW_SURFACE_DEPTH) |
             ((intelObj->mt->region->pitch) - 1);

   surf[4] = gen7_surface_msaa_bits(mt->num_samples, mt->msaa_layout);

   assert(brw->has_surface_tile_offset || (tile_x == 0 && tile_y == 0));
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
              (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT |
              SET_FIELD(mocs, GEN7_SURFACE_MOCS) |
              /* mip count */
              (intelObj->_MaxLevel - tObj->BaseLevel));

   if (brw->is_haswell) {
      /* Handling GL_ALPHA as a surface format override breaks 1.30+ style
       * texturing functions that return a float, as our code generation always
       * selects the .x channel (which would always be 0).
       */
      const bool alpha_depth = tObj->DepthMode == GL_ALPHA &&
         (firstImage->_BaseFormat == GL_DEPTH_COMPONENT ||
          firstImage->_BaseFormat == GL_DEPTH_STENCIL);

      const int swizzle = unlikely(alpha_depth)
         ? SWIZZLE_XYZW : brw_get_texture_swizzle(ctx, tObj);

      surf[7] =
         SET_FIELD(swizzle_to_scs(GET_SWZ(swizzle, 0)), GEN7_SURFACE_SCS_R) |
         SET_FIELD(swizzle_to_scs(GET_SWZ(swizzle, 1)), GEN7_SURFACE_SCS_G) |
         SET_FIELD(swizzle_to_scs(GET_SWZ(swizzle, 2)), GEN7_SURFACE_SCS_B) |
         SET_FIELD(swizzle_to_scs(GET_SWZ(swizzle, 3)), GEN7_SURFACE_SCS_A);
   }

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->batch.bo,
			   binding_table[surf_index] + 4,
			   intelObj->mt->region->bo,
                           surf[1] - intelObj->mt->region->bo->offset,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will
 * be read from this buffer with Data Port Read instructions/messages.
 */
static void
gen7_create_constant_surface(struct brw_context *brw,
			     drm_intel_bo *bo,
			     uint32_t offset,
			     uint32_t size,
			     uint32_t *out_offset,
                             bool dword_pitch)
{
   uint32_t stride = dword_pitch ? 4 : 16;
   uint32_t elements = ALIGN(size, stride) / stride;
   const GLint w = elements - 1;

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
                                    8 * 4, 32, out_offset);
   memset(surf, 0, 8 * 4);

   surf[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
             BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_SURFACE_FORMAT_SHIFT |
             BRW_SURFACE_RC_READ_WRITE;

   assert(bo);
   surf[1] = bo->offset + offset; /* reloc */

   /* note that these differ from GEN6 */
   surf[2] = SET_FIELD(w & 0x7f, GEN7_SURFACE_WIDTH) |
             SET_FIELD((w >> 7) & 0x3fff, GEN7_SURFACE_HEIGHT);
   surf[3] = SET_FIELD((w >> 21) & 0x3f, BRW_SURFACE_DEPTH) |
             (stride - 1);

   if (brw->is_haswell) {
      surf[7] = SET_FIELD(HSW_SCS_RED,   GEN7_SURFACE_SCS_R) |
                SET_FIELD(HSW_SCS_GREEN, GEN7_SURFACE_SCS_G) |
                SET_FIELD(HSW_SCS_BLUE,  GEN7_SURFACE_SCS_B) |
                SET_FIELD(HSW_SCS_ALPHA, GEN7_SURFACE_SCS_A);
   }

   drm_intel_bo_emit_reloc(brw->batch.bo,
			   *out_offset + 4,
			   bo, offset,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

/**
 * Create a surface for shader time.
 */
void
gen7_create_shader_time_surface(struct brw_context *brw, uint32_t *out_offset)
{
   const int w = brw->shader_time.bo->size - 1;

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
                                    8 * 4, 32, out_offset);
   memset(surf, 0, 8 * 4);

   surf[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
             BRW_SURFACEFORMAT_RAW << BRW_SURFACE_FORMAT_SHIFT |
             BRW_SURFACE_RC_READ_WRITE;

   surf[1] = brw->shader_time.bo->offset; /* reloc */

   /* note that these differ from GEN6 */
   surf[2] = SET_FIELD(w & 0x7f, GEN7_SURFACE_WIDTH) |
             SET_FIELD((w >> 7) & 0x3fff, GEN7_SURFACE_HEIGHT);
   surf[3] = SET_FIELD((w >> 21) & 0x3f, BRW_SURFACE_DEPTH);

   /* Unlike texture or renderbuffer surfaces, we only do untyped operations
    * on the shader_time surface, so there's no need to set HSW channel
    * overrides.
    */

   drm_intel_bo_emit_reloc(brw->batch.bo,
                           *out_offset + 4,
                           brw->shader_time.bo, 0,
                           I915_GEM_DOMAIN_SAMPLER, 0);

   gen7_check_surface_setup(surf, false /* is_render_target */);
}

static void
gen7_update_null_renderbuffer_surface(struct brw_context *brw, unsigned unit)
{
   /* From the Ivy bridge PRM, Vol4 Part1 p62 (Surface Type: Programming
    * Notes):
    *
    *     A null surface is used in instances where an actual surface is not
    *     bound. When a write message is generated to a null surface, no
    *     actual surface is written to. When a read message (including any
    *     sampling engine message) is generated to a null surface, the result
    *     is all zeros. Note that a null surface type is allowed to be used
    *     with all messages, even if it is not specificially indicated as
    *     supported. All of the remaining fields in surface state are ignored
    *     for null surfaces, with the following exceptions: Width, Height,
    *     Depth, LOD, and Render Target View Extent fields must match the
    *     depth buffer’s corresponding state for all render target surfaces,
    *     including null.
    */
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   const struct gl_framebuffer *fb = ctx->DrawBuffer;

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			            8 * 4, 32, &brw->wm.surf_offset[unit]);
   memset(surf, 0, 8 * 4);

   /* From the Ivybridge PRM, Volume 4, Part 1, page 65,
    * Tiled Surface: Programming Notes:
    * "If Surface Type is SURFTYPE_NULL, this field must be TRUE."
    */
   surf[0] = BRW_SURFACE_NULL << BRW_SURFACE_TYPE_SHIFT |
             BRW_SURFACEFORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT |
             GEN7_SURFACE_TILING_Y;

   surf[2] = SET_FIELD(fb->Width - 1, GEN7_SURFACE_WIDTH) |
             SET_FIELD(fb->Height - 1, GEN7_SURFACE_HEIGHT);

   gen7_check_surface_setup(surf, true /* is_render_target */);
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
gen7_update_renderbuffer_surface(struct brw_context *brw,
				 struct gl_renderbuffer *rb,
				 bool layered,
				 unsigned int unit)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb->mt->region;
   uint32_t format;
   /* _NEW_BUFFERS */
   gl_format rb_format = _mesa_get_render_format(ctx, intel_rb_format(irb));
   uint32_t surftype;
   bool is_array = false;
   int depth = MAX2(rb->Depth, 1);
   int min_array_element;
   uint8_t mocs = brw->is_haswell ? GEN7_MOCS_L3 : 0;
   GLenum gl_target = rb->TexImage ?
                         rb->TexImage->TexObject->Target : GL_TEXTURE_2D;

   uint32_t *surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
                                    8 * 4, 32, &brw->wm.surf_offset[unit]);
   memset(surf, 0, 8 * 4);

   intel_miptree_used_for_rendering(irb->mt);

   /* Render targets can't use IMS layout */
   assert(irb->mt->msaa_layout != INTEL_MSAA_LAYOUT_IMS);

   assert(brw_render_target_supported(brw, rb));
   format = brw->render_target_format[rb_format];
   if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
      _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
                    __FUNCTION__, _mesa_get_format_name(rb_format));
   }

   switch (gl_target) {
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
      surftype = BRW_SURFACE_2D;
      is_array = true;
      depth *= 6;
      break;
   default:
      surftype = translate_tex_target(gl_target);
      is_array = _mesa_tex_target_is_array(gl_target);
      break;
   }

   if (layered) {
      min_array_element = 0;
   } else if (irb->mt->num_samples > 1) {
      min_array_element = irb->mt_layer / irb->mt->num_samples;
   } else {
      min_array_element = irb->mt_layer;
   }

   surf[0] = surftype << BRW_SURFACE_TYPE_SHIFT |
             format << BRW_SURFACE_FORMAT_SHIFT |
             (irb->mt->array_spacing_lod0 ? GEN7_SURFACE_ARYSPC_LOD0
                                          : GEN7_SURFACE_ARYSPC_FULL) |
             gen7_surface_tiling_mode(region->tiling);

   if (irb->mt->align_h == 4)
      surf[0] |= GEN7_SURFACE_VALIGN_4;
   if (irb->mt->align_w == 8)
      surf[0] |= GEN7_SURFACE_HALIGN_8;

   if (is_array) {
      surf[0] |= GEN7_SURFACE_IS_ARRAY;
   }

   surf[1] = region->bo->offset;

   assert(brw->has_surface_tile_offset);

   surf[5] = SET_FIELD(mocs, GEN7_SURFACE_MOCS) |
             (irb->mt_level - irb->mt->first_level);

   surf[2] = SET_FIELD(irb->mt->logical_width0 - 1, GEN7_SURFACE_WIDTH) |
             SET_FIELD(irb->mt->logical_height0 - 1, GEN7_SURFACE_HEIGHT);

   surf[3] = ((depth - 1) << BRW_SURFACE_DEPTH_SHIFT) |
             (region->pitch - 1);

   surf[4] = gen7_surface_msaa_bits(irb->mt->num_samples, irb->mt->msaa_layout) |
             min_array_element << GEN7_SURFACE_MIN_ARRAY_ELEMENT_SHIFT |
             (depth - 1) << GEN7_SURFACE_RENDER_TARGET_VIEW_EXTENT_SHIFT;

   if (irb->mt->mcs_mt) {
      gen7_set_surface_mcs_info(brw, surf, brw->wm.surf_offset[unit],
                                irb->mt->mcs_mt, true /* is RT */);
   }

   surf[7] = irb->mt->fast_clear_color_value;

   if (brw->is_haswell) {
      surf[7] |= (SET_FIELD(HSW_SCS_RED,   GEN7_SURFACE_SCS_R) |
                  SET_FIELD(HSW_SCS_GREEN, GEN7_SURFACE_SCS_G) |
                  SET_FIELD(HSW_SCS_BLUE,  GEN7_SURFACE_SCS_B) |
                  SET_FIELD(HSW_SCS_ALPHA, GEN7_SURFACE_SCS_A));
   }

   drm_intel_bo_emit_reloc(brw->batch.bo,
			   brw->wm.surf_offset[unit] + 4,
			   region->bo,
			   surf[1] - region->bo->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);

   gen7_check_surface_setup(surf, true /* is_render_target */);
}

void
gen7_init_vtable_surface_functions(struct brw_context *brw)
{
   brw->vtbl.update_texture_surface = gen7_update_texture_surface;
   brw->vtbl.update_renderbuffer_surface = gen7_update_renderbuffer_surface;
   brw->vtbl.update_null_renderbuffer_surface =
      gen7_update_null_renderbuffer_surface;
   brw->vtbl.create_constant_surface = gen7_create_constant_surface;
}
