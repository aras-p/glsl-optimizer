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


#define MAX_LOOP_NESTING 50


static GLboolean dbg = GL_FALSE;

/* Returns the mask of channels read from the given srcreg in this instruction.
 */
static GLuint
get_src_arg_mask(const struct prog_instruction *inst, int arg)
{
   int writemask = inst->DstReg.WriteMask;

   if (inst->CondUpdate)
      writemask = WRITEMASK_XYZW;

   switch (inst->Opcode) {
   case OPCODE_MOV:
   case OPCODE_ABS:
   case OPCODE_ADD:
   case OPCODE_MUL:
   case OPCODE_SUB:
      return writemask;
   case OPCODE_RCP:
   case OPCODE_SIN:
   case OPCODE_COS:
   case OPCODE_RSQ:
   case OPCODE_POW:
   case OPCODE_EX2:
      return WRITEMASK_X;
   case OPCODE_DP2:
      return WRITEMASK_XY;
   case OPCODE_DP3:
   case OPCODE_XPD:
      return WRITEMASK_XYZ;
   default:
      return WRITEMASK_XYZW;
   }
}

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
   /* Finish removing if the first instruction was to be removed. */
   if (removeCount > 0) {
      GLint removeStart = removeEnd - removeCount + 1;
      _mesa_delete_instructions(prog, removeStart, removeCount);
   }
   return totalRemoved;
}


/**
 * Remap register indexes according to map.
 * \param prog  the program to search/replace
 * \param file  the type of register file to search/replace
 * \param map  maps old register indexes to new indexes
 */
static void
replace_regs(struct gl_program *prog, gl_register_file file, const GLint map[])
{
   GLuint i;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == file) {
            GLuint index = inst->SrcReg[j].Index;
            ASSERT(map[index] >= 0);
            inst->SrcReg[j].Index = map[index];
         }
      }
      if (inst->DstReg.File == file) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(map[index] >= 0);
         inst->DstReg.Index = map[index];
      }
   }
}


/**
 * Consolidate temporary registers to use low numbers.  For example, if the
 * shader only uses temps 4, 5, 8, replace them with 0, 1, 2.
 */
