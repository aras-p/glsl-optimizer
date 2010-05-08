/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "draw/draw_context.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"

#include "tgsi/tgsi_parse.h"

#include "pipe/p_config.h"

#include "r300_context.h"
#include "r300_emit.h"
#include "r300_reg.h"
#include "r300_screen.h"
#include "r300_screen_buffer.h"
#include "r300_state.h"
#include "r300_state_inlines.h"
#include "r300_fs.h"
#include "r300_texture.h"
#include "r300_vs.h"
#include "r300_winsys.h"

/* r300_state: Functions used to intialize state context by translating
 * Gallium state objects into semi-native r300 state objects. */

#define UPDATE_STATE(cso, atom) \
    if (cso != atom.state) { \
        atom.state = cso;    \
        atom.dirty = TRUE;   \
    }

static boolean blend_discard_if_src_alpha_0(unsigned srcRGB, unsigned srcA,
                                            unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_ALPHA == 0, and the following state is set, the colorbuffer
     * will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_SRC_COLOR ||
            srcA == PIPE_BLENDFACTOR_SRC_ALPHA ||
            srcA == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
            srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            dstA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            dstA == PIPE_BLENDFACTOR_ONE);
}

static boolean blend_discard_if_src_alpha_1(unsigned srcRGB, unsigned srcA,
                                            unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_ALPHA == 1, and the following state is set, the colorbuffer
     * will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            srcA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
            dstA == PIPE_BLENDFACTOR_SRC_ALPHA ||
            dstA == PIPE_BLENDFACTOR_ONE);
}

static boolean blend_discard_if_src_color_0(unsigned srcRGB, unsigned srcA,
                                            unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_COLOR == (0,0,0), and the following state is set, the colorbuffer
     * will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_ONE);
}

static boolean blend_discard_if_src_color_1(unsigned srcRGB, unsigned srcA,
                                            unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_COLOR == (1,1,1), and the following state is set, the colorbuffer
     * will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_ONE);
}

static boolean blend_discard_if_src_alpha_color_0(unsigned srcRGB, unsigned srcA,
                                                  unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_ALPHA_COLOR == (0,0,0,0), and the following state is set,
     * the colorbuffer will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_SRC_COLOR ||
            srcA == PIPE_BLENDFACTOR_SRC_ALPHA ||
            srcA == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
            srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            dstA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            dstA == PIPE_BLENDFACTOR_ONE);
}

static boolean blend_discard_if_src_alpha_color_1(unsigned srcRGB, unsigned srcA,
                                                  unsigned dstRGB, unsigned dstA)
{
    /* If the blend equation is ADD or REVERSE_SUBTRACT,
     * SRC_ALPHA_COLOR == (1,1,1,1), and the following state is set,
     * the colorbuffer will not be changed.
     * Notice that the dst factors are the src factors inverted. */
    return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_ZERO) &&
           (srcA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            srcA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            srcA == PIPE_BLENDFACTOR_ZERO) &&
           (dstRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
            dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            dstRGB == PIPE_BLENDFACTOR_ONE) &&
           (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
            dstA == PIPE_BLENDFACTOR_SRC_ALPHA ||
            dstA == PIPE_BLENDFACTOR_ONE);
}

static unsigned bgra_cmask(unsigned mask)
{
    /* Gallium uses RGBA color ordering while R300 expects BGRA. */

    return ((mask & PIPE_MASK_R) << 2) |
           ((mask & PIPE_MASK_B) >> 2) |
           (mask & (PIPE_MASK_G | PIPE_MASK_A));
}

/* Create a new blend state based on the CSO blend state.
 *
 * This encompasses alpha blending, logic/raster ops, and blend dithering. */
