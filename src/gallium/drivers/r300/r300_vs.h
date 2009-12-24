/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#ifndef R300_VS_H
#define R300_VS_H

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"
#include "radeon_code.h"

#include "r300_shader_semantics.h"

struct r300_context;

struct r300_vertex_shader {
    /* Parent class */
    struct pipe_shader_state state;

    struct tgsi_shader_info info;
    struct r300_shader_semantics outputs;
    uint hwfmt[4];

    /* Stream locations for SWTCL or if TCL is bypassed. */
    int stream_loc_notcl[16];

    /* Output stream location for WPOS. */
    int wpos_tex_output;

    /* Has this shader been translated yet? */
    boolean translated;

    /* Machine code (if translated) */
    struct r300_vertex_program_code code;
};

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs);

/* Return TRUE if VAP (hwfmt) needs to be re-emitted. */
boolean r300_vertex_shader_setup_wpos(struct r300_context* r300);

#endif /* R300_VS_H */
