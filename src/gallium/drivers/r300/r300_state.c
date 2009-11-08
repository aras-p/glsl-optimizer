/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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
#include "pipe/internal/p_winsys_screen.h"

#include "r300_context.h"
#include "r300_reg.h"
#include "r300_screen.h"
#include "r300_state_inlines.h"
#include "r300_fs.h"
#include "r300_vs.h"

/* r300_state: Functions used to intialize state context by translating
 * Gallium state objects into semi-native r300 state objects. */

/* Create a new blend state based on the CSO blend state.
 *
 * This encompasses alpha blending, logic/raster ops, and blend dithering. */
static void* r300_create_blend_state(struct pipe_context* pipe,
                                     const struct pipe_blend_state* state)
{
    struct r300_blend_state* blend = CALLOC_STRUCT(r300_blend_state);

    if (state->blend_enable)
    {
        unsigned eqRGB = state->rgb_func;
        unsigned srcRGB = state->rgb_src_factor;
        unsigned dstRGB = state->rgb_dst_factor;

        unsigned eqA = state->alpha_func;
        unsigned srcA = state->alpha_src_factor;
        unsigned dstA = state->alpha_dst_factor;

        /* despite the name, ALPHA_BLEND_ENABLE has nothing to do with alpha,
         * this is just the crappy D3D naming */
        blend->blend_control = R300_ALPHA_BLEND_ENABLE |
            r300_translate_blend_function(eqRGB) |
            ( r300_translate_blend_factor(srcRGB) << R300_SRC_BLEND_SHIFT) |
            ( r300_translate_blend_factor(dstRGB) << R300_DST_BLEND_SHIFT);

        /* optimization: some operations do not require the destination color */
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
            srcA == PIPE_BLENDFACTOR_INV_DST_ALPHA)
            blend->blend_control |= R300_READ_ENABLE;

        /* XXX implement the optimization with DISCARD_SRC_PIXELS*/
        /* XXX implement the optimization with SRC_ALPHA_?_NO_READ */

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

    /* Color Channel Mask */
    if (state->colormask & PIPE_MASK_R) {
        blend->color_channel_mask |= RB3D_COLOR_CHANNEL_MASK_RED_MASK0;
    }
    if (state->colormask & PIPE_MASK_G) {
        blend->color_channel_mask |= RB3D_COLOR_CHANNEL_MASK_GREEN_MASK0;
    }
    if (state->colormask & PIPE_MASK_B) {
        blend->color_channel_mask |= RB3D_COLOR_CHANNEL_MASK_BLUE_MASK0;
    }
    if (state->colormask & PIPE_MASK_A) {
        blend->color_channel_mask |= RB3D_COLOR_CHANNEL_MASK_ALPHA_MASK0;
    }

    if (state->dither) {
        blend->dither = R300_RB3D_DITHER_CTL_DITHER_MODE_LUT |
                R300_RB3D_DITHER_CTL_ALPHA_DITHER_MODE_LUT;
    }

    return (void*)blend;
}

/* Bind blend state. */
static void r300_bind_blend_state(struct pipe_context* pipe,
                                  void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->blend_state = (struct r300_blend_state*)state;
    r300->dirty_state |= R300_NEW_BLEND;
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

    util_pack_color(color->color, PIPE_FORMAT_A8R8G8B8_UNORM,
            &r300->blend_color_state->blend_color);

    /* XXX if FP16 blending is enabled, we should use the FP16 format */
    r300->blend_color_state->blend_color_red_alpha =
        float_to_fixed10(color->color[0]) |
        (float_to_fixed10(color->color[3]) << 16);
    r300->blend_color_state->blend_color_green_blue =
        float_to_fixed10(color->color[2]) |
        (float_to_fixed10(color->color[1]) << 16);

    r300->dirty_state |= R300_NEW_BLEND_COLOR;
}

