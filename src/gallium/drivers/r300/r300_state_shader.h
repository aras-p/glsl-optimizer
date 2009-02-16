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

#ifndef R300_STATE_SHADER_H
#define R300_STATE_SHADER_H

#include "r300_context.h"
#include "r300_reg.h"
#include "r300_screen.h"

void r300_translate_fragment_shader(struct r300_context* r300,
                           struct r300_fragment_shader* fs);

void r500_translate_fragment_shader(struct r300_context* r300,
                           struct r500_fragment_shader* fs);

static const struct r300_fragment_shader r300_passthrough_fragment_shader = {
    /* XXX This is the emission code. TODO: decode
    OUT_CS_REG(R300_US_CONFIG, 0);
    OUT_CS_REG(R300_US_CODE_OFFSET, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_0, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_1, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_2, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_3, 0x400000);
*/
    .alu_instruction_count = 1,
    .tex_instruction_count = 0,
    .indirections = 1,
    .shader.stack_size = 2,

    /* XXX decode these */
    .instructions[0].alu_rgb_inst = 0x50A80,
    .instructions[0].alu_rgb_inst = 0x1C000000,
    .instructions[0].alu_alpha_inst = 0x40889,
    .instructions[0].alu_alpha_inst = 0x1000000,
};

static const struct r500_fragment_shader r500_passthrough_fragment_shader = {
    .shader.stack_size = 0,
    .instruction_count = 1,
    .instructions[0].inst0 = R500_INST_TYPE_OUT |
        R500_INST_TEX_SEM_WAIT | R500_INST_LAST |
        R500_INST_RGB_OMASK_RGB | R500_INST_ALPHA_OMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .instructions[0].inst1 =
        R500_RGB_ADDR0(0) | R500_RGB_ADDR1(0) | R500_RGB_ADDR1_CONST |
        R500_RGB_ADDR2(0) | R500_RGB_ADDR2_CONST,
    .instructions[0].inst2 =
        R500_ALPHA_ADDR0(0) | R500_ALPHA_ADDR1(0) | R500_ALPHA_ADDR1_CONST |
        R500_ALPHA_ADDR2(0) | R500_ALPHA_ADDR2_CONST,
    .instructions[0].inst3 =
        R500_ALU_RGB_SEL_A_SRC0 | R500_ALU_RGB_R_SWIZ_A_R |
        R500_ALU_RGB_G_SWIZ_A_G | R500_ALU_RGB_B_SWIZ_A_B |
        R500_ALU_RGB_SEL_B_SRC0 | R500_ALU_RGB_R_SWIZ_B_R |
        R500_ALU_RGB_B_SWIZ_B_G | R500_ALU_RGB_G_SWIZ_B_B,
    .instructions[0].inst4 =
        R500_ALPHA_OP_CMP | R500_ALPHA_SWIZ_A_A | R500_ALPHA_SWIZ_B_A,
    .instructions[0].inst5 =
        R500_ALU_RGBA_OP_CMP | R500_ALU_RGBA_R_SWIZ_0 |
        R500_ALU_RGBA_G_SWIZ_0 | R500_ALU_RGBA_B_SWIZ_0 |
        R500_ALU_RGBA_A_SWIZ_0,
};

#endif /* R300_STATE_SHADER_H */
