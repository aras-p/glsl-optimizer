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

/** @file brw_eu_compact.c
 *
 * Instruction compaction is a feature of gm45 and newer hardware that allows
 * for a smaller instruction encoding.
 *
 * The instruction cache is on the order of 32KB, and many programs generate
 * far more instructions than that.  The instruction cache is built to barely
 * keep up with instruction dispatch abaility in cache hit cases -- L1
 * instruction cache misses that still hit in the next level could limit
 * throughput by around 50%.
 *
 * The idea of instruction compaction is that most instructions use a tiny
 * subset of the GPU functionality, so we can encode what would be a 16 byte
 * instruction in 8 bytes using some lookup tables for various fields.
 */

#include "brw_context.h"
#include "brw_eu.h"

static const uint32_t gen6_control_index_table[32] = {
   0b00000000000000000,
   0b01000000000000000,
   0b00110000000000000,
   0b00000000100000000,
   0b00010000000000000,
   0b00001000100000000,
   0b00000000100000010,
   0b00000000000000010,
   0b01000000100000000,
   0b01010000000000000,
   0b10110000000000000,
   0b00100000000000000,
   0b11010000000000000,
   0b11000000000000000,
   0b01001000100000000,
   0b01000000000001000,
   0b01000000000000100,
   0b00000000000001000,
   0b00000000000000100,
   0b00111000100000000,
   0b00001000100000010,
   0b00110000100000000,
   0b00110000000000001,
   0b00100000000000001,
   0b00110000000000010,
   0b00110000000000101,
   0b00110000000001001,
   0b00110000000010000,
   0b00110000000000011,
   0b00110000000000100,
   0b00110000100001000,
   0b00100000000001001
};

static const uint32_t gen6_datatype_table[32] = {
   0b001001110000000000,
   0b001000110000100000,
   0b001001110000000001,
   0b001000000001100000,
   0b001010110100101001,
   0b001000000110101101,
   0b001100011000101100,
   0b001011110110101101,
   0b001000000111101100,
   0b001000000001100001,
   0b001000110010100101,
   0b001000000001000001,
   0b001000001000110001,
   0b001000001000101001,
   0b001000000000100000,
   0b001000001000110010,
   0b001010010100101001,
   0b001011010010100101,
   0b001000000110100101,
   0b001100011000101001,
   0b001011011000101100,
   0b001011010110100101,
   0b001011110110100101,
   0b001111011110111101,
   0b001111011110111100,
   0b001111011110111101,
   0b001111011110011101,
   0b001111011110111110,
   0b001000000000100001,
   0b001000000000100010,
   0b001001111111011101,
   0b001000001110111110,
};

static const uint16_t gen6_subreg_table[32] = {
   0b000000000000000,
   0b000000000000100,
   0b000000110000000,
   0b111000000000000,
   0b011110000001000,
   0b000010000000000,
   0b000000000010000,
   0b000110000001100,
   0b001000000000000,
   0b000001000000000,
   0b000001010010100,
   0b000000001010110,
   0b010000000000000,
   0b110000000000000,
   0b000100000000000,
   0b000000010000000,
   0b000000000001000,
   0b100000000000000,
   0b000001010000000,
   0b001010000000000,
   0b001100000000000,
   0b000000001010100,
   0b101101010010100,
   0b010100000000000,
   0b000000010001111,
   0b011000000000000,
   0b111110000000000,
   0b101000000000000,
   0b000000000001111,
   0b000100010001111,
   0b001000010001111,
   0b000110000000000,
};

static const uint16_t gen6_src_index_table[32] = {
   0b000000000000,
   0b010110001000,
   0b010001101000,
   0b001000101000,
   0b011010010000,
   0b000100100000,
   0b010001101100,
   0b010101110000,
   0b011001111000,
   0b001100101000,
   0b010110001100,
   0b001000100000,
   0b010110001010,
   0b000000000010,
   0b010101010000,
   0b010101101000,
   0b111101001100,
   0b111100101100,
   0b011001110000,
   0b010110001001,
   0b010101011000,
   0b001101001000,
   0b010000101100,
   0b010000000000,
   0b001101110000,
   0b001100010000,
   0b001100000000,
   0b010001101010,
   0b001101111000,
   0b000001110000,
   0b001100100000,
   0b001101010000,
};

