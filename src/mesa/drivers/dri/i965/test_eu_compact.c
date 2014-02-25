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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "util/ralloc.h"
#include "brw_context.h"
#include "brw_eu.h"

static bool
test_compact_instruction(struct brw_compile *p, brw_inst src)
{
   struct brw_context *brw = p->brw;

   brw_compact_inst dst;
   memset(&dst, 0xd0, sizeof(dst));

   if (brw_try_compact_instruction(brw, &dst, &src)) {
      brw_inst uncompacted;

      brw_uncompact_instruction(brw, &uncompacted, &dst);
      if (memcmp(&uncompacted, &src, sizeof(src))) {
	 brw_debug_compact_uncompact(brw, &src, &uncompacted);
	 return false;
      }
   } else {
      brw_compact_inst unchanged;
      memset(&unchanged, 0xd0, sizeof(unchanged));
      /* It's not supposed to change dst unless it compacted. */
      if (memcmp(&unchanged, &dst, sizeof(dst))) {
	 fprintf(stderr, "Failed to compact, but dst changed\n");
	 fprintf(stderr, "  Instruction: ");
	 brw_disassemble_inst(stderr, brw, &src, false);
	 return false;
      }
   }

   return true;
}

/**
 * When doing fuzz testing, pad bits won't round-trip.
 *
 * This sort of a superset of skip_bit, which is testing for changing bits that
 * aren't worth testing for fuzzing.  We also just want to clear bits that
 * become meaningless once fuzzing twiddles a related bit.
 */
static void
clear_pad_bits(const struct brw_context *brw, brw_inst *inst)
{
   if (brw_inst_opcode(brw, inst) != BRW_OPCODE_SEND &&
       brw_inst_opcode(brw, inst) != BRW_OPCODE_SENDC &&
       brw_inst_opcode(brw, inst) != BRW_OPCODE_BREAK &&
       brw_inst_opcode(brw, inst) != BRW_OPCODE_CONTINUE &&
       brw_inst_src0_reg_file(brw, inst) != BRW_IMMEDIATE_VALUE &&
       brw_inst_src1_reg_file(brw, inst) != BRW_IMMEDIATE_VALUE) {
      brw_inst_set_bits(inst, 127, 111, 0);
   }
}

static bool
skip_bit(const struct brw_context *brw, brw_inst *src, int bit)
{
   /* pad bit */
   if (bit == 7)
      return true;

   /* The compact bit -- uncompacted can't have it set. */
   if (bit == 29)
      return true;

   /* pad bit */
   if (bit == 47)
      return true;

   /* pad bits */
   if (bit >= 90 && bit <= 95)
      return true;

   /* sometimes these are pad bits. */
   if (brw_inst_opcode(brw, src) != BRW_OPCODE_SEND &&
       brw_inst_opcode(brw, src) != BRW_OPCODE_SENDC &&
       brw_inst_opcode(brw, src) != BRW_OPCODE_BREAK &&
       brw_inst_opcode(brw, src) != BRW_OPCODE_CONTINUE &&
       brw_inst_src0_reg_file(brw, src) != BRW_IMMEDIATE_VALUE &&
       brw_inst_src1_reg_file(brw, src) != BRW_IMMEDIATE_VALUE &&
       bit >= 121) {
      return true;
   }

   return false;
}

static bool
test_fuzz_compact_instruction(struct brw_compile *p, brw_inst src)
{
   for (int bit0 = 0; bit0 < 128; bit0++) {
      if (skip_bit(p->brw, &src, bit0))
	 continue;

      for (int bit1 = 0; bit1 < 128; bit1++) {
         brw_inst instr = src;
	 uint32_t *bits = (uint32_t *)&instr;

         if (skip_bit(p->brw, &src, bit1))
	    continue;

	 bits[bit0 / 32] ^= (1 << (bit0 & 31));
	 bits[bit1 / 32] ^= (1 << (bit1 & 31));

         clear_pad_bits(p->brw, &instr);

	 if (!test_compact_instruction(p, instr)) {
	    printf("  twiddled bits for fuzzing %d, %d\n", bit0, bit1);
	    return false;
	 }
      }
   }

   return true;
}

