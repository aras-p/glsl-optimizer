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

#include "pipe/p_shader_tokens.h"
#include "toy_compiler.h"
#include "toy_tgsi.h"
#include "toy_helpers.h"
#include "toy_legalize.h"

/**
 * Lower an instruction to GEN6_OPCODE_SEND(C).
 */
void
toy_compiler_lower_to_send(struct toy_compiler *tc, struct toy_inst *inst,
                           bool sendc, unsigned sfid)
{
   assert(inst->opcode >= 128);

   inst->opcode = (sendc) ? GEN6_OPCODE_SENDC : GEN6_OPCODE_SEND;

   /* thread control is reserved */
   assert(inst->thread_ctrl == 0);

   assert(inst->cond_modifier == GEN6_COND_NORMAL);
   inst->cond_modifier = sfid;
}

static int
math_op_to_func(unsigned opcode)
{
   switch (opcode) {
   case TOY_OPCODE_INV:    return GEN6_MATH_INV;
   case TOY_OPCODE_LOG:    return GEN6_MATH_LOG;
   case TOY_OPCODE_EXP:    return GEN6_MATH_EXP;
   case TOY_OPCODE_SQRT:   return GEN6_MATH_SQRT;
   case TOY_OPCODE_RSQ:    return GEN6_MATH_RSQ;
   case TOY_OPCODE_SIN:    return GEN6_MATH_SIN;
   case TOY_OPCODE_COS:    return GEN6_MATH_COS;
   case TOY_OPCODE_FDIV:   return GEN6_MATH_FDIV;
   case TOY_OPCODE_POW:    return GEN6_MATH_POW;
   case TOY_OPCODE_INT_DIV_QUOTIENT:   return GEN6_MATH_INT_DIV_QUOTIENT;
   case TOY_OPCODE_INT_DIV_REMAINDER:  return GEN6_MATH_INT_DIV_REMAINDER;
   default:
       assert(!"unknown math opcode");
       return -1;
   }
}

/**
 * Lower virtual math opcodes to GEN6_OPCODE_MATH.
 */
void
toy_compiler_lower_math(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_dst tmp;
   int i;

   /* see commit 250770b74d33bb8625c780a74a89477af033d13a */
   for (i = 0; i < Elements(inst->src); i++) {
      if (tsrc_is_null(inst->src[i]))
         break;

      /* no swizzling in align1 */
      /* XXX how about source modifiers? */
      if (toy_file_is_virtual(inst->src[i].file) &&
          !tsrc_is_swizzled(inst->src[i]) &&
          !inst->src[i].absolute &&
          !inst->src[i].negate)
         continue;

      tmp = tdst_type(tc_alloc_tmp(tc), inst->src[i].type);
      tc_MOV(tc, tmp, inst->src[i]);
      inst->src[i] = tsrc_from(tmp);
   }

   /* FC[0:3] */
   assert(inst->cond_modifier == GEN6_COND_NORMAL);
   inst->cond_modifier = math_op_to_func(inst->opcode);
   /* FC[4:5] */
   assert(inst->thread_ctrl == 0);
   inst->thread_ctrl = 0;

   inst->opcode = GEN6_OPCODE_MATH;
   tc_move_inst(tc, inst);

   /* no writemask in align1 */
   if (inst->dst.writemask != TOY_WRITEMASK_XYZW) {
      struct toy_dst dst = inst->dst;
      struct toy_inst *inst2;

      tmp = tc_alloc_tmp(tc);
      tmp.type = inst->dst.type;
      inst->dst = tmp;

      inst2 = tc_MOV(tc, dst, tsrc_from(tmp));
      inst2->pred_ctrl = inst->pred_ctrl;
   }
}

static uint32_t
absolute_imm(uint32_t imm32, enum toy_type type)
{
   union fi val = { .ui = imm32 };

   switch (type) {
   case TOY_TYPE_F:
      val.f = fabs(val.f);
      break;
   case TOY_TYPE_D:
      if (val.i < 0)
         val.i = -val.i;
      break;
   case TOY_TYPE_W:
      if ((int16_t) (val.ui & 0xffff) < 0)
         val.i = -((int16_t) (val.ui & 0xffff));
      break;
   case TOY_TYPE_V:
      assert(!"cannot take absoulte of immediates of type V");
      break;
   default:
      break;
   }

   return val.ui;
}

