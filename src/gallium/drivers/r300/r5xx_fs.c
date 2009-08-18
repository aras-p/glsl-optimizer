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

#include "r5xx_fs.h"

#include "r300_reg.h"

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
#define R500_W_OMASK (1 << 31)

struct rX00_fragment_program_code r5xx_passthrough_fragment_shader = {
    .code.r500.max_temp_idx = 0,
    .code.r500.inst_end = 0,

    .code.r500.inst[0].inst0 = R500_INST_TYPE_OUT |
        R500_INST_TEX_SEM_WAIT | R500_INST_LAST |
        R500_INST_RGB_OMASK_RGB | R500_INST_ALPHA_OMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .code.r500.inst[0].inst1 =
        R500_RGB_ADDR0(0) | R500_RGB_ADDR1(0) | R500_RGB_ADDR1_CONST |
        R500_RGB_ADDR2(0) | R500_RGB_ADDR2_CONST,
    .code.r500.inst[0].inst2 =
        R500_ALPHA_ADDR0(0) | R500_ALPHA_ADDR1(0) | R500_ALPHA_ADDR1_CONST |
        R500_ALPHA_ADDR2(0) | R500_ALPHA_ADDR2_CONST,
    .code.r500.inst[0].inst3 =
        R500_ALU_RGB_SEL_A_SRC0 | R500_ALU_RGB_R_SWIZ_A_R |
        R500_ALU_RGB_G_SWIZ_A_G | R500_ALU_RGB_B_SWIZ_A_B |
        R500_ALU_RGB_SEL_B_SRC0 | R500_ALU_RGB_R_SWIZ_B_R |
        R500_ALU_RGB_B_SWIZ_B_G | R500_ALU_RGB_G_SWIZ_B_B,
    .code.r500.inst[0].inst4 =
        R500_ALPHA_OP_CMP | R500_ALPHA_SWIZ_A_A | R500_ALPHA_SWIZ_B_A,
    .code.r500.inst[0].inst5 =
        R500_ALU_RGBA_OP_CMP | R500_ALU_RGBA_R_SWIZ_0 |
        R500_ALU_RGBA_G_SWIZ_0 | R500_ALU_RGBA_B_SWIZ_0 |
        R500_ALU_RGBA_A_SWIZ_0,
};

struct rX00_fragment_program_code r5xx_texture_fragment_shader = {
    .code.r500.max_temp_idx = 0,
    .code.r500.inst_end = 1,

    .code.r500.inst[0].inst0 = R500_INST_TYPE_TEX |
        R500_INST_TEX_SEM_WAIT |
        R500_INST_RGB_WMASK_RGB | R500_INST_ALPHA_WMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .code.r500.inst[0].inst1 = R500_TEX_ID(0) | R500_TEX_INST_LD |
        R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED,
    .code.r500.inst[0].inst2 = R500_TEX_SRC_ADDR(0) |
        R500_TEX_SRC_S_SWIZ_R | R500_TEX_SRC_T_SWIZ_G |
        R500_TEX_SRC_R_SWIZ_B | R500_TEX_SRC_Q_SWIZ_A |
        R500_TEX_DST_ADDR(0) |
        R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G |
        R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A,
    .code.r500.inst[0].inst3 = 0x0,
    .code.r500.inst[0].inst4 = 0x0,
    .code.r500.inst[0].inst5 = 0x0,

    .code.r500.inst[1].inst0 = R500_INST_TYPE_OUT |
        R500_INST_TEX_SEM_WAIT | R500_INST_LAST |
        R500_INST_RGB_OMASK_RGB | R500_INST_ALPHA_OMASK |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP,
    .code.r500.inst[1].inst1 =
        R500_RGB_ADDR0(0) | R500_RGB_ADDR1(0) | R500_RGB_ADDR1_CONST |
        R500_RGB_ADDR2(0) | R500_RGB_ADDR2_CONST,
    .code.r500.inst[1].inst2 =
        R500_ALPHA_ADDR0(0) | R500_ALPHA_ADDR1(0) | R500_ALPHA_ADDR1_CONST |
        R500_ALPHA_ADDR2(0) | R500_ALPHA_ADDR2_CONST,
    .code.r500.inst[1].inst3 =
        R500_ALU_RGB_SEL_A_SRC0 | R500_ALU_RGB_R_SWIZ_A_R |
        R500_ALU_RGB_G_SWIZ_A_G | R500_ALU_RGB_B_SWIZ_A_B |
        R500_ALU_RGB_SEL_B_SRC0 | R500_ALU_RGB_R_SWIZ_B_R |
        R500_ALU_RGB_B_SWIZ_B_G | R500_ALU_RGB_G_SWIZ_B_B,
    .code.r500.inst[1].inst4 =
        R500_ALPHA_OP_CMP | R500_ALPHA_SWIZ_A_A | R500_ALPHA_SWIZ_B_A,
    .code.r500.inst[1].inst5 =
        R500_ALU_RGBA_OP_CMP | R500_ALU_RGBA_R_SWIZ_0 |
        R500_ALU_RGBA_G_SWIZ_0 | R500_ALU_RGBA_B_SWIZ_0 |
        R500_ALU_RGBA_A_SWIZ_0,
};
