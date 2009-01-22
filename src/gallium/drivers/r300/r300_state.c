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

#include "util/u_math.h"

#include "r300_context.h"
#include "r300_reg.h"

/* r300_state: Functions used to intialize state context by translating
 * Gallium state objects into semi-native r300 state objects.
 *
 * XXX break this file up into pieces if it gets too big! */

/* Pack a float into a dword. */
static uint32_t pack_float_32(float f)
{
    union {
        float f;
        uint32_t u;
    } u;

    u.f = f;
    return u.u;
}

static uint32_t translate_blend_function(int blend_func) {
    switch (blend_func) {
        case PIPE_BLEND_ADD:
            return R300_COMB_FCN_ADD_CLAMP;
        case PIPE_BLEND_SUBTRACT:
            return R300_COMB_FCN_SUB_CLAMP;
        case PIPE_BLEND_REVERSE_SUBTRACT:
            return R300_COMB_FCN_RSUB_CLAMP;
        case PIPE_BLEND_MIN:
            return R300_COMB_FCN_MIN;
        case PIPE_BLEND_MAX:
            return R300_COMB_FCN_MAX;
        default:
            /* XXX should be unreachable, handle this */
            break;
    }
    return 0;
}

/* XXX we can also offer the D3D versions of some of these... */
static uint32_t translate_blend_factor(int blend_fact) {
    switch (blend_fact) {
        case PIPE_BLENDFACTOR_ONE:
            return R300_BLEND_GL_ONE;
        case PIPE_BLENDFACTOR_SRC_COLOR:
            return R300_BLEND_GL_SRC_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA:
            return R300_BLEND_GL_SRC_ALPHA;
        case PIPE_BLENDFACTOR_DST_ALPHA:
            return R300_BLEND_GL_DST_ALPHA;
        case PIPE_BLENDFACTOR_DST_COLOR:
            return R300_BLEND_GL_DST_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            return R300_BLEND_GL_SRC_ALPHA_SATURATE;
        case PIPE_BLENDFACTOR_CONST_COLOR:
            return R300_BLEND_GL_CONST_COLOR;
        case PIPE_BLENDFACTOR_CONST_ALPHA:
            return R300_BLEND_GL_CONST_ALPHA;
        /* XXX WTF are these?
        case PIPE_BLENDFACTOR_SRC1_COLOR:
        case PIPE_BLENDFACTOR_SRC1_ALPHA: */
        case PIPE_BLENDFACTOR_ZERO:
            return R300_BLEND_GL_ZERO;
        case PIPE_BLENDFACTOR_INV_SRC_COLOR:
            return R300_BLEND_GL_ONE_MINUS_SRC_COLOR;
        case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_SRC_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_DST_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_COLOR:
            return R300_BLEND_GL_ONE_MINUS_DST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_COLOR:
            return R300_BLEND_GL_ONE_MINUS_CONST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_CONST_ALPHA;
        /* XXX see above
        case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
        case PIPE_BLENDFACTOR_INV_SRC1_ALPHA: */
        default:
            /* XXX the mythical 0x16 blend factor! */
            break;
    }
    return 0;
}

/* Create a new blend state based on the CSO blend state.
 *
 * This encompasses alpha blending, logic/raster ops, and blend dithering. */