static uint32_t
negate_imm(uint32_t imm32, enum toy_type type)
{
   union fi val = { .ui = imm32 };

   switch (type) {
   case TOY_TYPE_F:
      val.f = -val.f;
      break;
   case TOY_TYPE_D:
   case TOY_TYPE_UD:
      val.i = -val.i;
      break;
   case TOY_TYPE_W:
   case TOY_TYPE_UW:
      val.i = -((int16_t) (val.ui & 0xffff));
      break;
   default:
      assert(!"negate immediate of unknown type");
      break;
   }

   return val.ui;
}

static void
validate_imm(struct toy_compiler *tc, struct toy_inst *inst)
{
   bool move_inst = false;
   int i;

   for (i = 0; i < Elements(inst->src); i++) {
      struct toy_dst tmp;

      if (tsrc_is_null(inst->src[i]))
         break;

      if (inst->src[i].file != TOY_FILE_IMM)
         continue;

      if (inst->src[i].absolute) {
         inst->src[i].val32 =
            absolute_imm(inst->src[i].val32, inst->src[i].type);
         inst->src[i].absolute = false;
      }

      if (inst->src[i].negate) {
         inst->src[i].val32 =
            negate_imm(inst->src[i].val32, inst->src[i].type);
         inst->src[i].negate = false;
      }

      /* this is the last operand */
      if (i + 1 == Elements(inst->src) || tsrc_is_null(inst->src[i + 1]))
         break;

      /* need to use a temp if this imm is not the last operand */
      /* TODO we should simply swap the operands if the op is commutative */
      tmp = tc_alloc_tmp(tc);
      tmp = tdst_type(tmp, inst->src[i].type);
      tc_MOV(tc, tmp, inst->src[i]);
      inst->src[i] = tsrc_from(tmp);

      move_inst = true;
   }

   if (move_inst)
      tc_move_inst(tc, inst);
}

static void
lower_opcode_mul(struct toy_compiler *tc, struct toy_inst *inst)
{
   const enum toy_type inst_type = inst->dst.type;
   const struct toy_dst acc0 =
      tdst_type(tdst(TOY_FILE_ARF, GEN6_ARF_ACC0, 0), inst_type);
   struct toy_inst *inst2;

   /* only need to take care of integer multiplications */
   if (inst_type != TOY_TYPE_UD && inst_type != TOY_TYPE_D)
      return;

   /* acc0 = (src0 & 0x0000ffff) * src1 */
   tc_MUL(tc, acc0, inst->src[0], inst->src[1]);

   /* acc0 = (src0 & 0xffff0000) * src1 + acc0 */
   inst2 = tc_add2(tc, GEN6_OPCODE_MACH, tdst_type(tdst_null(), inst_type),
         inst->src[0], inst->src[1]);
   inst2->acc_wr_ctrl = true;

   /* dst = acc0 & 0xffffffff */
   tc_MOV(tc, inst->dst, tsrc_from(acc0));

   tc_discard_inst(tc, inst);
}

static void
lower_opcode_mac(struct toy_compiler *tc, struct toy_inst *inst)
{
   const enum toy_type inst_type = inst->dst.type;

   if (inst_type != TOY_TYPE_UD && inst_type != TOY_TYPE_D) {
      const struct toy_dst acc0 = tdst(TOY_FILE_ARF, GEN6_ARF_ACC0, 0);

      tc_MOV(tc, acc0, inst->src[2]);
      inst->src[2] = tsrc_null();
      tc_move_inst(tc, inst);
   }
   else {
      struct toy_dst tmp = tdst_type(tc_alloc_tmp(tc), inst_type);
      struct toy_inst *inst2;

      inst2 = tc_MUL(tc, tmp, inst->src[0], inst->src[1]);
      lower_opcode_mul(tc, inst2);

      tc_ADD(tc, inst->dst, tsrc_from(tmp), inst->src[2]);

      tc_discard_inst(tc, inst);
   }
}

/**
 * Legalize the instructions for register allocation.
 */
void
toy_compiler_legalize_for_ra(struct toy_compiler *tc)
{
   struct toy_inst *inst;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case GEN6_OPCODE_MAC:
         lower_opcode_mac(tc, inst);
         break;
      case GEN6_OPCODE_MAD:
         /* TODO operands must be floats */
         break;
      case GEN6_OPCODE_MUL:
         lower_opcode_mul(tc, inst);
         break;
      default:
         if (inst->opcode > TOY_OPCODE_LAST_HW)
            tc_fail(tc, "internal opcodes not lowered");
      }
   }

   /* loop again as the previous pass may add new instructions */
   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      validate_imm(tc, inst);
   }
}

