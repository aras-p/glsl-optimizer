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

#include "genhw/genhw.h"
#include "util/u_resource.h"

#include "ilo_format.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_gpe_gen7.h"

#define SET_FIELD(value, field) (((value) << field ## __SHIFT) & field ## __MASK)

void
ilo_gpe_init_gs_cso_gen7(const struct ilo_dev_info *dev,
                         const struct ilo_shader_state *gs,
                         struct ilo_shader_cso *cso)
{
   int start_grf, vue_read_len, max_threads;
   uint32_t dw2, dw4, dw5;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   start_grf = ilo_shader_get_kernel_param(gs, ILO_KERNEL_URB_DATA_START_REG);
   vue_read_len = ilo_shader_get_kernel_param(gs, ILO_KERNEL_INPUT_COUNT);

   /* in pairs */
   vue_read_len = (vue_read_len + 1) / 2;

   switch (dev->gen) {
   case ILO_GEN(7.5):
      max_threads = (dev->gt >= 2) ? 256 : 70;
      break;
   case ILO_GEN(7):
      max_threads = (dev->gt == 2) ? 128 : 36;
      break;
   default:
      max_threads = 1;
      break;
   }

   dw2 = (true) ? 0 : GEN6_THREADDISP_FP_MODE_ALT;

   dw4 = vue_read_len << GEN7_GS_DW4_URB_READ_LEN__SHIFT |
         GEN7_GS_DW4_INCLUDE_VERTEX_HANDLES |
         0 << GEN7_GS_DW4_URB_READ_OFFSET__SHIFT |
         start_grf << GEN7_GS_DW4_URB_GRF_START__SHIFT;

   dw5 = (max_threads - 1) << GEN7_GS_DW5_MAX_THREADS__SHIFT |
         GEN7_GS_DW5_STATISTICS |
         GEN7_GS_DW5_GS_ENABLE;

   STATIC_ASSERT(Elements(cso->payload) >= 3);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
}

void
ilo_gpe_init_rasterizer_wm_gen7(const struct ilo_dev_info *dev,
                                const struct pipe_rasterizer_state *state,
                                struct ilo_rasterizer_wm *wm)
{
   uint32_t dw1, dw2;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   dw1 = GEN7_WM_DW1_ZW_INTERP_PIXEL |
         GEN7_WM_DW1_AA_LINE_WIDTH_2_0 |
         GEN7_WM_DW1_MSRASTMODE_OFF_PIXEL;

   /* same value as in 3DSTATE_SF */
   if (state->line_smooth)
      dw1 |= GEN7_WM_DW1_AA_LINE_CAP_1_0;

   if (state->poly_stipple_enable)
      dw1 |= GEN7_WM_DW1_POLY_STIPPLE_ENABLE;
   if (state->line_stipple_enable)
      dw1 |= GEN7_WM_DW1_LINE_STIPPLE_ENABLE;

   if (state->bottom_edge_rule)
      dw1 |= GEN7_WM_DW1_POINT_RASTRULE_UPPER_RIGHT;

   dw2 = GEN7_WM_DW2_MSDISPMODE_PERSAMPLE;

   /*
    * assertion that makes sure
    *
    *   dw1 |= wm->dw_msaa_rast;
    *   dw2 |= wm->dw_msaa_disp;
    *
    * is valid
    */
   STATIC_ASSERT(GEN7_WM_DW1_MSRASTMODE_OFF_PIXEL == 0 &&
                 GEN7_WM_DW2_MSDISPMODE_PERSAMPLE == 0);

   wm->dw_msaa_rast =
      (state->multisample) ? GEN7_WM_DW1_MSRASTMODE_ON_PATTERN : 0;
   wm->dw_msaa_disp = GEN7_WM_DW2_MSDISPMODE_PERPIXEL;

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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   start_grf = ilo_shader_get_kernel_param(fs, ILO_KERNEL_URB_DATA_START_REG);

   dw2 = (true) ? 0 : GEN6_THREADDISP_FP_MODE_ALT;

   dw4 = GEN7_PS_DW4_POSOFFSET_NONE;

