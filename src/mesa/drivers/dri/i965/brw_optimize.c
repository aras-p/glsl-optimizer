/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "main/macros.h"
#include "program/program.h"
#include "program/prog_print.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"

const struct brw_instruction_info brw_opcodes[128] = {
    [BRW_OPCODE_MOV] = { .name = "mov", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_FRC] = { .name = "frc", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDU] = { .name = "rndu", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDD] = { .name = "rndd", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDE] = { .name = "rnde", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDZ] = { .name = "rndz", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_NOT] = { .name = "not", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_LZD] = { .name = "lzd", .nsrc = 1, .ndst = 1 },

    [BRW_OPCODE_MUL] = { .name = "mul", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MAC] = { .name = "mac", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MACH] = { .name = "mach", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_LINE] = { .name = "line", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_PLN] = { .name = "pln", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SAD2] = { .name = "sad2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SADA2] = { .name = "sada2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP4] = { .name = "dp4", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DPH] = { .name = "dph", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP3] = { .name = "dp3", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP2] = { .name = "dp2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MATH] = { .name = "math", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_AVG] = { .name = "avg", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_ADD] = { .name = "add", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SEL] = { .name = "sel", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_AND] = { .name = "and", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_OR] = { .name = "or", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_XOR] = { .name = "xor", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SHR] = { .name = "shr", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SHL] = { .name = "shl", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_ASR] = { .name = "asr", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMP] = { .name = "cmp", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMPN] = { .name = "cmpn", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_SEND] = { .name = "send", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_NOP] = { .name = "nop", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_JMPI] = { .name = "jmpi", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_IF] = { .name = "if", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_IFF] = { .name = "iff", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_WHILE] = { .name = "while", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_ELSE] = { .name = "else", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_BREAK] = { .name = "break", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_CONTINUE] = { .name = "cont", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_HALT] = { .name = "halt", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_MSAVE] = { .name = "msave", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_PUSH] = { .name = "push", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_MRESTORE] = { .name = "mrest", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_POP] = { .name = "pop", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_WAIT] = { .name = "wait", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_DO] = { .name = "do", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_ENDIF] = { .name = "endif", .nsrc = 2, .ndst = 0 },
};

static bool
is_single_channel_dp4(struct brw_instruction *insn)
{
   if (insn->header.opcode != BRW_OPCODE_DP4 ||
       insn->header.execution_size != BRW_EXECUTE_8 ||
       insn->header.access_mode != BRW_ALIGN_16 ||
       insn->bits1.da1.dest_reg_file != BRW_GENERAL_REGISTER_FILE)
      return false;

   if (!is_power_of_two(insn->bits1.da16.dest_writemask))
      return false;

   return true;
}

/**
 * Sets the dependency control fields on DP4 instructions.
 *
 * The hardware only tracks dependencies on a register basis, so when
 * you do:
 *
 * DP4 dst.x src1 src2
 * DP4 dst.y src1 src3
 * DP4 dst.z src1 src4
 * DP4 dst.w src1 src5
 *
 * It will wait to do the DP4 dst.y until the dst.x is resolved, etc.
 * We can examine our instruction stream and set the dependency
 * control fields to tell the hardware when to do it.
 *
 * We may want to extend this to other instructions that are used to
 * fill in a channel at a time of the destination register.
 */
static void
brw_set_dp4_dependency_control(struct brw_compile *p)
{
   int i;

   for (i = 1; i < p->nr_insn; i++) {
      struct brw_instruction *insn = &p->store[i];
      struct brw_instruction *prev = &p->store[i - 1];

      if (!is_single_channel_dp4(prev))
	 continue;

      if (!is_single_channel_dp4(insn)) {
	 i++;
	 continue;
      }

      /* Only avoid hw dep control if the write masks are different
       * channels of one reg.
       */
      if (insn->bits1.da16.dest_writemask == prev->bits1.da16.dest_writemask)
	 continue;
      if (insn->bits1.da16.dest_reg_nr != prev->bits1.da16.dest_reg_nr)
	 continue;

      /* Check if the second instruction depends on the previous one
       * for a src.
       */
      if (insn->bits1.da1.src0_reg_file == BRW_GENERAL_REGISTER_FILE &&
	  (insn->bits2.da1.src0_address_mode != BRW_ADDRESS_DIRECT ||
	   insn->bits2.da1.src0_reg_nr == insn->bits1.da16.dest_reg_nr))
	  continue;
      if (insn->bits1.da1.src1_reg_file == BRW_GENERAL_REGISTER_FILE &&
	  (insn->bits3.da1.src1_address_mode != BRW_ADDRESS_DIRECT ||
	   insn->bits3.da1.src1_reg_nr == insn->bits1.da16.dest_reg_nr))
	  continue;

      prev->header.dependency_control |= BRW_DEPENDENCY_NOTCLEARED;
      insn->header.dependency_control |= BRW_DEPENDENCY_NOTCHECKED;
   }
}

void
brw_optimize(struct brw_compile *p)
{
   brw_set_dp4_dependency_control(p);
}