static void
patch_while_jip(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_inst *inst2;
   int nest_level, dist;

   nest_level = 0;
   dist = -1;

   /* search backward */
   LIST_FOR_EACH_ENTRY_FROM_REV(inst2, inst->list.prev,
         &tc->instructions, list) {
      if (inst2->marker) {
         if (inst2->opcode == TOY_OPCODE_DO) {
            if (nest_level) {
               nest_level--;
            }
            else {
               /* the following instruction */
               dist++;
               break;
            }
         }

         continue;
      }

      if (inst2->opcode == GEN6_OPCODE_WHILE)
         nest_level++;

      dist--;
   }

   if (tc->dev->gen >= ILO_GEN(7))
      inst->src[1] = tsrc_imm_w(dist * 2);
   else
      inst->dst = tdst_imm_w(dist * 2);
}

static void
patch_if_else_jip(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_inst *inst2;
   int nest_level, dist;
   int jip, uip;

   nest_level = 0;
   dist = 1;
   jip = 0;
   uip = 0;

   /* search forward */
   LIST_FOR_EACH_ENTRY_FROM(inst2, inst->list.next, &tc->instructions, list) {
      if (inst2->marker)
         continue;

      if (inst2->opcode == GEN6_OPCODE_ENDIF) {
         if (nest_level) {
            nest_level--;
         }
         else {
            uip = dist * 2;
            if (!jip)
               jip = uip;
            break;
         }
      }
      else if (inst2->opcode == GEN6_OPCODE_ELSE &&
               inst->opcode == GEN6_OPCODE_IF) {
         if (!nest_level) {
            /* the following instruction */
            jip = (dist + 1) * 2;

            if (tc->dev->gen == ILO_GEN(6)) {
               uip = jip;
               break;
            }
         }
      }
      else if (inst2->opcode == GEN6_OPCODE_IF) {
         nest_level++;
      }

      dist++;
   }

   if (tc->dev->gen >= ILO_GEN(7)) {
      /* what should the type be? */
      inst->dst.type = TOY_TYPE_D;
      inst->src[0].type = TOY_TYPE_D;
      inst->src[1] = tsrc_imm_d(uip << 16 | jip);
   }
   else {
      inst->dst = tdst_imm_w(jip);
   }

   inst->thread_ctrl = GEN6_THREADCTRL_SWITCH;
}

static void
patch_endif_jip(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_inst *inst2;
   bool found = false;
   int dist = 1;

   /* search forward for instructions that may enable channels */
   LIST_FOR_EACH_ENTRY_FROM(inst2, inst->list.next, &tc->instructions, list) {
      if (inst2->marker)
         continue;

      switch (inst2->opcode) {
      case GEN6_OPCODE_ENDIF:
      case GEN6_OPCODE_ELSE:
      case GEN6_OPCODE_WHILE:
         found = true;
         break;
      default:
         break;
      }

      if (found)
         break;

      dist++;
   }

   /* should we set dist to (dist - 1) or 1? */
   if (!found)
      dist = 1;

   if (tc->dev->gen >= ILO_GEN(7))
      inst->src[1] = tsrc_imm_w(dist * 2);
   else
      inst->dst = tdst_imm_w(dist * 2);

   inst->thread_ctrl = GEN6_THREADCTRL_SWITCH;
}

static void
patch_break_continue_jip(struct toy_compiler *tc, struct toy_inst *inst)
{
   struct toy_inst *inst2, *inst3;
   int nest_level, dist, jip, uip;

   nest_level = 0;
   dist = 1;
   jip = 1 * 2;
   uip = 1 * 2;

   /* search forward */
   LIST_FOR_EACH_ENTRY_FROM(inst2, inst->list.next, &tc->instructions, list) {
      if (inst2->marker) {
         if (inst2->opcode == TOY_OPCODE_DO)
            nest_level++;
         continue;
      }

      if (inst2->opcode == GEN6_OPCODE_ELSE ||
          inst2->opcode == GEN6_OPCODE_ENDIF ||
          inst2->opcode == GEN6_OPCODE_WHILE) {
         jip = dist * 2;
         break;
      }

      dist++;
   }

   /* go on to determine uip */
   inst3 = inst2;
   LIST_FOR_EACH_ENTRY_FROM(inst2, &inst3->list, &tc->instructions, list) {
      if (inst2->marker) {
         if (inst2->opcode == TOY_OPCODE_DO)
            nest_level++;
         continue;
      }

      if (inst2->opcode == GEN6_OPCODE_WHILE) {
         if (nest_level) {
            nest_level--;
         }
         else {
            /* the following instruction */
            if (tc->dev->gen == ILO_GEN(6) && inst->opcode == GEN6_OPCODE_BREAK)
               dist++;

            uip = dist * 2;
            break;
         }
      }

      dist++;
   }

   /* should the type be D or W? */
   inst->dst.type = TOY_TYPE_D;
   inst->src[0].type = TOY_TYPE_D;
   inst->src[1] = tsrc_imm_d(uip << 16 | jip);
}

