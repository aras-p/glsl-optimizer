/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#include <stdio.h>
#include "genhw/genhw.h"
#include "toy_compiler.h"

#define DISASM_PRINTER_BUFFER_SIZE 256
#define DISASM_PRINTER_COLUMN_WIDTH 16

#define READ(dw, field) (((dw) & field ## __MASK) >> field ## __SHIFT)

struct disasm_printer {
   char buf[DISASM_PRINTER_BUFFER_SIZE];
   int len;
};

struct disasm_operand {
   unsigned file:2;
   unsigned type:3;

   unsigned addr_mode:1;
   unsigned reg:8;
   unsigned subreg:5;
   unsigned addr_subreg:3;
   unsigned addr_imm:10;
};

struct disasm_dst_operand {
   struct disasm_operand base;

   unsigned horz_stride:2;
   unsigned writemask:4;
};

struct disasm_src_operand {
   struct disasm_operand base;

   unsigned vert_stride:4;
   unsigned width:3;
   unsigned horz_stride:2;
   unsigned swizzle_x:2;
   unsigned swizzle_y:2;
   unsigned swizzle_z:2;
   unsigned swizzle_w:2;
   unsigned negate:1;
   unsigned absolute:1;
};

struct disasm_inst {
   const struct ilo_dev_info *dev;

   unsigned has_jip:1;
   unsigned has_uip:1;

   unsigned opcode:7;
   unsigned access_mode:1;
   unsigned mask_ctrl:1;
   unsigned dep_ctrl:2;
   unsigned qtr_ctrl:2;
   unsigned thread_ctrl:2;
   unsigned pred_ctrl:4;
   unsigned pred_inv:1;
   unsigned exec_size:3;

   unsigned cond_modifier:4;
   unsigned sfid:4;
   unsigned fc:4;

   unsigned acc_wr_ctrl:1;
   unsigned cmpt_ctrl:1;
   unsigned debug_ctrl:1;
   unsigned saturate:1;

   unsigned nib_ctrl:1;

   unsigned flag_reg:1;
   unsigned flag_subreg:1;

   struct disasm_dst_operand dst;
   struct disasm_src_operand src0;
   struct disasm_src_operand src1;
   union {
      struct disasm_src_operand src2;
      uint32_t imm32;
   } u;
};

static const struct {
   const char *name;
   int src_count;
} disasm_opcode_table[128] = {
   [GEN6_OPCODE_ILLEGAL]     = { "illegal",  0 },
   [GEN6_OPCODE_MOV]         = { "mov",      1 },
   [GEN6_OPCODE_SEL]         = { "sel",      2 },
   [GEN6_OPCODE_MOVI]        = { "movi",     1 },
   [GEN6_OPCODE_NOT]         = { "not",      1 },
   [GEN6_OPCODE_AND]         = { "and",      2 },
   [GEN6_OPCODE_OR]          = { "or",       2 },
   [GEN6_OPCODE_XOR]         = { "xor",      2 },
   [GEN6_OPCODE_SHR]         = { "shr",      2 },
   [GEN6_OPCODE_SHL]         = { "shl",      2 },
   [GEN6_OPCODE_DIM]         = { "dim",      1 },
   [GEN6_OPCODE_ASR]         = { "asr",      2 },
   [GEN6_OPCODE_CMP]         = { "cmp",      2 },
   [GEN6_OPCODE_CMPN]        = { "cmpn",     2 },
   [GEN7_OPCODE_CSEL]        = { "csel",     3 },
   [GEN7_OPCODE_F32TO16]     = { "f32to16",  1 },
   [GEN7_OPCODE_F16TO32]     = { "f16to32",  1 },
   [GEN7_OPCODE_BFREV]       = { "bfrev",    1 },
   [GEN7_OPCODE_BFE]         = { "bfe",      3 },
   [GEN7_OPCODE_BFI1]        = { "bfi1",     2 },
   [GEN7_OPCODE_BFI2]        = { "bfi2",     3 },
   [GEN6_OPCODE_JMPI]        = { "jmpi",     1 },
   [GEN7_OPCODE_BRD]         = { "brd",      1 },
   [GEN6_OPCODE_IF]          = { "if",       2 },
   [GEN7_OPCODE_BRC]         = { "brc",      1 },
   [GEN6_OPCODE_ELSE]        = { "else",     1 },
   [GEN6_OPCODE_ENDIF]       = { "endif",    0 },
   [GEN6_OPCODE_CASE]        = { "case",     2 },
   [GEN6_OPCODE_WHILE]       = { "while",    1 },
   [GEN6_OPCODE_BREAK]       = { "break",    1 },
   [GEN6_OPCODE_CONT]        = { "cont",     1 },
   [GEN6_OPCODE_HALT]        = { "halt",     1 },
   [GEN75_OPCODE_CALLA]      = { "calla",    1 },
   [GEN6_OPCODE_CALL]        = { "call",     1 },
   [GEN6_OPCODE_RETURN]      = { "return",   1 },
   [GEN6_OPCODE_WAIT]        = { "wait",     1 },
   [GEN6_OPCODE_SEND]        = { "send",     1 },
   [GEN6_OPCODE_SENDC]       = { "sendc",    1 },
   [GEN6_OPCODE_MATH]        = { "math",     2 },
   [GEN6_OPCODE_ADD]         = { "add",      2 },
   [GEN6_OPCODE_MUL]         = { "mul",      2 },
   [GEN6_OPCODE_AVG]         = { "avg",      2 },
   [GEN6_OPCODE_FRC]         = { "frc",      1 },
   [GEN6_OPCODE_RNDU]        = { "rndu",     1 },
   [GEN6_OPCODE_RNDD]        = { "rndd",     1 },
   [GEN6_OPCODE_RNDE]        = { "rnde",     1 },
   [GEN6_OPCODE_RNDZ]        = { "rndz",     1 },
   [GEN6_OPCODE_MAC]         = { "mac",      2 },
   [GEN6_OPCODE_MACH]        = { "mach",     2 },
   [GEN6_OPCODE_LZD]         = { "lzd",      1 },
   [GEN7_OPCODE_FBH]         = { "fbh",      1 },
   [GEN7_OPCODE_FBL]         = { "fbl",      1 },
   [GEN7_OPCODE_CBIT]        = { "cbit",     1 },
   [GEN7_OPCODE_ADDC]        = { "addc",     2 },
   [GEN7_OPCODE_SUBB]        = { "subb",     2 },
   [GEN6_OPCODE_SAD2]        = { "sad2",     2 },
   [GEN6_OPCODE_SADA2]       = { "sada2",    2 },
   [GEN6_OPCODE_DP4]         = { "dp4",      2 },
   [GEN6_OPCODE_DPH]         = { "dph",      2 },
   [GEN6_OPCODE_DP3]         = { "dp3",      2 },
   [GEN6_OPCODE_DP2]         = { "dp2",      2 },
   [GEN6_OPCODE_LINE]        = { "line",     2 },
   [GEN6_OPCODE_PLN]         = { "pln",      2 },
   [GEN6_OPCODE_MAD]         = { "mad",      3 },
   [GEN6_OPCODE_LRP]         = { "lrp",      3 },
   [GEN6_OPCODE_NOP]         = { "nop",      0 },
};