static const uint32_t gen7_control_index_table[32] = {
   0b0000000000000000010,
   0b0000100000000000000,
   0b0000100000000000001,
   0b0000100000000000010,
   0b0000100000000000011,
   0b0000100000000000100,
   0b0000100000000000101,
   0b0000100000000000111,
   0b0000100000000001000,
   0b0000100000000001001,
   0b0000100000000001101,
   0b0000110000000000000,
   0b0000110000000000001,
   0b0000110000000000010,
   0b0000110000000000011,
   0b0000110000000000100,
   0b0000110000000000101,
   0b0000110000000000111,
   0b0000110000000001001,
   0b0000110000000001101,
   0b0000110000000010000,
   0b0000110000100000000,
   0b0001000000000000000,
   0b0001000000000000010,
   0b0001000000000000100,
   0b0001000000100000000,
   0b0010110000000000000,
   0b0010110000000010000,
   0b0011000000000000000,
   0b0011000000100000000,
   0b0101000000000000000,
   0b0101000000100000000
};

static const uint32_t gen7_datatype_table[32] = {
   0b001000000000000001,
   0b001000000000100000,
   0b001000000000100001,
   0b001000000001100001,
   0b001000000010111101,
   0b001000001011111101,
   0b001000001110100001,
   0b001000001110100101,
   0b001000001110111101,
   0b001000010000100001,
   0b001000110000100000,
   0b001000110000100001,
   0b001001010010100101,
   0b001001110010100100,
   0b001001110010100101,
   0b001111001110111101,
   0b001111011110011101,
   0b001111011110111100,
   0b001111011110111101,
   0b001111111110111100,
   0b000000001000001100,
   0b001000000000111101,
   0b001000000010100101,
   0b001000010000100000,
   0b001001010010100100,
   0b001001110010000100,
   0b001010010100001001,
   0b001101111110111101,
   0b001111111110111101,
   0b001011110110101100,
   0b001010010100101000,
   0b001010110100101000
};

static const uint16_t gen7_subreg_table[32] = {
   0b000000000000000,
   0b000000000000001,
   0b000000000001000,
   0b000000000001111,
   0b000000000010000,
   0b000000010000000,
   0b000000100000000,
   0b000000110000000,
   0b000001000000000,
   0b000001000010000,
   0b000010100000000,
   0b001000000000000,
   0b001000000000001,
   0b001000010000001,
   0b001000010000010,
   0b001000010000011,
   0b001000010000100,
   0b001000010000111,
   0b001000010001000,
   0b001000010001110,
   0b001000010001111,
   0b001000110000000,
   0b001000111101000,
   0b010000000000000,
   0b010000110000000,
   0b011000000000000,
   0b011110010000111,
   0b100000000000000,
   0b101000000000000,
   0b110000000000000,
   0b111000000000000,
   0b111000000011100
};

static const uint16_t gen7_src_index_table[32] = {
   0b000000000000,
   0b000000000010,
   0b000000010000,
   0b000000010010,
   0b000000011000,
   0b000000100000,
   0b000000101000,
   0b000001001000,
   0b000001010000,
   0b000001110000,
   0b000001111000,
   0b001100000000,
   0b001100000010,
   0b001100001000,
   0b001100010000,
   0b001100010010,
   0b001100100000,
   0b001100101000,
   0b001100111000,
   0b001101000000,
   0b001101000010,
   0b001101001000,
   0b001101010000,
   0b001101100000,
   0b001101101000,
   0b001101110000,
   0b001101110001,
   0b001101111000,
   0b010001101000,
   0b010001101001,
   0b010001101010,
   0b010110001000
};

static const uint32_t *control_index_table;
static const uint32_t *datatype_table;
static const uint16_t *subreg_table;
static const uint16_t *src_index_table;