   /* see brwCreateContext() */
   switch (dev->gen) {
   case ILO_GEN(7.5):
      max_threads = (dev->gt == 3) ? 408 : (dev->gt == 2) ? 204 : 102;
      dw4 |= (max_threads - 1) << GEN75_PS_DW4_MAX_THREADS__SHIFT;
      dw4 |= 1 << GEN75_PS_DW4_SAMPLE_MASK__SHIFT;
      break;
   case ILO_GEN(7):
   default:
      max_threads = (dev->gt == 2) ? 172 : 48;
      dw4 |= (max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
      break;
   }

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_PCB_CBUF0_SIZE))
      dw4 |= GEN7_PS_DW4_PUSH_CONSTANT_ENABLE;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_INPUT_COUNT))
      dw4 |= GEN7_PS_DW4_ATTR_ENABLE;

   assert(!ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_DISPATCH_16_OFFSET));
   dw4 |= GEN7_PS_DW4_8_PIXEL_DISPATCH;

   dw5 = start_grf << GEN7_PS_DW5_URB_GRF_START0__SHIFT |
         0 << GEN7_PS_DW5_URB_GRF_START1__SHIFT |
         0 << GEN7_PS_DW5_URB_GRF_START2__SHIFT;

   /* FS affects 3DSTATE_WM too */
   wm_dw1 = 0;

   /*
    * TODO set this bit only when
    *
    *  a) fs writes colors and color is not masked, or
    *  b) fs writes depth, or
    *  c) fs or cc kills
    */
   wm_dw1 |= GEN7_WM_DW1_PS_ENABLE;

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
      wm_dw1 |= GEN7_WM_DW1_PS_KILL;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_OUTPUT_Z))
      wm_dw1 |= GEN7_WM_DW1_PSCDEPTH_ON;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_Z))
      wm_dw1 |= GEN7_WM_DW1_PS_USE_DEPTH;

   if (ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_INPUT_W))
      wm_dw1 |= GEN7_WM_DW1_PS_USE_W;

   wm_interps = ilo_shader_get_kernel_param(fs,
         ILO_KERNEL_FS_BARYCENTRIC_INTERPOLATIONS);

   wm_dw1 |= wm_interps << GEN7_WM_DW1_BARYCENTRIC_INTERP__SHIFT;

   STATIC_ASSERT(Elements(cso->payload) >= 4);
   cso->payload[0] = dw2;
   cso->payload[1] = dw4;
   cso->payload[2] = dw5;
   cso->payload[3] = wm_dw1;
}

void
ilo_gpe_init_view_surface_null_gen7(const struct ilo_dev_info *dev,
                                    unsigned width, unsigned height,
                                    unsigned depth, unsigned level,
                                    struct ilo_view_surface *surf)
{
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

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

   dw[0] = GEN6_SURFTYPE_NULL << GEN7_SURFACE_DW0_TYPE__SHIFT |
           GEN6_FORMAT_B8G8R8A8_UNORM << GEN7_SURFACE_DW0_FORMAT__SHIFT |
           GEN6_TILING_X << 13;

   dw[1] = 0;

   dw[2] = SET_FIELD(height - 1, GEN7_SURFACE_DW2_HEIGHT) |
           SET_FIELD(width  - 1, GEN7_SURFACE_DW2_WIDTH);

   dw[3] = SET_FIELD(depth - 1, GEN7_SURFACE_DW3_DEPTH);

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

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   surface_type = (structured) ? GEN7_SURFTYPE_STRBUF : GEN6_SURFTYPE_BUFFER;

   surface_format = (typed) ?
      ilo_translate_color_format(elem_format) : GEN6_FORMAT_RAW;

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

   dw[0] = surface_type << GEN7_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN7_SURFACE_DW0_FORMAT__SHIFT;
   if (render_cache_rw)
      dw[0] |= GEN7_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = offset;

   dw[2] = SET_FIELD(height, GEN7_SURFACE_DW2_HEIGHT) |
           SET_FIELD(width, GEN7_SURFACE_DW2_WIDTH);

   dw[3] = SET_FIELD(depth, GEN7_SURFACE_DW3_DEPTH) |
           pitch;

   dw[4] = 0;
   dw[5] = 0;

   dw[6] = 0;
   dw[7] = 0;

