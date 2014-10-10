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

#ifndef TOY_COMPILER_H
#define TOY_COMPILER_H

#include "genhw/genhw.h"
#include "util/u_slab.h"

#include "ilo_common.h"
#include "toy_compiler_reg.h"

/**
 * Toy opcodes.
 */
enum toy_opcode {
   /* 0..127 are reserved for GEN6_OPCODE_x */
   TOY_OPCODE_LAST_HW = 127,

   TOY_OPCODE_DO,

   /* TGSI register functions */
   TOY_OPCODE_TGSI_IN,
   TOY_OPCODE_TGSI_CONST,
   TOY_OPCODE_TGSI_SV,
   TOY_OPCODE_TGSI_IMM,
   TOY_OPCODE_TGSI_INDIRECT_FETCH,
   TOY_OPCODE_TGSI_INDIRECT_STORE,

   /* TGSI sampling functions */
   TOY_OPCODE_TGSI_TEX,
   TOY_OPCODE_TGSI_TXB,
   TOY_OPCODE_TGSI_TXD,
   TOY_OPCODE_TGSI_TXL,
   TOY_OPCODE_TGSI_TXP,
   TOY_OPCODE_TGSI_TXF,
   TOY_OPCODE_TGSI_TXQ,
   TOY_OPCODE_TGSI_TXQ_LZ,
   TOY_OPCODE_TGSI_TEX2,
   TOY_OPCODE_TGSI_TXB2,
   TOY_OPCODE_TGSI_TXL2,
   TOY_OPCODE_TGSI_SAMPLE,
   TOY_OPCODE_TGSI_SAMPLE_I,
   TOY_OPCODE_TGSI_SAMPLE_I_MS,
   TOY_OPCODE_TGSI_SAMPLE_B,
   TOY_OPCODE_TGSI_SAMPLE_C,
   TOY_OPCODE_TGSI_SAMPLE_C_LZ,
   TOY_OPCODE_TGSI_SAMPLE_D,
   TOY_OPCODE_TGSI_SAMPLE_L,
   TOY_OPCODE_TGSI_GATHER4,
   TOY_OPCODE_TGSI_SVIEWINFO,
   TOY_OPCODE_TGSI_SAMPLE_POS,
   TOY_OPCODE_TGSI_SAMPLE_INFO,

   /* math functions */
   TOY_OPCODE_INV,
   TOY_OPCODE_LOG,
   TOY_OPCODE_EXP,
   TOY_OPCODE_SQRT,
   TOY_OPCODE_RSQ,
   TOY_OPCODE_SIN,
   TOY_OPCODE_COS,
   TOY_OPCODE_FDIV,
   TOY_OPCODE_POW,
   TOY_OPCODE_INT_DIV_QUOTIENT,
   TOY_OPCODE_INT_DIV_REMAINDER,

   /* URB functions */
   TOY_OPCODE_URB_WRITE,

   /* GS-specific functions */
   TOY_OPCODE_EMIT,
   TOY_OPCODE_ENDPRIM,

   /* FS-specific functions */
   TOY_OPCODE_DDX,
   TOY_OPCODE_DDY,
   TOY_OPCODE_FB_WRITE,
   TOY_OPCODE_KIL,
};

/**
 * Toy instruction.
 */
struct toy_inst {
   unsigned opcode:8;            /* enum toy_opcode      */
   unsigned access_mode:1;       /* GEN6_ALIGN_x          */
   unsigned mask_ctrl:1;         /* GEN6_MASKCTRL_x           */
   unsigned dep_ctrl:2;          /* GEN6_DEPCTRL_x     */
   unsigned qtr_ctrl:2;          /* GEN6_QTRCTRL_x   */
   unsigned thread_ctrl:2;       /* GEN6_THREADCTRL_x         */
   unsigned pred_ctrl:4;         /* GEN6_PREDCTRL_x      */
   unsigned pred_inv:1;          /* true or false        */
   unsigned exec_size:3;         /* GEN6_EXECSIZE_x        */
   unsigned cond_modifier:4;     /* GEN6_COND_x    */
   unsigned acc_wr_ctrl:1;       /* true or false        */
   unsigned saturate:1;          /* true or false        */

   /* true if the instruction should be ignored for instruction iteration */
   unsigned marker:1;

   unsigned pad:1;

   struct toy_dst dst;
   struct toy_src src[5];        /* match TGSI_FULL_MAX_SRC_REGISTERS */

   struct {
      int target;                /* TGSI_TEXTURE_x */
      struct toy_src offsets[1]; /* need to be 4 when GATHER4 is supported */
   } tex;

   struct list_head list;
};

struct toy_compaction_table {
   uint32_t control[32];
   uint32_t datatype[32];
   uint32_t subreg[32];
   uint32_t src[32];
};

/**
 * Toy compiler.
 */
struct toy_compiler {
   const struct ilo_dev_info *dev;