static void* r300_create_blend_state(struct pipe_context* pipe,
                                     const struct pipe_blend_state* state)
{
    struct r300_screen* r300screen = r300_screen(pipe->screen);
    struct r300_blend_state* blend = CALLOC_STRUCT(r300_blend_state);

    if (state->rt[0].blend_enable)
    {
        unsigned eqRGB = state->rt[0].rgb_func;
        unsigned srcRGB = state->rt[0].rgb_src_factor;
        unsigned dstRGB = state->rt[0].rgb_dst_factor;

        unsigned eqA = state->rt[0].alpha_func;
        unsigned srcA = state->rt[0].alpha_src_factor;
        unsigned dstA = state->rt[0].alpha_dst_factor;

        /* despite the name, ALPHA_BLEND_ENABLE has nothing to do with alpha,
         * this is just the crappy D3D naming */
        blend->blend_control = R300_ALPHA_BLEND_ENABLE |
            r300_translate_blend_function(eqRGB) |
            ( r300_translate_blend_factor(srcRGB) << R300_SRC_BLEND_SHIFT) |
            ( r300_translate_blend_factor(dstRGB) << R300_DST_BLEND_SHIFT);

        /* Optimization: some operations do not require the destination color.
         *
         * When SRC_ALPHA_SATURATE is used, colorbuffer reads must be enabled,
         * otherwise blending gives incorrect results. It seems to be
         * a hardware bug. */
        if (eqRGB == PIPE_BLEND_MIN || eqA == PIPE_BLEND_MIN ||
            eqRGB == PIPE_BLEND_MAX || eqA == PIPE_BLEND_MAX ||
            dstRGB != PIPE_BLENDFACTOR_ZERO ||
            dstA != PIPE_BLENDFACTOR_ZERO ||
            srcRGB == PIPE_BLENDFACTOR_DST_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_DST_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_INV_DST_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_INV_DST_ALPHA ||
            srcA == PIPE_BLENDFACTOR_DST_COLOR ||
            srcA == PIPE_BLENDFACTOR_DST_ALPHA ||
            srcA == PIPE_BLENDFACTOR_INV_DST_COLOR ||
            srcA == PIPE_BLENDFACTOR_INV_DST_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE) {
            /* Enable reading from the colorbuffer. */
            blend->blend_control |= R300_READ_ENABLE;

            if (r300screen->caps.is_r500) {
                /* Optimization: Depending on incoming pixels, we can
                 * conditionally disable the reading in hardware... */
                if (eqRGB != PIPE_BLEND_MIN && eqA != PIPE_BLEND_MIN &&
                    eqRGB != PIPE_BLEND_MAX && eqA != PIPE_BLEND_MAX) {
                    /* Disable reading if SRC_ALPHA == 0. */
                    if ((dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
                         dstRGB == PIPE_BLENDFACTOR_ZERO) &&
                        (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
                         dstA == PIPE_BLENDFACTOR_SRC_ALPHA ||
                         dstA == PIPE_BLENDFACTOR_ZERO)) {
                         blend->blend_control |= R500_SRC_ALPHA_0_NO_READ;
                    }

                    /* Disable reading if SRC_ALPHA == 1. */
                    if ((dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
                         dstRGB == PIPE_BLENDFACTOR_ZERO) &&
                        (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
                         dstA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
                         dstA == PIPE_BLENDFACTOR_ZERO)) {
                         blend->blend_control |= R500_SRC_ALPHA_1_NO_READ;
                    }
                }
            }
        }

        /* Optimization: discard pixels which don't change the colorbuffer.
         *
         * The code below is non-trivial and some math is involved.
         *
         * Discarding pixels must be disabled when FP16 AA is enabled.
         * This is a hardware bug. Also, this implementation wouldn't work
         * with FP blending enabled and equation clamping disabled.
         *
         * Equations other than ADD are rarely used and therefore won't be
         * optimized. */
        if ((eqRGB == PIPE_BLEND_ADD || eqRGB == PIPE_BLEND_REVERSE_SUBTRACT) &&
            (eqA == PIPE_BLEND_ADD || eqA == PIPE_BLEND_REVERSE_SUBTRACT)) {
            /* ADD: X+Y
             * REVERSE_SUBTRACT: Y-X
             *
             * The idea is:
             * If X = src*srcFactor = 0 and Y = dst*dstFactor = 1,
             * then CB will not be changed.
             *
             * Given the srcFactor and dstFactor variables, we can derive
             * what src and dst should be equal to and discard appropriate
             * pixels.
             */
            if (blend_discard_if_src_alpha_0(srcRGB, srcA, dstRGB, dstA)) {
                blend->blend_control |= R300_DISCARD_SRC_PIXELS_SRC_ALPHA_0;
            } else if (blend_discard_if_src_alpha_1(srcRGB, srcA,
                                                    dstRGB, dstA)) {
                blend->blend_control |= R300_DISCARD_SRC_PIXELS_SRC_ALPHA_1;
            } else if (blend_discard_if_src_color_0(srcRGB, srcA,
                                                    dstRGB, dstA)) {
                blend->blend_control |= R300_DISCARD_SRC_PIXELS_SRC_COLOR_0;
            } else if (blend_discard_if_src_color_1(srcRGB, srcA,
                                                    dstRGB, dstA)) {
                blend->blend_control |= R300_DISCARD_SRC_PIXELS_SRC_COLOR_1;
            } else if (blend_discard_if_src_alpha_color_0(srcRGB, srcA,
                                                          dstRGB, dstA)) {
                blend->blend_control |=
                    R300_DISCARD_SRC_PIXELS_SRC_ALPHA_COLOR_0;
            } else if (blend_discard_if_src_alpha_color_1(srcRGB, srcA,
                                                          dstRGB, dstA)) {
                blend->blend_control |=
                    R300_DISCARD_SRC_PIXELS_SRC_ALPHA_COLOR_1;
            }
        }

        /* separate alpha */
        if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
            blend->blend_control |= R300_SEPARATE_ALPHA_ENABLE;
            blend->alpha_blend_control =
                r300_translate_blend_function(eqA) |
                (r300_translate_blend_factor(srcA) << R300_SRC_BLEND_SHIFT) |
                (r300_translate_blend_factor(dstA) << R300_DST_BLEND_SHIFT);
        }
    }

    /* PIPE_LOGICOP_* don't need to be translated, fortunately. */
    if (state->logicop_enable) {
        blend->rop = R300_RB3D_ROPCNTL_ROP_ENABLE |
                (state->logicop_func) << R300_RB3D_ROPCNTL_ROP_SHIFT;
    }

    /* Color channel masks for all MRTs. */
    blend->color_channel_mask = bgra_cmask(state->rt[0].colormask);
    if (r300screen->caps.is_r500 && state->independent_blend_enable) {
        if (state->rt[1].blend_enable) {
            blend->color_channel_mask |= bgra_cmask(state->rt[1].colormask) << 4;
        }
        if (state->rt[2].blend_enable) {
            blend->color_channel_mask |= bgra_cmask(state->rt[2].colormask) << 8;
        }
        if (state->rt[3].blend_enable) {
            blend->color_channel_mask |= bgra_cmask(state->rt[3].colormask) << 12;
        }
    }

    /* Neither fglrx nor classic r300 ever set this, regardless of dithering
     * state. Since it's an optional implementation detail, we can leave it
     * out and never dither.
     *
     * This could be revisited if we ever get quality or conformance hints.
     *
    if (state->dither) {
        blend->dither = R300_RB3D_DITHER_CTL_DITHER_MODE_LUT |
                        R300_RB3D_DITHER_CTL_ALPHA_DITHER_MODE_LUT;
    }
    */

    return (void*)blend;
}

/* Bind blend state. */
static void r300_bind_blend_state(struct pipe_context* pipe,
                                  void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    UPDATE_STATE(state, r300->blend_state);
}

/* Free blend state. */
static void r300_delete_blend_state(struct pipe_context* pipe,
                                    void* state)
{
    FREE(state);
}

/* Convert float to 10bit integer */
static unsigned float_to_fixed10(float f)
{
    return CLAMP((unsigned)(f * 1023.9f), 0, 1023);
}

/* Set blend color.
 * Setup both R300 and R500 registers, figure out later which one to write. */
static void r300_set_blend_color(struct pipe_context* pipe,
                                 const struct pipe_blend_color* color)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_blend_color_state* state =
        (struct r300_blend_color_state*)r300->blend_color_state.state;
    union util_color uc;

    util_pack_color(color->color, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
    state->blend_color = uc.ui;

    /* XXX if FP16 blending is enabled, we should use the FP16 format */
    state->blend_color_red_alpha =
        float_to_fixed10(color->color[0]) |
        (float_to_fixed10(color->color[3]) << 16);
    state->blend_color_green_blue =
        float_to_fixed10(color->color[2]) |
        (float_to_fixed10(color->color[1]) << 16);

    r300->blend_color_state.size = r300->screen->caps.is_r500 ? 3 : 2;
    r300->blend_color_state.dirty = TRUE;
}

static void r300_set_clip_state(struct pipe_context* pipe,
                                const struct pipe_clip_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->clip = *state;

    if (r300->screen->caps.has_tcl) {
        memcpy(r300->clip_state.state, state, sizeof(struct pipe_clip_state));
        r300->clip_state.size = 29;
    } else {
        draw_flush(r300->draw);
        draw_set_clip_state(r300->draw, state);
        r300->clip_state.size = 2;
    }

    r300->clip_state.dirty = TRUE;
}

