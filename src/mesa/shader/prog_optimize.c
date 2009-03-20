/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_optimize.h"
#include "prog_print.h"


static GLboolean dbg = GL_FALSE;


/**
 * In 'prog' remove instruction[i] if removeFlags[i] == TRUE.
 * \return number of instructions removed
 */
static GLuint
remove_instructions(struct gl_program *prog, const GLboolean *removeFlags)
{
   GLint i, removeEnd = 0, removeCount = 0;
   GLuint totalRemoved = 0;

   /* go backward */
   for (i = prog->NumInstructions - 1; i >= 0; i--) {
      if (removeFlags[i]) {
         totalRemoved++;
         if (removeCount == 0) {
            /* begin a run of instructions to remove */
            removeEnd = i;
            removeCount = 1;
         }
         else {
            /* extend the run of instructions to remove */
            removeCount++;
         }
      }
      else {
         /* don't remove this instruction, but check if the preceeding
          * instructions are to be removed.
          */
         if (removeCount > 0) {
            GLint removeStart = removeEnd - removeCount + 1;
            _mesa_delete_instructions(prog, removeStart, removeCount);
            removeStart = removeCount = 0; /* reset removal info */
         }
      }
   }
   return totalRemoved;
}


/**
 * Consolidate temporary registers to use low numbers.  For example, if the
 * shader only uses temps 4, 5, 8, replace them with 0, 1, 2.
 */
static void
_mesa_consolidate_registers(struct gl_program *prog)
{
   GLboolean tempUsed[MAX_PROGRAM_TEMPS];
   GLuint tempMap[MAX_PROGRAM_TEMPS];
   GLuint tempMax = 0, i;

   if (dbg) {
      _mesa_printf("Optimize: Begin register consolidation\n");
   }

   memset(tempUsed, 0, sizeof(tempUsed));

   /* set tempUsed[i] if temporary [i] is referenced */
   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->SrcReg[j].Index;
            ASSERT(index < MAX_PROGRAM_TEMPS);
            tempUsed[index] = GL_TRUE;
            tempMax = MAX2(tempMax, index);
            break;
         }
      }
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(index < MAX_PROGRAM_TEMPS);
         tempUsed[index] = GL_TRUE;
         tempMax = MAX2(tempMax, index);
      }
   }

   /* allocate a new index for each temp that's used */
   {
      GLuint freeTemp = 0;
      for (i = 0; i <= tempMax; i++) {
         if (tempUsed[i]) {
            tempMap[i] = freeTemp++;
            /*_mesa_printf("replace %u with %u\n", i, tempMap[i]);*/
         }
      }
      if (freeTemp == tempMax + 1) {
         /* no consolidation possible */
         return;
      }         
      if (dbg) {
         _mesa_printf("Replace regs 0..%u with 0..%u\n", tempMax, freeTemp-1);
      }
   }

   /* now replace occurances of old temp indexes with new indexes */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            GLuint index = inst->SrcReg[j].Index;
            assert(index <= tempMax);
            assert(tempUsed[index]);
            inst->SrcReg[j].Index = tempMap[index];
         }
      }
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         const GLuint index = inst->DstReg.Index;
         assert(tempUsed[index]);
         assert(index <= tempMax);
         inst->DstReg.Index = tempMap[index];
      }
   }
   if (dbg) {
      _mesa_printf("Optimize: End register consolidation\n");
   }
}


/**
 * Remove dead instructions from the given program.
 * This is very primitive for now.  Basically look for temp registers
 * that are written to but never read.  Remove any instructions that
 * write to such registers.  Be careful with condition code setters.
 */
