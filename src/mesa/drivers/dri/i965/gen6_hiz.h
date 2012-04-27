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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct intel_context;
struct intel_mipmap_tree;

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
};

/**
 * \name HiZ internals
 * \{
 *
 * Used internally by gen6_hiz_exec() and gen7_hiz_exec().
 */

void
gen6_hiz_init(struct brw_context *brw);

void
gen6_hiz_emit_batch_head(struct brw_context *brw);

void
gen6_hiz_emit_vertices(struct brw_context *brw,
                       struct intel_mipmap_tree *mt,
                       unsigned int level,
                       unsigned int layer);

void
gen6_hiz_emit_depth_stencil_state(struct brw_context *brw,
                                  enum gen6_hiz_op op,
                                  uint32_t *out_offset);
/** \} */

void
gen6_resolve_hiz_slice(struct intel_context *intel,
                       struct intel_mipmap_tree *mt,
                       uint32_t level,
                       uint32_t layer);

void
gen6_resolve_depth_slice(struct intel_context *intel,
                         struct intel_mipmap_tree *mt,
                         uint32_t level,
                         uint32_t layer);

#ifdef __cplusplus
}
#endif