/* Create a new depth, stencil, and alpha state based on the CSO dsa state.
 *
 * This contains the depth buffer, stencil buffer, alpha test, and such.
 * On the Radeon, depth and stencil buffer setup are intertwined, which is
 * the reason for some of the strange-looking assignments across registers. */
static void*
        r300_create_dsa_state(struct pipe_context* pipe,
                              const struct pipe_depth_stencil_alpha_state* state)
{
    struct r300_capabilities *caps = &r300_screen(pipe->screen)->caps;
    struct r300_dsa_state* dsa = CALLOC_STRUCT(r300_dsa_state);

    /* Depth test setup. */
    if (state->depth.enabled) {
        dsa->z_buffer_control |= R300_Z_ENABLE;

        if (state->depth.writemask) {
            dsa->z_buffer_control |= R300_Z_WRITE_ENABLE;
        }

        dsa->z_stencil_control |=
            (r300_translate_depth_stencil_function(state->depth.func) <<
                R300_Z_FUNC_SHIFT);
    }

    /* Stencil buffer setup. */
    if (state->stencil[0].enabled) {
        dsa->z_buffer_control |= R300_STENCIL_ENABLE;
        dsa->z_stencil_control |=
            (r300_translate_depth_stencil_function(state->stencil[0].func) <<
                R300_S_FRONT_FUNC_SHIFT) |
            (r300_translate_stencil_op(state->stencil[0].fail_op) <<
                R300_S_FRONT_SFAIL_OP_SHIFT) |
            (r300_translate_stencil_op(state->stencil[0].zpass_op) <<
                R300_S_FRONT_ZPASS_OP_SHIFT) |
            (r300_translate_stencil_op(state->stencil[0].zfail_op) <<
                R300_S_FRONT_ZFAIL_OP_SHIFT);

        dsa->stencil_ref_mask =
                (state->stencil[0].valuemask << R300_STENCILMASK_SHIFT) |
                (state->stencil[0].writemask << R300_STENCILWRITEMASK_SHIFT);

        if (state->stencil[1].enabled) {
            dsa->two_sided = TRUE;

            dsa->z_buffer_control |= R300_STENCIL_FRONT_BACK;
            dsa->z_stencil_control |=
            (r300_translate_depth_stencil_function(state->stencil[1].func) <<
                R300_S_BACK_FUNC_SHIFT) |
            (r300_translate_stencil_op(state->stencil[1].fail_op) <<
                R300_S_BACK_SFAIL_OP_SHIFT) |
            (r300_translate_stencil_op(state->stencil[1].zpass_op) <<
                R300_S_BACK_ZPASS_OP_SHIFT) |
            (r300_translate_stencil_op(state->stencil[1].zfail_op) <<
                R300_S_BACK_ZFAIL_OP_SHIFT);

            dsa->stencil_ref_bf =
                (state->stencil[1].valuemask << R300_STENCILMASK_SHIFT) |
                (state->stencil[1].writemask << R300_STENCILWRITEMASK_SHIFT);

            if (caps->is_r500) {
                dsa->z_buffer_control |= R500_STENCIL_REFMASK_FRONT_BACK;
            } else {
                dsa->stencil_ref_bf_fallback =
                  (state->stencil[0].valuemask != state->stencil[1].valuemask ||
                   state->stencil[0].writemask != state->stencil[1].writemask);
            }
        }
    }

    /* Alpha test setup. */
    if (state->alpha.enabled) {
        dsa->alpha_function =
            r300_translate_alpha_function(state->alpha.func) |
            R300_FG_ALPHA_FUNC_ENABLE;

        /* We could use 10bit alpha ref but who needs that? */
        dsa->alpha_function |= float_to_ubyte(state->alpha.ref_value);

        if (caps->is_r500)
            dsa->alpha_function |= R500_FG_ALPHA_FUNC_8BIT;
    }

    return (void*)dsa;
}

static void r300_update_stencil_ref_fallback_status(struct r300_context *r300)
{
    struct r300_dsa_state *dsa = (struct r300_dsa_state*)r300->dsa_state.state;

    if (r300->screen->caps.is_r500) {
        return;
    }

    r300->stencil_ref_bf_fallback =
        dsa->stencil_ref_bf_fallback ||
        (dsa->two_sided &&
         r300->stencil_ref.ref_value[0] != r300->stencil_ref.ref_value[1]);
}

/* Bind DSA state. */
static void r300_bind_dsa_state(struct pipe_context* pipe,
                                void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    if (!state) {
        return;
    }

    UPDATE_STATE(state, r300->dsa_state);

    r300_update_stencil_ref_fallback_status(r300);
}

/* Free DSA state. */
static void r300_delete_dsa_state(struct pipe_context* pipe,
                                  void* state)
{
    FREE(state);
}

static void r300_set_stencil_ref(struct pipe_context* pipe,
                                 const struct pipe_stencil_ref* sr)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->stencil_ref = *sr;
    r300->dsa_state.dirty = TRUE;

    r300_update_stencil_ref_fallback_status(r300);
}

/* This switcheroo is needed just because of goddamned MACRO_SWITCH. */
static void r300_fb_set_tiling_flags(struct r300_context *r300,
                               const struct pipe_framebuffer_state *old_state,
                               const struct pipe_framebuffer_state *new_state)
{
    struct r300_texture *tex;
    unsigned i, level;

    /* Set tiling flags for new surfaces. */
    for (i = 0; i < new_state->nr_cbufs; i++) {
        tex = r300_texture(new_state->cbufs[i]->texture);
        level = new_state->cbufs[i]->level;

        r300->rws->buffer_set_tiling(r300->rws, tex->buffer,
                                        tex->pitch[0],
                                        tex->microtile,
                                        tex->mip_macrotile[level]);
    }
    if (new_state->zsbuf) {
        tex = r300_texture(new_state->zsbuf->texture);
        level = new_state->zsbuf->level;

        r300->rws->buffer_set_tiling(r300->rws, tex->buffer,
                                        tex->pitch[0],
                                        tex->microtile,
                                        tex->mip_macrotile[level]);
    }
}