static bool
set_control_index(struct brw_context *brw,
                  struct brw_compact_instruction *dst,
                  struct brw_instruction *src)
{
   uint32_t *src_u32 = (uint32_t *)src;
   uint32_t uncompacted = 0;

   uncompacted |= ((src_u32[0] >> 8) & 0xffff) << 0;
   uncompacted |= ((src_u32[0] >> 31) & 0x1) << 16;
   /* On gen7, the flag register number gets integrated into the control
    * index.
    */
   if (brw->gen >= 7)
      uncompacted |= ((src_u32[2] >> 25) & 0x3) << 17;

   for (int i = 0; i < 32; i++) {
      if (control_index_table[i] == uncompacted) {
	 dst->dw0.control_index = i;
	 return true;
      }
   }

   return false;
}

static bool
set_datatype_index(struct brw_compact_instruction *dst,
                   struct brw_instruction *src)
{
   uint32_t uncompacted = 0;

   uncompacted |= src->bits1.ud & 0x7fff;
   uncompacted |= (src->bits1.ud >> 29) << 15;

   for (int i = 0; i < 32; i++) {
      if (datatype_table[i] == uncompacted) {
	 dst->dw0.data_type_index = i;
	 return true;
      }
   }

   return false;
}

static bool
set_subreg_index(struct brw_compact_instruction *dst,
                 struct brw_instruction *src)
{
   uint16_t uncompacted = 0;

   uncompacted |= src->bits1.da1.dest_subreg_nr << 0;
   uncompacted |= src->bits2.da1.src0_subreg_nr << 5;
   uncompacted |= src->bits3.da1.src1_subreg_nr << 10;

   for (int i = 0; i < 32; i++) {
      if (subreg_table[i] == uncompacted) {
	 dst->dw0.sub_reg_index = i;
	 return true;
      }
   }

   return false;
}

static bool
get_src_index(uint16_t uncompacted,
              uint16_t *compacted)
{
   for (int i = 0; i < 32; i++) {
      if (src_index_table[i] == uncompacted) {
	 *compacted = i;
	 return true;
      }
   }

   return false;
}

static bool
set_src0_index(struct brw_compact_instruction *dst,
               struct brw_instruction *src)
{
   uint16_t compacted, uncompacted = 0;

   uncompacted |= (src->bits2.ud >> 13) & 0xfff;

   if (!get_src_index(uncompacted, &compacted))
      return false;

   dst->dw0.src0_index = compacted & 0x3;
   dst->dw1.src0_index = compacted >> 2;

   return true;
}

static bool
set_src1_index(struct brw_compact_instruction *dst,
               struct brw_instruction *src)
{
   uint16_t compacted, uncompacted = 0;

   uncompacted |= (src->bits3.ud >> 13) & 0xfff;

   if (!get_src_index(uncompacted, &compacted))
      return false;

   dst->dw1.src1_index = compacted;

   return true;
}

/**
 * Tries to compact instruction src into dst.
 *
 * It doesn't modify dst unless src is compactable, which is relied on by
 * brw_compact_instructions().
 */
bool
brw_try_compact_instruction(struct brw_compile *p,
                            struct brw_compact_instruction *dst,
                            struct brw_instruction *src)
{
   struct brw_context *brw = p->brw;
   struct brw_compact_instruction temp;

   if (src->header.opcode == BRW_OPCODE_IF ||
       src->header.opcode == BRW_OPCODE_ELSE ||
       src->header.opcode == BRW_OPCODE_ENDIF ||
       src->header.opcode == BRW_OPCODE_HALT ||
       src->header.opcode == BRW_OPCODE_DO ||
       src->header.opcode == BRW_OPCODE_WHILE) {
      /* FINISHME: The fixup code below, and brw_set_uip_jip and friends, needs
       * to be able to handle compacted flow control instructions..
       */
      return false;
   }

   /* FINISHME: immediates */
   if (src->bits1.da1.src0_reg_file == BRW_IMMEDIATE_VALUE ||
       src->bits1.da1.src1_reg_file == BRW_IMMEDIATE_VALUE)
      return false;

   memset(&temp, 0, sizeof(temp));

