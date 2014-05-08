/*
 * Copyright © 2012 Intel Corporation
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

/** @file gen8_generator.cpp
 *
 * Code generation for Gen8+ hardware, replacing the brw_eu_emit.c layer.
 */

extern "C" {
#include "main/compiler.h"
#include "main/macros.h"
#include "brw_context.h"
} /* extern "C" */

#include "glsl/ralloc.h"
#include "brw_eu.h"
#include "brw_reg.h"
#include "gen8_generator.h"

gen8_generator::gen8_generator(struct brw_context *brw,
                               struct gl_shader_program *shader_prog,
                               struct gl_program *prog,
                               void *mem_ctx)
   : shader_prog(shader_prog), prog(prog), brw(brw), mem_ctx(mem_ctx)
{
   ctx = &brw->ctx;

   memset(&default_state, 0, sizeof(default_state));
   default_state.mask_control = BRW_MASK_ENABLE;

   store_size = 1024;
   store = rzalloc_array(mem_ctx, gen8_instruction, store_size);
   nr_inst = 0;
   next_inst_offset = 0;

   /* Set up the control flow stacks. */
   if_stack_depth = 0;
   if_stack_array_size = 16;
   if_stack = rzalloc_array(mem_ctx, int, if_stack_array_size);

   loop_stack_depth = 0;
   loop_stack_array_size = 16;
   loop_stack = rzalloc_array(mem_ctx, int, loop_stack_array_size);
}

gen8_generator::~gen8_generator()
{
}

gen8_instruction *
gen8_generator::next_inst(unsigned opcode)
{
   gen8_instruction *inst;

   if (nr_inst + 1 > unsigned(store_size)) {
      store_size <<= 1;
      store = reralloc(mem_ctx, store, gen8_instruction, store_size);
      assert(store);
   }

   next_inst_offset += 16;
   inst = &store[nr_inst++];

   memset(inst, 0, sizeof(gen8_instruction));

   gen8_set_opcode(inst, opcode);
   gen8_set_exec_size(inst, default_state.exec_size);
   gen8_set_access_mode(inst, default_state.access_mode);
   gen8_set_mask_control(inst, default_state.mask_control);
   gen8_set_qtr_control(inst, default_state.qtr_control);
   gen8_set_cond_modifier(inst, default_state.conditional_mod);
   gen8_set_pred_control(inst, default_state.predicate);
   gen8_set_pred_inv(inst, default_state.predicate_inverse);
   gen8_set_saturate(inst, default_state.saturate);
   gen8_set_flag_subreg_nr(inst, default_state.flag_subreg_nr);
   return inst;
}