static void
disasm_inst_decode_dw0_gen6(struct disasm_inst *inst, uint32_t dw0)
{
   inst->opcode = READ(dw0, GEN6_INST_OPCODE);

   switch (inst->opcode) {
   case GEN6_OPCODE_IF:
      inst->has_jip = true;
      inst->has_uip = (inst->dev->gen >= ILO_GEN(7));
      break;
   case GEN6_OPCODE_BREAK:
   case GEN6_OPCODE_CONT:
   case GEN6_OPCODE_HALT:
      inst->has_uip = true;
      /* fall through */
   case GEN6_OPCODE_JMPI:
   case GEN7_OPCODE_BRD:
   case GEN7_OPCODE_BRC:
   case GEN6_OPCODE_ELSE:
   case GEN6_OPCODE_ENDIF:
   case GEN6_OPCODE_CASE:
   case GEN6_OPCODE_WHILE:
   case GEN75_OPCODE_CALLA:
   case GEN6_OPCODE_CALL:
   case GEN6_OPCODE_RETURN:
      inst->has_jip = true;
      break;
   default:
      break;
   }

   inst->access_mode = READ(dw0, GEN6_INST_ACCESSMODE);
   inst->mask_ctrl = READ(dw0, GEN6_INST_MASKCTRL);
   inst->dep_ctrl = READ(dw0, GEN6_INST_DEPCTRL);
   inst->qtr_ctrl = READ(dw0, GEN6_INST_QTRCTRL);
   inst->thread_ctrl = READ(dw0, GEN6_INST_THREADCTRL);
   inst->pred_ctrl = READ(dw0, GEN6_INST_PREDCTRL);

   inst->pred_inv = (bool) (dw0 & GEN6_INST_PREDINV);

   inst->exec_size = READ(dw0, GEN6_INST_EXECSIZE);

   switch (inst->opcode) {
   case GEN6_OPCODE_SEND:
   case GEN6_OPCODE_SENDC:
      inst->sfid = READ(dw0, GEN6_INST_SFID);
      break;
   case GEN6_OPCODE_MATH:
      inst->fc = READ(dw0, GEN6_INST_FC);
      break;
   default:
      inst->cond_modifier = READ(dw0, GEN6_INST_CONDMODIFIER);
      break;
   }

   inst->acc_wr_ctrl = (bool) (dw0 & GEN6_INST_ACCWRCTRL);
   inst->cmpt_ctrl = (bool) (dw0 & GEN6_INST_CMPTCTRL);
   inst->debug_ctrl = (bool) (dw0 & GEN6_INST_DEBUGCTRL);
   inst->saturate = (bool) (dw0 & GEN6_INST_SATURATE);
}

static bool
disasm_inst_jip_in_dw1_high_gen6(const struct disasm_inst *inst)
{
   return (inst->dev->gen == ILO_GEN(6) && inst->has_jip && !inst->has_uip);
}

static void
disasm_inst_decode_dw1_gen6(struct disasm_inst *inst, uint32_t dw1)
{
   inst->dst.base.file = READ(dw1, GEN6_INST_DST_FILE);
   inst->dst.base.type = READ(dw1, GEN6_INST_DST_TYPE);
   inst->src0.base.file = READ(dw1, GEN6_INST_SRC0_FILE);
   inst->src0.base.type = READ(dw1, GEN6_INST_SRC0_TYPE);
   inst->src1.base.file = READ(dw1, GEN6_INST_SRC1_FILE);
   inst->src1.base.type = READ(dw1, GEN6_INST_SRC1_TYPE);

   if (inst->dev->gen >= ILO_GEN(7))
      inst->nib_ctrl = (bool) (dw1 & GEN7_INST_NIBCTRL);

   if (disasm_inst_jip_in_dw1_high_gen6(inst)) {
      inst->u.imm32 = dw1 >> 16;
      return;
   }

   inst->dst.base.addr_mode = READ(dw1, GEN6_INST_DST_ADDRMODE);

   if (inst->dst.base.addr_mode == GEN6_ADDRMODE_DIRECT) {
      inst->dst.base.reg = READ(dw1, GEN6_INST_DST_REG);

      if (inst->access_mode == GEN6_ALIGN_1) {
         inst->dst.base.subreg = READ(dw1, GEN6_INST_DST_SUBREG);
      } else {
         inst->dst.base.subreg = READ(dw1, GEN6_INST_DST_SUBREG_ALIGN16) <<
            GEN6_INST_DST_SUBREG_ALIGN16__SHR;
      }
   } else {
      inst->dst.base.addr_subreg = READ(dw1, GEN6_INST_DST_ADDR_SUBREG);

      if (inst->access_mode == GEN6_ALIGN_1) {
         inst->dst.base.addr_imm = READ(dw1, GEN6_INST_DST_ADDR_IMM);
      } else {
         inst->dst.base.addr_imm = READ(dw1, GEN6_INST_DST_ADDR_IMM_ALIGN16) <<
            GEN6_INST_DST_ADDR_IMM_ALIGN16__SHR;
      }
   }

   inst->dst.horz_stride = READ(dw1, GEN6_INST_DST_HORZSTRIDE);

   if (inst->access_mode == GEN6_ALIGN_1)
      inst->dst.writemask = 0xf;
   else
      inst->dst.writemask = READ(dw1, GEN6_INST_DST_WRITEMASK);
}

static void
disasm_inst_decode_dw2_dw3_gen6(struct disasm_inst *inst,
                                uint32_t dw2, uint32_t dw3)
{
   int count, i;

   if (inst->dev->gen >= ILO_GEN(7))
      inst->flag_reg = READ(dw2, GEN7_INST_FLAG_REG);

   inst->flag_subreg = READ(dw2, GEN6_INST_FLAG_SUBREG);

   if (inst->src0.base.file == GEN6_FILE_IMM ||
       inst->src1.base.file == GEN6_FILE_IMM) {
      count = 1;
      if (!disasm_inst_jip_in_dw1_high_gen6(inst))
         inst->u.imm32 = dw3;
   } else {
      count = 2;
   }

   for (i = 0; i < count; i++) {
      struct disasm_src_operand *src = (i == 0) ? &inst->src0 : &inst->src1;
      const uint32_t dw = (i == 0) ? dw2 : dw3;

      src->base.addr_mode = READ(dw, GEN6_INST_SRC_ADDRMODE);

      if (src->base.addr_mode == GEN6_ADDRMODE_DIRECT) {
         src->base.reg = READ(dw, GEN6_INST_SRC_REG);

         if (inst->access_mode == GEN6_ALIGN_1) {
            src->base.subreg = READ(dw, GEN6_INST_SRC_SUBREG);
         } else {
            src->base.subreg = READ(dw, GEN6_INST_SRC_SUBREG_ALIGN16) <<
               GEN6_INST_SRC_SUBREG_ALIGN16__SHR;
         }
      } else {
         src->base.addr_subreg = READ(dw, GEN6_INST_SRC_ADDR_SUBREG);

         if (inst->access_mode == GEN6_ALIGN_1) {
            src->base.addr_imm = READ(dw, GEN6_INST_SRC_ADDR_IMM);
         } else {
            src->base.addr_imm = READ(dw, GEN6_INST_SRC_ADDR_IMM_ALIGN16) <<
               GEN6_INST_SRC_ADDR_IMM_ALIGN16__SHR;
         }
      }

      src->vert_stride = READ(dw, GEN6_INST_SRC_VERTSTRIDE);

      if (inst->access_mode == GEN6_ALIGN_1) {
         src->width = READ(dw, GEN6_INST_SRC_WIDTH);
         src->horz_stride = READ(dw, GEN6_INST_SRC_HORZSTRIDE);

         src->swizzle_x = GEN6_SWIZZLE_X;
         src->swizzle_y = GEN6_SWIZZLE_Y;
         src->swizzle_z = GEN6_SWIZZLE_Z;
         src->swizzle_w = GEN6_SWIZZLE_W;
      } else {
         src->width = GEN6_WIDTH_4;
         src->horz_stride = GEN6_HORZSTRIDE_1;

         src->swizzle_x = READ(dw, GEN6_INST_SRC_SWIZZLE_X);
         src->swizzle_y = READ(dw, GEN6_INST_SRC_SWIZZLE_Y);
         src->swizzle_z = READ(dw, GEN6_INST_SRC_SWIZZLE_Z);
         src->swizzle_w = READ(dw, GEN6_INST_SRC_SWIZZLE_W);
      }

      src->negate = (bool) (dw & GEN6_INST_SRC_NEGATE);
      src->absolute = (bool) (dw & GEN6_INST_SRC_ABSOLUTE);
   }
}