   struct toy_inst templ;
   struct util_slab_mempool mempool;
   struct list_head instructions;
   struct list_head *iter, *iter_next;

   /* this is not set until toy_compiler_legalize_for_asm() */
   int num_instructions;

   int rect_linear_width;
   int next_vrf;

   bool fail;
   const char *reason;
};

/**
 * Allocate the given number of VRF registers.
 */
static inline int
tc_alloc_vrf(struct toy_compiler *tc, int count)
{
   const int vrf = tc->next_vrf;

   tc->next_vrf += count;

   return vrf;
}

/**
 * Allocate a temporary register.
 */
static inline struct toy_dst
tc_alloc_tmp(struct toy_compiler *tc)
{
   return tdst(TOY_FILE_VRF, tc_alloc_vrf(tc, 1), 0);
}

/**
 * Allocate four temporary registers.
 */
static inline void
tc_alloc_tmp4(struct toy_compiler *tc, struct toy_dst *tmp)
{
   tmp[0] = tc_alloc_tmp(tc);
   tmp[1] = tc_alloc_tmp(tc);
   tmp[2] = tc_alloc_tmp(tc);
   tmp[3] = tc_alloc_tmp(tc);
}

/**
 * Duplicate an instruction at the current location.
 */
static inline struct toy_inst *
tc_duplicate_inst(struct toy_compiler *tc, const struct toy_inst *inst)
{
   struct toy_inst *new_inst;

   new_inst = util_slab_alloc(&tc->mempool);
   if (!new_inst)
      return NULL;

   *new_inst = *inst;
   list_addtail(&new_inst->list, tc->iter_next);

   return new_inst;
}

/**
 * Move an instruction to the current location.
 */
static inline void
tc_move_inst(struct toy_compiler *tc, struct toy_inst *inst)
{
   list_del(&inst->list);
   list_addtail(&inst->list, tc->iter_next);
}

/**
 * Discard an instruction.
 */
static inline void
tc_discard_inst(struct toy_compiler *tc, struct toy_inst *inst)
{
   list_del(&inst->list);
   util_slab_free(&tc->mempool, inst);
}

/**
 * Add a new instruction at the current location, using tc->templ as the
 * template.
 */
static inline struct toy_inst *
tc_add(struct toy_compiler *tc)
{
   return tc_duplicate_inst(tc, &tc->templ);
}

/**
 * A convenient version of tc_add() for instructions with 3 source operands.
 */
static inline struct toy_inst *
tc_add3(struct toy_compiler *tc, unsigned opcode,
        struct toy_dst dst,
        struct toy_src src0,
        struct toy_src src1,
        struct toy_src src2)
{
   struct toy_inst *inst;

   inst = tc_add(tc);
   if (!inst)
      return NULL;

   inst->opcode = opcode;
   inst->dst = dst;
   inst->src[0] = src0;
   inst->src[1] = src1;
   inst->src[2] = src2;

   return inst;
}

/**
 * A convenient version of tc_add() for instructions with 2 source operands.
 */
static inline struct toy_inst *
tc_add2(struct toy_compiler *tc, int opcode,
            struct toy_dst dst,
            struct toy_src src0,
            struct toy_src src1)
{
   return tc_add3(tc, opcode, dst, src0, src1, tsrc_null());
}

/**
 * A convenient version of tc_add() for instructions with 1 source operand.
 */
static inline struct toy_inst *
tc_add1(struct toy_compiler *tc, unsigned opcode,
        struct toy_dst dst,
        struct toy_src src0)
{
   return tc_add2(tc, opcode, dst, src0, tsrc_null());
}

/**
 * A convenient version of tc_add() for instructions without source or
 * destination operands.
 */
static inline struct toy_inst *
tc_add0(struct toy_compiler *tc, unsigned opcode)
{
   return tc_add1(tc, opcode, tdst_null(), tsrc_null());
}

#define TC_ALU0(func, opcode)             \
static inline struct toy_inst *           \
func(struct toy_compiler *tc)             \
{                                         \
   return tc_add0(tc, opcode);            \
}

#define TC_ALU1(func, opcode)             \
static inline struct toy_inst *           \
func(struct toy_compiler *tc,             \
     struct toy_dst dst,                  \
     struct toy_src src)                  \
{                                         \
   return tc_add1(tc, opcode, dst, src);  \
}

#define TC_ALU2(func, opcode)             \
static inline struct toy_inst *           \
func(struct toy_compiler *tc,             \
     struct toy_dst dst,                  \
     struct toy_src src0,                 \
     struct toy_src src1)                 \
{                                         \
   return tc_add2(tc, opcode,             \
         dst, src0, src1);                \
}

#define TC_ALU3(func, opcode)             \
static inline struct toy_inst *           \
func(struct toy_compiler *tc,             \
     struct toy_dst dst,                  \
     struct toy_src src0,                 \
     struct toy_src src1,                 \
     struct toy_src src2)                 \
{                                         \
   return tc_add3(tc, opcode,             \
         dst, src0, src1, src2);          \
}

