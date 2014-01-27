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
 * @file gen8_instruction.h
 *
 * A representation of a Gen8+ EU instruction, with helper methods to get
 * and set various fields.  This is the actual hardware format.
 */

#ifndef GEN8_INSTRUCTION_H
#define GEN8_INSTRUCTION_H

#include <stdio.h>
#include <stdint.h>

#include "brw_context.h"
#include "brw_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gen8_instruction {
   uint32_t data[4];
};

static inline unsigned gen8_instruction_bits(struct gen8_instruction *inst,
                                             unsigned high,
                                             unsigned low);
static inline void gen8_instruction_set_bits(struct gen8_instruction *inst,
                                             unsigned high,
                                             unsigned low,
                                             unsigned value);

#define F(name, high, low) \
static inline void gen8_set_##name(struct gen8_instruction *inst, unsigned v) \
{ \
   gen8_instruction_set_bits(inst, high, low, v); \
} \
static inline unsigned gen8_##name(struct gen8_instruction *inst) \
{ \
   return gen8_instruction_bits(inst, high, low); \
}

F(src1_vert_stride,    120, 117)
F(src1_da1_width,      116, 114)
F(src1_da16_swiz_w,    115, 114)
F(src1_da16_swiz_z,    113, 112)
F(src1_da1_hstride,    113, 112)
F(src1_address_mode,   111, 111)
/** Src1.SrcMod @{ */
F(src1_negate,         110, 110)
F(src1_abs,            109, 109)
/** @} */
F(src1_ia1_subreg_nr,  108, 105)
F(src1_da_reg_nr,      108, 101)
F(src1_da16_subreg_nr, 100, 100)
F(src1_da1_subreg_nr,  100,  96)
F(src1_da16_swiz_y,     99,  98)
F(src1_da16_swiz_x,     97,  96)
F(src1_reg_type,        94,  91)
F(src1_reg_file,        90,  89)
F(src0_vert_stride,     88,  85)
F(src0_da1_width,       84,  82)
F(src0_da16_swiz_w,     83,  82)
F(src0_da16_swiz_z,     81,  80)
F(src0_da1_hstride,     81,  80)
F(src0_address_mode,    79,  79)
/** Src0.SrcMod @{ */
F(src0_negate,          78,  78)
F(src0_abs,             77,  77)
/** @} */
F(src0_ia1_subreg_nr,   76,  73)
F(src0_da_reg_nr,       76,  69)
F(src0_da16_subreg_nr,  68,  68)
F(src0_da1_subreg_nr,   68,  64)
F(src0_da16_swiz_y,     67,  66)
F(src0_da16_swiz_x,     65,  64)
F(dst_address_mode,     63,  63)
F(dst_da1_hstride,      62,  61)
F(dst_ia1_subreg_nr,    60,  57)
F(dst_da_reg_nr,        60,  53)
F(dst_da16_subreg_nr,   52,  52)
F(dst_da1_subreg_nr,    52,  48)
F(da16_writemask,       51,  48) /* Dst.ChanEn */
F(src0_reg_type,        46,  43)
F(src0_reg_file,        42,  41)
F(dst_reg_type,         40,  37)
F(dst_reg_file,         36,  35)
F(mask_control,         34,  34)
F(flag_reg_nr,          33,  33)
F(flag_subreg_nr,       32,  32)
F(saturate,             31,  31)
F(branch_control,       30,  30)
F(debug_control,        30,  30)
F(cmpt_control,         29,  29)
F(acc_wr_control,       28,  28)
F(cond_modifier,        27,  24)
F(exec_size,            23,  21)
F(pred_inv,             20,  20)
F(pred_control,         19,  16)
F(thread_control,       15,  14)
F(qtr_control,          13,  12)
F(nib_control,          11,  11)
F(no_dd_check,          10,  10)
F(no_dd_clear,           9,   9)
F(access_mode,           8,   8)
/* Bit 7 is Reserved (for future Opcode expansion) */
F(opcode,                6,   0)

/**
 * Three-source instructions:
 *  @{
 */
