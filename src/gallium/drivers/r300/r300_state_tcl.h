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

#ifndef R300_STATE_TCL_H
#define R300_STATE_TCL_H

#include "tgsi/tgsi_parse.h"

#include "r300_context.h"
#include "r300_debug.h"
#include "r300_reg.h"
#include "r300_screen.h"

/* XXX get these to r300_reg */
#define R300_PVS_DST_OPCODE(x)   ((x) << 0)
#   define R300_VE_ADD                    3
#define R300_PVS_DST_REG_TYPE(x) ((x) << 8)
#   define R300_PVS_DST_REG_TEMPORARY     0
#   define R300_PVS_DST_REG_A0            1
#   define R300_PVS_DST_REG_OUT           2
#   define R300_PVS_DST_REG_OUT_REPL_X    3
#   define R300_PVS_DST_REG_ALT_TEMPORARY 4
#   define R300_PVS_DST_REG_INPUT         5
#define R300_PVS_DST_OFFSET(x)   ((x) << 13)
#define R300_PVS_DST_WE_SHIFT    20

/* Temporary struct used to hold assembly state while putting together
 * fragment programs. */
struct r300_vs_asm {
    /* Pipe context. */
    struct r300_context* r300;
    /* Number of colors. */
    unsigned color_count;
    /* Number of texcoords. */
    unsigned tex_count;
    /* Offset for temporary registers. Inputs and temporaries have no
     * distinguishing markings, so inputs start at 0 and the first usable
     * temporary register is after all inputs. */
    unsigned temp_offset;
    /* Number of requested temporary registers. */
    unsigned temp_count;
    /* Offset for immediate constants. Neither R300 nor R500 can do four
     * inline constants per source, so instead we copy immediates into the
     * constant buffer. */
    unsigned imm_offset;
    /* Number of immediate constants. */
    unsigned imm_count;
};

static struct r300_vertex_shader r300_passthrough_vertex_shader = {
        /* XXX translate these back into normal instructions */
    .instruction_count = 2,
    .instructions[0].inst0 = 0xF00203,
    .instructions[0].inst1 = 0xD10001,
    .instructions[0].inst2 = 0x1248001,
    .instructions[0].inst3 = 0x0,
    .instructions[1].inst0 = 0xF00203,
    .instructions[1].inst1 = 0xD10021,
    .instructions[1].inst2 = 0x1248021,
    .instructions[1].inst3 = 0x0,
};

static struct r300_vertex_shader r300_texture_vertex_shader = {
        /* XXX translate these back into normal instructions */
    .instruction_count = 2,
    .instructions[0].inst0 = 0xF00203,
    .instructions[0].inst1 = 0xD10001,
    .instructions[0].inst2 = 0x1248001,
    .instructions[0].inst3 = 0x0,
    .instructions[1].inst0 = 0xF00203,
    .instructions[1].inst1 = 0xD10061,
    .instructions[1].inst2 = 0x1248061,
    .instructions[1].inst3 = 0x0,
};

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs);

#endif /* R300_STATE_TCL_H */
