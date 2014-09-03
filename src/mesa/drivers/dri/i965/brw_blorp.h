/*
 * Copyright © 2012 Intel Corporation
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

#pragma once

#include <stdint.h>

#include "brw_context.h"
#include "brw_reg.h"
#include "intel_mipmap_tree.h"

struct brw_context;

#ifdef __cplusplus
extern "C" {
#endif

void
brw_blorp_blit_miptrees(struct brw_context *brw,
                        struct intel_mipmap_tree *src_mt,
                        unsigned src_level, unsigned src_layer,
                        mesa_format src_format,
                        struct intel_mipmap_tree *dst_mt,
                        unsigned dst_level, unsigned dst_layer,
                        mesa_format dst_format,
                        float src_x0, float src_y0,
                        float src_x1, float src_y1,
                        float dst_x0, float dst_y0,
                        float dst_x1, float dst_y1,
                        GLenum filter, bool mirror_x, bool mirror_y);

#ifdef __cplusplus
} /* end extern "C" */

/**
 * Binding table indices used by BLORP.
 */
enum {
   BRW_BLORP_TEXTURE_BINDING_TABLE_INDEX,
   BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX,
   BRW_BLORP_NUM_BINDING_TABLE_ENTRIES
};


class brw_blorp_mip_info
{
public:
   brw_blorp_mip_info();

   void set(struct intel_mipmap_tree *mt,
            unsigned int level, unsigned int layer);

   struct intel_mipmap_tree *mt;

   /**
    * The miplevel to use.
    */
   uint32_t level;

   /**
    * The 2D layer within the miplevel. Combined, level and layer define the
    * 2D miptree slice to use.
    *
    * Note: if mt is a 2D multisample array texture on Gen7+ using
    * INTEL_MSAA_LAYOUT_UMS or INTEL_MSAA_LAYOUT_CMS, layer is the physical
    * layer holding sample 0.  So, for example, if mt->num_samples == 4, then
    * logical layer n corresponds to layer == 4*n.
    */
   uint32_t layer;

   /**
    * Width of the miplevel to be used.  For surfaces using
    * INTEL_MSAA_LAYOUT_IMS, this is measured in samples, not pixels.
    */
   uint32_t width;

   /**
    * Height of the miplevel to be used.  For surfaces using
    * INTEL_MSAA_LAYOUT_IMS, this is measured in samples, not pixels.
    */
   uint32_t height;

   /**
    * X offset within the surface to texture from (or render to).  For
    * surfaces using INTEL_MSAA_LAYOUT_IMS, this is measured in samples, not
    * pixels.
    */
   uint32_t x_offset;

   /**
    * Y offset within the surface to texture from (or render to).  For
    * surfaces using INTEL_MSAA_LAYOUT_IMS, this is measured in samples, not
    * pixels.
    */
   uint32_t y_offset;
};

class brw_blorp_surface_info : public brw_blorp_mip_info
{
public:
   brw_blorp_surface_info();

   void set(struct brw_context *brw,
            struct intel_mipmap_tree *mt,
            unsigned int level, unsigned int layer,
            mesa_format format, bool is_render_target);

   uint32_t compute_tile_offsets(uint32_t *tile_x, uint32_t *tile_y) const;

   /* Setting this flag indicates that the buffer's contents are W-tiled
    * stencil data, but the surface state should be set up for Y tiled
    * MESA_FORMAT_R_UNORM8 data (this is necessary because surface states don't
    * support W tiling).
    *
    * Since W tiles are 64 pixels wide by 64 pixels high, whereas Y tiles of
    * MESA_FORMAT_R_UNORM8 data are 128 pixels wide by 32 pixels high, the width and
    * pitch stored in the surface state will be multiplied by 2, and the
    * height will be halved.  Also, since W and Y tiles store their data in a
    * different order, the width and height will be rounded up to a multiple
    * of the tile size, to ensure that the WM program can access the full
    * width and height of the buffer.
    */
   bool map_stencil_as_y_tiled;

   unsigned num_samples;