F(src2_3src_reg_nr,    125, 118)
F(src2_3src_subreg_nr, 117, 115)
F(src2_3src_swizzle,   114, 107)
F(src2_3src_rep_ctrl,  106, 106)
F(src1_3src_reg_nr,    104,  97)
/* src1_3src_subreg_nr spans word boundaries and has to be handled specially */
F(src1_3src_swizzle,    93,  86)
F(src1_3src_rep_ctrl,   85,  85)
F(src0_3src_reg_nr,     83,  76)
F(src0_3src_subreg_nr,  75,  73)
F(src0_3src_swizzle,    72,  65)
F(src0_3src_rep_ctrl,   64,  64)
F(dst_3src_reg_nr,      63,  56)
F(dst_3src_subreg_nr,   55,  53)
F(dst_3src_writemask,   52,  49)
F(dst_3src_type,        48,  46)
F(src_3src_type,        45,  43)
F(src2_3src_negate,     42,  42)
F(src2_3src_abs,        41,  41)
F(src1_3src_negate,     40,  40)
F(src1_3src_abs,        39,  39)
F(src0_3src_negate,     38,  38)
F(src0_3src_abs,        37,  37)
/** @} */

/**
 * Fields for SEND messages:
 *  @{
 */
F(eot,                 127, 127)
F(mlen,                124, 121)
F(rlen,                120, 116)
F(header_present,      115, 115)
F(function_control,    114,  96)
F(sfid,                 27,  24)
F(math_function,        27,  24)
/** @} */

/**
 * URB message function control bits:
 *  @{
 */
F(urb_per_slot_offset, 113, 113)
F(urb_interleave,      111, 111)
F(urb_global_offset,   110, 100)
F(urb_opcode,           99,  96)
/** @} */

/* Message descriptor bits */
#define MD(name, high, low) F(name, (high + 96), (low + 96))

/**
 * Sampler message function control bits:
 *  @{
 */
MD(sampler_simd_mode,   18,  17)
MD(sampler_msg_type,    16,  12)
MD(sampler,             11,   8)
MD(binding_table_index,  7,   0) /* also used by other messages */
/** @} */

/**
 * Data port message function control bits:
 *  @{
 */
MD(dp_category,         18,  18)
MD(dp_message_type,     17,  14)
MD(dp_message_control,  13,   8)
/** @} */

/**
 * Scratch message bits:
 *  @{
 */
MD(scratch_read_write,  17,  17) /* 0 = read,  1 = write */
MD(scratch_type,        16,  16) /* 0 = OWord, 1 = DWord */
MD(scratch_invalidate_after_read, 15, 15)
MD(scratch_block_size,  13,  12)
MD(scratch_addr_offset, 11,   0)
/** @} */

/**
 * Render Target message function control bits:
 *  @{
 */
MD(rt_last,             12,  12)
MD(rt_slot_group,       11,  11)
MD(rt_message_type,     10,   8)
/** @} */

/**
 * Thread Spawn message function control bits:
 *  @{
 */
MD(ts_resource_select,   4,   4)
MD(ts_request_type,      1,   1)
MD(ts_opcode,            0,   0)
/** @} */

/**
 * Video Motion Estimation message function control bits:
 *  @{
 */
F(vme_message_type,     14,  13)
/** @} */

/**
 * Check & Refinement Engine message function control bits:
 *  @{
 */
F(cre_message_type,     14,  13)
/** @} */

#undef MD
#undef F

static inline void
gen8_set_src1_3src_subreg_nr(struct gen8_instruction *inst, unsigned v)
{
   assert((v & ~0x7) == 0);

   gen8_instruction_set_bits(inst, 95, 94, v & 0x3);
   gen8_instruction_set_bits(inst, 96, 96, v >> 2);
}

static inline unsigned
gen8_src1_3src_subreg_nr(struct gen8_instruction *inst)
{
   return gen8_instruction_bits(inst, 95, 94) |
          (gen8_instruction_bits(inst, 96, 96) << 2);
}

#define GEN8_IA1_ADDR_IMM(reg, nine, high, low)                              \
static inline void                                                           \
gen8_set_##reg##_ia1_addr_imm(struct gen8_instruction *inst, unsigned value) \
{                                                                            \
   assert((value & ~0x3ff) == 0);                                            \
   gen8_instruction_set_bits(inst, high, low, value & 0x1ff);                \
   gen8_instruction_set_bits(inst, nine, nine, value >> 9);                  \
}                                                                            \
                                                                             \
static inline unsigned                                                       \
gen8_##reg##_ia1_addr_imm(struct gen8_instruction *inst)                     \
{                                                                            \
   return gen8_instruction_bits(inst, high, low) |                           \
          (gen8_instruction_bits(inst, nine, nine) << 9);                    \
}

