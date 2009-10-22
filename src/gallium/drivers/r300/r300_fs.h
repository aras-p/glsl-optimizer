/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
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

struct r300_fragment_shader {
    /* Parent class */
    struct pipe_shader_state state;
    struct tgsi_shader_info info;

    /* Has this shader been translated yet? */
    boolean translated;

    /* Compiled code */
    struct rX00_fragment_program_code code;
};


void r300_translate_fragment_shader(struct r300_context* r300,
                                    struct r300_fragment_shader* fs);

static inline boolean r300_fragment_shader_writes_depth(struct r300_fragment_shader *fs)
{
    if (!fs)
	return FALSE;
    return (fs->code.writes_depth) ? TRUE : FALSE;
}
#endif /* R300_FS_H */
