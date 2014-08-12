/*
 * Copyright © 2014 Broadcom
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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

#ifndef VC4_CONTEXT_H
#define VC4_CONTEXT_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_slab.h"

#define __user
#include "vc4_drm.h"
#include "vc4_bufmgr.h"
#include "vc4_resource.h"
#include "vc4_cl.h"
#include "vc4_qir.h"

#define VC4_DIRTY_BLEND         (1 <<  0)
#define VC4_DIRTY_RASTERIZER    (1 <<  1)
#define VC4_DIRTY_ZSA           (1 <<  2)
#define VC4_DIRTY_FRAGTEX       (1 <<  3)
#define VC4_DIRTY_VERTTEX       (1 <<  4)
#define VC4_DIRTY_TEXSTATE      (1 <<  5)
#define VC4_DIRTY_PROG          (1 <<  6)
#define VC4_DIRTY_BLEND_COLOR   (1 <<  7)
#define VC4_DIRTY_STENCIL_REF   (1 <<  8)
#define VC4_DIRTY_SAMPLE_MASK   (1 <<  9)
#define VC4_DIRTY_FRAMEBUFFER   (1 << 10)
#define VC4_DIRTY_STIPPLE       (1 << 11)
#define VC4_DIRTY_VIEWPORT      (1 << 12)
#define VC4_DIRTY_CONSTBUF      (1 << 13)
#define VC4_DIRTY_VTXSTATE      (1 << 14)
#define VC4_DIRTY_VTXBUF        (1 << 15)
#define VC4_DIRTY_INDEXBUF      (1 << 16)
#define VC4_DIRTY_SCISSOR       (1 << 17)

#define VC4_SHADER_DIRTY_VP     (1 << 0)
#define VC4_SHADER_DIRTY_FP     (1 << 1)

struct vc4_texture_stateobj {
        struct pipe_sampler_view *textures[PIPE_MAX_SAMPLERS];
        unsigned num_textures;
        struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
        unsigned num_samplers;
        unsigned dirty_samplers;
};

struct vc4_shader_uniform_info {
        enum quniform_contents *contents;
        uint32_t *data;
        uint32_t count;
        uint32_t num_texture_samples;
};

struct vc4_compiled_shader {
        struct vc4_bo *bo;

        struct vc4_shader_uniform_info uniforms[2];

        uint32_t coord_shader_offset;
        uint8_t num_inputs;
};

struct vc4_program_stateobj {
        struct pipe_shader_state *bind_vs, *bind_fs;
        struct vc4_compiled_shader *vs, *fs;
        uint32_t dirty;
        uint8_t num_exports;
        /* Indexed by semantic name or TGSI_SEMANTIC_COUNT + semantic index
         * for TGSI_SEMANTIC_GENERIC.  Special vs exports (position and point-
         * size) are not included in this
         */
        uint8_t export_linkage[63];
};

struct vc4_constbuf_stateobj {
        struct pipe_constant_buffer cb[PIPE_MAX_CONSTANT_BUFFERS];
        uint32_t enabled_mask;
        uint32_t dirty_mask;
};

struct vc4_vertexbuf_stateobj {
        struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
        unsigned count;
        uint32_t enabled_mask;
        uint32_t dirty_mask;
};

struct vc4_vertex_stateobj {
        struct pipe_vertex_element pipe[PIPE_MAX_ATTRIBS];
        unsigned num_elements;
};

struct vc4_context {
        struct pipe_context base;

        int fd;
        struct vc4_screen *screen;

        struct vc4_cl bcl;
        struct vc4_cl rcl;
        struct vc4_cl shader_rec;
        struct vc4_cl uniforms;
        struct vc4_cl bo_handles;
        struct vc4_cl bo_pointers;
        uint32_t shader_rec_count;

        struct vc4_bo *tile_alloc;
        struct vc4_bo *tile_state;

        struct util_slab_mempool transfer_pool;
        struct blitter_context *blitter;

        /** bitfield of VC4_DIRTY_* */
        uint32_t dirty;
        /* Bitmask of PIPE_CLEAR_* of buffers that were cleared before the
         * first rendering.
         */
        uint32_t cleared;
        /* Bitmask of PIPE_CLEAR_* of buffers that have been rendered to
         * (either clears or draws).
         */
        uint32_t resolve;
        uint32_t clear_color[2];
        uint32_t clear_depth; /**< 24-bit unorm depth */

        /**
         * Set if some drawing (triangles, blits, or just a glClear()) has
         * been done to the FBO, meaning that we need to
         * DRM_IOCTL_VC4_SUBMIT_CL.
         */
        bool needs_flush;

        /**
         * Set when needs_flush, and the queued rendering is not just composed
         * of full-buffer clears.
         */
        bool draw_call_queued;

        struct primconvert_context *primconvert;

        struct util_hash_table *fs_cache, *vs_cache;

        /** @{ Current pipeline state objects */
        struct pipe_scissor_state scissor;
        struct pipe_blend_state *blend;
        struct vc4_rasterizer_state *rasterizer;
        struct vc4_depth_stencil_alpha_state *zsa;

        struct vc4_texture_stateobj verttex, fragtex;

        struct vc4_program_stateobj prog;

        struct vc4_vertex_stateobj *vtx;

        struct pipe_blend_color blend_color;
        struct pipe_stencil_ref stencil_ref;
        unsigned sample_mask;
        struct pipe_framebuffer_state framebuffer;
        struct pipe_poly_stipple stipple;
        struct pipe_viewport_state viewport;
        struct vc4_constbuf_stateobj constbuf[PIPE_SHADER_TYPES];
        struct vc4_vertexbuf_stateobj vertexbuf;
        struct pipe_index_buffer indexbuf;
        /** @} */
};

struct vc4_rasterizer_state {
        struct pipe_rasterizer_state base;

        /* VC4_CONFIGURATION_BITS */
        uint8_t config_bits[3];

        float point_size;
};

struct vc4_depth_stencil_alpha_state {
        struct pipe_depth_stencil_alpha_state base;

        /* VC4_CONFIGURATION_BITS */
        uint8_t config_bits[3];
};

static inline struct vc4_context *
vc4_context(struct pipe_context *pcontext)
{
        return (struct vc4_context *)pcontext;
}

struct pipe_context *vc4_context_create(struct pipe_screen *pscreen,
                                        void *priv);
void vc4_draw_init(struct pipe_context *pctx);
void vc4_state_init(struct pipe_context *pctx);
void vc4_program_init(struct pipe_context *pctx);
void vc4_simulator_init(struct vc4_screen *screen);
int vc4_simulator_flush(struct vc4_context *vc4,
                        struct drm_vc4_submit_cl *args);

void vc4_write_uniforms(struct vc4_context *vc4,
                        struct vc4_compiled_shader *shader,
                        struct vc4_constbuf_stateobj *cb,
                        struct vc4_texture_stateobj *texstate,
                        int shader_index);

void vc4_flush(struct pipe_context *pctx);
void vc4_flush_for_bo(struct pipe_context *pctx, struct vc4_bo *bo);
void vc4_emit_state(struct pipe_context *pctx);
void vc4_generate_code(struct qcompile *c);
void vc4_update_compiled_shaders(struct vc4_context *vc4, uint8_t prim_mode);

#endif /* VC4_CONTEXT_H */
