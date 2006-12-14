/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "prog_instruction.h"


/**
 * Initialize program instruction fields to defaults.
 * \param inst  first instruction to initialize
 * \param count  number of instructions to initialize
 */
void
_mesa_init_instructions(struct prog_instruction *inst, GLuint count)
{
   GLuint i;

   _mesa_bzero(inst, count * sizeof(struct prog_instruction));

   for (i = 0; i < count; i++) {
      inst[i].SrcReg[0].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      inst[i].SrcReg[1].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
      inst[i].SrcReg[2].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[2].Swizzle = SWIZZLE_NOOP;

      inst[i].DstReg.File = PROGRAM_UNDEFINED;
      inst[i].DstReg.WriteMask = WRITEMASK_XYZW;
      inst[i].DstReg.CondMask = COND_TR;
      inst[i].DstReg.CondSwizzle = SWIZZLE_NOOP;

      inst[i].SaturateMode = SATURATE_OFF;
      inst[i].Precision = FLOAT32;
   }
}


/**
 * Allocate an array of program instructions.
 * \param numInst  number of instructions
 * \return pointer to instruction memory
 */
struct prog_instruction *
_mesa_alloc_instructions(GLuint numInst)
{
   return (struct prog_instruction *)
      _mesa_calloc(numInst * sizeof(struct prog_instruction));
}


/**
 * Reallocate memory storing an array of program instructions.
 * This is used when we need to append additional instructions onto an
 * program.
 * \param oldInst  pointer to first of old/src instructions
 * \param numOldInst  number of instructions at <oldInst>
 * \param numNewInst  desired size of new instruction array.
 * \return  pointer to start of new instruction array.
 */
struct prog_instruction *
_mesa_realloc_instructions(struct prog_instruction *oldInst,
                           GLuint numOldInst, GLuint numNewInst)
{
   struct prog_instruction *newInst;

   newInst = (struct prog_instruction *)
      _mesa_realloc(oldInst,
                    numOldInst * sizeof(struct prog_instruction),
                    numNewInst * sizeof(struct prog_instruction));

   return newInst;
}