static void
disasm_inst_decode_3src_dw1_gen6(struct disasm_inst *inst, uint32_t dw1)
{
   static unsigned type_mapping[4] = {
      [GEN7_TYPE_F_3SRC]   = GEN6_TYPE_F,
      [GEN7_TYPE_D_3SRC]   = GEN6_TYPE_D,
      [GEN7_TYPE_UD_3SRC]  = GEN6_TYPE_UD,
      [GEN7_TYPE_DF_3SRC]  = GEN7_TYPE_DF,
   };

   inst->flag_subreg = READ(dw1, GEN6_3SRC_FLAG_SUBREG);

   if (inst->dev->gen >= ILO_GEN(7)) {
      inst->nib_ctrl = (bool) (dw1 & GEN7_3SRC_NIBCTRL);
      inst->flag_reg = READ(dw1, GEN7_3SRC_FLAG_REG);

      inst->dst.base.file = GEN6_FILE_GRF;
      inst->dst.base.type = READ(dw1, GEN7_3SRC_DST_TYPE);
      inst->dst.base.type = type_mapping[inst->dst.base.type];

      inst->src0.base.type = READ(dw1, GEN7_3SRC_SRC_TYPE);
      inst->src0.base.type = type_mapping[inst->src0.base.type];

      inst->src1.base.type = inst->src0.base.type;
      inst->u.src2.base.type = inst->src0.base.type;
   } else {
      inst->dst.base.file = (dw1 & GEN6_3SRC_DST_FILE_MRF) ?
         GEN6_FILE_MRF: GEN6_FILE_GRF;
      inst->dst.base.type = GEN6_TYPE_F;

      inst->src0.base.type = GEN6_TYPE_F;
      inst->src1.base.type = GEN6_TYPE_F;
      inst->u.src2.base.type = GEN6_TYPE_F;
   }

   inst->dst.base.addr_mode = GEN6_ADDRMODE_DIRECT;
   inst->dst.base.reg = READ(dw1, GEN6_3SRC_DST_REG);
   inst->dst.base.subreg = READ(dw1, GEN6_3SRC_DST_SUBREG) <<
      GEN6_3SRC_DST_SUBREG__SHR;

   inst->dst.horz_stride = GEN6_HORZSTRIDE_1;
   inst->dst.writemask = READ(dw1, GEN6_3SRC_DST_WRITEMASK);

   inst->src0.base.file = GEN6_FILE_GRF;
   inst->src0.negate = (bool) (dw1 & GEN6_3SRC_SRC0_NEGATE);
   inst->src0.absolute = (bool) (dw1 & GEN6_3SRC_SRC0_ABSOLUTE);
   inst->src1.base.file = GEN6_FILE_GRF;
   inst->src1.negate = (bool) (dw1 & GEN6_3SRC_SRC1_NEGATE);
   inst->src1.absolute = (bool) (dw1 & GEN6_3SRC_SRC1_ABSOLUTE);
   inst->u.src2.base.file = GEN6_FILE_GRF;
   inst->u.src2.negate = (bool) (dw1 & GEN6_3SRC_SRC2_NEGATE);
   inst->u.src2.absolute = (bool) (dw1 & GEN6_3SRC_SRC2_ABSOLUTE);
}

static void
disasm_inst_decode_3src_dw2_dw3_gen6(struct disasm_inst *inst,
                                     uint32_t dw2, uint32_t dw3)
{
   const uint64_t qw = (uint64_t) dw3 << 32 | dw2;
   int i;

   for (i = 0; i < 3; i++) {
      struct disasm_src_operand *src = (i == 0) ? &inst->src0 :
                                       (i == 1) ? &inst->src1 :
                                       &inst->u.src2;
      const uint32_t dw = (i == 0) ? READ(qw, GEN6_3SRC_SRC_0) :
                          (i == 1) ? READ(qw, GEN6_3SRC_SRC_1) :
                          READ(qw, GEN6_3SRC_SRC_2);

      src->base.addr_mode = GEN6_ADDRMODE_DIRECT;
      src->base.reg = READ(dw, GEN6_3SRC_SRC_REG);
      src->base.subreg = READ(dw, GEN6_3SRC_SRC_SUBREG) <<
         GEN6_3SRC_SRC_SUBREG__SHR;

      if (dw & GEN6_3SRC_SRC_REPCTRL) {
         src->vert_stride = GEN6_VERTSTRIDE_0;
         src->width = GEN6_WIDTH_1;
         src->horz_stride = GEN6_HORZSTRIDE_0;
      } else {
         src->vert_stride = GEN6_VERTSTRIDE_4;
         src->width = GEN6_WIDTH_4;
         src->horz_stride = GEN6_HORZSTRIDE_1;
      }

      src->swizzle_x = READ(dw, GEN6_3SRC_SRC_SWIZZLE_X);
      src->swizzle_y = READ(dw, GEN6_3SRC_SRC_SWIZZLE_Y);
      src->swizzle_z = READ(dw, GEN6_3SRC_SRC_SWIZZLE_Z);
      src->swizzle_w = READ(dw, GEN6_3SRC_SRC_SWIZZLE_W);
   }
}

/*
 * When GEN6_INST_CMPTCTRL of DW0 is set, the instruction has 64 bits and is
 * in EU_INSTRUCTION_COMPACT_TWO_SRC form.  We should have expanded it to its
 * original form.
 *
 * Depending on the opcode, the 128-bits instruction is in one of the
 * following forms
 *
 *  - EU_INSTRUCTION_BASIC_ONE_SRC
 *  - EU_INSTRUCTION_BASIC_TWO_SRC
 *  - EU_INSTRUCTION_BASIC_THREE_SRC
 *  - EU_INSTRUCTION_BRANCH_CONDITIONAL
 *  - EU_INSTRUCTION_BRANCH_ONE_SRC
 *  - EU_INSTRUCTION_BRANCH_TWO_SRC
 *  - EU_INSTRUCTION_ILLEGAL
 *  - EU_INSTRUCTION_MATH
 *  - EU_INSTRUCTION_NOP
 *  - EU_INSTRUCTION_SEND
 *
 * In EU_INSTRUCTION_BASIC_ONE_SRC form,
 *
 *  - DW0 is EU_INSTRUCTION_HEADER
 *  - DW1 is EU_INSTRUCTION_OPERAND_CONTROLS
 *  - DW2 is Source 0 and EU_INSTRUCTION_FLAGS
 *  - DW3 is reserved unless Source 0 is an immediate
 *
 * All other forms except EU_INSTRUCTION_BASIC_THREE_SRC are quite compatible
 * with EU_INSTRUCTION_BASIC_ONE_SRC.
 */