   /**
    * Indicates if we use the standard miptree layout (ALL_LOD_IN_EACH_SLICE),
    * or if we tightly pack array slices at each LOD (ALL_SLICES_AT_EACH_LOD).
    *
    * If ALL_SLICES_AT_EACH_LOD is set, then ARYSPC_LOD0 can be used. Ignored
    * prior to Gen7.
    */
   enum miptree_array_layout array_layout;

   /**
    * Format that should be used when setting up the surface state for this
    * surface.  Should correspond to one of the BRW_SURFACEFORMAT_* enums.
    */
   uint32_t brw_surfaceformat;

   /**
    * For MSAA surfaces, MSAA layout that should be used when setting up the
    * surface state for this surface.
    */
   intel_msaa_layout msaa_layout;
};


struct brw_blorp_coord_transform_params
{
   void setup(GLfloat src0, GLfloat src1, GLfloat dst0, GLfloat dst1,
              bool mirror);

   float multiplier;
   float offset;
};


struct brw_blorp_wm_push_constants
{
   uint32_t dst_x0;
   uint32_t dst_x1;
   uint32_t dst_y0;
   uint32_t dst_y1;
   /* Top right coordinates of the rectangular grid used for scaled blitting */
   float rect_grid_x1;
   float rect_grid_y1;
   brw_blorp_coord_transform_params x_transform;
   brw_blorp_coord_transform_params y_transform;
   /* Pad out to an integral number of registers */
   uint32_t pad[6];
};

/* Every 32 bytes of push constant data constitutes one GEN register. */
const unsigned int BRW_BLORP_NUM_PUSH_CONST_REGS =
   sizeof(brw_blorp_wm_push_constants) / 32;

struct brw_blorp_prog_data
{
   unsigned int first_curbe_grf;

   /**
    * True if the WM program should be run in MSDISPMODE_PERSAMPLE with more
    * than one sample per pixel.
    */
   bool persample_msaa_dispatch;
};


enum gen7_fast_clear_op {
   GEN7_FAST_CLEAR_OP_NONE,
   GEN7_FAST_CLEAR_OP_FAST_CLEAR,
   GEN7_FAST_CLEAR_OP_RESOLVE,
};


class brw_blorp_params
{
public:
   brw_blorp_params();

   virtual uint32_t get_wm_prog(struct brw_context *brw,
                                brw_blorp_prog_data **prog_data) const = 0;

   uint32_t x0;
   uint32_t y0;
   uint32_t x1;
   uint32_t y1;
   brw_blorp_mip_info depth;
   uint32_t depth_format;
   brw_blorp_surface_info src;
   brw_blorp_surface_info dst;
   enum gen6_hiz_op hiz_op;
   enum gen7_fast_clear_op fast_clear_op;
   bool use_wm_prog;
   brw_blorp_wm_push_constants wm_push_consts;
   bool color_write_disable[4];
};


void
brw_blorp_exec(struct brw_context *brw, const brw_blorp_params *params);


/**
 * Parameters for a HiZ or depth resolve operation.
 *
 * For an overview of HiZ ops, see the following sections of the Sandy Bridge
 * PRM, Volume 1, Part 2:
 *   - 7.5.3.1 Depth Buffer Clear
 *   - 7.5.3.2 Depth Buffer Resolve
 *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
 */
class brw_hiz_op_params : public brw_blorp_params
{
public:
   brw_hiz_op_params(struct intel_mipmap_tree *mt,
                     unsigned int level, unsigned int layer,
                     gen6_hiz_op op);

   virtual uint32_t get_wm_prog(struct brw_context *brw,
                                brw_blorp_prog_data **prog_data) const;
};

struct brw_blorp_blit_prog_key
{
   /* Number of samples per pixel that have been configured in the surface
    * state for texturing from.
    */
   unsigned tex_samples;

   /* MSAA layout that has been configured in the surface state for texturing
    * from.
    */
   intel_msaa_layout tex_layout;

   /* Actual number of samples per pixel in the source image. */
   unsigned src_samples;

   /* Actual MSAA layout used by the source image. */
   intel_msaa_layout src_layout;

   /* Number of samples per pixel that have been configured in the render
    * target.
    */
   unsigned rt_samples;

   /* MSAA layout that has been configured in the render target. */
   intel_msaa_layout rt_layout;

   /* Actual number of samples per pixel in the destination image. */
   unsigned dst_samples;

