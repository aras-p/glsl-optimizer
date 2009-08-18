/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#ifndef R300_VS_H
#define R300_VS_H

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

#include "radeon_code.h"

struct r300_context;

struct r300_vertex_shader {
    /* Parent class */
    struct pipe_shader_state state;
    struct tgsi_shader_info info;

    /* Fallback shader, because Draw has issues */
    struct draw_vertex_shader* draw;

    /* Has this shader been translated yet? */
    boolean translated;

    /* Machine code (if translated) */
    struct r300_vertex_program_code code;
};


extern struct r300_vertex_program_code r300_passthrough_vertex_shader;

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs);

#endif /* R300_VS_H */