static void r300_set_clip_state(struct pipe_context* pipe,
                                const struct pipe_clip_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    if (r300_screen(pipe->screen)->caps->has_tcl) {
        r300->clip_state = *state;
        r300->dirty_state |= R300_NEW_CLIP;
    } else {
        draw_flush(r300->draw);
        draw_set_clip_state(r300->draw, state);
    }
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
    struct r300_capabilities *caps =
        r300_screen(r300_context(pipe)->context.screen)->caps;
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

        dsa->stencil_ref_mask = (state->stencil[0].ref_value) |
                (state->stencil[0].valuemask << R300_STENCILMASK_SHIFT) |
                (state->stencil[0].writemask << R300_STENCILWRITEMASK_SHIFT);

        if (state->stencil[1].enabled) {
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

            /* XXX it seems r3xx doesn't support STENCILREFMASK_BF */
            if (caps->is_r500)
            {
                dsa->z_buffer_control |= R500_STENCIL_REFMASK_FRONT_BACK;
                dsa->stencil_ref_bf = (state->stencil[1].ref_value) |
                    (state->stencil[1].valuemask <<
                    R300_STENCILMASK_SHIFT) |
                    (state->stencil[1].writemask <<
                    R300_STENCILWRITEMASK_SHIFT);
            }
        }
    }

    /* Alpha test setup. */
    if (state->alpha.enabled) {
        dsa->alpha_function =
            r300_translate_alpha_function(state->alpha.func) |
            R300_FG_ALPHA_FUNC_ENABLE;

        /* XXX figure out why emitting 10bit alpha ref causes CS to dump */
        /* always use 8bit alpha ref */
        dsa->alpha_function |= float_to_ubyte(state->alpha.ref_value);

        if (caps->is_r500)
            dsa->alpha_function |= R500_FG_ALPHA_FUNC_8BIT;
    }

    return (void*)dsa;
}

/* Bind DSA state. */
static void r300_bind_dsa_state(struct pipe_context* pipe,
                                void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->dsa_state = (struct r300_dsa_state*)state;
    r300->dirty_state |= R300_NEW_DSA;
}

/* Free DSA state. */
static void r300_delete_dsa_state(struct pipe_context* pipe,
                                  void* state)
{
    FREE(state);
}

static void r300_set_edgeflags(struct pipe_context* pipe,
                               const unsigned* bitfield)
{
    /* XXX you know it's bad when i915 has this blank too */
    /* XXX and even worse, I have no idea WTF the bitfield is */
}

static void
    r300_set_framebuffer_state(struct pipe_context* pipe,
                               const struct pipe_framebuffer_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    if (r300->draw) {
        draw_flush(r300->draw);
    }

    r300->framebuffer_state = *state;

    r300->dirty_state |= R300_NEW_FRAMEBUFFERS;
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

    tgsi_scan_shader(shader->tokens, &fs->info);

    return (void*)fs;
}

/* Bind fragment shader state. */
static void r300_bind_fs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_fragment_shader* fs = (struct r300_fragment_shader*)shader;

    if (fs == NULL) {
        r300->fs = NULL;
        return;
    } else if (!fs->translated) {
        r300_translate_fragment_shader(r300, fs);
    }

    r300->fs = fs;

    r300->dirty_state |= R300_NEW_FRAGMENT_SHADER | R300_NEW_FRAGMENT_SHADER_CONSTANTS;
}

/* Delete fragment shader state. */
static void r300_delete_fs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_fragment_shader* fs = (struct r300_fragment_shader*)shader;
    rc_constants_destroy(&fs->code.constants);
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

    /* Copy rasterizer state for Draw. */
    rs->rs = *state;

    rs->enable_vte = !state->bypass_vs_clip_and_viewport;

#ifdef PIPE_ARCH_LITTLE_ENDIAN
    rs->vap_control_status = R300_VC_NO_SWAP;
#else
    rs->vap_control_status = R300_VC_32BIT_SWAP;
