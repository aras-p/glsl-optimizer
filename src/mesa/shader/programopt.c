/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file  programopt.c 
 * Vertex/Fragment program optimizations and transformations for program
 * options, etc.
 *
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "program.h"
#include "programopt.h"
#include "program_instruction.h"


/**
 * This function inserts instructions for coordinate modelview * projection
 * into a vertex program.
 * May be used to implement the position_invariant option.
 */
void
_mesa_insert_mvp_code(GLcontext *ctx, struct gl_vertex_program *vprog)
{
   struct prog_instruction *newInst;
   const GLuint origLen = vprog->Base.NumInstructions;
   const GLuint newLen = origLen + 4;
   GLuint i;

   /*
    * Setup state references for the modelview/projection matrix.
    * XXX we should check if these state vars are already declared.
    */
   static const GLint mvpState[4][5] = {
      { STATE_MATRIX, STATE_MVP, 0, 0, 0 },  /* state.matrix.mvp.row[0] */
      { STATE_MATRIX, STATE_MVP, 0, 1, 1 },  /* state.matrix.mvp.row[1] */
      { STATE_MATRIX, STATE_MVP, 0, 2, 2 },  /* state.matrix.mvp.row[2] */
      { STATE_MATRIX, STATE_MVP, 0, 3, 3 },  /* state.matrix.mvp.row[3] */
   };
   GLint mvpRef[4];

   for (i = 0; i < 4; i++) {
      mvpRef[i] = _mesa_add_state_reference(vprog->Base.Parameters,
                                            mvpState[i]);
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting position_invariant code)");
      return;
   }

   /*
    * Generated instructions:
    * newInst[0] = DP4 result.position.x, mvp.row[0], vertex.position;
    * newInst[1] = DP4 result.position.y, mvp.row[1], vertex.position;
    * newInst[2] = DP4 result.position.z, mvp.row[2], vertex.position;
    * newInst[3] = DP4 result.position.w, mvp.row[3], vertex.position;
    */
   for (i = 0; i < 4; i++) {
      _mesa_init_instruction(newInst + i);
      newInst[i].Opcode = OPCODE_DP4;
      newInst[i].DstReg.File = PROGRAM_OUTPUT;
      newInst[i].DstReg.Index = VERT_RESULT_HPOS;
      newInst[i].DstReg.WriteMask = (WRITEMASK_X << i);
      newInst[i].SrcReg[0].File = PROGRAM_STATE_VAR;
      newInst[i].SrcReg[0].Index = mvpRef[i];
      newInst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      newInst[i].SrcReg[1].File = PROGRAM_INPUT;
      newInst[i].SrcReg[1].Index = VERT_ATTRIB_POS;
      newInst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
   }

   /* Append original instructions after new instructions */
   _mesa_memcpy(newInst + 4, vprog->Base.Instructions,
                origLen * sizeof(struct prog_instruction));

   /* free old instructions */
   _mesa_free(vprog->Base.Instructions);

   /* install new instructions */
   vprog->Base.Instructions = newInst;
   vprog->Base.NumInstructions = newLen;
   vprog->Base.InputsRead |= VERT_BIT_POS;
   vprog->Base.OutputsWritten |= (1 << VERT_RESULT_HPOS);
}



/**
 * Append extra instructions onto the given fragment program to implement
 * the fog mode specified by program->FogOption.
 * XXX incomplete.
 */
void
_mesa_append_fog_code(GLcontext *ctx, struct gl_fragment_program *fprog)
{
   struct prog_instruction newInst[10];

   switch (fprog->FogOption) {
   case GL_LINEAR:
      /* lerp */
      break;
   case GL_EXP:
      break;
   case GL_EXP2:
      break;
   case GL_NONE:
      /* no-op */
      return;
   default:
      _mesa_problem(ctx, "Invalid fog option");
      return;
   }


   (void) newInst;

}