   /* Actual MSAA layout used by the destination image. */
   intel_msaa_layout dst_layout;

   /* Type of the data to be read from the texture (one of
    * BRW_REGISTER_TYPE_{UD,D,F}).
    */
   enum brw_reg_type texture_data_type;

   /* True if the source image is W tiled.  If true, the surface state for the
    * source image must be configured as Y tiled, and tex_samples must be 0.
    */
   bool src_tiled_w;

   /* True if the destination image is W tiled.  If true, the surface state
    * for the render target must be configured as Y tiled, and rt_samples must
    * be 0.
    */
   bool dst_tiled_w;

   /* True if all source samples should be blended together to produce each
    * destination pixel.  If true, src_tiled_w must be false, tex_samples must
    * equal src_samples, and tex_samples must be nonzero.
    */
   bool blend;

   /* True if the rectangle being sent through the rendering pipeline might be
    * larger than the destination rectangle, so the WM program should kill any
    * pixels that are outside the destination rectangle.
    */
   bool use_kill;

   /**
    * True if the WM program should be run in MSDISPMODE_PERSAMPLE with more
    * than one sample per pixel.
    */
   bool persample_msaa_dispatch;

   /* True for scaled blitting. */
   bool blit_scaled;

   /* Scale factors between the pixel grid and the grid of samples. We're
    * using grid of samples for bilinear filetring in multisample scaled blits.
    */
   float x_scale;
   float y_scale;

   /* True for blits with filter = GL_LINEAR. */
   bool bilinear_filter;
};

class brw_blorp_blit_params : public brw_blorp_params
{
public:
   brw_blorp_blit_params(struct brw_context *brw,
                         struct intel_mipmap_tree *src_mt,
                         unsigned src_level, unsigned src_layer,
                         mesa_format src_format,
                         struct intel_mipmap_tree *dst_mt,
                         unsigned dst_level, unsigned dst_layer,
                         mesa_format dst_format,
                         GLfloat src_x0, GLfloat src_y0,
                         GLfloat src_x1, GLfloat src_y1,
                         GLfloat dst_x0, GLfloat dst_y0,
                         GLfloat dst_x1, GLfloat dst_y1,
                         GLenum filter, bool mirror_x, bool mirror_y);

   virtual uint32_t get_wm_prog(struct brw_context *brw,
                                brw_blorp_prog_data **prog_data) const;

private:
   brw_blorp_blit_prog_key wm_prog_key;
};

/**
 * \name BLORP internals
 * \{
 *
 * Used internally by gen6_blorp_exec() and gen7_blorp_exec().
 */

void
gen6_blorp_init(struct brw_context *brw);

void
gen6_blorp_emit_state_base_address(struct brw_context *brw,
                                   const brw_blorp_params *params);

void
gen6_blorp_emit_vertices(struct brw_context *brw,
                         const brw_blorp_params *params);

uint32_t
gen6_blorp_emit_blend_state(struct brw_context *brw,
                            const brw_blorp_params *params);

uint32_t
gen6_blorp_emit_cc_state(struct brw_context *brw,
                         const brw_blorp_params *params);

uint32_t
gen6_blorp_emit_wm_constants(struct brw_context *brw,
                             const brw_blorp_params *params);

void
gen6_blorp_emit_vs_disable(struct brw_context *brw,
                           const brw_blorp_params *params);

uint32_t
gen6_blorp_emit_binding_table(struct brw_context *brw,
                              const brw_blorp_params *params,
                              uint32_t wm_surf_offset_renderbuffer,
                              uint32_t wm_surf_offset_texture);

uint32_t
gen6_blorp_emit_depth_stencil_state(struct brw_context *brw,
                                    const brw_blorp_params *params);

void
gen6_blorp_emit_gs_disable(struct brw_context *brw,
                           const brw_blorp_params *params);

void
gen6_blorp_emit_clip_disable(struct brw_context *brw,
                             const brw_blorp_params *params);

void
gen6_blorp_emit_drawing_rectangle(struct brw_context *brw,
                                  const brw_blorp_params *params);

uint32_t
gen6_blorp_emit_sampler_state(struct brw_context *brw,
                              const brw_blorp_params *params);
/** \} */

#endif /* __cplusplus */