#endif

    /* If bypassing TCL, or if no TCL engine is present, turn off the HW TCL.
     * Else, enable HW TCL and force Draw's TCL off. */
    if (state->bypass_vs_clip_and_viewport ||
            !r300_screen(pipe->screen)->caps->has_tcl) {
        rs->vap_control_status |= R300_VAP_TCL_BYPASS;
    } else {
        rs->rs.bypass_vs_clip_and_viewport = TRUE;
    }

    rs->point_size = pack_float_16_6x(state->point_size) |
        (pack_float_16_6x(state->point_size) << R300_POINTSIZE_X_SHIFT);

    rs->point_minmax =
        ((int)(state->point_size_min * 6.0) <<
         R300_GA_POINT_MINMAX_MIN_SHIFT) |
        ((int)(state->point_size_max * 6.0) <<
         R300_GA_POINT_MINMAX_MAX_SHIFT);

    rs->line_control = pack_float_16_6x(state->line_width) |
        R300_GA_LINE_CNTL_END_TYPE_COMP;

    /* XXX I think there is something wrong with the polygon mode,
     * XXX re-test when r300g is in a better shape */

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
        rs->depth_offset_front = rs->depth_offset_back =
            fui(state->offset_units);
        rs->depth_scale_front = rs->depth_scale_back =
            fui(state->offset_scale);
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

    if (!state->flatshade_first) {
        rs->color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
    }

    return (void*)rs;
}

/* Bind rasterizer state. */
static void r300_bind_rs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_rs_state* rs = (struct r300_rs_state*)state;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_rasterizer_state(r300->draw, &rs->rs);
    }

    r300->rs_state = rs;
    /* XXX Clean these up when we move to atom emits */
    r300->dirty_state |= R300_NEW_RASTERIZER;
    r300->dirty_state |= R300_NEW_RS_BLOCK;
    r300->dirty_state |= R300_NEW_SCISSOR;
    r300->dirty_state |= R300_NEW_VIEWPORT;
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
    int lod_bias;

    sampler->filter0 |=
        (r300_translate_wrap(state->wrap_s) << R300_TX_WRAP_S_SHIFT) |
        (r300_translate_wrap(state->wrap_t) << R300_TX_WRAP_T_SHIFT) |
        (r300_translate_wrap(state->wrap_r) << R300_TX_WRAP_R_SHIFT);

    sampler->filter0 |= r300_translate_tex_filters(state->min_img_filter,
                                                   state->mag_img_filter,
                                                   state->min_mip_filter);

    lod_bias = CLAMP((int)(state->lod_bias * 32), -(1 << 9), (1 << 9) - 1);

    sampler->filter1 |= lod_bias << R300_LOD_BIAS_SHIFT;

    sampler->filter1 |= r300_anisotropy(state->max_anisotropy);

    util_pack_color(state->border_color, PIPE_FORMAT_A8R8G8B8_UNORM,
                    &sampler->border_color);

    /* R500-specific fixups and optimizations */
    if (r300_screen(r300->context.screen)->caps->is_r500) {
        sampler->filter1 |= R500_BORDER_FIX;
    }

    return (void*)sampler;
}

static void r300_bind_sampler_states(struct pipe_context* pipe,
                                     unsigned count,
                                     void** states)
{
    struct r300_context* r300 = r300_context(pipe);
    int i;

    if (count > 8) {
        return;
    }

    for (i = 0; i < count; i++) {
        if (r300->sampler_states[i] != states[i]) {
            r300->sampler_states[i] = (struct r300_sampler_state*)states[i];
            r300->dirty_state |= (R300_NEW_SAMPLER << i);
        }
    }

    r300->sampler_count = count;
}

static void r300_delete_sampler_state(struct pipe_context* pipe, void* state)
{
    FREE(state);
}

static void r300_set_sampler_textures(struct pipe_context* pipe,
                                      unsigned count,
                                      struct pipe_texture** texture)
{
    struct r300_context* r300 = r300_context(pipe);
    int i;

    /* XXX magic num */
    if (count > 8) {
        return;
    }
    
    r300->context.flush(&r300->context, 0, NULL);

    for (i = 0; i < count; i++) {
        if (r300->textures[i] != (struct r300_texture*)texture[i]) {
            pipe_texture_reference((struct pipe_texture**)&r300->textures[i],
                texture[i]);
            r300->dirty_state |= (R300_NEW_TEXTURE << i);
        }
    }

    for (i = count; i < 8; i++) {
        if (r300->textures[i]) {
            pipe_texture_reference((struct pipe_texture**)&r300->textures[i],
                NULL);
            r300->dirty_state |= (R300_NEW_TEXTURE << i);
        }
    }

