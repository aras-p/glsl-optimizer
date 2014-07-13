/*
 * Copyright © 2012, 2013, 2014 Intel Corporation
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
 */

#include "brw_vec4.h"
#include "brw_cfg.h"

using namespace brw;

/** @file brw_vec4_cse.cpp
 *
 * Support for local common subexpression elimination.
 *
 * See Muchnick's Advanced Compiler Design and Implementation, section
 * 13.1 (p378).
 */

namespace {
struct aeb_entry : public exec_node {
   /** The instruction that generates the expression value. */
   vec4_instruction *generator;

   /** The temporary where the value is stored. */
   src_reg tmp;
};
}

static bool
is_expression(const vec4_instruction *const inst)
{
   switch (inst->opcode) {
   case BRW_OPCODE_SEL:
   case BRW_OPCODE_NOT:
   case BRW_OPCODE_AND:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_XOR:
   case BRW_OPCODE_SHR:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_CMP:
   case BRW_OPCODE_CMPN:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_MUL:
   case BRW_OPCODE_FRC:
   case BRW_OPCODE_RNDU:
   case BRW_OPCODE_RNDD:
   case BRW_OPCODE_RNDE:
   case BRW_OPCODE_RNDZ:
   case BRW_OPCODE_LINE:
   case BRW_OPCODE_PLN:
   case BRW_OPCODE_MAD:
   case BRW_OPCODE_LRP:
      return true;
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      return inst->mlen == 0;
   default:
      return false;
   }
}

static bool
is_expression_commutative(enum opcode op)
{
   switch (op) {
   case BRW_OPCODE_AND:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_XOR:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_MUL:
      return true;
   default:
      return false;
   }
}

static bool
operands_match(enum opcode op, src_reg *xs, src_reg *ys)
{
   if (!is_expression_commutative(op)) {
      return xs[0].equals(ys[0]) && xs[1].equals(ys[1]) && xs[2].equals(ys[2]);
   } else {
      return (xs[0].equals(ys[0]) && xs[1].equals(ys[1])) ||
             (xs[1].equals(ys[0]) && xs[0].equals(ys[1]));
   }
}

static bool
instructions_match(vec4_instruction *a, vec4_instruction *b)
{
   return a->opcode == b->opcode &&
          a->saturate == b->saturate &&
          a->conditional_mod == b->conditional_mod &&
          a->dst.type == b->dst.type &&
          a->dst.writemask == b->dst.writemask &&
          operands_match(a->opcode, a->src, b->src);
}

bool
vec4_visitor::opt_cse_local(bblock_t *block)
{
   bool progress = false;
   exec_list aeb;

   void *cse_ctx = ralloc_context(NULL);

   int ip = block->start_ip;
   foreach_inst_in_block (vec4_instruction, inst, block) {
      /* Skip some cases. */
      if (is_expression(inst) && !inst->predicate && inst->mlen == 0 &&
          (inst->dst.file != HW_REG || inst->dst.is_null()))
      {
         bool found = false;

         foreach_in_list_use_after(aeb_entry, entry, &aeb) {
            /* Match current instruction's expression against those in AEB. */
            if (instructions_match(inst, entry->generator)) {
               found = true;
               progress = true;
               break;
            }
         }

         if (!found) {
            /* Our first sighting of this expression.  Create an entry. */
            aeb_entry *entry = ralloc(cse_ctx, aeb_entry);
            entry->tmp = src_reg(); /* file will be BAD_FILE */
            entry->generator = inst;
            aeb.push_tail(entry);
         } else {
            /* This is at least our second sighting of this expression.
             * If we don't have a temporary already, make one.
             */
            bool no_existing_temp = entry->tmp.file == BAD_FILE;
            if (no_existing_temp && !entry->generator->dst.is_null()) {
               entry->tmp = src_reg(this, glsl_type::float_type);
               entry->tmp.type = inst->dst.type;
               entry->tmp.swizzle = BRW_SWIZZLE_XYZW;

               vec4_instruction *copy = MOV(entry->generator->dst, entry->tmp);
               entry->generator->insert_after(block, copy);
               entry->generator->dst = dst_reg(entry->tmp);
            }

            /* dest <- temp */
            if (!inst->dst.is_null()) {
               assert(inst->dst.type == entry->tmp.type);
               vec4_instruction *copy = MOV(inst->dst, entry->tmp);
               copy->force_writemask_all = inst->force_writemask_all;
               inst->insert_before(block, copy);
            }

            /* Set our iterator so that next time through the loop inst->next
             * will get the instruction in the basic block after the one we've
             * removed.
             */
            vec4_instruction *prev = (vec4_instruction *)inst->prev;

            inst->remove(block);

            /* Appending an instruction may have changed our bblock end. */
            if (inst == block->end) {
               block->end = prev;
            }

            inst = prev;
         }
      }

      foreach_in_list_safe(aeb_entry, entry, &aeb) {
         /* Kill all AEB entries that write a different value to or read from
          * the flag register if we just wrote it.
          */
         if (inst->writes_flag()) {
            if (entry->generator->reads_flag() ||
                (entry->generator->writes_flag() &&
                 !instructions_match(inst, entry->generator))) {
               entry->remove();
               ralloc_free(entry);
               continue;
            }
         }

         for (int i = 0; i < 3; i++) {
            src_reg *src = &entry->generator->src[i];

            /* Kill all AEB entries that use the destination we just
             * overwrote.
             */
            if (inst->dst.file == entry->generator->src[i].file &&
                inst->dst.reg == entry->generator->src[i].reg) {
               entry->remove();
               ralloc_free(entry);
               break;
            }

            /* Kill any AEB entries using registers that don't get reused any
             * more -- a sure sign they'll fail operands_match().
             */
            int last_reg_use = MAX2(MAX2(virtual_grf_end[src->reg * 4 + 0],
                                         virtual_grf_end[src->reg * 4 + 1]),
                                    MAX2(virtual_grf_end[src->reg * 4 + 2],
                                         virtual_grf_end[src->reg * 4 + 3]));
            if (src->file == GRF && last_reg_use < ip) {
               entry->remove();
               ralloc_free(entry);
               break;
            }
         }
      }

      ip++;
   }

   ralloc_free(cse_ctx);

   return progress;
}

bool
vec4_visitor::opt_cse()
{
   bool progress = false;

   calculate_live_intervals();

   foreach_block (block, cfg) {
      progress = opt_cse_local(block) || progress;
   }

   if (progress)
      invalidate_live_intervals(false);

   return progress;
}
