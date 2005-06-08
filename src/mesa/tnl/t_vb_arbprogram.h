/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/**
 * \file t_arb_program.c
 * Compile vertex programs to an intermediate representation.
 * Execute vertex programs over a buffer of vertices.
 * \author Keith Whitwell, Brian Paul
 */


#ifndef _T_VB_ARBPROGRAM_H_
#define _T_VB_ARBPROGRAM_H_


/* New, internal instructions:
 */
#define RSW        (VP_MAX_OPCODE)
#define MSK        (VP_MAX_OPCODE+1)
#define REL        (VP_MAX_OPCODE+2)

#define FILE_REG         0
#define FILE_LOCAL_PARAM 1
#define FILE_ENV_PARAM   2
#define FILE_STATE_PARAM 3

#define REG_ARG0   0
#define REG_ARG1   1
#define REG_ARG2   2
#define REG_RES    3
#define REG_ADDR   4
#define REG_TMP0   5
#define REG_TMP11  16
#define REG_OUT0   17
#define REG_OUT14  31
#define REG_IN0    32
#define REG_IN15   47
#define REG_ID     48		/* 0,0,0,1 */
#define REG_ONES   49		/* 1,1,1,1 */
#define REG_SWZ    50		/* -1,1,0,0 */
#define REG_NEG    51		/* -1,-1,-1,-1 */
#define REG_UNDEF  127		/* special case - never used */
#define REG_MAX    128
#define REG_INVALID ~0

/* ARB_vp instructions are broken down into one or more of the
 * following micro-instructions, each representable in a 32 bit packed
 * structure.
 */
struct reg {
   GLuint file:2;
   GLuint idx:7;
};


union instruction {
   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint file1:2;
      GLuint idx1:7;
      GLuint pad:3;
   } alu;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint neg:4;
      GLuint swz:8;		/* xyzw only */
   } rsw;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file:2;
      GLuint idx:7;
      GLuint mask:4;
      GLuint pad:1;
   } msk;

   GLuint dword;
};

#define RSW_NOOP ((0<<0) | (1<<2) | (2<<4) | (3<<6))
#define GET_RSW(swz, idx)      (((swz) >> ((idx)*2)) & 0x3)


struct input {
   GLuint idx;
   GLfloat *data;
   GLuint stride;
   GLuint size;
};

struct output {
   GLuint idx;
   GLfloat *data;
};

/*--------------------------------------------------------------------------- */

/*!
 * Private storage for the vertex program pipeline stage.
 */
struct arb_vp_machine {
   GLfloat (*File[4])[4];	/* All values referencable from the program. */
   GLint AddressReg;

   struct input input[16];
   GLuint nr_inputs;

   struct output output[15];
   GLuint nr_outputs;

   union instruction store[1024];
   union instruction *instructions;
   GLint nr_instructions;

   GLvector4f attribs[VERT_RESULT_MAX]; /**< result vectors. */
   GLvector4f ndcCoords;              /**< normalized device coords */
   GLubyte *clipmask;                 /**< clip flags */
   GLubyte ormask, andmask;           /**< for clipping */

   GLuint vtx_nr;		/**< loop counter */

   void (*func)( struct arb_vp_machine * ); /**< codegen'd program? */

   struct vertex_buffer *VB;
   GLcontext *ctx;

   GLboolean try_codegen;
};

void _tnl_disassem_vba_insn( union instruction op );

#endif