static void
gen_ADD_GRF_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);
   struct brw_reg g4 = brw_vec8_grf(4, 0);

   brw_ADD(p, g0, g2, g4);
}

static void
gen_ADD_GRF_GRF_IMM(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);

   brw_ADD(p, g0, g2, brw_imm_f(1.0));
}

static void
gen_ADD_GRF_GRF_IMM_d(struct brw_compile *p)
{
   struct brw_reg g0 = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_D);
   struct brw_reg g2 = retype(brw_vec8_grf(2, 0), BRW_REGISTER_TYPE_D);

   brw_ADD(p, g0, g2, brw_imm_d(1));
}

static void
gen_MOV_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);

   brw_MOV(p, g0, g2);
}

static void
gen_ADD_MRF_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg m6 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 6, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);
   struct brw_reg g4 = brw_vec8_grf(4, 0);

   brw_ADD(p, m6, g2, g4);
}

static void
gen_ADD_vec1_GRF_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec1_grf(0, 0);
   struct brw_reg g2 = brw_vec1_grf(2, 0);
   struct brw_reg g4 = brw_vec1_grf(4, 0);

   brw_ADD(p, g0, g2, g4);
}

static void
gen_PLN_MRF_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg m6 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 6, 0);
   struct brw_reg interp = brw_vec1_grf(2, 0);
   struct brw_reg g4 = brw_vec8_grf(4, 0);

   brw_PLN(p, m6, interp, g4);
}

static void
gen_f0_0_MOV_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);

   brw_push_insn_state(p);
   brw_set_default_predicate_control(p, true);
   brw_MOV(p, g0, g2);
   brw_pop_insn_state(p);
}

/* The handling of f0.1 vs f0.0 changes between gen6 and gen7.  Explicitly test
 * it, so that we run the fuzzing can run over all the other bits that might
 * interact with it.
 */
static void
gen_f0_1_MOV_GRF_GRF(struct brw_compile *p)
{
   struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);

   brw_push_insn_state(p);
   brw_set_default_predicate_control(p, true);
   brw_inst *mov = brw_MOV(p, g0, g2);
   brw_inst_set_flag_subreg_nr(p->brw, mov, 1);
   brw_pop_insn_state(p);
}

struct {
   void (*func)(struct brw_compile *p);
} tests[] = {
   { gen_MOV_GRF_GRF },
   { gen_ADD_GRF_GRF_GRF },
   { gen_ADD_GRF_GRF_IMM },
   { gen_ADD_GRF_GRF_IMM_d },
   { gen_ADD_MRF_GRF_GRF },
   { gen_ADD_vec1_GRF_GRF_GRF },
   { gen_PLN_MRF_GRF_GRF },
   { gen_f0_0_MOV_GRF_GRF },
   { gen_f0_1_MOV_GRF_GRF },
};

static bool
run_tests(struct brw_context *brw)
{
   bool fail = false;

   for (int i = 0; i < ARRAY_SIZE(tests); i++) {
      for (int align_16 = 0; align_16 <= 1; align_16++) {
	 struct brw_compile *p = rzalloc(NULL, struct brw_compile);
	 brw_init_compile(brw, p, p);

	 brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
	 if (align_16)
	    brw_set_default_access_mode(p, BRW_ALIGN_16);
	 else
	    brw_set_default_access_mode(p, BRW_ALIGN_1);

	 tests[i].func(p);
	 assert(p->nr_insn == 1);

	 if (!test_compact_instruction(p, p->store[0])) {
	    fail = true;
	    continue;
	 }

	 if (!test_fuzz_compact_instruction(p, p->store[0])) {
	    fail = true;
	    continue;
	 }

	 ralloc_free(p);
      }
   }

   return fail;
}

int
main(int argc, char **argv)
{
   struct brw_context *brw = calloc(1, sizeof(*brw));
   brw->gen = 6;
   bool fail = false;

   for (brw->gen = 6; brw->gen <= 7; brw->gen++) {
      fail |= run_tests(brw);
   }

   return fail;
}