static void
    r300_set_framebuffer_state(struct pipe_context* pipe,
                               const struct pipe_framebuffer_state* state)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_framebuffer_state *old_state = r300->fb_state.state;
    unsigned max_width, max_height;
    uint32_t zbuffer_bpp = 0;

    if (state->nr_cbufs > 4) {
        fprintf(stderr, "r300: Implementation error: Too many MRTs in %s, "
            "refusing to bind framebuffer state!\n", __FUNCTION__);
        return;
    }

    if (r300->screen->caps.is_r500) {
        max_width = max_height = 4096;
    } else if (r300->screen->caps.is_r400) {
        max_width = max_height = 4021;
    } else {
        max_width = max_height = 2560;
    }

    if (state->width > max_width || state->height > max_height) {
        fprintf(stderr, "r300: Implementation error: Render targets are too "
        "big in %s, refusing to bind framebuffer state!\n", __FUNCTION__);
        return;
    }

    if (r300->draw) {
        draw_flush(r300->draw);
    }

    r300->fb_state.dirty = TRUE;

    /* If nr_cbufs is changed from zero to non-zero or vice versa... */
    if (!!old_state->nr_cbufs != !!state->nr_cbufs) {
        r300->blend_state.dirty = TRUE;
    }
    /* If zsbuf is set from NULL to non-NULL or vice versa.. */
    if (!!old_state->zsbuf != !!state->zsbuf) {
        r300->dsa_state.dirty = TRUE;
    }

    /* The tiling flags are dependent on the surface miplevel, unfortunately. */
    r300_fb_set_tiling_flags(r300, r300->fb_state.state, state);

    memcpy(r300->fb_state.state, state, sizeof(struct pipe_framebuffer_state));

    r300->fb_state.size = (10 * state->nr_cbufs) + (2 * (4 - state->nr_cbufs)) +
                          (state->zsbuf ? 10 : 0) + 9;

    /* Polygon offset depends on the zbuffer bit depth. */
    if (state->zsbuf && r300->polygon_offset_enabled) {
        switch (util_format_get_blocksize(state->zsbuf->texture->format)) {
            case 2:
                zbuffer_bpp = 16;
                break;
            case 4:
                zbuffer_bpp = 24;
                break;
        }

        if (r300->zbuffer_bpp != zbuffer_bpp) {
            r300->zbuffer_bpp = zbuffer_bpp;
            r300->rs_state.dirty = TRUE;
        }
    }
}

/* Create fragment shader state. */
static void* r300_create_fs_state(struct pipe_context* pipe,
                                  const struct pipe_shader_state* shader)
{
    struct r300_fragment_shader* fs = NULL;

    fs = (struct r300_fragment_shader*)CALLOC_STRUCT(r300_fragment_shader);

    /* Copy state directly into shader. */
    fs->state = *shader;
    fs->state.tokens = tgsi_dup_tokens(shader->tokens);

    return (void*)fs;
}

void r300_mark_fs_code_dirty(struct r300_context *r300)
{
    struct r300_fragment_shader* fs = r300_fs(r300);

    r300->fs.dirty = TRUE;
    r300->fs_rc_constant_state.dirty = TRUE;
    r300->fs_constants.dirty = TRUE;

    if (r300->screen->caps.is_r500) {
        r300->fs.size = r500_get_fs_atom_size(r300);
        r300->fs_rc_constant_state.size = fs->shader->rc_state_count * 7;
        r300->fs_constants.size = fs->shader->externals_count * 4 + 3;
    } else {
        r300->fs.size = r300_get_fs_atom_size(r300);
        r300->fs_rc_constant_state.size = fs->shader->rc_state_count * 5;
        r300->fs_constants.size = fs->shader->externals_count * 4 + 1;
    }
}

/* Bind fragment shader state. */
static void r300_bind_fs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_fragment_shader* fs = (struct r300_fragment_shader*)shader;

    if (fs == NULL) {
        r300->fs.state = NULL;
        return;
    }

    r300->fs.state = fs;
    r300_pick_fragment_shader(r300);
    r300_mark_fs_code_dirty(r300);

    r300->rs_block_state.dirty = TRUE; /* Will be updated before the emission. */
}

/* Delete fragment shader state. */
static void r300_delete_fs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_fragment_shader* fs = (struct r300_fragment_shader*)shader;
    struct r300_fragment_shader_code *tmp, *ptr = fs->first;

    while (ptr) {
        tmp = ptr;
        ptr = ptr->next;
        rc_constants_destroy(&tmp->code.constants);
        FREE(tmp);
    }
    FREE((void*)fs->state.tokens);
    FREE(shader);
}

static void r300_set_polygon_stipple(struct pipe_context* pipe,
                                     const struct pipe_poly_stipple* state)
{
    /* XXX no idea how to set this up, but not terribly important */
}

/* Create a new rasterizer state based on the CSO rasterizer state.
 *
 * This is a very large chunk of state, and covers most of the graphics
 * backend (GB), geometry assembly (GA), and setup unit (SU) blocks.
 *
 * In a not entirely unironic sidenote, this state has nearly nothing to do
 * with the actual block on the Radeon called the rasterizer (RS). */
