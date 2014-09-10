/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "toy_compiler.h"
#include "toy_tgsi.h"
#include "toy_optimize.h"

/**
 * This just eliminates instructions with null dst so far.
 */
static void
eliminate_dead_code(struct toy_compiler *tc)
{
   struct toy_inst *inst;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case GEN6_OPCODE_IF:
      case GEN6_OPCODE_ELSE:
      case GEN6_OPCODE_ENDIF:
      case GEN6_OPCODE_WHILE:
      case GEN6_OPCODE_BREAK:
      case GEN6_OPCODE_CONT:
      case GEN6_OPCODE_SEND:
      case GEN6_OPCODE_SENDC:
      case GEN6_OPCODE_NOP:
         /* never eliminated */
         break;
      default:
         if (tdst_is_null(inst->dst) || !inst->dst.writemask) {
            /* math is always GEN6_COND_NORMAL */
            if ((inst->opcode == GEN6_OPCODE_MATH ||
                 inst->cond_modifier == GEN6_COND_NONE) &&
                !inst->acc_wr_ctrl)
               tc_discard_inst(tc, inst);
         }
         break;
      }
   }
}

void
toy_compiler_optimize(struct toy_compiler *tc)
{
   eliminate_dead_code(tc);
}