   temp.dw0.opcode = src->header.opcode;
   temp.dw0.debug_control = src->header.debug_control;
   if (!set_control_index(brw, &temp, src))
      return false;
   if (!set_datatype_index(&temp, src))
      return false;
   if (!set_subreg_index(&temp, src))
      return false;
   temp.dw0.acc_wr_control = src->header.acc_wr_control;
   temp.dw0.conditionalmod = src->header.destreg__conditionalmod;
   if (brw->gen <= 6)
      temp.dw0.flag_subreg_nr = src->bits2.da1.flag_subreg_nr;
   temp.dw0.cmpt_ctrl = 1;
   if (!set_src0_index(&temp, src))
      return false;
   if (!set_src1_index(&temp, src))
      return false;
   temp.dw1.dst_reg_nr = src->bits1.da1.dest_reg_nr;
   temp.dw1.src0_reg_nr = src->bits2.da1.src0_reg_nr;
   temp.dw1.src1_reg_nr = src->bits3.da1.src1_reg_nr;

   *dst = temp;

   return true;
}

static void
set_uncompacted_control(struct brw_context *brw,
                        struct brw_instruction *dst,
                        struct brw_compact_instruction *src)
{
   uint32_t *dst_u32 = (uint32_t *)dst;
   uint32_t uncompacted = control_index_table[src->dw0.control_index];

   dst_u32[0] |= ((uncompacted >> 0) & 0xffff) << 8;
   dst_u32[0] |= ((uncompacted >> 16) & 0x1) << 31;

   if (brw->gen >= 7)
      dst_u32[2] |= ((uncompacted >> 17) & 0x3) << 25;
}

static void
set_uncompacted_datatype(struct brw_instruction *dst,
                         struct brw_compact_instruction *src)
{
   uint32_t uncompacted = datatype_table[src->dw0.data_type_index];

   dst->bits1.ud &= ~(0x7 << 29);
   dst->bits1.ud |= ((uncompacted >> 15) & 0x7) << 29;
   dst->bits1.ud &= ~0x7fff;
   dst->bits1.ud |= uncompacted & 0x7fff;
}

static void
set_uncompacted_subreg(struct brw_instruction *dst,
                       struct brw_compact_instruction *src)
{
   uint16_t uncompacted = subreg_table[src->dw0.sub_reg_index];

   dst->bits1.da1.dest_subreg_nr = (uncompacted >> 0)  & 0x1f;
   dst->bits2.da1.src0_subreg_nr = (uncompacted >> 5)  & 0x1f;
   dst->bits3.da1.src1_subreg_nr = (uncompacted >> 10) & 0x1f;
}

static void
set_uncompacted_src0(struct brw_instruction *dst,
                     struct brw_compact_instruction *src)
{
   uint32_t compacted = src->dw0.src0_index | src->dw1.src0_index << 2;
   uint16_t uncompacted = src_index_table[compacted];

   dst->bits2.ud |= uncompacted << 13;
}

static void
set_uncompacted_src1(struct brw_instruction *dst,
                     struct brw_compact_instruction *src)
{
   uint16_t uncompacted = src_index_table[src->dw1.src1_index];

   dst->bits3.ud |= uncompacted << 13;
}

void
brw_uncompact_instruction(struct brw_context *brw,
                          struct brw_instruction *dst,
                          struct brw_compact_instruction *src)
{
   memset(dst, 0, sizeof(*dst));

   dst->header.opcode = src->dw0.opcode;
   dst->header.debug_control = src->dw0.debug_control;

   set_uncompacted_control(brw, dst, src);
   set_uncompacted_datatype(dst, src);
   set_uncompacted_subreg(dst, src);
   dst->header.acc_wr_control = src->dw0.acc_wr_control;
   dst->header.destreg__conditionalmod = src->dw0.conditionalmod;
   if (brw->gen <= 6)
      dst->bits2.da1.flag_subreg_nr = src->dw0.flag_subreg_nr;
   set_uncompacted_src0(dst, src);
   set_uncompacted_src1(dst, src);
   dst->bits1.da1.dest_reg_nr = src->dw1.dst_reg_nr;
   dst->bits2.da1.src0_reg_nr = src->dw1.src0_reg_nr;
   dst->bits3.da1.src1_reg_nr = src->dw1.src1_reg_nr;
}