static void
_mesa_consolidate_registers(struct gl_program *prog)
{
   GLboolean tempUsed[MAX_PROGRAM_TEMPS];
   GLint tempMap[MAX_PROGRAM_TEMPS];
   GLuint tempMax = 0, i;

   if (dbg) {
      printf("Optimize: Begin register consolidation\n");
   }

   memset(tempUsed, 0, sizeof(tempUsed));

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      tempMap[i] = -1;
   }

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
            /*printf("replace %u with %u\n", i, tempMap[i]);*/
         }
      }
      if (freeTemp == tempMax + 1) {
         /* no consolidation possible */
         return;
      }         
      if (dbg) {
         printf("Replace regs 0..%u with 0..%u\n", tempMax, freeTemp-1);
      }
   }

   replace_regs(prog, PROGRAM_TEMPORARY, tempMap);

   if (dbg) {
      printf("Optimize: End register consolidation\n");
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
   GLboolean tempRead[MAX_PROGRAM_TEMPS][4];
   GLboolean *removeInst; /* per-instruction removal flag */
   GLuint i, rem = 0, comp;

   memset(tempRead, 0, sizeof(tempRead));

   if (dbg) {
      printf("Optimize: Begin dead code removal\n");
      /*_mesa_print_program(prog);*/
   }

   removeInst = (GLboolean *)
      calloc(1, prog->NumInstructions * sizeof(GLboolean));

   /* Determine which temps are read and written */
   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;

      /* check src regs */
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->SrcReg[j].Index;
            GLuint read_mask;
            ASSERT(index < MAX_PROGRAM_TEMPS);
	    read_mask = get_src_arg_mask(inst, j);

            if (inst->SrcReg[j].RelAddr) {
               if (dbg)
                  printf("abort remove dead code (indirect temp)\n");
               goto done;
            }

	    for (comp = 0; comp < 4; comp++) {
	       GLuint swz = (inst->SrcReg[j].Swizzle >> (3 * comp)) & 0x7;

	       if ((read_mask & (1 << comp)) == 0)
		  continue;

	       switch (swz) {
	       case SWIZZLE_X:
		  tempRead[index][0] = GL_TRUE;
		  break;
	       case SWIZZLE_Y:
		  tempRead[index][1] = GL_TRUE;
		  break;
	       case SWIZZLE_Z:
		  tempRead[index][2] = GL_TRUE;
		  break;
	       case SWIZZLE_W:
		  tempRead[index][3] = GL_TRUE;
		  break;
	       }
	    }
         }
      }

      /* check dst reg */
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(index < MAX_PROGRAM_TEMPS);

         if (inst->DstReg.RelAddr) {
            if (dbg)
               printf("abort remove dead code (indirect temp)\n");
            goto done;
         }

         if (inst->CondUpdate) {
            /* If we're writing to this register and setting condition
             * codes we cannot remove the instruction.  Prevent removal
             * by setting the 'read' flag.
             */
            tempRead[index][0] = GL_TRUE;
            tempRead[index][1] = GL_TRUE;
            tempRead[index][2] = GL_TRUE;
            tempRead[index][3] = GL_TRUE;
         }
      }
   }

   /* find instructions that write to dead registers, flag for removal */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numDst = _mesa_num_inst_dst_regs(inst->Opcode);

      if (numDst != 0 && inst->DstReg.File == PROGRAM_TEMPORARY) {
         GLint chan, index = inst->DstReg.Index;

	 for (chan = 0; chan < 4; chan++) {
	    if (!tempRead[index][chan] &&
		inst->DstReg.WriteMask & (1 << chan)) {
	       if (dbg) {
		  printf("Remove writemask on %u.%c\n", i,
			       chan == 3 ? 'w' : 'x' + chan);
	       }
	       inst->DstReg.WriteMask &= ~(1 << chan);
	       rem++;
	    }
	 }

	 if (inst->DstReg.WriteMask == 0) {
	    /* If we cleared all writes, the instruction can be removed. */
	    if (dbg)
	       printf("Remove instruction %u: \n", i);
	    removeInst[i] = GL_TRUE;
	 }
      }
   }

   /* now remove the instructions which aren't needed */
   rem = remove_instructions(prog, removeInst);

   if (dbg) {
      printf("Optimize: End dead code removal.\n");
      printf("  %u channel writes removed\n", rem);
      printf("  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }

done:
   free(removeInst);
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

static GLboolean _mesa_is_flow_control_opcode(enum prog_opcode opcode)
{
   switch (opcode) {
   case OPCODE_BGNLOOP:
   case OPCODE_BGNSUB:
   case OPCODE_BRA:
   case OPCODE_CAL:
   case OPCODE_CONT:
   case OPCODE_IF:
   case OPCODE_ELSE:
   case OPCODE_END:
   case OPCODE_ENDIF:
   case OPCODE_ENDLOOP:
   case OPCODE_ENDSUB:
   case OPCODE_RET:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

/**
 * Try to remove use of extraneous MOV instructions, to free them up for dead
 * code removal.
 */
static void
_mesa_remove_extra_move_use(struct gl_program *prog)
{
   GLuint i, j;

   if (dbg) {
      printf("Optimize: Begin remove extra move use\n");
      _mesa_print_program(prog);
   }

   /*
    * Look for sequences such as this:
    *    MOV tmpX, arg0;
    *    ...
    *    FOO tmpY, tmpX, arg1;
    * and convert into:
    *    MOV tmpX, arg0;
    *    ...
    *    FOO tmpY, arg0, arg1;
    */

   for (i = 0; i + 1 < prog->NumInstructions; i++) {
      const struct prog_instruction *mov = prog->Instructions + i;

      if (mov->Opcode != OPCODE_MOV ||
	  mov->DstReg.File != PROGRAM_TEMPORARY ||
	  mov->DstReg.RelAddr ||
	  mov->DstReg.CondMask != COND_TR ||
	  mov->SaturateMode != SATURATE_OFF ||
	  mov->SrcReg[0].RelAddr)
	 continue;

      /* Walk through remaining instructions until the or src reg gets
       * rewritten or we get into some flow-control, eliminating the use of
       * this MOV.
       */
      for (j = i + 1; j < prog->NumInstructions; j++) {
	 struct prog_instruction *inst2 = prog->Instructions + j;
	 GLuint arg;

	 if (_mesa_is_flow_control_opcode(inst2->Opcode))
	     break;

	 /* First rewrite this instruction's args if appropriate. */
	 for (arg = 0; arg < _mesa_num_inst_src_regs(inst2->Opcode); arg++) {
	    int comp;
	    int read_mask = get_src_arg_mask(inst2, arg);

	    if (inst2->SrcReg[arg].File != mov->DstReg.File ||
		inst2->SrcReg[arg].Index != mov->DstReg.Index ||
		inst2->SrcReg[arg].RelAddr ||
		inst2->SrcReg[arg].Abs)
	       continue;

	    /* Check that all the sources for this arg of inst2 come from inst1
	     * or constants.
	     */
	    for (comp = 0; comp < 4; comp++) {
	       int src_swz = GET_SWZ(inst2->SrcReg[arg].Swizzle, comp);

	       /* If the MOV didn't write that channel, can't use it. */
	       if ((read_mask & (1 << comp)) &&
		   src_swz <= SWIZZLE_W &&
		   (mov->DstReg.WriteMask & (1 << src_swz)) == 0)
		  break;
	    }
	    if (comp != 4)
	       continue;

	    /* Adjust the swizzles of inst2 to point at MOV's source */
	    for (comp = 0; comp < 4; comp++) {
	       int inst2_swz = GET_SWZ(inst2->SrcReg[arg].Swizzle, comp);

	       if (inst2_swz <= SWIZZLE_W) {
		  GLuint s = GET_SWZ(mov->SrcReg[0].Swizzle, inst2_swz);
		  inst2->SrcReg[arg].Swizzle &= ~(7 << (3 * comp));
		  inst2->SrcReg[arg].Swizzle |= s << (3 * comp);
		  inst2->SrcReg[arg].Negate ^= (((mov->SrcReg[0].Negate >>
						  inst2_swz) & 0x1) << comp);
	       }
	    }
	    inst2->SrcReg[arg].File = mov->SrcReg[0].File;
	    inst2->SrcReg[arg].Index = mov->SrcReg[0].Index;
	 }

	 /* If this instruction overwrote part of the move, our time is up. */
	 if ((inst2->DstReg.File == mov->DstReg.File &&
	      (inst2->DstReg.RelAddr ||
	       inst2->DstReg.Index == mov->DstReg.Index)) ||
	     (inst2->DstReg.File == mov->SrcReg[0].File &&
	      (inst2->DstReg.RelAddr ||
	       inst2->DstReg.Index == mov->SrcReg[0].Index)))
	    break;
      }
   }

   if (dbg) {
      printf("Optimize: End remove extra move use.\n");
      /*_mesa_print_program(prog);*/
   }
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
      printf("Optimize: Begin remove extra moves\n");
      _mesa_print_program(prog);
   }

   removeInst = (GLboolean *)
      calloc(1, prog->NumInstructions * sizeof(GLboolean));

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
            while (prevI > 0 && removeInst[prevI])
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
                     printf("Remove MOV at %u\n", i);
                     printf("new prev inst %u: ", prevI);
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

   free(removeInst);

   if (dbg) {
      printf("Optimize: End remove extra moves.  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }
}


/** A live register interval */
struct interval
{
   GLuint Reg;         /** The temporary register index */
   GLuint Start, End;  /** Start/end instruction numbers */
};


/** A list of register intervals */
struct interval_list
{
   GLuint Num;
   struct interval Intervals[MAX_PROGRAM_TEMPS];
};


static void
append_interval(struct interval_list *list, const struct interval *inv)
{
   list->Intervals[list->Num++] = *inv;
}


/** Insert interval inv into list, sorted by interval end */
static void
insert_interval_by_end(struct interval_list *list, const struct interval *inv)
{
   /* XXX we could do a binary search insertion here since list is sorted */
   GLint i = list->Num - 1;
   while (i >= 0 && list->Intervals[i].End > inv->End) {
      list->Intervals[i + 1] = list->Intervals[i];
      i--;
   }
   list->Intervals[i + 1] = *inv;
   list->Num++;

#ifdef DEBUG
   {
      GLuint i;
      for (i = 0; i + 1 < list->Num; i++) {
         ASSERT(list->Intervals[i].End <= list->Intervals[i + 1].End);
      }
   }
#endif
}


/** Remove the given interval from the interval list */
static void
remove_interval(struct interval_list *list, const struct interval *inv)
{
   /* XXX we could binary search since list is sorted */
   GLuint k;
   for (k = 0; k < list->Num; k++) {
      if (list->Intervals[k].Reg == inv->Reg) {
         /* found, remove it */
         ASSERT(list->Intervals[k].Start == inv->Start);
         ASSERT(list->Intervals[k].End == inv->End);
         while (k < list->Num - 1) {
            list->Intervals[k] = list->Intervals[k + 1];
            k++;
         }
         list->Num--;
         return;
      }
   }
}


/** called by qsort() */
static int
compare_start(const void *a, const void *b)
{
   const struct interval *ia = (const struct interval *) a;
   const struct interval *ib = (const struct interval *) b;
   if (ia->Start < ib->Start)
      return -1;
   else if (ia->Start > ib->Start)
      return +1;
   else
      return 0;
}

/** sort the interval list according to interval starts */
static void
sort_interval_list_by_start(struct interval_list *list)
{
   qsort(list->Intervals, list->Num, sizeof(struct interval), compare_start);
#ifdef DEBUG
   {
      GLuint i;
      for (i = 0; i + 1 < list->Num; i++) {
         ASSERT(list->Intervals[i].Start <= list->Intervals[i + 1].Start);
      }
   }
#endif
}


/**
 * Update the intermediate interval info for register 'index' and
 * instruction 'ic'.
 */
static void
update_interval(GLint intBegin[], GLint intEnd[], GLuint index, GLuint ic)
{
   ASSERT(index < MAX_PROGRAM_TEMPS);
   if (intBegin[index] == -1) {
      ASSERT(intEnd[index] == -1);
      intBegin[index] = intEnd[index] = ic;
   }
   else {
      intEnd[index] = ic;
   }
}


/**
 * Find first/last instruction that references each temporary register.
 */
GLboolean
_mesa_find_temp_intervals(const struct prog_instruction *instructions,
                          GLuint numInstructions,
                          GLint intBegin[MAX_PROGRAM_TEMPS],
                          GLint intEnd[MAX_PROGRAM_TEMPS])
{
   struct loop_info
   {
      GLuint Start, End;  /**< Start, end instructions of loop */
   };
   struct loop_info loopStack[MAX_LOOP_NESTING];
   GLuint loopStackDepth = 0;
   GLuint i;

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++){
      intBegin[i] = intEnd[i] = -1;
   }

   /* Scan instructions looking for temporary registers */
   for (i = 0; i < numInstructions; i++) {
      const struct prog_instruction *inst = instructions + i;
      if (inst->Opcode == OPCODE_BGNLOOP) {
         loopStack[loopStackDepth].Start = i;
         loopStack[loopStackDepth].End = inst->BranchTarget;
         loopStackDepth++;
      }
      else if (inst->Opcode == OPCODE_ENDLOOP) {
         loopStackDepth--;
      }
      else if (inst->Opcode == OPCODE_CAL) {
         return GL_FALSE;
      }
      else {
         const GLuint numSrc = 3;/*_mesa_num_inst_src_regs(inst->Opcode);*/
         GLuint j;
         for (j = 0; j < numSrc; j++) {
            if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
               const GLuint index = inst->SrcReg[j].Index;
               if (inst->SrcReg[j].RelAddr)
                  return GL_FALSE;
               update_interval(intBegin, intEnd, index, i);
               if (loopStackDepth > 0) {
                  /* extend temp register's interval to end of loop */
                  GLuint loopEnd = loopStack[loopStackDepth - 1].End;
                  update_interval(intBegin, intEnd, index, loopEnd);
               }
            }
         }
         if (inst->DstReg.File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->DstReg.Index;
            if (inst->DstReg.RelAddr)
               return GL_FALSE;
            update_interval(intBegin, intEnd, index, i);
            if (loopStackDepth > 0) {
               /* extend temp register's interval to end of loop */
               GLuint loopEnd = loopStack[loopStackDepth - 1].End;
               update_interval(intBegin, intEnd, index, loopEnd);
            }
         }
      }
   }

   return GL_TRUE;
}


/**
 * Find the live intervals for each temporary register in the program.
 * For register R, the interval [A,B] indicates that R is referenced
 * from instruction A through instruction B.
 * Special consideration is needed for loops and subroutines.
 * \return GL_TRUE if success, GL_FALSE if we cannot proceed for some reason
 */
static GLboolean
find_live_intervals(struct gl_program *prog,
                    struct interval_list *liveIntervals)
{
   GLint intBegin[MAX_PROGRAM_TEMPS], intEnd[MAX_PROGRAM_TEMPS];
   GLuint i;

   /*
    * Note: we'll return GL_FALSE below if we find relative indexing
    * into the TEMP register file.  We can't handle that yet.
    * We also give up on subroutines for now.
    */

   if (dbg) {
      printf("Optimize: Begin find intervals\n");
   }

   /* build intermediate arrays */
   if (!_mesa_find_temp_intervals(prog->Instructions, prog->NumInstructions,
                                  intBegin, intEnd))
      return GL_FALSE;

   /* Build live intervals list from intermediate arrays */
   liveIntervals->Num = 0;
   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      if (intBegin[i] >= 0) {
         struct interval inv;
         inv.Reg = i;
         inv.Start = intBegin[i];
         inv.End = intEnd[i];
         append_interval(liveIntervals, &inv);
      }
   }

   /* Sort the list according to interval starts */
   sort_interval_list_by_start(liveIntervals);

   if (dbg) {
      /* print interval info */
      for (i = 0; i < liveIntervals->Num; i++) {
         const struct interval *inv = liveIntervals->Intervals + i;
         printf("Reg[%d] live [%d, %d]:",
                      inv->Reg, inv->Start, inv->End);
         if (1) {
            GLuint j;
            for (j = 0; j < inv->Start; j++)
               printf(" ");
            for (j = inv->Start; j <= inv->End; j++)
               printf("x");
         }
         printf("\n");
      }
   }

   return GL_TRUE;
}


/** Scan the array of used register flags to find free entry */
static GLint
alloc_register(GLboolean usedRegs[MAX_PROGRAM_TEMPS])
{
   GLuint k;
   for (k = 0; k < MAX_PROGRAM_TEMPS; k++) {
      if (!usedRegs[k]) {
         usedRegs[k] = GL_TRUE;
         return k;
      }
   }
   return -1;
}


/**
 * This function implements "Linear Scan Register Allocation" to reduce
 * the number of temporary registers used by the program.
 *
 * We compute the "live interval" for all temporary registers then
 * examine the overlap of the intervals to allocate new registers.
 * Basically, if two intervals do not overlap, they can use the same register.
 */
static void
_mesa_reallocate_registers(struct gl_program *prog)
{
   struct interval_list liveIntervals;
   GLint registerMap[MAX_PROGRAM_TEMPS];
   GLboolean usedRegs[MAX_PROGRAM_TEMPS];
   GLuint i;
   GLint maxTemp = -1;

   if (dbg) {
      printf("Optimize: Begin live-interval register reallocation\n");
      _mesa_print_program(prog);
   }

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++){
      registerMap[i] = -1;
      usedRegs[i] = GL_FALSE;
   }

   if (!find_live_intervals(prog, &liveIntervals)) {
      if (dbg)
         printf("Aborting register reallocation\n");
      return;
   }

   {
      struct interval_list activeIntervals;
      activeIntervals.Num = 0;

      /* loop over live intervals, allocating a new register for each */
      for (i = 0; i < liveIntervals.Num; i++) {
         const struct interval *live = liveIntervals.Intervals + i;

         if (dbg)
            printf("Consider register %u\n", live->Reg);

         /* Expire old intervals.  Intervals which have ended with respect
          * to the live interval can have their remapped registers freed.
          */
         {
            GLint j;
            for (j = 0; j < (GLint) activeIntervals.Num; j++) {
               const struct interval *inv = activeIntervals.Intervals + j;
               if (inv->End >= live->Start) {
                  /* Stop now.  Since the activeInterval list is sorted
                   * we know we don't have to go further.
                   */
                  break;
               }
               else {
                  /* Interval 'inv' has expired */
                  const GLint regNew = registerMap[inv->Reg];
                  ASSERT(regNew >= 0);

                  if (dbg)
                     printf("  expire interval for reg %u\n", inv->Reg);

                  /* remove interval j from active list */
                  remove_interval(&activeIntervals, inv);
                  j--;  /* counter-act j++ in for-loop above */

                  /* return register regNew to the free pool */
                  if (dbg)
                     printf("  free reg %d\n", regNew);
                  ASSERT(usedRegs[regNew] == GL_TRUE);
                  usedRegs[regNew] = GL_FALSE;
               }
            }
         }

         /* find a free register for this live interval */
         {
            const GLint k = alloc_register(usedRegs);
            if (k < 0) {
               /* out of registers, give up */
               return;
            }
            registerMap[live->Reg] = k;
            maxTemp = MAX2(maxTemp, k);
            if (dbg)
               printf("  remap register %u -> %d\n", live->Reg, k);
         }

         /* Insert this live interval into the active list which is sorted
          * by increasing end points.
          */
         insert_interval_by_end(&activeIntervals, live);
      }
   }

   if (maxTemp + 1 < (GLint) liveIntervals.Num) {
      /* OK, we've reduced the number of registers needed.
       * Scan the program and replace all the old temporary register
       * indexes with the new indexes.
       */
      replace_regs(prog, PROGRAM_TEMPORARY, registerMap);

      prog->NumTemporaries = maxTemp + 1;
   }

   if (dbg) {
      printf("Optimize: End live-interval register reallocation\n");
      printf("Num temp regs before: %u  after: %u\n",
                   liveIntervals.Num, maxTemp + 1);
      _mesa_print_program(prog);
   }
}


/**
 * Apply optimizations to the given program to eliminate unnecessary
 * instructions, temp regs, etc.
 */
void
_mesa_optimize_program(GLcontext *ctx, struct gl_program *program)
{
   _mesa_remove_extra_move_use(program);

   if (1)
      _mesa_remove_dead_code(program);

   if (0) /* not tested much yet */
      _mesa_remove_extra_moves(program);

   if (0)
      _mesa_consolidate_registers(program);
   else
      _mesa_reallocate_registers(program);
}
