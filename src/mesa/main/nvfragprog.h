/* $Id: nvfragprog.h,v 1.4 2003/03/14 15:40:59 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Private vertex program types and constants only used by files
 * related to vertex programs.
 */


#ifndef NVFRAGPROG_H
#define NVFRAGPROG_H

#include "config.h"


/* Location of register sets within the whole register file */
#define FP_INPUT_REG_START  0
#define FP_INPUT_REG_END    (FP_INPUT_REG_START + MAX_NV_FRAGMENT_PROGRAM_INPUTS - 1)
#define FP_OUTPUT_REG_START (FP_INPUT_REG_END + 1)
#define FP_OUTPUT_REG_END   (FP_OUTPUT_REG_START + MAX_NV_FRAGMENT_PROGRAM_OUTPUTS - 1)
#define FP_TEMP_REG_START   (FP_OUTPUT_REG_END + 1)
#define FP_TEMP_REG_END     (FP_TEMP_REG_START + MAX_NV_FRAGMENT_PROGRAM_TEMPS - 1)
#define FP_PROG_REG_START   (FP_TEMP_REG_END + 1)
#define FP_PROG_REG_END     (FP_PROG_REG_START + MAX_NV_FRAGMENT_PROGRAM_PARAMS - 1)
#define FP_DUMMY_REG_START  (FP_PROG_REG_END + 1)
#define FP_DUMMY_REG_END    (FP_DUMMY_REG_START + MAX_NV_FRAGMENT_PROGRAM_WRITE_ONLYS - 1)


/* condition codes */
#define COND_GT  1  /* greater than zero */
#define COND_EQ  2  /* equal to zero */
#define COND_LT  3  /* less than zero */
#define COND_UN  4  /* unordered (NaN) */
#define COND_GE  5  /* greater then or equal to zero */
#define COND_LE  6  /* less then or equal to zero */
#define COND_NE  7  /* not equal to zero */
#define COND_TR  8  /* always true */
#define COND_FL  9  /* always false */


/* instruction precision */
#define FLOAT32  0x1
#define FLOAT16  0x2
#define FIXED12  0x4


enum fp_opcode {
   FP_OPCODE_ADD = 1000,
   FP_OPCODE_COS,
   FP_OPCODE_DDX,
   FP_OPCODE_DDY,
   FP_OPCODE_DP3,
   FP_OPCODE_DP4,
   FP_OPCODE_DST,
   FP_OPCODE_EX2,
   FP_OPCODE_FLR,
   FP_OPCODE_FRC,
   FP_OPCODE_KIL,
   FP_OPCODE_LG2,
   FP_OPCODE_LIT,
   FP_OPCODE_LRP,
   FP_OPCODE_MAD,
   FP_OPCODE_MAX,
   FP_OPCODE_MIN,
   FP_OPCODE_MOV,
   FP_OPCODE_MUL,
   FP_OPCODE_PK2H,
   FP_OPCODE_PK2US,
   FP_OPCODE_PK4B,
   FP_OPCODE_PK4UB,
   FP_OPCODE_POW,
   FP_OPCODE_RCP,
   FP_OPCODE_RFL,
   FP_OPCODE_RSQ,
   FP_OPCODE_SEQ,
   FP_OPCODE_SFL,
   FP_OPCODE_SGE,
   FP_OPCODE_SGT,
   FP_OPCODE_SIN,
   FP_OPCODE_SLE,
   FP_OPCODE_SLT,
   FP_OPCODE_SNE,
   FP_OPCODE_STR,
   FP_OPCODE_SUB,
   FP_OPCODE_TEX,
   FP_OPCODE_TXD,
   FP_OPCODE_TXP,
   FP_OPCODE_UP2H,
   FP_OPCODE_UP2US,
   FP_OPCODE_UP4B,
   FP_OPCODE_UP4UB,
   FP_OPCODE_X2D,
   FP_OPCODE_END /* private opcode */
};


struct fp_src_register
{
   GLint RegType;  /* constant, param, temp or attribute register */
   GLint Register;    /* or the offset from the address register */
   GLuint Swizzle[4];
   GLboolean NegateBase; /* negate before absolute value? */
   GLboolean Abs;        /* take absolute value? */
   GLboolean NegateAbs;  /* negate after absolute value? */
};


/* Instruction destination register */
struct fp_dst_register
{
   GLint Register;
   GLboolean WriteMask[4];
   GLuint CondMask;
   GLuint CondSwizzle[4];
};


struct fp_instruction
{
   enum fp_opcode Opcode;
   struct fp_src_register SrcReg[3];
   struct fp_dst_register DstReg;
   GLboolean Saturate;
   GLboolean UpdateCondRegister;
   GLubyte Precision;    /* FLOAT32, FLOAT16 or FIXED12 */
   GLubyte TexSrcUnit;   /* texture unit for TEX, TXD, TXP instructions */
   GLubyte TexSrcBit;    /* TEXTURE_1D,2D,3D,CUBE,RECT_BIT source target */
};



#endif