void brw_debug_compact_uncompact(struct brw_context *brw,
                                 struct brw_instruction *orig,
                                 struct brw_instruction *uncompacted)
{
   fprintf(stderr, "Instruction compact/uncompact changed (gen%d):\n",
           brw->gen);

   fprintf(stderr, "  before: ");
   brw_disasm(stderr, orig, brw->gen);

   fprintf(stderr, "  after:  ");
   brw_disasm(stderr, uncompacted, brw->gen);

   uint32_t *before_bits = (uint32_t *)orig;
   uint32_t *after_bits = (uint32_t *)uncompacted;
   fprintf(stderr, "  changed bits:\n");
   for (int i = 0; i < 128; i++) {
      uint32_t before = before_bits[i / 32] & (1 << (i & 31));
      uint32_t after = after_bits[i / 32] & (1 << (i & 31));

      if (before != after) {
         fprintf(stderr, "  bit %d, %s to %s\n", i,
                 before ? "set" : "unset",
                 after ? "set" : "unset");
      }
   }
}

static int
compacted_between(int old_ip, int old_target_ip, int *compacted_counts)
{
   int this_compacted_count = compacted_counts[old_ip];
   int target_compacted_count = compacted_counts[old_target_ip];
   return target_compacted_count - this_compacted_count;
}

static void
update_uip_jip(struct brw_instruction *insn, int this_old_ip,
               int *compacted_counts)
{
   int target_old_ip;

   target_old_ip = this_old_ip + insn->bits3.break_cont.jip;
   insn->bits3.break_cont.jip -= compacted_between(this_old_ip,
                                                   target_old_ip,
                                                   compacted_counts);

   target_old_ip = this_old_ip + insn->bits3.break_cont.uip;
   insn->bits3.break_cont.uip -= compacted_between(this_old_ip,
                                                   target_old_ip,
                                                   compacted_counts);
}

void
brw_init_compaction_tables(struct brw_context *brw)
{
   assert(gen6_control_index_table[ARRAY_SIZE(gen6_control_index_table) - 1] != 0);
   assert(gen6_datatype_table[ARRAY_SIZE(gen6_datatype_table) - 1] != 0);
   assert(gen6_subreg_table[ARRAY_SIZE(gen6_subreg_table) - 1] != 0);
   assert(gen6_src_index_table[ARRAY_SIZE(gen6_src_index_table) - 1] != 0);
   assert(gen7_control_index_table[ARRAY_SIZE(gen6_control_index_table) - 1] != 0);
   assert(gen7_datatype_table[ARRAY_SIZE(gen6_datatype_table) - 1] != 0);
   assert(gen7_subreg_table[ARRAY_SIZE(gen6_subreg_table) - 1] != 0);
   assert(gen7_src_index_table[ARRAY_SIZE(gen6_src_index_table) - 1] != 0);

   switch (brw->gen) {
   case 7:
      control_index_table = gen7_control_index_table;
      datatype_table = gen7_datatype_table;
      subreg_table = gen7_subreg_table;
      src_index_table = gen7_src_index_table;
      break;
   case 6:
      control_index_table = gen6_control_index_table;
      datatype_table = gen6_datatype_table;
      subreg_table = gen6_subreg_table;
      src_index_table = gen6_src_index_table;
      break;
   default:
      return;
   }
}

