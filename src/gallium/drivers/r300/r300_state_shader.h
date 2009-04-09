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

#include "tgsi/tgsi_parse.h"

#include "r300_context.h"
#include "r300_debug.h"
#include "r300_reg.h"
#include "r300_screen.h"

/* XXX this all should find its way back to r300_reg */
/* Swizzle tools */
#define R500_SWIZZLE_ZERO 4
#define R500_SWIZZLE_HALF 5
#define R500_SWIZZLE_ONE 6
#define R500_SWIZ_RGB_ZERO ((4 << 0) | (4 << 3) | (4 << 6))
#define R500_SWIZ_RGB_ONE ((6 << 0) | (6 << 3) | (6 << 6))
#define R500_SWIZ_RGB_RGB ((0 << 0) | (1 << 3) | (2 << 6))
#define R500_SWIZ_MOD_NEG 1
#define R500_SWIZ_MOD_ABS 2
#define R500_SWIZ_MOD_NEG_ABS 3
/* Swizzles for inst2 */
#define R500_SWIZ_TEX_STRQ(x) ((x) << 8)
#define R500_SWIZ_TEX_RGBA(x) ((x) << 24)
/* Swizzles for inst3 */
#define R500_SWIZ_RGB_A(x) ((x) << 2)
#define R500_SWIZ_RGB_B(x) ((x) << 15)
/* Swizzles for inst4 */
#define R500_SWIZ_ALPHA_A(x) ((x) << 14)
#define R500_SWIZ_ALPHA_B(x) ((x) << 21)
/* Swizzle for inst5 */
#define R500_SWIZ_RGBA_C(x) ((x) << 14)
#define R500_SWIZ_ALPHA_C(x) ((x) << 27)
/* Writemasks */
#define R500_TEX_WMASK(x) ((x) << 11)
#define R500_ALU_WMASK(x) ((x) << 11)
#define R500_ALU_OMASK(x) ((x) << 15)

/* TGSI constants. TGSI is like XML: If it can't solve your problems, you're
 * not using enough of it. */
static const struct tgsi_full_src_register r500_constant_zero = {
    .SrcRegister.Extended = TRUE,
    .SrcRegister.File = TGSI_FILE_NULL,
    .SrcRegisterExtSwz.ExtSwizzleX = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleY = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleZ = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleW = TGSI_EXTSWIZZLE_ZERO,
};

static const struct tgsi_full_src_register r500_constant_one = {
    .SrcRegister.Extended = TRUE,
    .SrcRegister.File = TGSI_FILE_NULL,
    .SrcRegisterExtSwz.ExtSwizzleX = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleY = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleZ = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleW = TGSI_EXTSWIZZLE_ONE,
};

/* Temporary struct used to hold assembly state while putting together
 * fragment programs. */
struct r300_fs_asm {
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

void r300_translate_fragment_shader(struct r300_context* r300,
                           struct r3xx_fragment_shader* fs);

static struct r300_fragment_shader r300_passthrough_fragment_shader = {
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

static struct r500_fragment_shader r500_passthrough_fragment_shader = {
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

static struct r300_fragment_shader r300_texture_fragment_shader = {
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

static struct r500_fragment_shader r500_texture_fragment_shader = {
    .shader.stack_size = 1,
    .instruction_count = 2,
    .instructions[0].inst0 = R500_INST_TYPE_TEX |
        R500_INST_TEX_SEM_WAIT |
        R500_INST_RGB_OMASK_RGB | R500_INST_ALPHA_OMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .instructions[0].inst1 = R500_TEX_ID(0) | R500_TEX_INST_LD |
        R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED,
    .instructions[0].inst2 = R500_TEX_SRC_ADDR(0) |
        R500_TEX_SRC_S_SWIZ_R | R500_TEX_SRC_T_SWIZ_G |
        R500_TEX_SRC_R_SWIZ_B | R500_TEX_SRC_Q_SWIZ_A |
        R500_TEX_DST_ADDR(0) |
        R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G |
        R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A,
    .instructions[0].inst3 = 0x0,
    .instructions[0].inst4 = 0x0,
    .instructions[0].inst5 = 0x0,
    .instructions[1].inst0 = R500_INST_TYPE_OUT |
        R500_INST_TEX_SEM_WAIT | R500_INST_LAST |
        R500_INST_RGB_OMASK_RGB | R500_INST_ALPHA_OMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .instructions[1].inst1 =
        R500_RGB_ADDR0(0) | R500_RGB_ADDR1(0) | R500_RGB_ADDR1_CONST |
        R500_RGB_ADDR2(0) | R500_RGB_ADDR2_CONST,
    .instructions[1].inst2 =
        R500_ALPHA_ADDR0(0) | R500_ALPHA_ADDR1(0) | R500_ALPHA_ADDR1_CONST |
        R500_ALPHA_ADDR2(0) | R500_ALPHA_ADDR2_CONST,
    .instructions[1].inst3 =
        R500_ALU_RGB_SEL_A_SRC0 | R500_ALU_RGB_R_SWIZ_A_R |
        R500_ALU_RGB_G_SWIZ_A_G | R500_ALU_RGB_B_SWIZ_A_B |
        R500_ALU_RGB_SEL_B_SRC0 | R500_ALU_RGB_R_SWIZ_B_R |
        R500_ALU_RGB_B_SWIZ_B_G | R500_ALU_RGB_G_SWIZ_B_B,
    .instructions[1].inst4 =
        R500_ALPHA_OP_CMP | R500_ALPHA_SWIZ_A_A | R500_ALPHA_SWIZ_B_A,
    .instructions[1].inst5 =
        R500_ALU_RGBA_OP_CMP | R500_ALU_RGBA_R_SWIZ_0 |
        R500_ALU_RGBA_G_SWIZ_0 | R500_ALU_RGBA_B_SWIZ_0 |
        R500_ALU_RGBA_A_SWIZ_0,
};

#endif /* R300_STATE_SHADER_H */