static void
disasm_inst_decode(struct disasm_inst *inst,
                   const uint32_t *dw)
{
   assert(!(dw[0] & GEN6_INST_CMPTCTRL));

   disasm_inst_decode_dw0_gen6(inst, dw[0]);

   switch (inst->opcode) {
   case GEN7_OPCODE_CSEL:
   case GEN7_OPCODE_BFE:
   case GEN7_OPCODE_BFI2:
   case GEN6_OPCODE_MAD:
   case GEN6_OPCODE_LRP:
      disasm_inst_decode_3src_dw1_gen6(inst, dw[1]);
      disasm_inst_decode_3src_dw2_dw3_gen6(inst, dw[2], dw[3]);
      break;
   default:
      disasm_inst_decode_dw1_gen6(inst, dw[1]);
      disasm_inst_decode_dw2_dw3_gen6(inst, dw[2], dw[3]);
      break;
   }
}

static const char *
disasm_inst_opcode(const struct disasm_inst *inst)
{
   return (disasm_opcode_table[inst->opcode].name) ?
      disasm_opcode_table[inst->opcode].name : "BAD";
}

static const char *
disasm_inst_pred_ctrl(const struct disasm_inst *inst)
{
   if (inst->access_mode == GEN6_ALIGN_1) {
      switch (inst->pred_ctrl) {
      case GEN6_PREDCTRL_NORMAL: return "";
      case GEN6_PREDCTRL_ANYV:   return ".anyv";
      case GEN6_PREDCTRL_ALLV:   return ".allv";
      case GEN6_PREDCTRL_ANY2H:  return ".any2h";
      case GEN6_PREDCTRL_ALL2H:  return ".all2h";
      case GEN6_PREDCTRL_ANY4H:  return ".any4h";
      case GEN6_PREDCTRL_ALL4H:  return ".all4h";
      case GEN6_PREDCTRL_ANY8H:  return ".any8h";
      case GEN6_PREDCTRL_ALL8H:  return ".all8h";
      case GEN6_PREDCTRL_ANY16H: return ".any16h";
      case GEN6_PREDCTRL_ALL16H: return ".all16h";
      case GEN7_PREDCTRL_ANY32H: return ".any32h";
      case GEN7_PREDCTRL_ALL32H: return ".all32h";
      default:                   return ".BAD";
      }
   } else {
      switch (inst->pred_ctrl) {
      case GEN6_PREDCTRL_NORMAL: return "";
      case GEN6_PREDCTRL_X:      return ".x";
      case GEN6_PREDCTRL_Y:      return ".y";
      case GEN6_PREDCTRL_Z:      return ".z";
      case GEN6_PREDCTRL_W:      return ".w";
      default:                   return ".BAD";
      }
   }
}

static char
disasm_inst_pred_inv(const struct disasm_inst *inst)
{
   return (inst->pred_inv) ? '-' : '+';
}

static const char *
disasm_inst_exec_size(const struct disasm_inst *inst)
{
   switch (inst->exec_size) {
   case GEN6_EXECSIZE_1:   return "1";
   case GEN6_EXECSIZE_2:   return "2";
   case GEN6_EXECSIZE_4:   return "4";
   case GEN6_EXECSIZE_8:   return "8";
   case GEN6_EXECSIZE_16:  return "16";
   case GEN6_EXECSIZE_32:  return "32";
   default:                return "BAD";
   }
}

static const char *
disasm_inst_fc(const struct disasm_inst *inst)
{
   assert(inst->opcode == GEN6_OPCODE_MATH);

   switch (inst->fc) {
   case GEN6_MATH_INV:                 return "inv";
   case GEN6_MATH_LOG:                 return "log";
   case GEN6_MATH_EXP:                 return "exp";
   case GEN6_MATH_SQRT:                return "sqrt";
   case GEN6_MATH_RSQ:                 return "rsq";
   case GEN6_MATH_SIN:                 return "sin";
   case GEN6_MATH_COS:                 return "cos";
   case GEN6_MATH_FDIV:                return "fdiv";
   case GEN6_MATH_POW:                 return "pow";
   case GEN6_MATH_INT_DIV:             return "int_div";
   case GEN6_MATH_INT_DIV_QUOTIENT:    return "int_div_quotient";
   case GEN6_MATH_INT_DIV_REMAINDER:   return "int_div_remainder";
   default:                            return "BAD";
   }
}

static const char *
disasm_inst_sfid(const struct disasm_inst *inst)
{
   assert(inst->opcode == GEN6_OPCODE_SEND ||
          inst->opcode == GEN6_OPCODE_SENDC);

   switch (inst->sfid) {
   case GEN6_SFID_NULL:          return "null";
   case GEN6_SFID_SAMPLER:       return "sampler";
   case GEN6_SFID_GATEWAY:       return "gateway";
   case GEN6_SFID_DP_SAMPLER:    return "dp sampler";
   case GEN6_SFID_DP_RC:         return "dp render";
   case GEN6_SFID_URB:           return "urb";
   case GEN6_SFID_SPAWNER:       return "spawner";
   case GEN6_SFID_VME:           return "vme";
   case GEN6_SFID_DP_CC:         return "dp const";
   case GEN7_SFID_DP_DC0:        return "dp data 0";
   case GEN7_SFID_PI:            return "pixel interp";
   case GEN75_SFID_DP_DC1:       return "dp data 1";
   default:                      return "BAD";
   }
}

static const char *
disasm_inst_cond_modifier(const struct disasm_inst *inst)
{
   switch (inst->cond_modifier) {
   case GEN6_COND_NONE:    return "";
   case GEN6_COND_Z:       return ".z";
   case GEN6_COND_NZ:      return ".nz";
   case GEN6_COND_G:       return ".g";
   case GEN6_COND_GE:      return ".ge";
   case GEN6_COND_L:       return ".l";
   case GEN6_COND_LE:      return ".le";
   case GEN6_COND_O:       return ".o";
   case GEN6_COND_U:       return ".u";
   default:                return ".BAD";
   }
}

static const char *
disasm_inst_debug_ctrl(const struct disasm_inst *inst)
{
   return (inst->debug_ctrl) ? ".breakpoint" : "";
}

static const char *
disasm_inst_saturate(const struct disasm_inst *inst)
{
   return (inst->saturate) ? ".sat" : "";
}

static const char *
disasm_inst_flag_reg(const struct disasm_inst *inst)
{
   static const char *flag_names[2][2] = {
      { "f0",   "f0.1" },
      { "f1.0", "f1.1" },
   };

   return (inst->flag_reg <= 1 && inst->flag_subreg <= 1) ?
      flag_names[inst->flag_reg][inst->flag_subreg] : "fBAD";
}

static const char *
disasm_inst_access_mode(const struct disasm_inst *inst)
{
   switch (inst->access_mode) {
   case GEN6_ALIGN_1:   return " align1";
   case GEN6_ALIGN_16:  return " align16";
   default:             return " alignBAD";
   }
}

static const char *
disasm_inst_mask_ctrl(const struct disasm_inst *inst)
{
   switch (inst->mask_ctrl) {
   case GEN6_MASKCTRL_NORMAL: return "";
   case GEN6_MASKCTRL_NOMASK: return " WE_all";
   default:                   return " WE_BAD";
   }
}

static const char *
disasm_inst_dep_ctrl(const struct disasm_inst *inst)
{
   switch (inst->dep_ctrl) {
   case GEN6_DEPCTRL_NORMAL:  return "";
   case GEN6_DEPCTRL_NODDCLR: return " NoDDClr";
   case GEN6_DEPCTRL_NODDCHK: return " NoDDChk";
   case GEN6_DEPCTRL_NEITHER: return " NoDDClr,NoDDChk";
   default:                   return " NoDDBAD";
   }
}

