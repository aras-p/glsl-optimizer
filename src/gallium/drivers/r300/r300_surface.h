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

const struct r300_blend_state blend_clear_state = {
    .blend_control = 0x0,
    .alpha_blend_control = 0x0,
    .rop = 0x0,
    .dither = 0x0,
};

const struct r300_blend_color_state blend_color_clear_state = {
    .blend_color = 0x0,
    .blend_color_red_alpha = 0x0,
    .blend_color_green_blue = 0x0,
};

const struct r300_dsa_state dsa_clear_state = {
    .alpha_function = 0x0,
    .alpha_reference = 0x0,
    .z_buffer_control = 0x0,
    .z_stencil_control = 0x0,
    .stencil_ref_mask = 0x0,
    .z_buffer_top = R300_ZTOP_ENABLE,
    .stencil_ref_bf = 0x0,
};

#endif /* R300_SURFACE_H */