#define ALU1(OP)                                           \
gen8_instruction *                                         \
gen8_generator::OP(struct brw_reg dst, struct brw_reg src) \
{                                                          \
   gen8_instruction *inst = next_inst(BRW_OPCODE_##OP);    \
   gen8_set_dst(brw, inst, dst);                           \
   gen8_set_src0(brw, inst, src);                          \
   return inst;                                            \
}

#define ALU2(OP)                                                             \
gen8_instruction *                                                           \
gen8_generator::OP(struct brw_reg dst, struct brw_reg s0, struct brw_reg s1) \
{                                                                            \
   gen8_instruction *inst = next_inst(BRW_OPCODE_##OP);                      \
   gen8_set_dst(brw, inst, dst);                                             \
   gen8_set_src0(brw, inst, s0);                                             \
   gen8_set_src1(brw, inst, s1);                                             \
   return inst;                                                              \
}

#define ALU2_ACCUMULATE(OP)                                                  \
gen8_instruction *                                                           \
gen8_generator::OP(struct brw_reg dst, struct brw_reg s0, struct brw_reg s1) \
{                                                                            \
   gen8_instruction *inst = next_inst(BRW_OPCODE_##OP);                      \
   gen8_set_dst(brw, inst, dst);                                             \
   gen8_set_src0(brw, inst, s0);                                             \
   gen8_set_src1(brw, inst, s1);                                             \
   gen8_set_acc_wr_control(inst, true);                                      \
   return inst;                                                              \
}

#define ALU3(OP)                                          \
gen8_instruction *                                        \
gen8_generator::OP(struct brw_reg dst, struct brw_reg s0, \
                   struct brw_reg s1,  struct brw_reg s2) \
{                                                         \
   return alu3(BRW_OPCODE_##OP, dst, s0, s1, s2);         \
}

#define ALU3F(OP) \
gen8_instruction *                                        \
gen8_generator::OP(struct brw_reg dst, struct brw_reg s0, \
                   struct brw_reg s1,  struct brw_reg s2) \
{                                                         \
   assert(dst.type == BRW_REGISTER_TYPE_F);               \
   assert(s0.type == BRW_REGISTER_TYPE_F);                \
   assert(s1.type == BRW_REGISTER_TYPE_F);                \
   assert(s2.type == BRW_REGISTER_TYPE_F);                \
   return alu3(BRW_OPCODE_##OP, dst, s0, s1, s2);         \
}

ALU2(ADD)
ALU2(AND)
ALU2(ASR)
ALU3(BFE)
ALU2(BFI1)
ALU3(BFI2)
ALU1(BFREV)
ALU1(CBIT)
ALU2_ACCUMULATE(ADDC)
ALU2_ACCUMULATE(SUBB)
ALU2(DP2)
ALU2(DP3)
ALU2(DP4)
ALU2(DPH)
ALU1(FBH)
ALU1(FBL)
ALU1(FRC)
ALU2(LINE)
ALU3F(LRP)
ALU3F(MAD)
ALU2(MUL)
ALU1(MOV)
ALU1(NOT)
ALU2(OR)
ALU2(PLN)
ALU1(RNDD)
ALU1(RNDE)
ALU1(RNDZ)
ALU2_ACCUMULATE(MAC)
ALU2_ACCUMULATE(MACH)
ALU2(SEL)
ALU2(SHL)
ALU2(SHR)
ALU2(XOR)

gen8_instruction *
gen8_generator::CMP(struct brw_reg dst, unsigned conditional,
                    struct brw_reg src0, struct brw_reg src1)
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_CMP);
   gen8_set_cond_modifier(inst, conditional);
   /* The CMP instruction appears to behave erratically for floating point
    * sources unless the destination type is also float.  Overriding it to
    * match src0 makes it work in all cases.
    */
   dst.type = src0.type;
   gen8_set_dst(brw, inst, dst);
   gen8_set_src0(brw, inst, src0);
   gen8_set_src1(brw, inst, src1);
   return inst;
}

static int
get_3src_subreg_nr(struct brw_reg reg)
{
   if (reg.vstride == BRW_VERTICAL_STRIDE_0) {
      assert(brw_is_single_value_swizzle(reg.dw1.bits.swizzle));
      return reg.subnr / 4 + BRW_GET_SWZ(reg.dw1.bits.swizzle, 0);
   } else {
      return reg.subnr / 4;
   }
}

gen8_instruction *
gen8_generator::alu3(unsigned opcode,
                     struct brw_reg dst,
                     struct brw_reg src0,
                     struct brw_reg src1,
                     struct brw_reg src2)
{
   /* MRFs haven't existed since Gen7, so we better not be using them. */
   if (dst.file == BRW_MESSAGE_REGISTER_FILE) {
      dst.file = BRW_GENERAL_REGISTER_FILE;
      dst.nr += GEN7_MRF_HACK_START;
   }

   gen8_instruction *inst = next_inst(opcode);
   assert(gen8_access_mode(inst) == BRW_ALIGN_16);

   assert(dst.file == BRW_GENERAL_REGISTER_FILE);
   assert(dst.nr < 128);
   assert(dst.address_mode == BRW_ADDRESS_DIRECT);
   assert(dst.type == BRW_REGISTER_TYPE_F ||
          dst.type == BRW_REGISTER_TYPE_D ||
          dst.type == BRW_REGISTER_TYPE_UD);
   gen8_set_dst_3src_reg_nr(inst, dst.nr);
   gen8_set_dst_3src_subreg_nr(inst, dst.subnr / 16);
   gen8_set_dst_3src_writemask(inst, dst.dw1.bits.writemask);

   assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   assert(src0.address_mode == BRW_ADDRESS_DIRECT);
   assert(src0.nr < 128);
   gen8_set_src0_3src_swizzle(inst, src0.dw1.bits.swizzle);
   gen8_set_src0_3src_subreg_nr(inst, get_3src_subreg_nr(src0));
   gen8_set_src0_3src_rep_ctrl(inst, src0.vstride == BRW_VERTICAL_STRIDE_0);
   gen8_set_src0_3src_reg_nr(inst, src0.nr);
   gen8_set_src0_3src_abs(inst, src0.abs);
   gen8_set_src0_3src_negate(inst, src0.negate);

   assert(src1.file == BRW_GENERAL_REGISTER_FILE);
   assert(src1.address_mode == BRW_ADDRESS_DIRECT);
   assert(src1.nr < 128);
   gen8_set_src1_3src_swizzle(inst, src1.dw1.bits.swizzle);
   gen8_set_src1_3src_subreg_nr(inst, get_3src_subreg_nr(src1));
   gen8_set_src1_3src_rep_ctrl(inst, src1.vstride == BRW_VERTICAL_STRIDE_0);
   gen8_set_src1_3src_reg_nr(inst, src1.nr);
   gen8_set_src1_3src_abs(inst, src1.abs);
   gen8_set_src1_3src_negate(inst, src1.negate);

   assert(src2.file == BRW_GENERAL_REGISTER_FILE);
   assert(src2.address_mode == BRW_ADDRESS_DIRECT);
   assert(src2.nr < 128);
   gen8_set_src2_3src_swizzle(inst, src2.dw1.bits.swizzle);
   gen8_set_src2_3src_subreg_nr(inst, get_3src_subreg_nr(src2));
   gen8_set_src2_3src_rep_ctrl(inst, src2.vstride == BRW_VERTICAL_STRIDE_0);
   gen8_set_src2_3src_reg_nr(inst, src2.nr);
   gen8_set_src2_3src_abs(inst, src2.abs);
   gen8_set_src2_3src_negate(inst, src2.negate);

   /* Set both the source and destination types based on dst.type, ignoring
    * the source register types.  The MAD and LRP emitters both ensure that
    * all register types are float.  The BFE and BFI2 emitters, however, may
    * send us mixed D and UD source types and want us to ignore that.
    */
   switch (dst.type) {
   case BRW_REGISTER_TYPE_F:
      gen8_set_src_3src_type(inst, BRW_3SRC_TYPE_F);
      gen8_set_dst_3src_type(inst, BRW_3SRC_TYPE_F);
      break;
   case BRW_REGISTER_TYPE_D:
      gen8_set_src_3src_type(inst, BRW_3SRC_TYPE_D);
      gen8_set_dst_3src_type(inst, BRW_3SRC_TYPE_D);
      break;
   case BRW_REGISTER_TYPE_UD:
      gen8_set_src_3src_type(inst, BRW_3SRC_TYPE_UD);
      gen8_set_dst_3src_type(inst, BRW_3SRC_TYPE_UD);
      break;
   }

   return inst;
}

gen8_instruction *
gen8_generator::math(unsigned math_function,
                     struct brw_reg dst,
                     struct brw_reg src0)
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_MATH);

   assert(src0.hstride == 0 || src0.hstride == dst.hstride);

   gen8_set_math_function(inst, math_function);
   gen8_set_dst(brw, inst, dst);
   gen8_set_src0(brw, inst, src0);
   return inst;
}

gen8_instruction *
gen8_generator::MATH(unsigned math_function,
                     struct brw_reg dst,
                     struct brw_reg src0)
{
   assert(src0.type == BRW_REGISTER_TYPE_F);
   gen8_instruction *inst = math(math_function, dst, src0);
   return inst;
}

gen8_instruction *
gen8_generator::MATH(unsigned math_function,
                     struct brw_reg dst,
                     struct brw_reg src0,
                     struct brw_reg src1)
{
   bool int_math =
      math_function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT ||
      math_function == BRW_MATH_FUNCTION_INT_DIV_REMAINDER ||
      math_function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER;

   if (int_math) {
      assert(src0.type != BRW_REGISTER_TYPE_F);
      assert(src1.type != BRW_REGISTER_TYPE_F);
   } else {
      assert(src0.type == BRW_REGISTER_TYPE_F);
   }

   gen8_instruction *inst = math(math_function, dst, src0);
   gen8_set_src1(brw, inst, src1);
   return inst;
}

gen8_instruction *
gen8_generator::MOV_RAW(struct brw_reg dst, struct brw_reg src0)
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_MOV);
   gen8_set_dst(brw, inst, retype(dst, BRW_REGISTER_TYPE_UD));
   gen8_set_src0(brw, inst, retype(src0, BRW_REGISTER_TYPE_UD));
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);

   return inst;
}


