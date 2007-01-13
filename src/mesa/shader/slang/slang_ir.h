/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * \file slang_ir.h
 * Mesa GLSL Intermediate Representation tree types and constants.
 * \author Brian Paul
 */


#ifndef SLANG_IR_H
#define SLANG_IR_H


#include "imports.h"
#include "slang_compile.h"
#include "mtypes.h"


/**
 * Intermediate Representation opcode
 */
typedef enum
{
   IR_NOP = 0,
   IR_SEQ,     /* sequence (eval left, then right) */
   IR_SCOPE,   /* new variable scope (one child) */
   IR_LABEL,   /* target of a jump or cjump */
   IR_JUMP,    /* unconditional jump */
   IR_CJUMP,   /* conditional jump */
   IR_COND,    /* conditional expression */
   IR_CALL,    /* call subroutine */
   IR_MOVE,
   IR_ADD,
   IR_SUB,
   IR_MUL,
   IR_DIV,
   IR_DOT4,
   IR_DOT3,
   IR_CROSS,   /* vec3 cross product */
   IR_MIN,
   IR_MAX,
   IR_SEQUAL,  /* Set if not equal */
   IR_SNEQUAL, /* Set if equal */
   IR_SGE,     /* Set if greater or equal */
   IR_SGT,     /* Set if greater than */
   IR_POW,     /* x^y */
   IR_EXP,     /* e^x */
   IR_EXP2,    /* 2^x */
   IR_LOG2,    /* log base 2 */
   IR_RSQ,     /* 1/sqrt() */
   IR_RCP,     /* reciprocol */
   IR_FLOOR,
   IR_FRAC,
   IR_ABS,     /* absolute value */
   IR_NEG,     /* negate */
   IR_DDX,     /* derivative w.r.t. X */
   IR_DDY,     /* derivative w.r.t. Y */
   IR_SIN,     /* sine */
   IR_COS,     /* cosine */
   IR_NOT,     /* logical not */
   IR_VAR,     /* variable reference */
   IR_VAR_DECL,/* var declaration */
   IR_TEX,     /* texture lookup */
   IR_TEXB,    /* texture lookup with LOD bias */
   IR_TEXP,    /* texture lookup with projection */
   IR_FLOAT,
   IR_FIELD,
   IR_I_TO_F
} slang_ir_opcode;


/**
 * Describes where data storage is allocated.
 */
typedef struct
{
   enum register_file File;  /**< PROGRAM_TEMPORARY, PROGRAM_INPUT, etc */
   GLint Index;  /**< -1 means unallocated */
   GLint Size;  /**< number of floats */
} slang_ir_storage;


/**
 * Intermediate Representation (IR) tree node
 */
typedef struct slang_ir_node_
{
   slang_ir_opcode Opcode;
   struct slang_ir_node_ *Children[2];
   const char *Comment;
   const char *Target;
   GLuint Swizzle;
   GLuint Writemask;  /**< If Opcode == IR_MOVE */
   GLfloat Value[4];    /**< If Opcode == IR_FLOAT */
   slang_variable *Var;
   slang_ir_storage *Store;
} slang_ir_node;


#endif /* SLANG_IR_H */