static const char *
disasm_inst_qtr_ctrl(const struct disasm_inst *inst)
{
   switch (inst->exec_size) {
   case GEN6_EXECSIZE_8:
      switch (inst->qtr_ctrl) {
      case GEN6_QTRCTRL_1Q:   return " 1Q";
      case GEN6_QTRCTRL_2Q:   return " 2Q";
      case GEN6_QTRCTRL_3Q:   return " 3Q";
      case GEN6_QTRCTRL_4Q:   return " 4Q";
      default:                return " BADQ";
      }
      break;
   case GEN6_EXECSIZE_16:
      switch (inst->qtr_ctrl) {
      case GEN6_QTRCTRL_1H:   return " 1H";
      case GEN6_QTRCTRL_2H:   return " 2H";
      default:                return " BADH";
      }
      break;
   default:
      return                  "";
   }

}

static const char *
disasm_inst_thread_ctrl(const struct disasm_inst *inst)
{
   switch (inst->thread_ctrl) {
   case GEN6_THREADCTRL_NORMAL:  return "";
   case GEN6_THREADCTRL_ATOMIC:  return " atomic";
   case GEN6_THREADCTRL_SWITCH:  return " switch";
   default:                      return " BAD";
   }
}

static const char *
disasm_inst_acc_wr_ctrl(const struct disasm_inst *inst)
{
   return (inst->acc_wr_ctrl) ? " AccWrEnable" : "";
}

static const char *
disasm_inst_cmpt_ctrl(const struct disasm_inst *inst)
{
   return (inst->cmpt_ctrl) ? " compacted" : "";
}

static const char *
disasm_inst_eot(const struct disasm_inst *inst)
{
   if (inst->opcode == GEN6_OPCODE_SEND ||
       inst->opcode == GEN6_OPCODE_SENDC)
      return (inst->u.imm32 & GEN6_MSG_EOT) ? " EOT" : "";
   else
      return "";
}

static const char *
disasm_inst_file(const struct disasm_inst *inst,
                 const struct disasm_operand *operand,
                 bool *multi_regs)
{
   switch (operand->file) {
   case GEN6_FILE_ARF:
      switch (operand->reg & 0xf0) {
      case GEN6_ARF_NULL:  *multi_regs = false; return "null";
      case GEN6_ARF_A0:    *multi_regs = true;  return "a";
      case GEN6_ARF_ACC0:  *multi_regs = true;  return "acc";
      case GEN6_ARF_F0:    *multi_regs = true;  return "f";
      case GEN6_ARF_SR0:   *multi_regs = true;  return "sr";
      case GEN6_ARF_CR0:   *multi_regs = true;  return "cr";
      case GEN6_ARF_N0:    *multi_regs = true;  return "n";
      case GEN6_ARF_IP:    *multi_regs = false; return "ip";
      case GEN6_ARF_TDR:   *multi_regs = false; return "tdr";
      case GEN7_ARF_TM0:   *multi_regs = true;  return "tm";
      default:             *multi_regs = false; return "BAD";
      }
      break;
   case GEN6_FILE_GRF:     *multi_regs = true;  return "g";
   case GEN6_FILE_MRF:     *multi_regs = true;  return "m";
   case GEN6_FILE_IMM:     *multi_regs = true;  return "";
   default:                *multi_regs = false; return "BAD";
   }
}

static const char *
disasm_inst_type(const struct disasm_inst *inst,
                 const struct disasm_operand *operand)
{
   if (operand->file == GEN6_FILE_IMM) {
      switch (operand->type) {
      case GEN6_TYPE_UD:      return "UD";
      case GEN6_TYPE_D:       return "D";
      case GEN6_TYPE_UW:      return "UW";
      case GEN6_TYPE_W:       return "W";
      case GEN6_TYPE_UV_IMM:  return "UV";
      case GEN6_TYPE_VF_IMM:  return "VF";
      case GEN6_TYPE_V_IMM:   return "V";
      case GEN6_TYPE_F:       return "F";
      default:                return "BAD";
      }
   } else {
      switch (operand->type) {
      case GEN6_TYPE_UD:      return "UD";
      case GEN6_TYPE_D:       return "D";
      case GEN6_TYPE_UW:      return "UW";
      case GEN6_TYPE_W:       return "W";
      case GEN6_TYPE_UB:      return "UB";
      case GEN6_TYPE_B:       return "B";
      case GEN7_TYPE_DF:      return "DF";
      case GEN6_TYPE_F:       return "F";
      default:                return "BAD";
      }
   }
}

static const char *
disasm_inst_vert_stride(const struct disasm_inst *inst, unsigned vert_stride)
{
   switch (vert_stride) {
   case GEN6_VERTSTRIDE_0:    return "0";
   case GEN6_VERTSTRIDE_1:    return "1";
   case GEN6_VERTSTRIDE_2:    return "2";
   case GEN6_VERTSTRIDE_4:    return "4";
   case GEN6_VERTSTRIDE_8:    return "8";
   case GEN6_VERTSTRIDE_16:   return "16";
   case GEN6_VERTSTRIDE_32:   return "32";
   case GEN6_VERTSTRIDE_VXH:  return "VxH";
   default:                   return "BAD";
   }
}

static const char *
disasm_inst_width(const struct disasm_inst *inst, unsigned width)
{
   switch (width) {
   case GEN6_WIDTH_1:   return "1";
   case GEN6_WIDTH_2:   return "2";
   case GEN6_WIDTH_4:   return "4";
   case GEN6_WIDTH_8:   return "8";
   case GEN6_WIDTH_16:  return "16";
   default:             return "BAD";
   }
}

static const char *
disasm_inst_horz_stride(const struct disasm_inst *inst, unsigned horz_stride)
{
   switch (horz_stride) {
   case GEN6_HORZSTRIDE_0: return "0";
   case GEN6_HORZSTRIDE_1: return "1";
   case GEN6_HORZSTRIDE_2: return "2";
   case GEN6_HORZSTRIDE_4: return "4";
   default:                return "BAD";
   }
}

static const char *
disasm_inst_writemask(const struct disasm_inst *inst, unsigned writemask)
{
   switch (writemask) {
   case 0x0:   return ".";
   case 0x1:   return ".x";
   case 0x2:   return ".y";
   case 0x3:   return ".xy";
   case 0x4:   return ".z";
   case 0x5:   return ".xz";
   case 0x6:   return ".yz";
   case 0x7:   return ".xyz";
   case 0x8:   return ".w";
   case 0x9:   return ".xw";
   case 0xa:   return ".yw";
   case 0xb:   return ".xyw";
   case 0xc:   return ".zw";
   case 0xd:   return ".xzw";
   case 0xe:   return ".yzw";
   case 0xf:   return "";
   default:    return ".BAD";
   }
}

static const char *
disasm_inst_negate(const struct disasm_inst *inst, bool negate)
{
   return (negate) ? "-" : "";
}

static const char *
disasm_inst_absolute(const struct disasm_inst *inst, bool absolute)
{
   return (absolute) ? "(abs)" : "";
}

static bool
disasm_inst_is_null(const struct disasm_inst *inst,
                    const struct disasm_operand *operand)
{
   return (operand->file == GEN6_FILE_ARF && operand->reg == GEN6_ARF_NULL);
}