gen8_instruction *
gen8_generator::NOP()
{
   return next_inst(BRW_OPCODE_NOP);
}

void
gen8_generator::push_if_stack(gen8_instruction *inst)
{
   if_stack[if_stack_depth] = inst - store;

   ++if_stack_depth;
   if (if_stack_array_size <= if_stack_depth) {
      if_stack_array_size *= 2;
      if_stack = reralloc(mem_ctx, if_stack, int, if_stack_array_size);
   }
}

gen8_instruction *
gen8_generator::pop_if_stack()
{
   --if_stack_depth;
   return &store[if_stack[if_stack_depth]];
}

/**
 * Patch the IF and ELSE instructions to set the jump offsets (JIP and UIP.)
 */
void
gen8_generator::patch_IF_ELSE(gen8_instruction *if_inst,
                              gen8_instruction *else_inst,
                              gen8_instruction *endif_inst)
{
   assert(if_inst != NULL && gen8_opcode(if_inst) == BRW_OPCODE_IF);
   assert(else_inst == NULL || gen8_opcode(else_inst) == BRW_OPCODE_ELSE);
   assert(endif_inst != NULL && gen8_opcode(endif_inst) == BRW_OPCODE_ENDIF);

   gen8_set_exec_size(endif_inst, gen8_exec_size(if_inst));

   if (else_inst == NULL) {
      /* Patch IF -> ENDIF */
      gen8_set_jip(if_inst, 16 * (endif_inst - if_inst));
      gen8_set_uip(if_inst, 16 * (endif_inst - if_inst));
   } else {
      gen8_set_exec_size(else_inst, gen8_exec_size(if_inst));

      /* Patch IF -> ELSE and ELSE -> ENDIF:
       *
       * The IF's JIP should point at the instruction after the ELSE.
       * The IF's UIP should point to the ENDIF.
       *
       * Both are expressed in bytes, hence the multiply by 16...128-bits.
       */
      gen8_set_jip(if_inst, 16 * (else_inst - if_inst + 1));
      gen8_set_uip(if_inst, 16 * (endif_inst - if_inst));

      /* Patch ELSE -> ENDIF:
       *
       * Since we don't set branch_ctrl, both JIP and UIP point to ENDIF.
       */
      gen8_set_jip(else_inst, 16 * (endif_inst - else_inst));
      gen8_set_uip(else_inst, 16 * (endif_inst - else_inst));
   }
   gen8_set_jip(endif_inst, 16);
}