static void* r300_create_rs_state(struct pipe_context* pipe,
                                  const struct pipe_rasterizer_state* state)
{
    struct r300_rs_state* rs = CALLOC_STRUCT(r300_rs_state);
    int i;
    float psiz;

    /* Copy rasterizer state for Draw. */
    rs->rs = *state;

#ifdef PIPE_ARCH_LITTLE_ENDIAN
    rs->vap_control_status = R300_VC_NO_SWAP;
#else
    rs->vap_control_status = R300_VC_32BIT_SWAP;
#endif

    /* If no TCL engine is present, turn off the HW TCL. */
    if (!r300_screen(pipe->screen)->caps.has_tcl) {
        rs->vap_control_status |= R300_VAP_TCL_BYPASS;
    }

    /* Point size width and height. */
    rs->point_size =
        pack_float_16_6x(state->point_size) |
        (pack_float_16_6x(state->point_size) << R300_POINTSIZE_X_SHIFT);

    /* Point size clamping. */
    if (state->point_size_per_vertex) {
        /* Per-vertex point size.
         * Clamp to [0, max FB size] */
        psiz = pipe->screen->get_paramf(pipe->screen,
                                        PIPE_CAP_MAX_POINT_WIDTH);
        rs->point_minmax =
            pack_float_16_6x(psiz) << R300_GA_POINT_MINMAX_MAX_SHIFT;
    } else {
        /* We cannot disable the point-size vertex output,
         * so clamp it. */
        psiz = state->point_size;
        rs->point_minmax =
            (pack_float_16_6x(psiz) << R300_GA_POINT_MINMAX_MIN_SHIFT) |
            (pack_float_16_6x(psiz) << R300_GA_POINT_MINMAX_MAX_SHIFT);
    }

    /* Line control. */
    rs->line_control = pack_float_16_6x(state->line_width) |
        R300_GA_LINE_CNTL_END_TYPE_COMP;

    /* Enable polygon mode */
    if (state->fill_cw != PIPE_POLYGON_MODE_FILL ||
        state->fill_ccw != PIPE_POLYGON_MODE_FILL) {
        rs->polygon_mode = R300_GA_POLY_MODE_DUAL;
    }

    /* Radeons don't think in "CW/CCW", they think in "front/back". */
    if (state->front_winding == PIPE_WINDING_CW) {
        rs->cull_mode = R300_FRONT_FACE_CW;

        /* Polygon offset */
        if (state->offset_cw) {
            rs->polygon_offset_enable |= R300_FRONT_ENABLE;
        }
        if (state->offset_ccw) {
            rs->polygon_offset_enable |= R300_BACK_ENABLE;
        }

        /* Polygon mode */
        if (rs->polygon_mode) {
            rs->polygon_mode |=
                r300_translate_polygon_mode_front(state->fill_cw);
            rs->polygon_mode |=
                r300_translate_polygon_mode_back(state->fill_ccw);
        }
    } else {
        rs->cull_mode = R300_FRONT_FACE_CCW;

        /* Polygon offset */
        if (state->offset_ccw) {
            rs->polygon_offset_enable |= R300_FRONT_ENABLE;
        }
        if (state->offset_cw) {
            rs->polygon_offset_enable |= R300_BACK_ENABLE;
        }

        /* Polygon mode */
        if (rs->polygon_mode) {
            rs->polygon_mode |=
                r300_translate_polygon_mode_front(state->fill_ccw);
            rs->polygon_mode |=
                r300_translate_polygon_mode_back(state->fill_cw);
        }
    }
    if (state->front_winding & state->cull_mode) {
        rs->cull_mode |= R300_CULL_FRONT;
    }
    if (~(state->front_winding) & state->cull_mode) {
        rs->cull_mode |= R300_CULL_BACK;
    }

    if (rs->polygon_offset_enable) {
        rs->depth_offset = state->offset_units;
        rs->depth_scale = state->offset_scale;
    }

    if (state->line_stipple_enable) {
        rs->line_stipple_config =
            R300_GA_LINE_STIPPLE_CONFIG_LINE_RESET_LINE |
            (fui((float)state->line_stipple_factor) &
                R300_GA_LINE_STIPPLE_CONFIG_STIPPLE_SCALE_MASK);
        /* XXX this might need to be scaled up */
        rs->line_stipple_value = state->line_stipple_pattern;
    }

    if (state->flatshade) {
        rs->color_control = R300_SHADE_MODEL_FLAT;
    } else {
        rs->color_control = R300_SHADE_MODEL_SMOOTH;
    }

    rs->clip_rule = state->scissor ? 0xAAAA : 0xFFFF;

    /* Point sprites */
    if (state->sprite_coord_enable) {
        rs->stuffing_enable = R300_GB_POINT_STUFF_ENABLE;
	for (i = 0; i < 8; i++) {
	    if (state->sprite_coord_enable & (1 << i))
		rs->stuffing_enable |=
		    R300_GB_TEX_STR << (R300_GB_TEX0_SOURCE_SHIFT + (i*2));
	}

        rs->point_texcoord_left = 0.0f;
        rs->point_texcoord_right = 1.0f;

        switch (state->sprite_coord_mode) {
            case PIPE_SPRITE_COORD_UPPER_LEFT:
                rs->point_texcoord_top = 0.0f;
                rs->point_texcoord_bottom = 1.0f;
                break;
            case PIPE_SPRITE_COORD_LOWER_LEFT:
                rs->point_texcoord_top = 1.0f;
                rs->point_texcoord_bottom = 0.0f;
                break;
        }
    }

    return (void*)rs;
}

/* Bind rasterizer state. */
static void r300_bind_rs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_rs_state* rs = (struct r300_rs_state*)state;
    int last_sprite_coord_enable = r300->sprite_coord_enable;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_rasterizer_state(r300->draw, &rs->rs, state);
    }

    if (rs) {
        r300->polygon_offset_enabled = rs->rs.offset_cw || rs->rs.offset_ccw;
        r300->sprite_coord_enable = rs->rs.sprite_coord_enable;
    } else {
        r300->polygon_offset_enabled = FALSE;
        r300->sprite_coord_enable = 0;
    }

    UPDATE_STATE(state, r300->rs_state);
    r300->rs_state.size = 27 + (r300->polygon_offset_enabled ? 5 : 0);

    if (last_sprite_coord_enable != r300->sprite_coord_enable) {
        r300->rs_block_state.dirty = TRUE;
    }
}

/* Free rasterizer state. */
static void r300_delete_rs_state(struct pipe_context* pipe, void* state)
{
    FREE(state);
}

static void*
        r300_create_sampler_state(struct pipe_context* pipe,
                                  const struct pipe_sampler_state* state)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_sampler_state* sampler = CALLOC_STRUCT(r300_sampler_state);
    boolean is_r500 = r300->screen->caps.is_r500;
    int lod_bias;
    union util_color uc;

    sampler->state = *state;

    sampler->filter0 |=
        (r300_translate_wrap(state->wrap_s) << R300_TX_WRAP_S_SHIFT) |
        (r300_translate_wrap(state->wrap_t) << R300_TX_WRAP_T_SHIFT) |
        (r300_translate_wrap(state->wrap_r) << R300_TX_WRAP_R_SHIFT);

    sampler->filter0 |= r300_translate_tex_filters(state->min_img_filter,
                                                   state->mag_img_filter,
                                                   state->min_mip_filter,
                                                   state->max_anisotropy > 0);

    sampler->filter0 |= r300_anisotropy(state->max_anisotropy);

    /* Unfortunately, r300-r500 don't support floating-point mipmap lods. */
    /* We must pass these to the merge function to clamp them properly. */
    sampler->min_lod = MAX2((unsigned)state->min_lod, 0);
    sampler->max_lod = MAX2((unsigned)ceilf(state->max_lod), 0);

    lod_bias = CLAMP((int)(state->lod_bias * 32 + 1), -(1 << 9), (1 << 9) - 1);

    sampler->filter1 |= lod_bias << R300_LOD_BIAS_SHIFT;

    /* This is very high quality anisotropic filtering for R5xx.
     * It's good for benchmarking the performance of texturing but
     * in practice we don't want to slow down the driver because it's
     * a pretty good performance killer. Feel free to play with it. */
    if (DBG_ON(r300, DBG_ANISOHQ) && is_r500) {
        sampler->filter1 |= r500_anisotropy(state->max_anisotropy);
    }

    util_pack_color(state->border_color, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
    sampler->border_color = uc.ui;

    /* R500-specific fixups and optimizations */
    if (r300->screen->caps.is_r500) {
        sampler->filter1 |= R500_BORDER_FIX;
    }

    return (void*)sampler;
}

static void r300_bind_sampler_states(struct pipe_context* pipe,
                                     unsigned count,
                                     void** states)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_textures_state* state =
        (struct r300_textures_state*)r300->textures_state.state;
    unsigned tex_units = r300->screen->caps.num_tex_units;

    if (count > tex_units) {
        return;
    }

    memcpy(state->sampler_states, states, sizeof(void*) * count);
    state->sampler_state_count = count;

    r300->textures_state.dirty = TRUE;
}

