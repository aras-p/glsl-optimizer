/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
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

#ifndef R300_FS_H
#define R300_FS_H

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"
#include "radeon_code.h"
#include "r300_shader_semantics.h"

struct r300_fragment_shader_code {
    struct r300_fragment_program_external_state compare_state;
    struct rX00_fragment_program_code code;

    struct r300_fragment_shader_code* next;
};

struct r300_fragment_shader {
    /* Parent class */
    struct pipe_shader_state state;

    struct tgsi_shader_info info;
    struct r300_shader_semantics inputs;

    /* Bits 0-15: TRUE if it's a shadow sampler, FALSE otherwise. */
    unsigned shadow_samplers;

    /* Currently-bound fragment shader. */
    struct r300_fragment_shader_code* shader;

    /* List of the same shaders compiled with different texture-compare
     * states. */
    struct r300_fragment_shader_code* first;
};

void r300_shader_read_fs_inputs(struct tgsi_shader_info* info,
                                struct r300_shader_semantics* fs_inputs);

/* Return TRUE if the shader was switched and should be re-emitted. */
boolean r300_pick_fragment_shader(struct r300_context* r300);

static INLINE boolean r300_fragment_shader_writes_depth(struct r300_fragment_shader *fs)
{
    if (!fs)
        return FALSE;
    return (fs->shader->code.writes_depth) ? TRUE : FALSE;
}

#endif /* R300_FS_H */
