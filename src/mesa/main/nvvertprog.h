/* $Id: nvvertprog.h,v 1.1 2003/01/14 04:55:46 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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

#ifndef NVVERTPROG_H
#define NVVERTPROG_H


#define VP_NUM_INPUT_REGS   MAX_NV_VERTEX_PROGRAM_INPUTS
#define VP_NUM_OUTPUT_REGS  MAX_NV_VERTEX_PROGRAM_INPUTS
#define VP_NUM_TEMP_REGS    MAX_NV_VERTEX_PROGRAM_TEMPS
#define VP_NUM_PROG_REGS    MAX_NV_VERTEX_PROGRAM_PARAMS

/* Location of register groups within the whole register file */
#define VP_INPUT_REG_START  0
#define VP_INPUT_REG_END    (VP_INPUT_REG_START + VP_NUM_INPUT_REGS - 1)
#define VP_OUTPUT_REG_START (VP_INPUT_REG_END + 1)
#define VP_OUTPUT_REG_END   (VP_OUTPUT_REG_START + VP_NUM_OUTPUT_REGS - 1)
#define VP_TEMP_REG_START   (VP_OUTPUT_REG_END + 1)
#define VP_TEMP_REG_END     (VP_TEMP_REG_START + VP_NUM_TEMP_REGS - 1)
#define VP_PROG_REG_START   (VP_TEMP_REG_END + 1)
#define VP_PROG_REG_END     (VP_PROG_REG_START + VP_NUM_PROG_REGS - 1)


/* Vertex program opcodes */
enum vp_opcode
{
   VP_OPCODE_MOV,
   VP_OPCODE_LIT,
   VP_OPCODE_RCP,
   VP_OPCODE_RSQ,
   VP_OPCODE_EXP,
   VP_OPCODE_LOG,
   VP_OPCODE_MUL,
   VP_OPCODE_ADD,
   VP_OPCODE_DP3,
   VP_OPCODE_DP4,
   VP_OPCODE_DST,
   VP_OPCODE_MIN,
   VP_OPCODE_MAX,
   VP_OPCODE_SLT,
   VP_OPCODE_SGE,
   VP_OPCODE_MAD,
   VP_OPCODE_ARL,
   VP_OPCODE_DPH,
   VP_OPCODE_RCC,
   VP_OPCODE_SUB,
   VP_OPCODE_ABS,
   VP_OPCODE_END
};


/* Instruction source register */
struct vp_src_register
{
   GLint Register;    /* or the offset from the address register */
   GLuint Swizzle[4];
   GLboolean Negate;
   GLboolean RelAddr;
};


/* Instruction destination register */
struct vp_dst_register
{
   GLint Register;
   GLboolean WriteMask[4];
};


/* Vertex program instruction */
struct vp_instruction
{
   enum vp_opcode Opcode;
   struct vp_src_register SrcReg[3];
   struct vp_dst_register DstReg;
};


#endif /* VERTPROG_H */