    r300->texture_count = count;
}

static void r300_set_scissor_state(struct pipe_context* pipe,
                                   const struct pipe_scissor_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    if (r300_screen(r300->context.screen)->caps->is_r500) {
        r300->scissor_state->scissor_top_left =
            (state->minx << R300_SCISSORS_X_SHIFT) |
            (state->miny << R300_SCISSORS_Y_SHIFT);
        r300->scissor_state->scissor_bottom_right =
            ((state->maxx - 1) << R300_SCISSORS_X_SHIFT) |
            ((state->maxy - 1) << R300_SCISSORS_Y_SHIFT);
    } else {
        /* Offset of 1440 in non-R500 chipsets. */
        r300->scissor_state->scissor_top_left =
            ((state->minx + 1440) << R300_SCISSORS_X_SHIFT) |
            ((state->miny + 1440) << R300_SCISSORS_Y_SHIFT);
        r300->scissor_state->scissor_bottom_right =
            (((state->maxx - 1) + 1440) << R300_SCISSORS_X_SHIFT) |
            (((state->maxy - 1) + 1440) << R300_SCISSORS_Y_SHIFT);
    }

    r300->dirty_state |= R300_NEW_SCISSOR;
}

static void r300_set_viewport_state(struct pipe_context* pipe,
                                    const struct pipe_viewport_state* state)
{
    struct r300_context* r300 = r300_context(pipe);

    /* Do the transform in HW. */
    r300->viewport_state->vte_control = R300_VTX_W0_FMT;

    if (state->scale[0] != 1.0f) {
        r300->viewport_state->xscale = state->scale[0];
        r300->viewport_state->vte_control |= R300_VPORT_X_SCALE_ENA;
    }
    if (state->scale[1] != 1.0f) {
        r300->viewport_state->yscale = state->scale[1];
        r300->viewport_state->vte_control |= R300_VPORT_Y_SCALE_ENA;
    }
    if (state->scale[2] != 1.0f) {
        r300->viewport_state->zscale = state->scale[2];
        r300->viewport_state->vte_control |= R300_VPORT_Z_SCALE_ENA;
    }
    if (state->translate[0] != 0.0f) {
        r300->viewport_state->xoffset = state->translate[0];
        r300->viewport_state->vte_control |= R300_VPORT_X_OFFSET_ENA;
    }
    if (state->translate[1] != 0.0f) {
        r300->viewport_state->yoffset = state->translate[1];
        r300->viewport_state->vte_control |= R300_VPORT_Y_OFFSET_ENA;
    }
    if (state->translate[2] != 0.0f) {
        r300->viewport_state->zoffset = state->translate[2];
        r300->viewport_state->vte_control |= R300_VPORT_Z_OFFSET_ENA;
    }

    r300->dirty_state |= R300_NEW_VIEWPORT;
}

static void r300_set_vertex_buffers(struct pipe_context* pipe,
                                    unsigned count,
                                    const struct pipe_vertex_buffer* buffers)
{
    struct r300_context* r300 = r300_context(pipe);

    memcpy(r300->vertex_buffer, buffers,
        sizeof(struct pipe_vertex_buffer) * count);
    r300->vertex_buffer_count = count;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_vertex_buffers(r300->draw, count, buffers);
    }
}

static void r300_set_vertex_elements(struct pipe_context* pipe,
                                    unsigned count,
                                    const struct pipe_vertex_element* elements)
{
    struct r300_context* r300 = r300_context(pipe);

    memcpy(r300->vertex_element,
           elements,
           sizeof(struct pipe_vertex_element) * count);
    r300->vertex_element_count = count;

    if (r300->draw) {
        draw_flush(r300->draw);
        draw_set_vertex_elements(r300->draw, count, elements);
    }
}

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  const struct pipe_shader_state* shader)
{
    struct r300_context* r300 = r300_context(pipe);

    if (r300_screen(pipe->screen)->caps->has_tcl) {
        struct r300_vertex_shader* vs = CALLOC_STRUCT(r300_vertex_shader);
        /* Copy state directly into shader. */
        vs->state = *shader;
        vs->state.tokens = tgsi_dup_tokens(shader->tokens);

        tgsi_scan_shader(shader->tokens, &vs->info);

        /* Appease Draw. */
        vs->draw = draw_create_vertex_shader(r300->draw, shader);

        return (void*)vs;
    } else {
        return draw_create_vertex_shader(r300->draw, shader);
    }
}