static int
disasm_inst_type_size(const struct disasm_inst *inst,
                      const struct disasm_operand *operand)
{
   assert(operand->file != GEN6_FILE_IMM);

   switch (operand->type) {
   case GEN6_TYPE_UD:      return 4;
   case GEN6_TYPE_D:       return 4;
   case GEN6_TYPE_UW:      return 2;
   case GEN6_TYPE_W:       return 2;
   case GEN6_TYPE_UB:      return 1;
   case GEN6_TYPE_B:       return 1;
   case GEN7_TYPE_DF:      return 8;
   case GEN6_TYPE_F:       return 4;
   default:                return 1;
   }
}

static void
disasm_printer_reset(struct disasm_printer *printer)
{
   printer->buf[0] = '\0';
   printer->len = 0;
}

static const char *
disasm_printer_get_string(struct disasm_printer *printer)
{
   return printer->buf;
}

static void _util_printf_format(2, 3)
disasm_printer_add(struct disasm_printer *printer, const char *format, ...)
{
   const size_t avail = sizeof(printer->buf) - printer->len;
   va_list ap;
   int written;

   va_start(ap, format);
   written = vsnprintf(printer->buf + printer->len, avail, format, ap);
   va_end(ap);

   /* truncated */
   if (written < 0 || written >= avail) {
      memcpy(printer->buf + sizeof(printer->buf) - 4, "...", 4);
      printer->len = sizeof(printer->buf) - 1;
   } else {
      printer->len += written;
   }
}

/**
 * Pad to the specified column.
 */
static void
disasm_printer_column(struct disasm_printer *printer, int col)
{
   int len = DISASM_PRINTER_COLUMN_WIDTH * col;

   if (len <= printer->len) {
      if (!printer->len)
         return;

      /* at least one space */
      len = printer->len + 1;
   }

   if (len >= sizeof(printer->buf)) {
      len = sizeof(printer->buf) - 1;

      if (len <= printer->len)
         return;
   }

   memset(printer->buf + printer->len, ' ', len - printer->len);
   printer->len = len;
   printer->buf[printer->len] = '\0';
}

static void
disasm_printer_add_op(struct disasm_printer *printer,
                      const struct disasm_inst *inst)
{
   if (inst->pred_ctrl != GEN6_PREDCTRL_NONE) {
      disasm_printer_add(printer, "(%c%s%s) ",
            disasm_inst_pred_inv(inst),
            disasm_inst_flag_reg(inst),
            disasm_inst_pred_ctrl(inst));
   }

   disasm_printer_add(printer, "%s%s%s%s",
         disasm_inst_opcode(inst),
         disasm_inst_saturate(inst),
         disasm_inst_debug_ctrl(inst),
         disasm_inst_cond_modifier(inst));

   if (inst->cond_modifier != GEN6_COND_NONE) {
      switch (inst->opcode) {
      case GEN6_OPCODE_SEL:
      case GEN6_OPCODE_IF:
      case GEN6_OPCODE_WHILE:
         /* these do not update flag registers */
         break;
      default:
         disasm_printer_add(printer, ".%s", disasm_inst_flag_reg(inst));
         break;
      }
   }

   if (inst->opcode == GEN6_OPCODE_MATH)
      disasm_printer_add(printer, " %s", disasm_inst_fc(inst));
   if (inst->opcode != GEN6_OPCODE_NOP)
      disasm_printer_add(printer, "(%s)", disasm_inst_exec_size(inst));
}

static void
disasm_printer_add_operand(struct disasm_printer *printer,
                           const struct disasm_inst *inst,
                           const struct disasm_operand *operand)
{
   const char *name;
   bool multi_regs;

   name = disasm_inst_file(inst, operand, &multi_regs);
   if (!multi_regs) {
      disasm_printer_add(printer, name);
      return;
   }

   if (operand->file == GEN6_FILE_IMM) {
      switch (operand->type) {
      case GEN6_TYPE_UD:
         disasm_printer_add(printer, "0x%08xUD", inst->u.imm32);
         break;
      case GEN6_TYPE_D:
         disasm_printer_add(printer, "%dD", inst->u.imm32);
         break;
      case GEN6_TYPE_UW:
         disasm_printer_add(printer, "0x%04xUW", (uint16_t) inst->u.imm32);
         break;
      case GEN6_TYPE_W:
         disasm_printer_add(printer, "%dW", (int16_t) inst->u.imm32);
         break;
      case GEN6_TYPE_UV_IMM:
         disasm_printer_add(printer, "0x%08xUV", inst->u.imm32);
         break;
      case GEN6_TYPE_VF_IMM:
         disasm_printer_add(printer, "Vector Float");
         break;
      case GEN6_TYPE_V_IMM:
         disasm_printer_add(printer, "0x%08xV", inst->u.imm32);
         break;
      case GEN6_TYPE_F:
         disasm_printer_add(printer, "%-gF", uif(inst->u.imm32));
         break;
      default:
         disasm_printer_add(printer, "BAD");
         break;
      }

      return;
   }

   if (operand->addr_mode == GEN6_ADDRMODE_DIRECT) {
      unsigned reg, subreg;

      reg = operand->reg;
      if (operand->file == GEN6_FILE_ARF)
         reg &= 0xf;

      subreg = operand->subreg / disasm_inst_type_size(inst, operand);

      if (subreg)
         disasm_printer_add(printer, "%s%d.%d", name, reg, subreg);
      else
         disasm_printer_add(printer, "%s%d", name, reg);
   } else {
      disasm_printer_add(printer, "%s[a0.%d %d]",
            name, operand->addr_subreg, operand->addr_imm);
   }
}

static void
disasm_printer_add_dst(struct disasm_printer *printer,
                       const struct disasm_inst *inst,
                       const struct disasm_dst_operand *dst)
{
   disasm_printer_add_operand(printer, inst, &dst->base);

   /* dst is an immediate when in EU_INSTRUCTION_BRANCH_CONDITIONAL form */
   if (disasm_inst_is_null(inst, &dst->base) ||
       dst->base.file == GEN6_FILE_IMM)
      return;

   disasm_printer_add(printer, "<%s>%s%s",
         disasm_inst_horz_stride(inst, dst->horz_stride),
         disasm_inst_writemask(inst, dst->writemask),
         disasm_inst_type(inst, &dst->base));
}

static void
disasm_printer_add_src(struct disasm_printer *printer,
                       const struct disasm_inst *inst,
                       const struct disasm_src_operand *src)
{
   static const char swizzle_chars[4] = { 'x', 'y', 'z', 'w' };
   char swizzle[5];

   disasm_printer_add(printer, "%s%s",
         disasm_inst_negate(inst, src->negate),
         disasm_inst_absolute(inst, src->absolute));

   disasm_printer_add_operand(printer, inst, &src->base);

   if (disasm_inst_is_null(inst, &src->base) ||
       src->base.file == GEN6_FILE_IMM)
      return;

   if (src->swizzle_x == 0 && src->swizzle_y == 1 &&
       src->swizzle_z == 2 && src->swizzle_w == 3) {
      swizzle[0] = '\0';
   } else if (src->swizzle_x == src->swizzle_y &&
              src->swizzle_x == src->swizzle_z &&
              src->swizzle_x == src->swizzle_w) {
      swizzle[0] = swizzle_chars[src->swizzle_x];
      swizzle[1] = '\0';
   } else {
      swizzle[0] = swizzle_chars[src->swizzle_x];
      swizzle[1] = swizzle_chars[src->swizzle_y];
      swizzle[2] = swizzle_chars[src->swizzle_z];
      swizzle[3] = swizzle_chars[src->swizzle_w];
      swizzle[4] = '\0';
   }

   disasm_printer_add(printer, "<%s,%s,%s>%s%s",
         disasm_inst_vert_stride(inst, src->vert_stride),
         disasm_inst_width(inst, src->width),
         disasm_inst_horz_stride(inst, src->horz_stride),
         swizzle,
         disasm_inst_type(inst, &src->base));
}