/**
 * Legalize the instructions for assembling.
 */
void
toy_compiler_legalize_for_asm(struct toy_compiler *tc)
{
   struct toy_inst *inst;
   int pc = 0;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      int i;

      pc++;

      /*
       * From the Sandy Bridge PRM, volume 4 part 2, page 112:
       *
       *     "Specifically, for instructions with a single source, it only
       *      uses the first source operand <src0>. In this case, the second
       *      source operand <src1> must be set to null and also with the same
       *      type as the first source operand <src0>.  It is a special case
       *      when <src0> is an immediate, as an immediate <src0> uses DW3 of
       *      the instruction word, which is normally used by <src1>.  In this
       *      case, <src1> must be programmed with register file ARF and the
       *      same data type as <src0>."
       *
       * Since we already fill unused operands with null, we only need to take
       * care of the type.
       */
      if (tsrc_is_null(inst->src[1]))
         inst->src[1].type = inst->src[0].type;

      switch (inst->opcode) {
      case GEN6_OPCODE_MATH:
         /* math does not support align16 nor exec_size > 8 */
         inst->access_mode = GEN6_ALIGN_1;

         if (inst->exec_size == GEN6_EXECSIZE_16) {
            /*
             * From the Ivy Bridge PRM, volume 4 part 3, page 192:
             *
             *     "INT DIV function does not support SIMD16."
             */
            if (tc->dev->gen < ILO_GEN(7) ||
                inst->cond_modifier == GEN6_MATH_INT_DIV_QUOTIENT ||
                inst->cond_modifier == GEN6_MATH_INT_DIV_REMAINDER) {
               struct toy_inst *inst2;

               inst->exec_size = GEN6_EXECSIZE_8;
               inst->qtr_ctrl = GEN6_QTRCTRL_1Q;

               inst2 = tc_duplicate_inst(tc, inst);
               inst2->qtr_ctrl = GEN6_QTRCTRL_2Q;
               inst2->dst = tdst_offset(inst2->dst, 1, 0);
               inst2->src[0] = tsrc_offset(inst2->src[0], 1, 0);
               if (!tsrc_is_null(inst2->src[1]))
                  inst2->src[1] = tsrc_offset(inst2->src[1], 1, 0);

               pc++;
            }
         }
         break;
      case GEN6_OPCODE_IF:
         if (tc->dev->gen >= ILO_GEN(7) &&
             inst->cond_modifier != GEN6_COND_NORMAL) {
            struct toy_inst *inst2;

            inst2 = tc_duplicate_inst(tc, inst);

            /* replace the original IF by CMP */
            inst->opcode = GEN6_OPCODE_CMP;

            /* predicate control instead of condition modifier */
            inst2->dst = tdst_null();
            inst2->src[0] = tsrc_null();
            inst2->src[1] = tsrc_null();
            inst2->cond_modifier = GEN6_COND_NORMAL;
            inst2->pred_ctrl = GEN6_PREDCTRL_NORMAL;

            pc++;
         }
         break;
      default:
         break;
      }

      /* MRF to GRF */
      if (tc->dev->gen >= ILO_GEN(7)) {
         for (i = 0; i < Elements(inst->src); i++) {
            if (inst->src[i].file != TOY_FILE_MRF)
               continue;
            else if (tsrc_is_null(inst->src[i]))
               break;

            inst->src[i].file = TOY_FILE_GRF;
         }

         if (inst->dst.file == TOY_FILE_MRF)
            inst->dst.file = TOY_FILE_GRF;
      }
   }

   tc->num_instructions = pc;

   /* set JIP/UIP */
   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case GEN6_OPCODE_IF:
      case GEN6_OPCODE_ELSE:
         patch_if_else_jip(tc, inst);
         break;
      case GEN6_OPCODE_ENDIF:
         patch_endif_jip(tc, inst);
         break;
      case GEN6_OPCODE_WHILE:
         patch_while_jip(tc, inst);
         break;
      case GEN6_OPCODE_BREAK:
      case GEN6_OPCODE_CONT:
         patch_break_continue_jip(tc, inst);
         break;
      default:
         break;
      }
   }
}