static void r300_lacks_vertex_textures(struct pipe_context* pipe,
                                       unsigned count,
                                       void** states)
{
}

static void r300_delete_sampler_state(struct pipe_context* pipe, void* state)
{
    FREE(state);
}

static void r300_set_fragment_sampler_views(struct pipe_context* pipe,
                                            unsigned count,
                                            struct pipe_sampler_view** views)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_textures_state* state =
        (struct r300_textures_state*)r300->textures_state.state;
    struct r300_texture *texture;
    unsigned i;
    unsigned tex_units = r300->screen->caps.num_tex_units;
    boolean dirty_tex = FALSE;

    if (count > tex_units) {
        return;
    }

    for (i = 0; i < count; i++) {
        if (&state->sampler_views[i]->base != views[i]) {
            pipe_sampler_view_reference(
                    (struct pipe_sampler_view**)&state->sampler_views[i],
                    views[i]);

            if (!views[i]) {
                continue;
            }

            /* A new sampler view (= texture)... */
            dirty_tex = TRUE;

            /* Set the texrect factor in the fragment shader.
             * Needed for RECT and NPOT fallback. */
            texture = r300_texture(views[i]->texture);
            if (texture->uses_pitch) {
                r300->fs_rc_constant_state.dirty = TRUE;
            }
        }
    }

    for (i = count; i < tex_units; i++) {
        if (state->sampler_views[i]) {
            pipe_sampler_view_reference(
                    (struct pipe_sampler_view**)&state->sampler_views[i],
                    NULL);
        }
    }

    state->sampler_view_count = count;

    r300->textures_state.dirty = TRUE;

    if (dirty_tex) {
        r300->texture_cache_inval.dirty = TRUE;
    }
}

static struct pipe_sampler_view *
r300_create_sampler_view(struct pipe_context *pipe,
                         struct pipe_resource *texture,
                         const struct pipe_sampler_view *templ)
{
    struct r300_sampler_view *view = CALLOC_STRUCT(r300_sampler_view);
    struct r300_texture *tex = r300_texture(texture);

    if (view) {
        view->base = *templ;
        view->base.reference.count = 1;
        view->base.context = pipe;
        view->base.texture = NULL;
        pipe_resource_reference(&view->base.texture, texture);

        view->swizzle[0] = templ->swizzle_r;
        view->swizzle[1] = templ->swizzle_g;
        view->swizzle[2] = templ->swizzle_b;
        view->swizzle[3] = templ->swizzle_a;

        view->format = tex->tx_format;
        view->format.format1 |= r300_translate_texformat(templ->format,
                                                         view->swizzle);
        if (r300_screen(pipe->screen)->caps.is_r500) {
            view->format.format2 |= r500_tx_format_msb_bit(templ->format);
        }
    }

    return (struct pipe_sampler_view*)view;
}

static void
r300_sampler_view_destroy(struct pipe_context *pipe,
                          struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}

static void r300_set_scissor_state(struct pipe_context* pipe,
                                   const struct pipe_scissor_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    memcpy(r300->scissor_state.state, state,
        sizeof(struct pipe_scissor_state));

    r300->scissor_state.dirty = TRUE;
}

static void r300_set_viewport_state(struct pipe_context* pipe,
                                    const struct pipe_viewport_state* state)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_viewport_state* viewport =
        (struct r300_viewport_state*)r300->viewport_state.state;

    r300->viewport = *state;

    /* Do the transform in HW. */
    viewport->vte_control = R300_VTX_W0_FMT;

    if (state->scale[0] != 1.0f) {
        viewport->xscale = state->scale[0];
        viewport->vte_control |= R300_VPORT_X_SCALE_ENA;
    }
    if (state->scale[1] != 1.0f) {
        viewport->yscale = state->scale[1];
        viewport->vte_control |= R300_VPORT_Y_SCALE_ENA;
    }
    if (state->scale[2] != 1.0f) {
        viewport->zscale = state->scale[2];
        viewport->vte_control |= R300_VPORT_Z_SCALE_ENA;
    }
    if (state->translate[0] != 0.0f) {
        viewport->xoffset = state->translate[0];
        viewport->vte_control |= R300_VPORT_X_OFFSET_ENA;
    }
    if (state->translate[1] != 0.0f) {
        viewport->yoffset = state->translate[1];
        viewport->vte_control |= R300_VPORT_Y_OFFSET_ENA;
    }
    if (state->translate[2] != 0.0f) {
        viewport->zoffset = state->translate[2];
        viewport->vte_control |= R300_VPORT_Z_OFFSET_ENA;
    }

    r300->viewport_state.dirty = TRUE;
    if (r300->fs.state && r300_fs(r300)->shader->inputs.wpos != ATTR_UNUSED) {
        r300->fs_rc_constant_state.dirty = TRUE;
    }
}

static void r300_set_vertex_buffers(struct pipe_context* pipe,
                                    unsigned count,
                                    const struct pipe_vertex_buffer* buffers)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_vertex_buffer *vbo;
    unsigned i, max_index = (1 << 24) - 1;
    boolean any_user_buffer = FALSE;

    if (count == r300->vertex_buffer_count &&
        memcmp(r300->vertex_buffer, buffers,
            sizeof(struct pipe_vertex_buffer) * count) == 0) {
        return;
    }

    /* Check if the stride is aligned to the size of DWORD. */
    for (i = 0; i < count; i++) {
        if (buffers[i].buffer) {
            if (buffers[i].stride % 4 != 0) {
                // XXX Shouldn't we align the buffer?
                fprintf(stderr, "r300: set_vertex_buffers: "
                        "Unaligned buffer stride %i isn't supported.\n",
                        buffers[i].stride);
                abort();
            }
        }
    }

    for (i = 0; i < count; i++) {
        /* Why, yes, I AM casting away constness. How did you know? */
        vbo = (struct pipe_vertex_buffer*)&buffers[i];

        /* Reference our buffer. */
        pipe_resource_reference(&r300->vertex_buffer[i].buffer, vbo->buffer);

        /* Skip NULL buffers */
        if (!buffers[i].buffer) {
            continue;
        }

        if (r300_buffer_is_user_buffer(vbo->buffer)) {
            any_user_buffer = TRUE;
        }

        if (vbo->max_index == ~0) {
	    /* if no VBO stride then only one vertex value so max index is 1 */
	    /* should think about converting to VS constants like svga does */
	    if (!vbo->stride)
		vbo->max_index = 1;
 	    else
            	vbo->max_index =
               		 (vbo->buffer->width0 - vbo->buffer_offset) / vbo->stride;
        }

        max_index = MIN2(vbo->max_index, max_index);
    }

    for (; i < r300->vertex_buffer_count; i++) {
        /* Dereference any old buffers. */
        pipe_resource_reference(&r300->vertex_buffer[i].buffer, NULL);
    }

    memcpy(r300->vertex_buffer, buffers,
        sizeof(struct pipe_vertex_buffer) * count);

    r300->vertex_buffer_count = count;
    r300->vertex_buffer_max_index = max_index;
    r300->any_user_vbs = any_user_buffer;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_vertex_buffers(r300->draw, count, buffers);
    }
}