static void
disasm_printer_add_ctrl(struct disasm_printer *printer,
                        const struct disasm_inst *inst)
{
   if (inst->opcode == GEN6_OPCODE_NOP) {
      disasm_printer_add(printer, ";");
      return;
   }

   disasm_printer_add(printer, "{%s%s%s%s%s%s%s%s };",
         disasm_inst_access_mode(inst),
         disasm_inst_mask_ctrl(inst),
         disasm_inst_dep_ctrl(inst),
         disasm_inst_qtr_ctrl(inst),
         disasm_inst_cmpt_ctrl(inst),
         disasm_inst_thread_ctrl(inst),
         disasm_inst_acc_wr_ctrl(inst),
         disasm_inst_eot(inst));
}

static void
disasm_printer_add_mdesc_sampler(struct disasm_printer *printer,
                                 const struct disasm_inst *inst,
                                 uint32_t mdesc)
{
   int op, simd;

   if (inst->dev->gen >= ILO_GEN(7)) {
      op = READ(mdesc, GEN7_MSG_SAMPLER_OP);
      simd = READ(mdesc, GEN7_MSG_SAMPLER_SIMD);
   } {
      op = READ(mdesc, GEN6_MSG_SAMPLER_OP);
      simd = READ(mdesc, GEN6_MSG_SAMPLER_SIMD);
   }

   disasm_printer_add(printer, " (%d, %d, %d, %d)",
         READ(mdesc, GEN6_MSG_SAMPLER_SURFACE),
         READ(mdesc, GEN6_MSG_SAMPLER_INDEX),
         op, simd);
}

static void
disasm_printer_add_mdesc_urb(struct disasm_printer *printer,
                             const struct disasm_inst *inst,
                             uint32_t mdesc)
{
   const char *op;
   int offset;
   bool interleaved, complete, allocate, used;

   if (inst->dev->gen >= ILO_GEN(7)) {
      switch (READ(mdesc, GEN7_MSG_URB_OP)) {
      case GEN7_MSG_URB_WRITE_HWORD:   op = "write HWord";  break;
      case GEN7_MSG_URB_WRITE_OWORD:   op = "write OWord";  break;
      case GEN7_MSG_URB_READ_HWORD:    op = "read HWord";   break;
      case GEN7_MSG_URB_READ_OWORD:    op = "read OWord";   break;
      case GEN7_MSG_URB_ATOMIC_MOV:    op = "atomic mov";   break;
      case GEN7_MSG_URB_ATOMIC_INC:    op = "atomic inc";   break;
      default:                         op = "BAD";          break;
      }

      offset = READ(mdesc, GEN7_MSG_URB_GLOBAL_OFFSET);
      interleaved = mdesc & GEN7_MSG_URB_INTERLEAVED;
      complete = mdesc & GEN7_MSG_URB_COMPLETE;

      allocate = false;
      used = false;
   } else {
      switch (READ(mdesc, GEN6_MSG_URB_OP)) {
      case GEN6_MSG_URB_WRITE:         op = "urb_write";    break;
      case GEN6_MSG_URB_FF_SYNC:       op = "ff_sync";      break;
      default:                         op = "BAD";          break;
      }

      offset = READ(mdesc, GEN6_MSG_URB_OFFSET);
      interleaved = mdesc & GEN6_MSG_URB_INTERLEAVED;
      complete = mdesc & GEN6_MSG_URB_COMPLETE;

      allocate = mdesc & GEN6_MSG_URB_ALLOCATE;
      used = mdesc & GEN6_MSG_URB_USED;
   }

   disasm_printer_add(printer, " %d %s%s%s%s%s", offset, op,
         (interleaved) ? " interleave" : "",
         (allocate) ? " allocate" : "",
         (used) ? " used" : "",
         (complete) ? " complete" : "");
}

static void
disasm_printer_add_mdesc_dp_sampler(struct disasm_printer *printer,
                                    const struct disasm_inst *inst,
                                    uint32_t mdesc)
{
   const int op = (inst->dev->gen >= ILO_GEN(7)) ?
      READ(mdesc, GEN7_MSG_DP_OP) : READ(mdesc, GEN6_MSG_DP_OP);
   const bool write_commit = (inst->dev->gen == ILO_GEN(6)) ?
         (mdesc & GEN6_MSG_DP_SEND_WRITE_COMMIT) : 0;

   disasm_printer_add(printer, " (%d, %d, %d, %d)",
         READ(mdesc, GEN6_MSG_DP_SURFACE),
         READ(mdesc, GEN6_MSG_DP_OWORD_BLOCK_SIZE),
         op, write_commit);
}

