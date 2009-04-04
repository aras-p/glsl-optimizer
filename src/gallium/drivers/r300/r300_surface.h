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

#ifndef R300_SURFACE_H
#define R300_SURFACE_H

#include "pipe/p_context.h"
#include "pipe/p_screen.h"

#include "util/u_rect.h"

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"
#include "r300_state_shader.h"
#include "r300_state_tcl.h"
#include "r300_state_inlines.h"

static struct r300_blend_state blend_clear_state = {
    .blend_control = 0x0,
    .alpha_blend_control = 0x0,
    .rop = 0x0,
    .dither = 0x0,
};

static struct r300_blend_color_state blend_color_clear_state = {
    .blend_color = 0x0,
    .blend_color_red_alpha = 0x0,
    .blend_color_green_blue = 0x0,
};

static struct r300_dsa_state dsa_clear_state = {
    .alpha_function = 0x0,
    .alpha_reference = 0x0,
    .z_buffer_control = 0x0,
    .z_stencil_control = 0x0,
    .stencil_ref_mask = R300_STENCILWRITEMASK_MASK,
    .z_buffer_top = R300_ZTOP_ENABLE,
    .stencil_ref_bf = 0x0,
};

static struct r300_rs_state rs_clear_state = {
    .point_minmax = 0x36000006,
    .line_control = 0x00030006,
    .depth_scale_front = 0x0,
    .depth_offset_front = 0x0,
    .depth_scale_back = 0x0,
    .depth_offset_back = 0x0,
    .polygon_offset_enable = 0x0,
    .cull_mode = 0x0,
    .line_stipple_config = 0x3BAAAAAB,
    .line_stipple_value = 0x0,
    .color_control = R300_SHADE_MODEL_FLAT,
};

static struct r300_rs_block r300_rs_block_clear_state = {
    .ip[0] = R500_RS_SEL_S(R300_RS_SEL_K0) |
        R500_RS_SEL_T(R300_RS_SEL_K0) |
        R500_RS_SEL_R(R300_RS_SEL_K0) |
        R500_RS_SEL_Q(R300_RS_SEL_K1),
    .inst[0] = R300_RS_INST_COL_CN_WRITE,
    .count = R300_IT_COUNT(0) | R300_IC_COUNT(1) | R300_HIRES_EN,
    .inst_count = 0,
};

static struct r300_rs_block r500_rs_block_clear_state = {
    .ip[0] = R500_RS_SEL_S(R500_RS_IP_PTR_K0) |
        R500_RS_SEL_T(R500_RS_IP_PTR_K0) |
        R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
        R500_RS_SEL_Q(R500_RS_IP_PTR_K1),
    .inst[0] = R500_RS_INST_COL_CN_WRITE,
    .count = R300_IT_COUNT(0) | R300_IC_COUNT(1) | R300_HIRES_EN,
    .inst_count = 0,
};

/* The following state is used for surface_copy only. */

static struct r300_rs_block r300_rs_block_copy_state = {
    .ip[0] = R500_RS_SEL_S(R300_RS_SEL_K0) |
        R500_RS_SEL_T(R300_RS_SEL_K0) |
        R500_RS_SEL_R(R300_RS_SEL_K0) |
        R500_RS_SEL_Q(R300_RS_SEL_K1),
    .inst[0] = R300_RS_INST_COL_CN_WRITE,
    .count = R300_IT_COUNT(2) | R300_IC_COUNT(0) | R300_HIRES_EN,
    .inst_count = R300_RS_TX_OFFSET(6),
};

static struct r300_rs_block r500_rs_block_copy_state = {
    .ip[0] = R500_RS_SEL_S(0) |
        R500_RS_SEL_T(1) |
        R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
        R500_RS_SEL_Q(R500_RS_IP_PTR_K1),
    .inst[0] = R500_RS_INST_TEX_CN_WRITE,
    .count = R300_IT_COUNT(2) | R300_IC_COUNT(0) | R300_HIRES_EN,
    .inst_count = R300_RS_TX_OFFSET(6),
};

static struct r300_sampler_state r300_sampler_copy_state = {
    .filter0 = R300_TX_WRAP_S(R300_TX_CLAMP) |
        R300_TX_WRAP_T(R300_TX_CLAMP) |
        R300_TX_MAG_FILTER_NEAREST |
        R300_TX_MIN_FILTER_NEAREST,
};

#endif /* R300_SURFACE_H */