static void
_mesa_remove_dead_code(struct gl_program *prog)
{
   GLboolean tempWritten[MAX_PROGRAM_TEMPS], tempRead[MAX_PROGRAM_TEMPS];
   GLboolean *removeInst; /* per-instruction removal flag */
   GLuint i, rem;

   memset(tempWritten, 0, sizeof(tempWritten));
   memset(tempRead, 0, sizeof(tempRead));

   if (dbg) {
      _mesa_printf("Optimize: Begin dead code removal\n");
      /*_mesa_print_program(prog);*/
   }

   removeInst = (GLboolean *)
      _mesa_calloc(prog->NumInstructions * sizeof(GLboolean));

   /* Determine which temps are read and written */
   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;

      /* check src regs */
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->SrcReg[j].Index;
            ASSERT(index < MAX_PROGRAM_TEMPS);

            if (inst->SrcReg[j].RelAddr) {
               if (dbg)
                  _mesa_printf("abort remove dead code (indirect temp)\n");
               return;
            }

            tempRead[index] = GL_TRUE;
         }
      }

      /* check dst reg */
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(index < MAX_PROGRAM_TEMPS);

         if (inst->DstReg.RelAddr) {
            if (dbg)
               _mesa_printf("abort remove dead code (indirect temp)\n");
            return;
         }

         tempWritten[index] = GL_TRUE;
         if (inst->CondUpdate) {
            /* If we're writing to this register and setting condition
             * codes we cannot remove the instruction.  Prevent removal
             * by setting the 'read' flag.
             */
            tempRead[index] = GL_TRUE;
         }
      }
   }

   if (dbg) {
      for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
         if (tempWritten[i] && !tempRead[i])
            _mesa_printf("Remove writes to tmp %u\n", i);
      }
   }

   /* find instructions that write to dead registers, flag for removal */
   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         GLint index = inst->DstReg.Index;
         removeInst[i] = (tempWritten[index] && !tempRead[index]);
         if (dbg && removeInst[i]) {
            _mesa_printf("Remove inst %u: ", i);
            _mesa_print_instruction(inst);
         }
      }
   }

   /* now remove the instructions which aren't needed */
   rem = remove_instructions(prog, removeInst);

   _mesa_free(removeInst);

   if (dbg) {
      _mesa_printf("Optimize: End dead code removal.  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }
}


enum temp_use
{
   READ,
   WRITE,
   FLOW,
   END
};

/**
 * Scan forward in program from 'start' for the next occurance of TEMP[index].
 * Return READ, WRITE, FLOW or END to indicate the next usage or an indicator
 * that we can't look further.
 */
static enum temp_use
find_next_temp_use(const struct gl_program *prog, GLuint start, GLuint index)
{
   GLuint i;

   for (i = start; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      switch (inst->Opcode) {
      case OPCODE_BGNLOOP:
      case OPCODE_ENDLOOP:
      case OPCODE_BGNSUB:
      case OPCODE_ENDSUB:
         return FLOW;
      default:
         {
            const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
            GLuint j;
            for (j = 0; j < numSrc; j++) {
               if (inst->SrcReg[j].File == PROGRAM_TEMPORARY &&
                   inst->SrcReg[j].Index == index)
                  return READ;
            }
            if (inst->DstReg.File == PROGRAM_TEMPORARY &&
                inst->DstReg.Index == index)
               return WRITE;
         }
      }
   }

   return END;
}


/**
 * Try to remove extraneous MOV instructions from the given program.
 */
static void
_mesa_remove_extra_moves(struct gl_program *prog)
{
   GLboolean *removeInst; /* per-instruction removal flag */
   GLuint i, rem, loopNesting = 0, subroutineNesting = 0;

   if (dbg) {
      _mesa_printf("Optimize: Begin remove extra moves\n");
      _mesa_print_program(prog);
   }

   removeInst = (GLboolean *)
      _mesa_calloc(prog->NumInstructions * sizeof(GLboolean));

   /*
    * Look for sequences such as this:
    *    FOO tmpX, arg0, arg1;
    *    MOV tmpY, tmpX;
    * and convert into:
    *    FOO tmpY, arg0, arg1;
    */

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;

      switch (inst->Opcode) {
      case OPCODE_BGNLOOP:
         loopNesting++;
         break;
      case OPCODE_ENDLOOP:
         loopNesting--;
         break;
      case OPCODE_BGNSUB:
         subroutineNesting++;
         break;
      case OPCODE_ENDSUB:
         subroutineNesting--;
         break;
      case OPCODE_MOV:
         if (i > 0 &&
             loopNesting == 0 &&
             subroutineNesting == 0 &&
             inst->SrcReg[0].File == PROGRAM_TEMPORARY &&
             inst->SrcReg[0].Swizzle == SWIZZLE_XYZW) {
            /* see if this MOV can be removed */
            const GLuint tempIndex = inst->SrcReg[0].Index;
            struct prog_instruction *prevInst;
            GLuint prevI;

            /* get pointer to previous instruction */
            prevI = i - 1;
            while (removeInst[prevI] && prevI > 0)
               prevI--;
            prevInst = prog->Instructions + prevI;

            if (prevInst->DstReg.File == PROGRAM_TEMPORARY &&
                prevInst->DstReg.Index == tempIndex &&
                prevInst->DstReg.WriteMask == WRITEMASK_XYZW) {

               enum temp_use next_use =
                  find_next_temp_use(prog, i + 1, tempIndex);

               if (next_use == WRITE || next_use == END) {
                  /* OK, we can safely remove this MOV instruction.
                   * Transform:
                   *   prevI: FOO tempIndex, x, y;
                   *       i: MOV z, tempIndex;
                   * Into:
                   *   prevI: FOO z, x, y;
                   */

                  /* patch up prev inst */
                  prevInst->DstReg.File = inst->DstReg.File;
                  prevInst->DstReg.Index = inst->DstReg.Index;

                  /* flag this instruction for removal */
                  removeInst[i] = GL_TRUE;

                  if (dbg) {
                     _mesa_printf("Remove MOV at %u\n", i);
                     _mesa_printf("new prev inst %u: ", prevI);
                     _mesa_print_instruction(prevInst);
                  }
               }
            }
         }
         break;
      default:
         ; /* nothing */
      }
   }

   /* now remove the instructions which aren't needed */
   rem = remove_instructions(prog, removeInst);

   if (dbg) {
      _mesa_printf("Optimize: End remove extra moves.  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }
}


/**
 * Apply optimizations to the given program to eliminate unnecessary
 * instructions, temp regs, etc.
 */
void
_mesa_optimize_program(GLcontext *ctx, struct gl_program *program)
{
   if (1)
      _mesa_remove_dead_code(program);

   if (0) /* not test much yet */
      _mesa_remove_extra_moves(program);

   if (1)
      _mesa_consolidate_registers(program);
}