static void
disasm_printer_add_mdesc_dp_rc(struct disasm_printer *printer,
                               const struct disasm_inst *inst,
                               uint32_t mdesc)
{
   const int op = (inst->dev->gen >= ILO_GEN(7)) ?
      READ(mdesc, GEN7_MSG_DP_OP) : READ(mdesc, GEN6_MSG_DP_OP);
   const char *str;
   bool is_rt_write;

   if (inst->dev->gen >= ILO_GEN(7.5)) {
      switch (op) {
      case GEN75_MSG_DP_RC_MEDIA_BLOCK_READ:       str = "media block read";           break;
      case GEN75_MSG_DP_RC_MEMORY_FENCE:           str = "memory fence";               break;
      case GEN75_MSG_DP_RC_MEDIA_BLOCK_WRITE:      str = "media block write";          break;
      case GEN75_MSG_DP_RC_RT_WRITE:               str = "RT write";                   break;
      default:                                     str = "BAD";                        break;
      }

      is_rt_write = (op == GEN75_MSG_DP_RC_RT_WRITE);
   } else if (inst->dev->gen >= ILO_GEN(7)) {
      switch (op) {
      case GEN7_MSG_DP_RC_MEDIA_BLOCK_READ:        str = "media block read";           break;
      case GEN7_MSG_DP_RC_TYPED_SURFACE_READ:      str = "typed surface read";         break;
      case GEN7_MSG_DP_RC_TYPED_ATOMIC_OP:         str = "typed atomic op";            break;
      case GEN7_MSG_DP_RC_MEMORY_FENCE:            str = "memory fence";               break;
      case GEN7_MSG_DP_RC_MEDIA_BLOCK_WRITE:       str = "media block write";          break;
      case GEN7_MSG_DP_RC_RT_WRITE:                str = "RT write";                   break;
      case GEN7_MSG_DP_RC_TYPED_SURFACE_WRITE:     str = "typed surface write";        break;
      default:                                     str = "BAD";                        break;
      }

      is_rt_write = (op == GEN7_MSG_DP_RC_RT_WRITE);
   } else {
      switch (op) {
      case GEN6_MSG_DP_OWORD_BLOCK_READ:           str = "OWORD block read";           break;
      case GEN6_MSG_DP_RT_UNORM_READ:              str = "RT UNORM read";              break;
      case GEN6_MSG_DP_OWORD_DUAL_BLOCK_READ:      str = "OWORD dual block read";      break;
      case GEN6_MSG_DP_MEDIA_BLOCK_READ:           str = "media block read";           break;
      case GEN6_MSG_DP_UNALIGNED_OWORD_BLOCK_READ: str = "unaligned OWORD block read"; break;
      case GEN6_MSG_DP_DWORD_SCATTERED_READ:       str = "DWORD scattered read";       break;
      case GEN6_MSG_DP_DWORD_ATOMIC_WRITE:         str = "DWORD atomic write";         break;
      case GEN6_MSG_DP_OWORD_BLOCK_WRITE:          str = "OWORD block write";          break;
      case GEN6_MSG_DP_OWORD_DUAL_BLOCK_WRITE:     str = "OWORD dual block_write";     break;
      case GEN6_MSG_DP_MEDIA_BLOCK_WRITE:          str = "media block write";          break;
      case GEN6_MSG_DP_DWORD_SCATTERED_WRITE:      str = "DWORD scattered write";      break;
      case GEN6_MSG_DP_RT_WRITE:                   str = "RT write";                   break;
      case GEN6_MSG_DP_SVB_WRITE:                  str = "SVB write";                  break;
      case GEN6_MSG_DP_RT_UNORM_WRITE:             str = "RT UNORM write";             break;
      default:                                     str = "BAD";                        break;
      }

      is_rt_write = (op == GEN6_MSG_DP_RT_WRITE);
   }

   disasm_printer_add(printer, " %s", str);

   if (is_rt_write) {
      switch (mdesc & GEN6_MSG_DP_RT_MODE__MASK) {
      case GEN6_MSG_DP_RT_MODE_SIMD16:             str = "SIMD16";            break;
      case GEN6_MSG_DP_RT_MODE_SIMD16_REPDATA:     str = "SIMD16/RepData";    break;
      case GEN6_MSG_DP_RT_MODE_SIMD8_DUALSRC_LO:   str = "SIMD8/DualSrcLow";  break;
      case GEN6_MSG_DP_RT_MODE_SIMD8_DUALSRC_HI:   str = "SIMD8/DualSrcHigh"; break;
      case GEN6_MSG_DP_RT_MODE_SIMD8_LO:           str = "SIMD8";             break;
      case GEN6_MSG_DP_RT_MODE_SIMD8_IMAGE_WR:     str = "SIMD8/ImageWrite";  break;
      default:                                     str = "BAD";               break;
      }

      disasm_printer_add(printer, " %s%s%s%s", str,
            (mdesc & GEN6_MSG_DP_SLOTGRP_HI) ? " Hi" : "",
            (mdesc & GEN6_MSG_DP_RT_LAST) ? " LastRT" : "",
            (inst->dev->gen == ILO_GEN(6) &&
             (mdesc & GEN6_MSG_DP_SEND_WRITE_COMMIT)) ? " WriteCommit" : "");
   }

   disasm_printer_add(printer, " Surface = %d",
         READ(mdesc, GEN6_MSG_DP_SURFACE));
}

static void
disasm_printer_add_mdesc(struct disasm_printer *printer,
                         const struct disasm_inst *inst)
{
   const uint32_t mdesc = inst->u.imm32;

   assert(inst->opcode == GEN6_OPCODE_SEND ||
          inst->opcode == GEN6_OPCODE_SENDC);
   assert(inst->src1.base.file == GEN6_FILE_IMM);

   disasm_printer_add(printer, "            %s", disasm_inst_sfid(inst));

   switch (inst->sfid) {
   case GEN6_SFID_SAMPLER:
      disasm_printer_add_mdesc_sampler(printer, inst, mdesc);
      break;
   case GEN6_SFID_DP_SAMPLER:
      disasm_printer_add_mdesc_dp_sampler(printer, inst, mdesc);
      break;
   case GEN6_SFID_DP_RC:
      disasm_printer_add_mdesc_dp_rc(printer, inst, mdesc);
      break;
   case GEN6_SFID_URB:
      disasm_printer_add_mdesc_urb(printer, inst, mdesc);
      break;
   case GEN6_SFID_DP_CC:
   case GEN7_SFID_DP_DC0:
   case GEN7_SFID_PI:
   case GEN75_SFID_DP_DC1:
   default:
      break;
   }

   disasm_printer_add(printer, " mlen %d rlen %d",
         READ(mdesc, GEN6_MSG_MLEN),
         READ(mdesc, GEN6_MSG_RLEN));
}

static void
disasm_printer_print_inst(struct disasm_printer *printer,
                          const struct disasm_inst *inst)
{
   int col = 0;

   disasm_printer_reset(printer);

   disasm_printer_column(printer, col++);
   disasm_printer_add_op(printer, inst);

   if (inst->has_jip || inst->has_uip) {
      if (inst->has_jip) {
         disasm_printer_column(printer, col++);
         disasm_printer_add(printer, "JIP: %d", (int16_t) inst->u.imm32);
      }

      if (inst->has_uip) {
         disasm_printer_column(printer, col++);
         disasm_printer_add(printer, "UIP: %d",
               (int16_t) (inst->u.imm32 >> 16));
      }
   } else {
      const int src_count = disasm_opcode_table[inst->opcode].src_count;
      const struct disasm_src_operand *src[3];
      int i;

      /* get src operands */
      switch (src_count) {
      case 3:
         src[2] = &inst->u.src2;
      case 2:
         src[1] = &inst->src1;
      case 1:
         src[0] = &inst->src0;
      case 0:
      default:
         break;
      }

      if (src_count) {
         disasm_printer_column(printer, col++);
         disasm_printer_add_dst(printer, inst, &inst->dst);

         for (i = 0; i < src_count; i++) {
            disasm_printer_column(printer, col++);
            disasm_printer_add_src(printer, inst, src[i]);
         }
      }
   }

   if (inst->opcode == GEN6_OPCODE_SEND ||
       inst->opcode == GEN6_OPCODE_SENDC) {
      /* start a new line */
      ilo_printf("%s\n", disasm_printer_get_string(printer));
      disasm_printer_reset(printer);
      col = 0;

      disasm_printer_column(printer, col++);

      disasm_printer_column(printer, col++);
      disasm_printer_add_mdesc(printer, inst);
   }

   if (col < 4)
      col = 4;

   disasm_printer_column(printer, col++);
   disasm_printer_add_ctrl(printer, inst);

   ilo_printf("%s\n", disasm_printer_get_string(printer));
}

void
toy_compiler_disassemble(const struct ilo_dev_info *dev,
                         const void *kernel, int size,
                         bool dump_hex)
{
   const uint32_t *cur = (const uint32_t *) kernel;
   const uint32_t *end = cur + size / sizeof(*cur);
   struct disasm_printer printer;

   disasm_printer_reset(&printer);

   while (cur < end) {
      struct disasm_inst inst;
      const bool compacted = (cur[0] & GEN6_INST_CMPTCTRL);
      const uint32_t *dw = cur;
      uint32_t temp[4];

      cur += (compacted) ? 2 : 4;
      /* incomplete instruction */
      if (cur > end)
         break;

      if (compacted) {
         /* no compaction support yet */
         memset(temp, 0, sizeof(temp));
         dw = temp;
      }

      if (dump_hex) {
         ilo_printf("0x%08x 0x%08x 0x%08x 0x%08x ",
               dw[0], dw[1], dw[2], dw[3]);
      }

      memset(&inst, 0, sizeof(inst));
      inst.dev = dev;
      disasm_inst_decode(&inst, dw);
      inst.cmpt_ctrl = compacted;

      disasm_printer_print_inst(&printer, &inst);
   }
}
