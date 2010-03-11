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
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"

static GLboolean
is_single_channel_dp4(struct brw_instruction *insn)
{
   if (insn->header.opcode != BRW_OPCODE_DP4 ||
       insn->header.execution_size != BRW_EXECUTE_8 ||
       insn->header.access_mode != BRW_ALIGN_16 ||
       insn->bits1.da1.dest_reg_file != BRW_GENERAL_REGISTER_FILE)
      return GL_FALSE;

   if (!is_power_of_two(insn->bits1.da16.dest_writemask))
      return GL_FALSE;

   return GL_TRUE;
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