   if (dev->gen >= ILO_GEN(7.5)) {
      dw[7] |= SET_FIELD(GEN75_SCS_RED,   GEN75_SURFACE_DW7_SCS_R) |
               SET_FIELD(GEN75_SCS_GREEN, GEN75_SURFACE_DW7_SCS_G) |
               SET_FIELD(GEN75_SCS_BLUE,  GEN75_SURFACE_DW7_SCS_B) |
               SET_FIELD(GEN75_SCS_ALPHA, GEN75_SURFACE_DW7_SCS_A);
   }

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
                                           bool is_rt, bool offset_to_layer,
                                           struct ilo_view_surface *surf)
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;
   unsigned layer_offset, x_offset, y_offset;
   uint32_t *dw;

   ILO_GPE_VALID_GEN(dev, 7, 7.5);

   surface_type = ilo_gpe_gen6_translate_texture(tex->base.target);
   assert(surface_type != GEN6_SURFTYPE_BUFFER);

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

   if (surface_type == GEN6_SURFTYPE_CUBE) {
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
         surface_type = GEN6_SURFTYPE_2D;
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
   case GEN6_SURFTYPE_1D:
      assert(width <= 16384 && height == 1 && depth <= 2048);
      break;
   case GEN6_SURFTYPE_2D:
      assert(width <= 16384 && height <= 16384 && depth <= 2048);
      break;
   case GEN6_SURFTYPE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case GEN6_SURFTYPE_CUBE:
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
      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      lod = num_levels - 1;
   }

   /*
    * Offset to the layer.  When rendering, the hardware requires LOD and
    * Depth to be the same for all render targets and the depth buffer.  We
    * need to offset to the layer manually and always set LOD and Depth to 0.
    */
   if (offset_to_layer) {
      /* we lose the capability for layered rendering */
      assert(is_rt && num_layers == 1);

      layer_offset = ilo_texture_get_slice_offset(tex,
            first_level, first_layer, &x_offset, &y_offset);

      assert(x_offset % 4 == 0);
      assert(y_offset % 2 == 0);
      x_offset /= 4;
      y_offset /= 2;

      /* derive the size for the LOD */
      width = u_minify(width, first_level);
      height = u_minify(height, first_level);

      first_level = 0;
      first_layer = 0;

      lod = 0;
      depth = 1;
   }
   else {
      layer_offset = 0;
      x_offset = 0;
      y_offset = 0;
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

   dw[0] = surface_type << GEN7_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN7_SURFACE_DW0_FORMAT__SHIFT |
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
   if (surface_type != GEN6_SURFTYPE_3D) {
      if (util_resource_is_array_texture(&tex->base))
         dw[0] |= GEN7_SURFACE_DW0_IS_ARRAY;
      else
         assert(depth == 1);
   }

   if (tex->valign_4)
      dw[0] |= GEN7_SURFACE_DW0_VALIGN_4;

   if (tex->halign_8)
      dw[0] |= GEN7_SURFACE_DW0_HALIGN_8;

   if (tex->array_spacing_full)
      dw[0] |= GEN7_SURFACE_DW0_ARYSPC_FULL;
   else
      dw[0] |= GEN7_SURFACE_DW0_ARYSPC_LOD0;

   if (is_rt)
      dw[0] |= GEN7_SURFACE_DW0_RENDER_CACHE_RW;

   if (surface_type == GEN6_SURFTYPE_CUBE && !is_rt)
      dw[0] |= GEN7_SURFACE_DW0_CUBE_FACE_ENABLES__MASK;

   dw[1] = layer_offset;

   dw[2] = SET_FIELD(height - 1, GEN7_SURFACE_DW2_HEIGHT) |
           SET_FIELD(width - 1, GEN7_SURFACE_DW2_WIDTH);

   dw[3] = SET_FIELD(depth - 1, GEN7_SURFACE_DW3_DEPTH) |
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
      dw[4] |= GEN7_SURFACE_DW4_MSFMT_DEPTH_STENCIL;
   }
   else {
      dw[4] |= GEN7_SURFACE_DW4_MSFMT_MSS;
   }

   if (tex->base.nr_samples > 4)
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_8;
   else if (tex->base.nr_samples > 2)
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_4;
   else
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_1;

   dw[5] = x_offset << GEN7_SURFACE_DW5_X_OFFSET__SHIFT |
           y_offset << GEN7_SURFACE_DW5_Y_OFFSET__SHIFT |
           SET_FIELD(first_level, GEN7_SURFACE_DW5_MIN_LOD) |
           lod;

   dw[6] = 0;
   dw[7] = 0;

   if (dev->gen >= ILO_GEN(7.5)) {
      dw[7] |= SET_FIELD(GEN75_SCS_RED,   GEN75_SURFACE_DW7_SCS_R) |
               SET_FIELD(GEN75_SCS_GREEN, GEN75_SURFACE_DW7_SCS_G) |
               SET_FIELD(GEN75_SCS_BLUE,  GEN75_SURFACE_DW7_SCS_B) |
               SET_FIELD(GEN75_SCS_ALPHA, GEN75_SURFACE_DW7_SCS_A);
   }

   /* do not increment reference count */
   surf->bo = tex->bo;
}
