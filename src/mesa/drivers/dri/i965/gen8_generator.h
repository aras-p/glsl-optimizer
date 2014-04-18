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

/**
 * @file gen8_generator.h
 *
 * Code generation for Gen8+ hardware, replacing the brw_eu_emit.c layer.
 */

#pragma once

extern "C" {
#include "main/macros.h"
} /* extern "C" */

#include "gen8_instruction.h"

class gen8_generator {
public:
   gen8_generator(struct brw_context *brw,
                  struct gl_shader_program *shader_prog,
                  struct gl_program *prog,
                  void *mem_ctx);
   ~gen8_generator();

   /**
    * Instruction emitters.
    * @{
    */
   #define ALU1(OP) \
   gen8_instruction *OP(struct brw_reg dst, struct brw_reg src);
   #define ALU2(OP) \
   gen8_instruction *OP(struct brw_reg d, struct brw_reg, struct brw_reg);
   #define ALU3(OP) \
   gen8_instruction *OP(struct brw_reg d, \
                        struct brw_reg, struct brw_reg, struct brw_reg);
   ALU2(ADD)
   ALU2(AND)
   ALU2(ASR)
   ALU3(BFE)
   ALU2(BFI1)
   ALU3(BFI2)
   ALU1(F32TO16)
   ALU1(F16TO32)
   ALU1(BFREV)
   ALU1(CBIT)
   ALU2(ADDC)
   ALU2(SUBB)
   ALU2(DP2)
   ALU2(DP3)
   ALU2(DP4)
   ALU2(DPH)
   ALU1(FBH)
   ALU1(FBL)
   ALU1(FRC)
   ALU2(LINE)
   ALU3(LRP)
   ALU2(MAC)
   ALU2(MACH)
   ALU3(MAD)
   ALU2(MUL)
   ALU1(MOV)
   ALU1(MOV_RAW)
   ALU1(NOT)
   ALU2(OR)
   ALU2(PLN)
   ALU1(RNDD)
   ALU1(RNDE)
   ALU1(RNDZ)
   ALU2(SEL)
   ALU2(SHL)
   ALU2(SHR)
   ALU2(XOR)
   #undef ALU1
   #undef ALU2
   #undef ALU3

   gen8_instruction *CMP(struct brw_reg dst, unsigned conditional,
                         struct brw_reg src0, struct brw_reg src1);
   gen8_instruction *IF(unsigned predicate);
   gen8_instruction *ELSE();
   gen8_instruction *ENDIF();
   void DO();
   gen8_instruction *BREAK();
   gen8_instruction *CONTINUE();
   gen8_instruction *WHILE();

   gen8_instruction *HALT();

   gen8_instruction *MATH(unsigned math_function,
                          struct brw_reg dst,
                          struct brw_reg src0);
   gen8_instruction *MATH(unsigned math_function,
                          struct brw_reg dst,
                          struct brw_reg src0,
                          struct brw_reg src1);
   gen8_instruction *NOP();
   /** @} */

   void disassemble(FILE *out, int start, int end);

protected:
   gen8_instruction *alu3(unsigned opcode,
                          struct brw_reg dst,
                          struct brw_reg src0,
                          struct brw_reg src1,
                          struct brw_reg src2);

   gen8_instruction *math(unsigned math_function,
                          struct brw_reg dst,
                          struct brw_reg src0);

   gen8_instruction *next_inst(unsigned opcode);

   struct gl_shader_program *shader_prog;
   struct gl_program *prog;

   struct brw_context *brw;
   struct intel_context *intel;
   struct gl_context *ctx;

   gen8_instruction *store;
   unsigned store_size;
   unsigned nr_inst;
   unsigned next_inst_offset;

   /**
    * Control flow stacks:
    *
    * if_stack contains IF and ELSE instructions which must be patched with
    * the final jump offsets (and popped) once the matching ENDIF is encountered.
    *
    * We actually store an array index into the store, rather than pointers
    * to the instructions.  This is necessary since we may realloc the store.
    *
    *  @{
    */
   int *if_stack;
   int if_stack_depth;
   int if_stack_array_size;

   int *loop_stack;
   int loop_stack_depth;
   int loop_stack_array_size;

   int if_depth_in_loop;

   void push_if_stack(gen8_instruction *inst);
   gen8_instruction *pop_if_stack();
   /** @} */

   void patch_IF_ELSE(gen8_instruction *if_inst,
                      gen8_instruction *else_inst,
                      gen8_instruction *endif_inst);

   unsigned next_ip(unsigned ip) const;
   unsigned find_next_block_end(unsigned start_ip) const;
   unsigned find_loop_end(unsigned start) const;

   void patch_jump_targets();

   /**
    * Default state for new instructions.
    */
   struct {
      unsigned exec_size;
      unsigned access_mode;
      unsigned mask_control;
      unsigned qtr_control;
      unsigned flag_subreg_nr;
      unsigned conditional_mod;
      unsigned predicate;
      bool predicate_inverse;
      bool saturate;
   } default_state;

   void *mem_ctx;
};
