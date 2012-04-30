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

#include "intel_mipmap_tree.h"

struct brw_context;


/**
 * For an overview of the HiZ operations, see the following sections of the
 * Sandy Bridge PRM, Volume 1, Part2:
 *   - 7.5.3.1 Depth Buffer Clear
 *   - 7.5.3.2 Depth Buffer Resolve
 *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
 */
enum gen6_hiz_op {
   GEN6_HIZ_OP_DEPTH_CLEAR,
   GEN6_HIZ_OP_DEPTH_RESOLVE,
   GEN6_HIZ_OP_HIZ_RESOLVE,
   GEN6_HIZ_OP_NONE,
};

class brw_blorp_mip_info
{
public:
   brw_blorp_mip_info();

   void set(struct intel_mipmap_tree *mt,
            unsigned int level, unsigned int layer);
   void get_draw_offsets(uint32_t *draw_x, uint32_t *draw_y) const;

   void get_miplevel_dims(uint32_t *width, uint32_t *height) const
   {
      *width = mt->level[level].width;
      *height = mt->level[level].height;
   }

   struct intel_mipmap_tree *mt;
   unsigned int level;
   unsigned int layer;
};

class brw_blorp_params
{
public:
   brw_blorp_params();

   void exec(struct intel_context *intel) const;

   uint32_t x0;
   uint32_t y0;
   uint32_t x1;
   uint32_t y1;
   brw_blorp_mip_info depth;
   uint32_t depth_format;
   enum gen6_hiz_op hiz_op;
};

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
gen6_blorp_compute_tile_masks(const brw_blorp_params *params,
                              uint32_t *tile_mask_x, uint32_t *tile_mask_y);

void
gen6_blorp_emit_batch_head(struct brw_context *brw,
                           const brw_blorp_params *params);

void
gen6_blorp_emit_vertices(struct brw_context *brw,
                         const brw_blorp_params *params);

void
gen6_blorp_emit_vs_disable(struct brw_context *brw,
                           const brw_blorp_params *params);

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
/** \} */