#define TC_CND2(func, opcode)             \
static inline struct toy_inst *           \
func(struct toy_compiler *tc,             \
     struct toy_dst dst,                  \
     struct toy_src src0,                 \
     struct toy_src src1,                 \
     unsigned cond_modifier)              \
{                                         \
   struct toy_inst *inst;                 \
   inst = tc_add2(tc, opcode,             \
         dst, src0, src1);                \
   inst->cond_modifier = cond_modifier;   \
   return inst;                           \
}

TC_ALU0(tc_NOP, GEN6_OPCODE_NOP)
TC_ALU0(tc_ELSE, GEN6_OPCODE_ELSE)
TC_ALU0(tc_ENDIF, GEN6_OPCODE_ENDIF)
TC_ALU1(tc_MOV, GEN6_OPCODE_MOV)
TC_ALU1(tc_RNDD, GEN6_OPCODE_RNDD)
TC_ALU1(tc_INV, TOY_OPCODE_INV)
TC_ALU1(tc_FRC, GEN6_OPCODE_FRC)
TC_ALU1(tc_EXP, TOY_OPCODE_EXP)
TC_ALU1(tc_LOG, TOY_OPCODE_LOG)
TC_ALU2(tc_ADD, GEN6_OPCODE_ADD)
TC_ALU2(tc_MUL, GEN6_OPCODE_MUL)
TC_ALU2(tc_AND, GEN6_OPCODE_AND)
TC_ALU2(tc_OR, GEN6_OPCODE_OR)
TC_ALU2(tc_DP2, GEN6_OPCODE_DP2)
TC_ALU2(tc_DP3, GEN6_OPCODE_DP3)
TC_ALU2(tc_DP4, GEN6_OPCODE_DP4)
TC_ALU2(tc_SHL, GEN6_OPCODE_SHL)
TC_ALU2(tc_SHR, GEN6_OPCODE_SHR)
TC_ALU2(tc_POW, TOY_OPCODE_POW)
TC_ALU3(tc_MAC, GEN6_OPCODE_MAC)
TC_CND2(tc_SEL, GEN6_OPCODE_SEL)
TC_CND2(tc_CMP, GEN6_OPCODE_CMP)
TC_CND2(tc_IF, GEN6_OPCODE_IF)
TC_CND2(tc_SEND, GEN6_OPCODE_SEND)

/**
 * Upcast a list_head to an instruction.
 */
static inline struct toy_inst *
tc_list_to_inst(struct toy_compiler *tc, struct list_head *item)
{
   return container_of(item, (struct toy_inst *) NULL, list);
}

/**
 * Return the instruction at the current location.
 */
static inline struct toy_inst *
tc_current(struct toy_compiler *tc)
{
   return (tc->iter != &tc->instructions) ?
      tc_list_to_inst(tc, tc->iter) : NULL;
}

/**
 * Set the current location to the head.
 */
static inline void
tc_head(struct toy_compiler *tc)
{
   tc->iter = &tc->instructions;
   tc->iter_next = tc->iter->next;
}

/**
 * Set the current location to the tail.
 */
static inline void
tc_tail(struct toy_compiler *tc)
{
   tc->iter = &tc->instructions;
   tc->iter_next = tc->iter;
}

/**
 * Advance the current location.
 */
static inline struct toy_inst *
tc_next_no_skip(struct toy_compiler *tc)
{
   /* stay at the tail so that new instructions are added there */
   if (tc->iter_next == &tc->instructions) {
      tc_tail(tc);
      return NULL;
   }

   tc->iter = tc->iter_next;
   tc->iter_next = tc->iter_next->next;

   return tc_list_to_inst(tc, tc->iter);
}

/**
 * Advance the current location, skipping markers.
 */
static inline struct toy_inst *
tc_next(struct toy_compiler *tc)
{
   struct toy_inst *inst;

   do {
      inst = tc_next_no_skip(tc);
   } while (inst && inst->marker);

   return inst;
}

static inline void
tc_fail(struct toy_compiler *tc, const char *reason)
{
   if (!tc->fail) {
      tc->fail = true;
      tc->reason = reason;
   }
}

void
toy_compiler_init(struct toy_compiler *tc, const struct ilo_dev_info *dev);

void
toy_compiler_cleanup(struct toy_compiler *tc);

void
toy_compiler_dump(struct toy_compiler *tc);

void *
toy_compiler_assemble(struct toy_compiler *tc, int *size);

const struct toy_compaction_table *
toy_compiler_get_compaction_table(const struct ilo_dev_info *dev);

void
toy_compiler_disassemble(const struct ilo_dev_info *dev,
                         const void *kernel, int size,
                         bool dump_hex);

#endif /* TOY_COMPILER_H */