gen8_instruction *
gen8_generator::IF(unsigned predicate)
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_IF);
   gen8_set_dst(brw, inst, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   gen8_set_src0(brw, inst, brw_imm_d(0));
   gen8_set_exec_size(inst, default_state.exec_size);
   gen8_set_pred_control(inst, predicate);
   gen8_set_mask_control(inst, BRW_MASK_ENABLE);
   push_if_stack(inst);

   return inst;
}

gen8_instruction *
gen8_generator::ELSE()
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_ELSE);
   gen8_set_dst(brw, inst, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   gen8_set_src0(brw, inst, brw_imm_d(0));
   gen8_set_mask_control(inst, BRW_MASK_ENABLE);
   push_if_stack(inst);
   return inst;
}

gen8_instruction *
gen8_generator::ENDIF()
{
   gen8_instruction *if_inst = NULL;
   gen8_instruction *else_inst = NULL;

   gen8_instruction *tmp = pop_if_stack();
   if (gen8_opcode(tmp) == BRW_OPCODE_ELSE) {
      else_inst = tmp;
      tmp = pop_if_stack();
   }
   assert(gen8_opcode(tmp) == BRW_OPCODE_IF);
   if_inst = tmp;

   gen8_instruction *endif_inst = next_inst(BRW_OPCODE_ENDIF);
   gen8_set_mask_control(endif_inst, BRW_MASK_ENABLE);
   gen8_set_src0(brw, endif_inst, brw_imm_d(0));
   patch_IF_ELSE(if_inst, else_inst, endif_inst);

   return endif_inst;
}

unsigned
gen8_generator::next_ip(unsigned ip) const
{
   return ip + 16;
}

unsigned
gen8_generator::find_next_block_end(unsigned start) const
{
   for (unsigned ip = next_ip(start); ip < next_inst_offset; ip = next_ip(ip)) {
      gen8_instruction *inst = &store[ip / 16];

      switch (gen8_opcode(inst)) {
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_WHILE:
      case BRW_OPCODE_HALT:
         return ip;
      }
   }

   return 0;
}

/* There is no DO instruction on Gen6+, so to find the end of the loop
 * we have to see if the loop is jumping back before our start
 * instruction.
 */
unsigned
gen8_generator::find_loop_end(unsigned start) const
{
   /* Always start after the instruction (such as a WHILE) we're trying to fix
    * up.
    */
   for (unsigned ip = next_ip(start); ip < next_inst_offset; ip = next_ip(ip)) {
      gen8_instruction *inst = &store[ip / 16];

      if (gen8_opcode(inst) == BRW_OPCODE_WHILE) {
         if (ip + gen8_jip(inst) <= start)
            return ip;
      }
   }
   assert(!"not reached");
   return start;
}