void
brw_compact_instructions(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   void *store = p->store;
   /* For an instruction at byte offset 8*i before compaction, this is the number
    * of compacted instructions that preceded it.
    */
   int compacted_counts[p->next_insn_offset / 8];
   /* For an instruction at byte offset 8*i after compaction, this is the
    * 8-byte offset it was at before compaction.
    */
   int old_ip[p->next_insn_offset / 8];

   if (brw->gen < 6)
      return;

   int src_offset;
   int offset = 0;
   int compacted_count = 0;
   for (src_offset = 0; src_offset < p->nr_insn * 16;) {
      struct brw_instruction *src = store + src_offset;
      void *dst = store + offset;

      old_ip[offset / 8] = src_offset / 8;
      compacted_counts[src_offset / 8] = compacted_count;

      struct brw_instruction saved = *src;

      if (!src->header.cmpt_control &&
          brw_try_compact_instruction(p, dst, src)) {
         compacted_count++;

         if (INTEL_DEBUG) {
            struct brw_instruction uncompacted;
            brw_uncompact_instruction(brw, &uncompacted, dst);
            if (memcmp(&saved, &uncompacted, sizeof(uncompacted))) {
               brw_debug_compact_uncompact(brw, &saved, &uncompacted);
            }
         }

         offset += 8;
         src_offset += 16;
      } else {
         int size = src->header.cmpt_control ? 8 : 16;

         /* It appears that the end of thread SEND instruction needs to be
          * aligned, or the GPU hangs.
          */
         if ((src->header.opcode == BRW_OPCODE_SEND ||
              src->header.opcode == BRW_OPCODE_SENDC) &&
             src->bits3.generic.end_of_thread &&
             (offset & 8) != 0) {
            struct brw_compact_instruction *align = store + offset;
            memset(align, 0, sizeof(*align));
            align->dw0.opcode = BRW_OPCODE_NOP;
            align->dw0.cmpt_ctrl = 1;
            offset += 8;
            old_ip[offset / 8] = src_offset / 8;
            dst = store + offset;
         }

         /* If we didn't compact this intruction, we need to move it down into
          * place.
          */
         if (offset != src_offset) {
            memmove(dst, src, size);
         }
         offset += size;
         src_offset += size;
      }
   }

   /* Fix up control flow offsets. */
   p->next_insn_offset = offset;
   for (offset = 0; offset < p->next_insn_offset;) {
      struct brw_instruction *insn = store + offset;
      int this_old_ip = old_ip[offset / 8];
      int this_compacted_count = compacted_counts[this_old_ip];
      int target_old_ip, target_compacted_count;

      switch (insn->header.opcode) {
      case BRW_OPCODE_BREAK:
      case BRW_OPCODE_CONTINUE:
      case BRW_OPCODE_HALT:
         update_uip_jip(insn, this_old_ip, compacted_counts);
         break;

      case BRW_OPCODE_IF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_WHILE:
         if (brw->gen == 6) {
            target_old_ip = this_old_ip + insn->bits1.branch_gen6.jump_count;
            target_compacted_count = compacted_counts[target_old_ip];
            insn->bits1.branch_gen6.jump_count -= (target_compacted_count -
                                                   this_compacted_count);
         } else {
            update_uip_jip(insn, this_old_ip, compacted_counts);
         }
         break;
      }

      if (insn->header.cmpt_control) {
         offset += 8;
      } else {
         offset += 16;
      }
   }

   /* p->nr_insn is counting the number of uncompacted instructions still, so
    * divide.  We do want to be sure there's a valid instruction in any
    * alignment padding, so that the next compression pass (for the FS 8/16
    * compile passes) parses correctly.
    */
   if (p->next_insn_offset & 8) {
      struct brw_compact_instruction *align = store + offset;
      memset(align, 0, sizeof(*align));
      align->dw0.opcode = BRW_OPCODE_NOP;
      align->dw0.cmpt_ctrl = 1;
      p->next_insn_offset += 8;
   }
   p->nr_insn = p->next_insn_offset / 16;

   if (0) {
      fprintf(stderr, "dumping compacted program\n");
      brw_dump_compile(p, stderr, 0, p->next_insn_offset);

      int cmp = 0;
      for (offset = 0; offset < p->next_insn_offset;) {
         struct brw_instruction *insn = store + offset;

         if (insn->header.cmpt_control) {
            offset += 8;
            cmp++;
         } else {
            offset += 16;
         }
      }
      fprintf(stderr, "%db/%db saved (%d%%)\n", cmp * 8, offset + cmp * 8,
              cmp * 8 * 100 / (offset + cmp * 8));
   }
}