/* Update the PSC tables. */
static void r300_vertex_psc(struct r300_vertex_element_state *velems)
{
    struct r300_vertex_stream_state *vstream = &velems->vertex_stream;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i;

    if (velems->count > 16) {
        fprintf(stderr, "r300: More than 16 vertex elements are not supported,"
                " requested %i, using 16.\n", velems->count);
        velems->count = 16;
    }

    /* Vertex shaders have no semantics on their inputs,
     * so PSC should just route stuff based on the vertex elements,
     * and not on attrib information. */
    for (i = 0; i < velems->count; i++) {
        format = velems->velem[i].src_format;

        type = r300_translate_vertex_data_type(format) |
            (i << R300_DST_VEC_LOC_SHIFT);
        swizzle = r300_translate_vertex_data_swizzle(format);

        if (i & 1) {
            vstream->vap_prog_stream_cntl[i >> 1] |= type << 16;
            vstream->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 16;
        } else {
            vstream->vap_prog_stream_cntl[i >> 1] |= type;
            vstream->vap_prog_stream_cntl_ext[i >> 1] |= swizzle;
        }
    }

    /* Set the last vector in the PSC. */
    if (i) {
        i -= 1;
    }
    vstream->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));

    vstream->count = (i >> 1) + 1;
}

static void* r300_create_vertex_elements_state(struct pipe_context* pipe,
                                               unsigned count,
                                               const struct pipe_vertex_element* attribs)
{
    struct r300_vertex_element_state *velems;
    unsigned i, size;
    enum pipe_format *format;

    assert(count <= PIPE_MAX_ATTRIBS);
    velems = CALLOC_STRUCT(r300_vertex_element_state);
    if (velems != NULL) {
        velems->count = count;
        memcpy(velems->velem, attribs, sizeof(struct pipe_vertex_element) * count);

        if (r300_screen(pipe->screen)->caps.has_tcl) {
            r300_vertex_psc(velems);

            /* Check if the format is aligned to the size of DWORD.
             * We only care about the blocksizes of the formats since
             * swizzles are already set up. */
            for (i = 0; i < count; i++) {
                format = &velems->velem[i].src_format;

                /* Replace some formats with their aligned counterparts,
                 * this is OK because we check for aligned strides too. */
                switch (*format) {
                    /* Align to RGBA8. */
                    case PIPE_FORMAT_R8_UNORM:
                    case PIPE_FORMAT_R8G8_UNORM:
                    case PIPE_FORMAT_R8G8B8_UNORM:
                        *format = PIPE_FORMAT_R8G8B8A8_UNORM;
                        continue;
                    case PIPE_FORMAT_R8_SNORM:
                    case PIPE_FORMAT_R8G8_SNORM:
                    case PIPE_FORMAT_R8G8B8_SNORM:
                        *format = PIPE_FORMAT_R8G8B8A8_SNORM;
                        continue;
                    case PIPE_FORMAT_R8_USCALED:
                    case PIPE_FORMAT_R8G8_USCALED:
                    case PIPE_FORMAT_R8G8B8_USCALED:
                        *format = PIPE_FORMAT_R8G8B8A8_USCALED;
                        continue;
                    case PIPE_FORMAT_R8_SSCALED:
                    case PIPE_FORMAT_R8G8_SSCALED:
                    case PIPE_FORMAT_R8G8B8_SSCALED:
                        *format = PIPE_FORMAT_R8G8B8A8_SSCALED;
                        continue;

                    /* Align to RG16. */
                    case PIPE_FORMAT_R16_UNORM:
                        *format = PIPE_FORMAT_R16G16_UNORM;
                        continue;
                    case PIPE_FORMAT_R16_SNORM:
                        *format = PIPE_FORMAT_R16G16_SNORM;
                        continue;
                    case PIPE_FORMAT_R16_USCALED:
                        *format = PIPE_FORMAT_R16G16_USCALED;
                        continue;
                    case PIPE_FORMAT_R16_SSCALED:
                        *format = PIPE_FORMAT_R16G16_SSCALED;
                        continue;
                    case PIPE_FORMAT_R16_FLOAT:
                        *format = PIPE_FORMAT_R16G16_FLOAT;
                        continue;

                    /* Align to RGBA16. */
                    case PIPE_FORMAT_R16G16B16_UNORM:
                        *format = PIPE_FORMAT_R16G16B16A16_UNORM;
                        continue;
                    case PIPE_FORMAT_R16G16B16_SNORM:
                        *format = PIPE_FORMAT_R16G16B16A16_SNORM;
                        continue;
                    case PIPE_FORMAT_R16G16B16_USCALED:
                        *format = PIPE_FORMAT_R16G16B16A16_USCALED;
                        continue;
                    case PIPE_FORMAT_R16G16B16_SSCALED:
                        *format = PIPE_FORMAT_R16G16B16A16_SSCALED;
                        continue;
                    case PIPE_FORMAT_R16G16B16_FLOAT:
                        *format = PIPE_FORMAT_R16G16B16A16_FLOAT;
                        continue;

                    default:;
                }

                size = util_format_get_blocksize(*format);

                if (size % 4 != 0) {
                    /* XXX Shouldn't we align the format? */
                    fprintf(stderr, "r300_create_vertex_elements_state: "
                            "Unaligned format %s:%i isn't supported\n",
                            util_format_short_name(*format), size);
                    assert(0);
                    abort();
                }
            }

        }
    }
    return velems;
}

static void r300_bind_vertex_elements_state(struct pipe_context *pipe,
                                            void *state)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_vertex_element_state *velems = state;

    if (velems == NULL) {
        return;
    }

    r300->velems = velems;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_vertex_elements(r300->draw, velems->count, velems->velem);
    }

    UPDATE_STATE(&velems->vertex_stream, r300->vertex_stream_state);
    r300->vertex_stream_state.size = (1 + velems->vertex_stream.count) * 2;
}