/* After program generation, go back and update the UIP and JIP of
 * BREAK, CONT, and HALT instructions to their correct locations.
 */
void
gen8_generator::patch_jump_targets()
{
   for (unsigned ip = 0; ip < next_inst_offset; ip = next_ip(ip)) {
      gen8_instruction *inst = &store[ip / 16];

      int block_end_ip = find_next_block_end(ip);
      switch (gen8_opcode(inst)) {
      case BRW_OPCODE_BREAK:
         assert(block_end_ip != 0);
         gen8_set_jip(inst, block_end_ip - ip);
         gen8_set_uip(inst, find_loop_end(ip) - ip);
         assert(gen8_uip(inst) != 0);
         assert(gen8_jip(inst) != 0);
         break;
      case BRW_OPCODE_CONTINUE:
         assert(block_end_ip != 0);
         gen8_set_jip(inst, block_end_ip - ip);
         gen8_set_uip(inst, find_loop_end(ip) - ip);
         assert(gen8_uip(inst) != 0);
         assert(gen8_jip(inst) != 0);
         break;
      case BRW_OPCODE_ENDIF:
         if (block_end_ip == 0)
            gen8_set_jip(inst, 16);
         else
            gen8_set_jip(inst, block_end_ip - ip);
         break;
      case BRW_OPCODE_HALT:
         /* From the Sandy Bridge PRM (volume 4, part 2, section 8.3.19):
          *
          *    "In case of the halt instruction not inside any conditional
          *     code block, the value of <JIP> and <UIP> should be the
          *     same. In case of the halt instruction inside conditional code
          *     block, the <UIP> should be the end of the program, and the
          *     <JIP> should be end of the most inner conditional code block."
          *
          * The uip will have already been set by whoever set up the
          * instruction.
          */
         if (block_end_ip == 0) {
            gen8_set_jip(inst, gen8_uip(inst));
         } else {
            gen8_set_jip(inst, block_end_ip - ip);
         }
         assert(gen8_uip(inst) != 0);
         assert(gen8_jip(inst) != 0);
         break;
      }
   }
}

void
gen8_generator::DO()
{
   if (loop_stack_array_size < loop_stack_depth) {
      loop_stack_array_size *= 2;
      loop_stack = reralloc(mem_ctx, loop_stack, int, loop_stack_array_size);
   }
   loop_stack[loop_stack_depth++] = nr_inst;
}

gen8_instruction *
gen8_generator::BREAK()
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_BREAK);
   gen8_set_dst(brw, inst, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   gen8_set_src0(brw, inst, brw_imm_d(0));
   gen8_set_exec_size(inst, default_state.exec_size);
   return inst;
}

gen8_instruction *
gen8_generator::CONTINUE()
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_CONTINUE);
   gen8_set_dst(brw, inst, brw_ip_reg());
   gen8_set_src0(brw, inst, brw_imm_d(0));
   gen8_set_exec_size(inst, default_state.exec_size);
   return inst;
}

gen8_instruction *
gen8_generator::WHILE()
{
   gen8_instruction *do_inst = &store[loop_stack[--loop_stack_depth]];
   gen8_instruction *while_inst = next_inst(BRW_OPCODE_WHILE);

   gen8_set_dst(brw, while_inst, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   gen8_set_src0(brw, while_inst, brw_imm_d(0));
   gen8_set_jip(while_inst, 16 * (do_inst - while_inst));
   gen8_set_exec_size(while_inst, default_state.exec_size);

   return while_inst;
}

gen8_instruction *
gen8_generator::HALT()
{
   gen8_instruction *inst = next_inst(BRW_OPCODE_HALT);
   gen8_set_dst(brw, inst, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   gen8_set_src0(brw, inst, brw_imm_d(0));
   gen8_set_exec_size(inst, default_state.exec_size);
   gen8_set_mask_control(inst, BRW_MASK_DISABLE);
   return inst;
}

void
gen8_generator::disassemble(FILE *out, int start, int end)
{
   bool dump_hex = false;

   for (int offset = start; offset < end; offset += 16) {
      gen8_instruction *inst = &store[offset / 16];
      fprintf(stderr, "0x%08x: ", offset);

      if (dump_hex) {
         fprintf(stderr, "0x%08x 0x%08x 0x%08x 0x%08x ",
                 ((uint32_t *) inst)[3],
                 ((uint32_t *) inst)[2],
                 ((uint32_t *) inst)[1],
                 ((uint32_t *) inst)[0]);
      }

      gen8_disassemble(stderr, inst, brw->gen);
   }
}
