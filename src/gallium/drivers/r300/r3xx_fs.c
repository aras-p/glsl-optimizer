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

#include "r3xx_fs.h"

#include "r300_reg.h"

struct rX00_fragment_program_code r3xx_passthrough_fragment_shader = {
    .code.r300.alu.length = 1,
    .code.r300.tex.length = 0,

    .code.r300.config = 0,
    .code.r300.pixsize = 0,
    .code.r300.code_offset = 0,
    .code.r300.code_addr[3] = R300_RGBA_OUT,

    .code.r300.alu.inst[0].rgb_inst = R300_RGB_SWIZA(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZB(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZC(R300_ALU_ARGC_ZERO) |
        R300_ALU_OUTC_CMP,
    .code.r300.alu.inst[0].rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ,
    .code.r300.alu.inst[0].alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        R300_ALU_OUTA_CMP,
    .code.r300.alu.inst[0].alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT,
};

struct rX00_fragment_program_code r3xx_texture_fragment_shader = {
    .code.r300.alu.length = 1,
    .code.r300.tex.length = 1,

    .code.r300.config = R300_PFS_CNTL_FIRST_NODE_HAS_TEX,
    .code.r300.pixsize = 0,
    .code.r300.code_offset = 0,
    .code.r300.code_addr[3] = R300_RGBA_OUT,

    .code.r300.tex.inst[0] = R300_TEX_OP_LD << R300_TEX_INST_SHIFT,

    .code.r300.alu.inst[0].rgb_inst = R300_RGB_SWIZA(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZB(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZC(R300_ALU_ARGC_ZERO) |
        R300_ALU_OUTC_CMP,
    .code.r300.alu.inst[0].rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ,
    .code.r300.alu.inst[0].alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        R300_ALU_OUTA_CMP,
    .code.r300.alu.inst[0].alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT,
};