static void* r300_create_blend_state(struct pipe_context* pipe,
                                     const struct pipe_blend_state* state)
{
    struct r300_blend_state* blend = CALLOC_STRUCT(r300_blend_state);

    if (state->blend_enable) {
        /* XXX for now, always do separate alpha...
         * is it faster to do it with one reg? */
        blend->blend_control = R300_ALPHA_BLEND_ENABLE |
                R300_SEPARATE_ALPHA_ENABLE |
                R300_READ_ENABLE |
                translate_blend_function(state->rgb_func) |
                (translate_blend_factor(state->rgb_src_factor) <<
                    R300_SRC_BLEND_SHIFT) |
                (translate_blend_factor(state->rgb_dst_factor) <<
                    R300_DST_BLEND_SHIFT);
        blend->alpha_blend_control =
                translate_blend_function(state->alpha_func) |
                (translate_blend_factor(state->alpha_src_factor) <<
                    R300_SRC_BLEND_SHIFT) |
                (translate_blend_factor(state->alpha_dst_factor) <<
                    R300_DST_BLEND_SHIFT);
    }

    /* PIPE_LOGICOP_* don't need to be translated, fortunately. */
    /* XXX are logicops still allowed if blending's disabled?
     * Does Gallium take care of it for us? */
    if (state->logicop_enable) {
        blend->rop = R300_RB3D_ROPCNTL_ROP_ENABLE |
                (state->logicop_func) << R300_RB3D_ROPCNTL_ROP_SHIFT;
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

/* Set blend color.
 * Setup both R300 and R500 registers, figure out later which one to write. */
static void r300_set_blend_color(struct pipe_context* pipe,
                                 const struct pipe_blend_color* color)
{
    struct r300_context* r300 = r300_context(pipe);
    uint32_t r, g, b, a;
    ubyte ur, ug, ub, ua;

    r = util_iround(color->color[0] * 1023.0f);
    g = util_iround(color->color[1] * 1023.0f);
    b = util_iround(color->color[2] * 1023.0f);
    a = util_iround(color->color[3] * 1023.0f);

    ur = float_to_ubyte(color->color[0]);
    ug = float_to_ubyte(color->color[1]);
    ub = float_to_ubyte(color->color[2]);
    ua = float_to_ubyte(color->color[3]);

    r300->blend_color_state->blend_color = (a << 24) | (r << 16) | (g << 8) | b;

    r300->blend_color_state->blend_color_red_alpha = ur | (ua << 16);
    r300->blend_color_state->blend_color_green_blue = ub | (ug << 16);

    r300->dirty_state |= R300_NEW_BLEND_COLOR;
}

static uint32_t translate_depth_stencil_function(int zs_func) {
    switch (zs_func) {
        case PIPE_FUNC_NEVER:
            return R300_ZS_NEVER;
        case PIPE_FUNC_LESS:
            return R300_ZS_LESS;
        case PIPE_FUNC_EQUAL:
            return R300_ZS_EQUAL;
        case PIPE_FUNC_LEQUAL:
            return R300_ZS_LEQUAL;
        case PIPE_FUNC_GREATER:
            return R300_ZS_GREATER;
        case PIPE_FUNC_NOTEQUAL:
            return R300_ZS_NOTEQUAL;
        case PIPE_FUNC_GEQUAL:
            return R300_ZS_GEQUAL;
        case PIPE_FUNC_ALWAYS:
            return R300_ZS_ALWAYS;
        default:
            /* XXX shouldn't be reachable */
            break;
    }
    return 0;
}

static uint32_t translate_stencil_op(int s_op) {
    switch (s_op) {
        case PIPE_STENCIL_OP_KEEP:
            return R300_ZS_KEEP;
        case PIPE_STENCIL_OP_ZERO:
            return R300_ZS_ZERO;
        case PIPE_STENCIL_OP_REPLACE:
            return R300_ZS_REPLACE;
        case PIPE_STENCIL_OP_INCR:
            return R300_ZS_INCR;
        case PIPE_STENCIL_OP_DECR:
            return R300_ZS_DECR;
        case PIPE_STENCIL_OP_INCR_WRAP:
            return R300_ZS_INCR_WRAP;
        case PIPE_STENCIL_OP_DECR_WRAP:
            return R300_ZS_DECR_WRAP;
        case PIPE_STENCIL_OP_INVERT:
            return R300_ZS_INVERT;
        default:
            /* XXX shouldn't be reachable */
            break;
    }
    return 0;
}

static uint32_t translate_alpha_function(int alpha_func) {
    switch (alpha_func) {
        case PIPE_FUNC_NEVER:
            return R300_FG_ALPHA_FUNC_NEVER;
        case PIPE_FUNC_LESS:
            return R300_FG_ALPHA_FUNC_LESS;
        case PIPE_FUNC_EQUAL:
            return R300_FG_ALPHA_FUNC_EQUAL;
        case PIPE_FUNC_LEQUAL:
            return R300_FG_ALPHA_FUNC_LE;
        case PIPE_FUNC_GREATER:
            return R300_FG_ALPHA_FUNC_GREATER;
        case PIPE_FUNC_NOTEQUAL:
            return R300_FG_ALPHA_FUNC_NOTEQUAL;
        case PIPE_FUNC_GEQUAL:
            return R300_FG_ALPHA_FUNC_GE;
        case PIPE_FUNC_ALWAYS:
            return R300_FG_ALPHA_FUNC_ALWAYS;
        default:
            /* XXX shouldn't be reachable */
            break;
    }
    return 0;
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
    struct r300_dsa_state* dsa = CALLOC_STRUCT(r300_dsa_state);

    /* Depth test setup. */
    if (state->depth.enabled) {
        dsa->z_buffer_control |= R300_Z_ENABLE;

        if (state->depth.writemask) {
            dsa->z_buffer_control |= R300_Z_WRITE_ENABLE;
        }

        dsa->z_stencil_control |=
                (translate_depth_stencil_function(state->depth.func) <<
                    R300_Z_FUNC_SHIFT);
    }

    /* Stencil buffer setup. */
    if (state->stencil[0].enabled) {
        dsa->z_buffer_control |= R300_STENCIL_ENABLE;
        dsa->z_stencil_control |=
                (translate_depth_stencil_function(state->stencil[0].func) <<
                    R300_S_FRONT_FUNC_SHIFT) |
                (translate_stencil_op(state->stencil[0].fail_op) <<
                    R300_S_FRONT_SFAIL_OP_SHIFT) |
                (translate_stencil_op(state->stencil[0].zpass_op) <<
                    R300_S_FRONT_ZPASS_OP_SHIFT) |
                (translate_stencil_op(state->stencil[0].zfail_op) <<
                    R300_S_FRONT_ZFAIL_OP_SHIFT);

        dsa->stencil_ref_mask = (state->stencil[0].ref_value) |
                (state->stencil[0].value_mask << R300_STENCILMASK_SHIFT) |
                (state->stencil[0].write_mask << R300_STENCILWRITEMASK_SHIFT);

        if (state->stencil[1].enabled) {
            dsa->z_buffer_control |= R300_STENCIL_FRONT_BACK;
            dsa->z_stencil_control |=
                    (translate_depth_stencil_function(state->stencil[1].func) <<
                        R300_S_BACK_FUNC_SHIFT) |
                    (translate_stencil_op(state->stencil[1].fail_op) <<
                        R300_S_BACK_SFAIL_OP_SHIFT) |
                    (translate_stencil_op(state->stencil[1].zpass_op) <<
                        R300_S_BACK_ZPASS_OP_SHIFT) |
                    (translate_stencil_op(state->stencil[1].zfail_op) <<
                        R300_S_BACK_ZFAIL_OP_SHIFT);

            dsa->stencil_ref_bf = (state->stencil[1].ref_value) |
                    (state->stencil[1].value_mask << R300_STENCILMASK_SHIFT) |
                    (state->stencil[1].write_mask << R300_STENCILWRITEMASK_SHIFT);
        }
    }

    /* Alpha test setup. */
    if (state->alpha.enabled) {
        dsa->alpha_function = translate_alpha_function(state->alpha.func) |
                R300_FG_ALPHA_FUNC_ENABLE;
        dsa->alpha_reference = CLAMP(state->alpha.ref * 1023.0f, 0, 1023);
    } else {
        dsa->z_buffer_top = R300_ZTOP_ENABLE;
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
#if 0
struct pipe_rasterizer_state
{
    unsigned flatshade:1;
    unsigned light_twoside:1;
    unsigned fill_cw:2;        /**< PIPE_POLYGON_MODE_x */
    unsigned fill_ccw:2;       /**< PIPE_POLYGON_MODE_x */
    unsigned scissor:1;
    unsigned poly_smooth:1;
    unsigned poly_stipple_enable:1;
    unsigned point_smooth:1;
    unsigned point_sprite:1;
    unsigned point_size_per_vertex:1; /**< size computed in vertex shader */
    unsigned multisample:1;         /* XXX maybe more ms state in future */
    unsigned line_smooth:1;
    unsigned line_stipple_enable:1;
    unsigned line_stipple_factor:8;  /**< [1..256] actually */
    unsigned line_stipple_pattern:16;
    unsigned line_last_pixel:1;
    unsigned bypass_clipping:1;
    unsigned bypass_vs:1; /**< Skip the vertex shader.  Note that the shader is
    still needed though, to indicate inputs/outputs */
    unsigned origin_lower_left:1;  /**< Is (0,0) the lower-left corner? */
    unsigned flatshade_first:1;   /**< take color attribute from the first vertex of a primitive */
    unsigned gl_rasterization_rules:1; /**< enable tweaks for GL rasterization?  */

    float line_width;
    float point_size;           /**< used when no per-vertex size */
    float point_size_min;        /* XXX - temporary, will go away */
    float point_size_max;        /* XXX - temporary, will go away */
    ubyte sprite_coord_mode[PIPE_MAX_SHADER_OUTPUTS]; /**< PIPE_SPRITE_COORD_ */
};
#endif
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

    /* Radeons don't think in "CW/CCW", they think in "front/back". */
    if (state->front_winding == PIPE_WINDING_CW) {
        rs->cull_mode = R300_FRONT_FACE_CW;

        if (state->offset_cw) {
            rs->polygon_offset_enable |= R300_FRONT_ENABLE;
        }
        if (state->offset_ccw) {
            rs->polygon_offset_enable |= R300_BACK_ENABLE;
        }
    } else {
        rs->cull_mode = R300_FRONT_FACE_CCW;

        if (state->offset_ccw) {
            rs->polygon_offset_enable |= R300_FRONT_ENABLE;
        }
        if (state->offset_cw) {
            rs->polygon_offset_enable |= R300_BACK_ENABLE;
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
                pack_float_32(state->offset_units);
        rs->depth_scale_front = rs->depth_scale_back =
                pack_float_32(state->offset_scale);
    }

    /* XXX this is part of HW TCL */
    /* XXX endian control */
    rs->vap_control_status = R300_VAP_TCL_BYPASS;

    return (void*)rs;
}

/* Bind rasterizer state. */
static void r300_bind_rs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->rs_state = (struct r300_rs_state*)state;
    r300->dirty_state |= R300_NEW_RS;
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
    struct r300_sampler_state* sampler = CALLOC_STRUCT(r300_sampler_state);

    return (void*)sampler;
}

static void r300_bind_sampler_states(struct pipe_context* pipe,
                                     unsigned count,
                                     void** states)
{
    struct r300_context* r300 = r300_context(pipe);
    int i = 0;

    if (count > 8) {
        return;
    }

    for (i; i < count; i++) {
        if (r300->sampler_states[i] != states[i]) {
            r300->sampler_states[i] = states[i];
            r300->dirty_state |= (R300_NEW_SAMPLER << i);
        }
    }

    r300->sampler_count = count;
}

static void r300_delete_sampler_state(struct pipe_context* pipe, void* state)
{
    FREE(state);
}

static void r300_set_scissor_state(struct pipe_context* pipe,
                                   const struct pipe_scissor_state* state)
{
    struct r300_context* r300 = r300_context(pipe);
    draw_flush(r300->draw);

    uint32_t left, top, right, bottom;

    /* So, a bit of info. The scissors are offset by R300_SCISSORS_OFFSET in
     * both directions for all values, and can only be 13 bits wide. Why?
     * We may never know. */
    left = (state->minx + R300_SCISSORS_OFFSET) & 0x1fff;
    top = (state->miny + R300_SCISSORS_OFFSET) & 0x1fff;
    right = (state->maxx + R300_SCISSORS_OFFSET) & 0x1fff;
    bottom = (state->maxy + R300_SCISSORS_OFFSET) & 0x1fff;

    r300->scissor_state->scissor_top_left = (left << R300_SCISSORS_X_SHIFT) |
            (top << R300_SCISSORS_Y_SHIFT);
    r300->scissor_state->scissor_bottom_right = (right << R300_SCISSORS_X_SHIFT) |
            (bottom << R300_SCISSORS_Y_SHIFT);

    r300->dirty_state |= R300_NEW_SCISSOR;
}

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  const struct pipe_shader_state* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    return draw_create_vertex_shader(context->draw, state);
}

static void r300_bind_vs_state(struct pipe_context* pipe, void* state) {
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_bind_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}

static void r300_delete_vs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_delete_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}

void r300_init_state_functions(struct r300_context* r300) {

    r300->context.create_blend_state = r300_create_blend_state;
    r300->context.bind_blend_state = r300_bind_blend_state;
    r300->context.delete_blend_state = r300_delete_blend_state;

    r300->context.set_blend_color = r300_set_blend_color;

    r300->context.create_depth_stencil_alpha_state = r300_create_dsa_state;
    r300->context.bind_depth_stencil_alpha_state = r300_bind_dsa_state;
    r300->context.delete_depth_stencil_alpha_state = r300_delete_dsa_state;

    r300->context.create_rasterizer_state = r300_create_rs_state;
    r300->context.bind_rasterizer_state = r300_bind_rs_state;
    r300->context.delete_rasterizer_state = r300_delete_rs_state;

    r300->context.create_sampler_state = r300_create_sampler_state;
    r300->context.bind_sampler_states = r300_bind_sampler_states;
    r300->context.delete_sampler_state = r300_delete_sampler_state;

    r300->context.set_scissor_state = r300_set_scissor_state;

    r300->context.create_vs_state = r300_create_vs_state;
    r300->context.bind_vs_state = r300_bind_vs_state;
    r300->context.delete_vs_state = r300_delete_vs_state;
}
