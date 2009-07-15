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

#ifndef R3XX_FS_H
#define R3XX_FS_H

#include "r300_fs_inlines.h"

static struct r3xx_fragment_shader r3xx_passthrough_fragment_shader = {
    .alu_instruction_count = 1,
    .tex_instruction_count = 0,
    .indirections = 0,
    .shader.stack_size = 1,

    .instructions[0].alu_rgb_inst = R300_RGB_SWIZA(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZB(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZC(R300_ALU_ARGC_ZERO) |
        R300_ALU_OUTC_CMP,
    .instructions[0].alu_rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ,
    .instructions[0].alu_alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        R300_ALU_OUTA_CMP,
    .instructions[0].alu_alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT,
};

static struct r3xx_fragment_shader r3xx_texture_fragment_shader = {
    .alu_instruction_count = 1,
    .tex_instruction_count = 0,
    .indirections = 0,
    .shader.stack_size = 1,

    .instructions[0].alu_rgb_inst = R300_RGB_SWIZA(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZB(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZC(R300_ALU_ARGC_ZERO) |
        R300_ALU_OUTC_CMP,
    .instructions[0].alu_rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ,
    .instructions[0].alu_alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        R300_ALU_OUTA_CMP,
    .instructions[0].alu_alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT,
};

void r3xx_fs_finalize(struct r300_fragment_shader* fs,
                      struct r300_fs_asm* assembler);

void r3xx_fs_instruction(struct r3xx_fragment_shader* fs,
                         struct r300_fs_asm* assembler,
                         struct tgsi_full_instruction* inst);

#endif /* R3XX_FS_H */
