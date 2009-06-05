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

#ifndef R300_DEBUG_H
#define R300_DEBUG_H

#include "r300_reg.h"
#include "r300_state_shader.h"
#include "r300_state_tcl.h"

static char* r500_fs_swiz[] = {
    " R",
    " G",
    " B",
    " A",
    " 0",
    ".5",
    " 1",
    " U",
};

static char* r500_fs_op_rgb[] = {
    "MAD",
    "DP3",
    "DP4",
    "D2A",
    "MIN",
    "MAX",
    "---",
    "CND",
    "CMP",
    "FRC",
    "SOP",
    "MDH",
    "MDV",
};

static char* r500_fs_op_alpha[] = {
    "MAD",
    " DP",
    "MIN",
    "MAX",
    "---",
    "CND",
    "CMP",
    "FRC",
    "EX2",
    "LN2",
    "RCP",
    "RSQ",
    "SIN",
    "COS",
    "MDH",
    "MDV",
};

static char* r500_fs_mask[] = {
    "NONE",
    "R   ",
    " G  ",
    "RG  ",
    "  B ",
    "R B ",
    " GB ",
    "RGB ",
    "   A",
    "R  A",
    " G A",
    "RG A",
    "  BA",
    "R BA",
    " GBA",
    "RGBA",
};

static char* r500_fs_tex[] = {
    "    NOP",
    "     LD",
    "TEXKILL",
    "   PROJ",
    "LODBIAS",
    "    LOD",
    "   DXDY",
};

static char* r300_vs_ve_ops[] = {
    /* R300 vector ops */
    "                 VE_NO_OP",
    "           VE_DOT_PRODUCT",
    "              VE_MULTIPLY",
    "                   VE_ADD",
    "          VE_MULTIPLY_ADD",
    "       VE_DISTANCE_FACTOR",
    "              VE_FRACTION",
    "               VE_MAXIMUM",
    "               VE_MINIMUM",
    "VE_SET_GREATER_THAN_EQUAL",
    "         VE_SET_LESS_THAN",
    "        VE_MULTIPLYX2_ADD",
    "        VE_MULTIPLY_CLAMP",
    "            VE_FLT2FIX_DX",
    "        VE_FLT2FIX_DX_RND",
    /* R500 vector ops */
    "      VE_PRED_SET_EQ_PUSH",
    "      VE_PRED_SET_GT_PUSH",
    "     VE_PRED_SET_GTE_PUSH",
    "     VE_PRED_SET_NEQ_PUSH",
    "         VE_COND_WRITE_EQ",
    "         VE_COND_WRITE_GT",
    "        VE_COND_WRITE_GTE",
    "        VE_COND_WRITE_NEQ",
    "      VE_SET_GREATER_THAN",
    "             VE_SET_EQUAL",
    "         VE_SET_NOT_EQUAL",
    "               (reserved)",
    "               (reserved)",
    "               (reserved)",
};

static char* r300_vs_me_ops[] = {
    /* R300 math ops */
    "                 ME_NO_OP",
    "          ME_EXP_BASE2_DX",
    "          ME_LOG_BASE2_DX",
    "          ME_EXP_BASEE_FF",
    "        ME_LIGHT_COEFF_DX",
    "         ME_POWER_FUNC_FF",
    "              ME_RECIP_DX",
    "              ME_RECIP_FF",
    "         ME_RECIP_SQRT_DX",
    "         ME_RECIP_SQRT_FF",
    "              ME_MULTIPLY",
    "     ME_EXP_BASE2_FULL_DX",
    "     ME_LOG_BASE2_FULL_DX",
    " ME_POWER_FUNC_FF_CLAMP_B",
    "ME_POWER_FUNC_FF_CLAMP_B1",
    "ME_POWER_FUNC_FF_CLAMP_01",
    "                   ME_SIN",
    "                   ME_COS",
    /* R500 math ops */
    "        ME_LOG_BASE2_IEEE",
    "            ME_RECIP_IEEE",
    "       ME_RECIP_SQRT_IEEE",
    "           ME_PRED_SET_EQ",
    "           ME_PRED_SET_GT",
    "          ME_PRED_SET_GTE",
    "          ME_PRED_SET_NEQ",
    "          ME_PRED_SET_CLR",
    "          ME_PRED_SET_INV",
    "          ME_PRED_SET_POP",
    "      ME_PRED_SET_RESTORE",
    "               (reserved)",
    "               (reserved)",
    "               (reserved)",
};

/* XXX refactor to avoid clashing symbols */
static char* r300_vs_src_debug[] = {
    "t",
    "i",
    "c",
    "a",
};

static char* r300_vs_dst_debug[] = {
    "t",
    "a0",
    "o",
    "ox",
    "a",
    "i",
    "u",
    "u",
};

static char* r300_vs_swiz_debug[] = {
    "X",
    "Y",
    "Z",
    "W",
    "0",
    "1",
    "U",
    "U",
};

void r500_fs_dump(struct r500_fragment_shader* fs);

void r300_vs_dump(struct r300_vertex_shader* vs);

#endif /* R300_DEBUG_H */