/* AddrImm[9:0] for Align1 Indirect Addressing */
GEN8_IA1_ADDR_IMM(src1, 121, 104, 96)
GEN8_IA1_ADDR_IMM(src0,  95,  72, 64)
GEN8_IA1_ADDR_IMM(dst,   47,  56, 48)

/**
 * Flow control instruction bits:
 *  @{
 */
static inline unsigned gen8_uip(struct gen8_instruction *inst)
{
   return inst->data[2];
}
static inline void gen8_set_uip(struct gen8_instruction *inst, unsigned uip)
{
   inst->data[2] = uip;
}
static inline unsigned gen8_jip(struct gen8_instruction *inst)
{
   return inst->data[3];
}
static inline void gen8_set_jip(struct gen8_instruction *inst, unsigned jip)
{
   inst->data[3] = jip;
}
/** @} */

static inline int gen8_src1_imm_d(struct gen8_instruction *inst)
{
   return inst->data[3];
}
static inline unsigned gen8_src1_imm_ud(struct gen8_instruction *inst)
{
   return inst->data[3];
}
static inline float gen8_src1_imm_f(struct gen8_instruction *inst)
{
   fi_type ft;

   ft.u = inst->data[3];
   return ft.f;
}

void gen8_set_dst(const struct brw_context *brw,
                  struct gen8_instruction *inst, struct brw_reg reg);
void gen8_set_src0(const struct brw_context *brw,
                   struct gen8_instruction *inst, struct brw_reg reg);
void gen8_set_src1(const struct brw_context *brw,
                   struct gen8_instruction *inst, struct brw_reg reg);

void gen8_set_urb_message(const struct brw_context *brw,
                          struct gen8_instruction *inst,
                          enum brw_urb_write_flags flags,
                          unsigned mlen, unsigned rlen,
                          unsigned offset, bool interleave);

void gen8_set_sampler_message(const struct brw_context *brw,
                              struct gen8_instruction *inst,
                              unsigned binding_table_index, unsigned sampler,
                              unsigned msg_type, unsigned rlen, unsigned mlen,
                              bool header_present, unsigned simd_mode);

void gen8_set_dp_message(const struct brw_context *brw,
                         struct gen8_instruction *inst,
                         enum brw_message_target sfid,
                         unsigned binding_table_index,
                         unsigned msg_type,
                         unsigned msg_control,
                         unsigned msg_length,
                         unsigned response_length,
                         bool header_present,
                         bool end_of_thread);

void gen8_set_dp_scratch_message(const struct brw_context *brw,
                                 struct gen8_instruction *inst,
                                 bool write,
                                 bool dword,
                                 bool invalidate_after_read,
                                 unsigned num_regs,
                                 unsigned addr_offset,
                                 unsigned msg_length,
                                 unsigned response_length,
                                 bool header_present,
                                 bool end_of_thread);

/** Disassemble the instruction. */
int gen8_disassemble(FILE *file, struct gen8_instruction *inst, int gen);


/**
 * Fetch a set of contiguous bits from the instruction.
 *
 * Bits indexes range from 0..127; fields may not cross 32-bit boundaries.
 */
static inline unsigned
gen8_instruction_bits(struct gen8_instruction *inst, unsigned high, unsigned low)
{
   /* We assume the field doesn't cross 32-bit boundaries. */
   const unsigned word = high / 32;
   assert(word == low / 32);

   high %= 32;
   low %= 32;

   const unsigned mask = (((1 << (high - low + 1)) - 1) << low);

   return (inst->data[word] & mask) >> low;
}

/**
 * Set bits in the instruction, with proper shifting and masking.
 *
 * Bits indexes range from 0..127; fields may not cross 32-bit boundaries.
 */
static inline void
gen8_instruction_set_bits(struct gen8_instruction *inst,
                          unsigned high,
                          unsigned low,
                          unsigned value)
{
   const unsigned word = high / 32;
   assert(word == low / 32);

   high %= 32;
   low %= 32;

   const unsigned mask = (((1 << (high - low + 1)) - 1) << low);

   /* Make sure the supplied value actually fits in the given bitfield. */
   assert((value & (mask >> low)) == value);

   inst->data[word] = (inst->data[word] & ~mask) | ((value << low) & mask);
}

#ifdef __cplusplus
}
#endif

#endif