static void r300_bind_vs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);

    draw_flush(r300->draw);

    if (r300_screen(pipe->screen)->caps->has_tcl) {
        struct r300_vertex_shader* vs = (struct r300_vertex_shader*)shader;

        if (vs == NULL) {
            r300->vs = NULL;
            return;
        } else if (!vs->translated) {
            r300_translate_vertex_shader(r300, vs);
        }

        draw_bind_vertex_shader(r300->draw, vs->draw);
        r300->vs = vs;
        r300->dirty_state |= R300_NEW_VERTEX_SHADER | R300_NEW_VERTEX_SHADER_CONSTANTS;
    } else {
        draw_bind_vertex_shader(r300->draw,
                (struct draw_vertex_shader*)shader);
    }
}

static void r300_delete_vs_state(struct pipe_context* pipe, void* shader)
{
    struct r300_context* r300 = r300_context(pipe);

    if (r300_screen(pipe->screen)->caps->has_tcl) {
        struct r300_vertex_shader* vs = (struct r300_vertex_shader*)shader;

        rc_constants_destroy(&vs->code.constants);
        draw_delete_vertex_shader(r300->draw, vs->draw);
        FREE((void*)vs->state.tokens);
        FREE(shader);
    } else {
        draw_delete_vertex_shader(r300->draw,
                (struct draw_vertex_shader*)shader);
    }
}

static void r300_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     const struct pipe_constant_buffer *buf)
{
    struct r300_context* r300 = r300_context(pipe);
    void *mapped;

    if (buf == NULL || buf->buffer->size == 0 ||
        (mapped = pipe_buffer_map(pipe->screen, buf->buffer, PIPE_BUFFER_USAGE_CPU_READ)) == NULL)
    {
        r300->shader_constants[shader].count = 0;
        return;
    }

    assert((buf->buffer->size % 4 * sizeof(float)) == 0);
    memcpy(r300->shader_constants[shader].constants, mapped, buf->buffer->size);
    r300->shader_constants[shader].count = buf->buffer->size / (4 * sizeof(float));
    pipe_buffer_unmap(pipe->screen, buf->buffer);

    if (shader == PIPE_SHADER_VERTEX)
        r300->dirty_state |= R300_NEW_VERTEX_SHADER_CONSTANTS;
    else if (shader == PIPE_SHADER_FRAGMENT)
        r300->dirty_state |= R300_NEW_FRAGMENT_SHADER_CONSTANTS;
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

    r300->context.set_edgeflags = r300_set_edgeflags;

    r300->context.set_framebuffer_state = r300_set_framebuffer_state;

    r300->context.create_fs_state = r300_create_fs_state;
    r300->context.bind_fs_state = r300_bind_fs_state;
    r300->context.delete_fs_state = r300_delete_fs_state;

    r300->context.set_polygon_stipple = r300_set_polygon_stipple;

    r300->context.create_rasterizer_state = r300_create_rs_state;
    r300->context.bind_rasterizer_state = r300_bind_rs_state;
    r300->context.delete_rasterizer_state = r300_delete_rs_state;

    r300->context.create_sampler_state = r300_create_sampler_state;
    r300->context.bind_sampler_states = r300_bind_sampler_states;
    r300->context.delete_sampler_state = r300_delete_sampler_state;

    r300->context.set_sampler_textures = r300_set_sampler_textures;

    r300->context.set_scissor_state = r300_set_scissor_state;

    r300->context.set_viewport_state = r300_set_viewport_state;

    r300->context.set_vertex_buffers = r300_set_vertex_buffers;
    r300->context.set_vertex_elements = r300_set_vertex_elements;

    r300->context.create_vs_state = r300_create_vs_state;
    r300->context.bind_vs_state = r300_bind_vs_state;
    r300->context.delete_vs_state = r300_delete_vs_state;
}
