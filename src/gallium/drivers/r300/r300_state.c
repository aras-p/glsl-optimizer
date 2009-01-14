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
                                     struct pipe_blend_state* state)
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

/* Create a new scissor state based on the CSO scissor state.
 *
 * This is only for the fragment scissors. */
static void* r300_create_scissor_state(struct pipe_context* pipe,
                                    struct pipe_scissor_state* state)
{
    uint32_t left, top, right, bottom;
    struct r300_scissor_state* scissor = CALLOC_STRUCT(r300_scissor_state);

    /* So, a bit of info. The scissors are offset by R300_SCISSORS_OFFSET in
     * both directions for all values, and can only be 13 bits wide. Why?
     * We may never know. */
    left = (state->minx + R300_SCISSORS_OFFSET) & 0x1fff;
    top = (state->miny + R300_SCISSORS_OFFSET) & 0x1fff;
    right = (state->maxx + R300_SCISSORS_OFFSET) & 0x1fff;
    bottom = (state->maxy + R300_SCISSORS_OFFSET) & 0x1fff;

    scissor->scissor_top_left = (left << R300_SCISSORS_X_SHIFT) |
            (top << R300_SCISSORS_Y_SHIFT);
    scissor->scissor_bottom_right = (right << R300_SCISSORS_X_SHIFT) |
            (bottom << R300_SCISSORS_Y_SHIFT);

    return (void*)scissor;
}

/* Bind scissor state.*/
static void r300_bind_scissor_state(struct pipe_context* pipe,
                                 void* state)
{
    struct r300_context* r300 = r300_context(pipe);

    r300->scissor_state = (struct r300_scissor_state*)state;
    r300->dirty_state |= R300_NEW_SCISSOR;
}

/* Delete scissor state. */
static void r300_delete_scissor_state(struct pipe_context* pipe,
                                   void* state)
{
    FREE(state);
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