static void r300_delete_vertex_elements_state(struct pipe_context *pipe, void *state)
{
   FREE(state);
}

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  const struct pipe_shader_state* shader)
{
    struct r300_context* r300 = r300_context(pipe);

    struct r300_vertex_shader* vs = CALLOC_STRUCT(r300_vertex_shader);

    /* Copy state directly into shader. */
    vs->state = *shader;
    vs->state.tokens = tgsi_dup_tokens(shader->tokens);

    if (r300->screen->caps.has_tcl) {
        r300_translate_vertex_shader(r300, vs, vs->state.tokens);
    } else {
        vs->draw_vs = draw_create_vertex_shader(r300->draw, shader);
    }

    return vs;
}

static void r300_bind_vs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_vertex_shader* vs = (struct r300_vertex_shader*)shader;

    if (vs == NULL) {
        r300->vs_state.state = NULL;
        return;
    }
    if (vs == r300->vs_state.state) {
        return;
    }
    r300->vs_state.state = vs;

    /* The majority of the RS block bits is dependent on the vertex shader. */
    r300->rs_block_state.dirty = TRUE; /* Will be updated before the emission. */

    if (r300->screen->caps.has_tcl) {
        r300->vs_state.dirty = TRUE;
        r300->vs_state.size =
                vs->code.length + 9 +
                (vs->immediates_count ? vs->immediates_count * 4 + 3 : 0);

        if (vs->externals_count) {
            r300->vs_constants.dirty = TRUE;
            r300->vs_constants.size = vs->externals_count * 4 + 3;
        } else {
            r300->vs_constants.size = 0;
        }

        r300->pvs_flush.dirty = TRUE;
    } else {
        draw_flush(r300->draw);
        draw_bind_vertex_shader(r300->draw,
                (struct draw_vertex_shader*)vs->draw_vs);
    }
}

static void r300_delete_vs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_vertex_shader* vs = (struct r300_vertex_shader*)shader;

    if (r300->screen->caps.has_tcl) {
        rc_constants_destroy(&vs->code.constants);
    } else {
        draw_delete_vertex_shader(r300->draw,
                (struct draw_vertex_shader*)vs->draw_vs);
    }

    FREE((void*)vs->state.tokens);
    FREE(shader);
}

static void r300_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     struct pipe_resource *buf)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_constant_buffer *cbuf;
    struct pipe_transfer *tr;
    void *mapped;
    int max_size = 0;

    switch (shader) {
        case PIPE_SHADER_VERTEX:
            cbuf = (struct r300_constant_buffer*)r300->vs_constants.state;
            max_size = 256;
            break;
        case PIPE_SHADER_FRAGMENT:
            cbuf = (struct r300_constant_buffer*)r300->fs_constants.state;
            if (r300->screen->caps.is_r500) {
                max_size = 256;
            } else {
                max_size = 32;
            }
            break;
        default:
            assert(0);
            return;
    }

    if (buf == NULL || buf->width0 == 0 ||
        (mapped = pipe_buffer_map(pipe, buf, PIPE_TRANSFER_READ, &tr)) == NULL)
    {
        cbuf->count = 0;
        return;
    }

    assert((buf->width0 % 4 * sizeof(float)) == 0);

    /* Check the size of the constant buffer. */
    /* XXX Subtract immediates and RC_STATE_* variables. */
    if (buf->width0 > (sizeof(float) * 4 * max_size)) {
        fprintf(stderr, "r300: Max size of the constant buffer is "
                      "%i*4 floats.\n", max_size);
        abort();
    }

    memcpy(cbuf->constants, mapped, buf->width0);
    cbuf->count = buf->width0 / (4 * sizeof(float));
    pipe_buffer_unmap(pipe, buf, tr);

    if (shader == PIPE_SHADER_VERTEX) {
        if (r300->screen->caps.has_tcl) {
            if (r300->vs_constants.size) {
                r300->vs_constants.dirty = TRUE;
            }
            r300->pvs_flush.dirty = TRUE;
        } else if (r300->draw) {
            draw_set_mapped_constant_buffer(r300->draw, PIPE_SHADER_VERTEX,
                0, cbuf->constants,
                buf->width0);
        }
    } else if (shader == PIPE_SHADER_FRAGMENT) {
        r300->fs_constants.dirty = TRUE;
    }
}

void r300_init_state_functions(struct r300_context* r300)
{
    r300->context.create_blend_state = r300_create_blend_state;
    r300->context.bind_blend_state = r300_bind_blend_state;
    r300->context.delete_blend_state = r300_delete_blend_state;

    r300->context.set_blend_color = r300_set_blend_color;

    r300->context.set_clip_state = r300_set_clip_state;

    r300->context.set_constant_buffer = r300_set_constant_buffer;

    r300->context.create_depth_stencil_alpha_state = r300_create_dsa_state;
    r300->context.bind_depth_stencil_alpha_state = r300_bind_dsa_state;
    r300->context.delete_depth_stencil_alpha_state = r300_delete_dsa_state;

    r300->context.set_stencil_ref = r300_set_stencil_ref;

    r300->context.set_framebuffer_state = r300_set_framebuffer_state;

    r300->context.create_fs_state = r300_create_fs_state;
    r300->context.bind_fs_state = r300_bind_fs_state;
    r300->context.delete_fs_state = r300_delete_fs_state;

    r300->context.set_polygon_stipple = r300_set_polygon_stipple;

    r300->context.create_rasterizer_state = r300_create_rs_state;
    r300->context.bind_rasterizer_state = r300_bind_rs_state;
    r300->context.delete_rasterizer_state = r300_delete_rs_state;

    r300->context.create_sampler_state = r300_create_sampler_state;
    r300->context.bind_fragment_sampler_states = r300_bind_sampler_states;
    r300->context.bind_vertex_sampler_states = r300_lacks_vertex_textures;
    r300->context.delete_sampler_state = r300_delete_sampler_state;

    r300->context.set_fragment_sampler_views = r300_set_fragment_sampler_views;
    r300->context.create_sampler_view = r300_create_sampler_view;
    r300->context.sampler_view_destroy = r300_sampler_view_destroy;

    r300->context.set_scissor_state = r300_set_scissor_state;

    r300->context.set_viewport_state = r300_set_viewport_state;

    r300->context.set_vertex_buffers = r300_set_vertex_buffers;

    r300->context.create_vertex_elements_state = r300_create_vertex_elements_state;
    r300->context.bind_vertex_elements_state = r300_bind_vertex_elements_state;
    r300->context.delete_vertex_elements_state = r300_delete_vertex_elements_state;

    r300->context.create_vs_state = r300_create_vs_state;
    r300->context.bind_vs_state = r300_bind_vs_state;
    r300->context.delete_vs_state = r300_delete_vs_state;
}
