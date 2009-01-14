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

#include "r300_context.h"
#include "r300_state.h"

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
static void* r300_create_dsa_state(struct pipe_context* pipe,
                                     struct pipe_depth_stencil_alpha_state* state)
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

static void r300_set_scissor_state(struct pipe_context* pipe,
                                   struct pipe_scissor_state* state)
{
    struct r300_context* r300 = r300_context(pipe);
    draw_flush(r300->draw);

    /* XXX figure out how this memory doesn't get lost in space
    memcpy(r300->scissor, scissor, sizeof(struct pipe_scissor_state)); */
    r300->dirty_state |= R300_NEW_SCISSOR;
}

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  struct pipe_shader_state* state)
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

    r300->context.create_depth_stencil_alpha_state = r300_create_dsa_state;
    r300->context.bind_depth_stencil_alpha_state = r300_bind_dsa_state;
    r300->context.delete_depth_stencil_alpha_state = r300_delete_dsa_state;

    r300->context.set_scissor_state = r300_set_scissor_state;

    r300->context.create_vs_state = r300_create_vs_state;
    r300->context.bind_vs_state = r300_bind_vs_state;
    r300->context.delete_vs_state = r300_delete_vs_state;
}