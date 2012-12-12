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

#define CG_REG_SHIFT 5
#define CG_REG_NUM(origin) ((origin) >> CG_REG_SHIFT)

struct codegen {
   const struct toy_inst *inst;
   int pc;

   unsigned flag_sub_reg_num;

   struct codegen_dst {
      unsigned file;
      unsigned type;
      bool indirect;
      unsigned indirect_subreg;
      unsigned origin; /* (RegNum << 5 | SubRegNumInBytes) */

      unsigned horz_stride;

      unsigned writemask;
   } dst;

   struct codegen_src {
      unsigned file;
      unsigned type;
      bool indirect;
      unsigned indirect_subreg;
      unsigned origin; /* (RegNum << 5 | SubRegNumInBytes) */

      unsigned vert_stride;
      unsigned width;
      unsigned horz_stride;

      unsigned swizzle[4];
      bool absolute;
      bool negate;
   } src[3];
};

/**
 * Return true if the source operand is null.
 */
static bool
src_is_null(const struct codegen *cg, int idx)
{
   const struct codegen_src *src = &cg->src[idx];

   return (src->file == BRW_ARCHITECTURE_REGISTER_FILE &&
           src->origin == BRW_ARF_NULL << CG_REG_SHIFT);
}

/**
 * Translate a source operand to DW2 or DW3 of the 1-src/2-src format.
 */
static uint32_t
translate_src(const struct codegen *cg, int idx)
{
   const struct codegen_src *src = &cg->src[idx];
   uint32_t dw;

   /* special treatment may be needed if any of the operand is immediate */
   if (cg->src[0].file == BRW_IMMEDIATE_VALUE) {
      assert(!cg->src[0].absolute && !cg->src[0].negate);
      /* only the last src operand can be an immediate */
      assert(src_is_null(cg, 1));

      if (idx == 0)
         return cg->flag_sub_reg_num << 25;
      else
         return cg->src[0].origin;
   }
   else if (idx && cg->src[1].file == BRW_IMMEDIATE_VALUE) {
      assert(!cg->src[1].absolute && !cg->src[1].negate);
      return cg->src[1].origin;
   }

   assert(src->file != BRW_IMMEDIATE_VALUE);

   if (src->indirect) {
      const int offset = (int) src->origin;

      assert(src->file == BRW_GENERAL_REGISTER_FILE);
      assert(offset < 512 && offset >= -512);

      if (cg->inst->access_mode == BRW_ALIGN_16) {
         assert(src->width == BRW_WIDTH_4);
         assert(src->horz_stride == BRW_HORIZONTAL_STRIDE_1);

         /* the lower 4 bits are reserved for the swizzle_[xy] */
         assert(!(src->origin & 0xf));

         dw = src->vert_stride << 21 |
              src->swizzle[3] << 18 |
              src->swizzle[2] << 16 |
              BRW_ADDRESS_REGISTER_INDIRECT_REGISTER << 15 |
              src->negate << 14 |
              src->absolute << 13 |
              src->indirect_subreg << 10 |
              (src->origin & 0x3f0) |
              src->swizzle[1] << 2 |
              src->swizzle[0];
      }
      else {
         assert(src->swizzle[0] == TOY_SWIZZLE_X &&
                src->swizzle[1] == TOY_SWIZZLE_Y &&
                src->swizzle[2] == TOY_SWIZZLE_Z &&
                src->swizzle[3] == TOY_SWIZZLE_W);

         dw = src->vert_stride << 21 |
              src->width << 18 |
              src->horz_stride << 16 |
              BRW_ADDRESS_REGISTER_INDIRECT_REGISTER << 15 |
              src->negate << 14 |
              src->absolute << 13 |
              src->indirect_subreg << 10 |
              (src->origin & 0x3ff);
      }
   }
   else {
      switch (src->file) {
      case BRW_ARCHITECTURE_REGISTER_FILE:
         break;
      case BRW_GENERAL_REGISTER_FILE:
         assert(CG_REG_NUM(src->origin) < 128);
         break;
      case BRW_MESSAGE_REGISTER_FILE:
         assert(cg->inst->opcode == BRW_OPCODE_SEND ||
                cg->inst->opcode == BRW_OPCODE_SENDC);
         assert(CG_REG_NUM(src->origin) < 16);
         break;
      case BRW_IMMEDIATE_VALUE:
      default:
         assert(!"invalid src file");
         break;
      }

      if (cg->inst->access_mode == BRW_ALIGN_16) {
         assert(src->width == BRW_WIDTH_4);
         assert(src->horz_stride == BRW_HORIZONTAL_STRIDE_1);

         /* the lower 4 bits are reserved for the swizzle_[xy] */
         assert(!(src->origin & 0xf));

         dw = src->vert_stride << 21 |
              src->swizzle[3] << 18 |
              src->swizzle[2] << 16 |
              BRW_ADDRESS_DIRECT << 15 |
              src->negate << 14 |
              src->absolute << 13 |
              src->origin |
              src->swizzle[1] << 2 |
              src->swizzle[0];
      }
      else {
         assert(src->swizzle[0] == TOY_SWIZZLE_X &&
                src->swizzle[1] == TOY_SWIZZLE_Y &&
                src->swizzle[2] == TOY_SWIZZLE_Z &&
                src->swizzle[3] == TOY_SWIZZLE_W);

         dw = src->vert_stride << 21 |
              src->width << 18 |
              src->horz_stride << 16 |
              BRW_ADDRESS_DIRECT << 15 |
              src->negate << 14 |
              src->absolute << 13 |
              src->origin;
      }
   }

   if (idx == 0)
      dw |= cg->flag_sub_reg_num << 25;

   return dw;
}

/**
 * Translate the destination operand to the higher 16 bits of DW1 of the
 * 1-src/2-src format.
 */
static uint16_t
translate_dst_region(const struct codegen *cg)
{
   const struct codegen_dst *dst = &cg->dst;
   uint16_t dw1_region;

   if (dst->file == BRW_IMMEDIATE_VALUE) {
      /* dst is immediate (JIP) when the opcode is a conditional branch */
      switch (cg->inst->opcode) {
      case BRW_OPCODE_IF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_WHILE:
         assert(dst->type == BRW_REGISTER_TYPE_W);
         dw1_region = (dst->origin & 0xffff);
         break;
      default:
         assert(!"dst cannot be immediate");
         dw1_region = 0;
         break;
      }

      return dw1_region;
   }

   if (dst->indirect) {
      const int offset = (int) dst->origin;

      assert(dst->file == BRW_GENERAL_REGISTER_FILE);
      assert(offset < 512 && offset >= -512);

      if (cg->inst->access_mode == BRW_ALIGN_16) {
         /*
          * From the Sandy Bridge PRM, volume 4 part 2, page 144:
          *
          *     "Allthough Dst.HorzStride is a don't care for Align16, HW
          *      needs this to be programmed as 01."
          */
         assert(dst->horz_stride == BRW_HORIZONTAL_STRIDE_1);
         /* the lower 4 bits are reserved for the writemask */
         assert(!(dst->origin & 0xf));

         dw1_region = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER << 15 |
                      dst->horz_stride << 13 |
                      dst->indirect_subreg << 10 |
                      (dst->origin & 0x3f0) |
                      dst->writemask;
      }
      else {
         assert(dst->writemask == TOY_WRITEMASK_XYZW);

         dw1_region = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER << 15 |
                      dst->horz_stride << 13 |
                      dst->indirect_subreg << 10 |
                      (dst->origin & 0x3ff);
      }
   }
   else {
      assert((dst->file == BRW_GENERAL_REGISTER_FILE &&
              CG_REG_NUM(dst->origin) < 128) ||
             (dst->file == BRW_MESSAGE_REGISTER_FILE &&
              CG_REG_NUM(dst->origin) < 16) ||
             (dst->file == BRW_ARCHITECTURE_REGISTER_FILE));

      if (cg->inst->access_mode == BRW_ALIGN_16) {
         /* similar to the indirect case */
         assert(dst->horz_stride == BRW_HORIZONTAL_STRIDE_1);
         assert(!(dst->origin & 0xf));

         dw1_region = BRW_ADDRESS_DIRECT << 15 |
                      dst->horz_stride << 13 |
                      dst->origin |
                      dst->writemask;
      }
      else {
         assert(dst->writemask == TOY_WRITEMASK_XYZW);

         dw1_region = BRW_ADDRESS_DIRECT << 15 |
                      dst->horz_stride << 13 |
                      dst->origin;
      }
   }

   return dw1_region;
}

/**
 * Translate the destination operand to DW1 of the 1-src/2-src format.
 */
static uint32_t
translate_dst(const struct codegen *cg)
{
   return translate_dst_region(cg) << 16 |
          cg->src[1].type << 12 |
          cg->src[1].file << 10 |
          cg->src[0].type << 7 |
          cg->src[0].file << 5 |
          cg->dst.type << 2 |
          cg->dst.file;
}

/**
 * Translate the instruction to DW0 of the 1-src/2-src format.
 */
static uint32_t
translate_inst(const struct codegen *cg)
{
   const bool debug_ctrl = false;
   const bool cmpt_ctrl = false;

   assert(cg->inst->opcode < 128);

   return cg->inst->saturate << 31 |
          debug_ctrl << 30 |
          cmpt_ctrl << 29 |
          cg->inst->acc_wr_ctrl << 28 |
          cg->inst->cond_modifier << 24 |
          cg->inst->exec_size << 21 |
          cg->inst->pred_inv << 20 |
          cg->inst->pred_ctrl << 16 |
          cg->inst->thread_ctrl << 14 |
          cg->inst->qtr_ctrl << 12 |
          cg->inst->dep_ctrl << 10 |
          cg->inst->mask_ctrl << 9 |
          cg->inst->access_mode << 8 |
          cg->inst->opcode;
}

/**
 * Codegen an instruction in 1-src/2-src format.
 */
static void
codegen_inst(const struct codegen *cg, uint32_t *code)
{
   code[0] = translate_inst(cg);
   code[1] = translate_dst(cg);
   code[2] = translate_src(cg, 0);
   code[3] = translate_src(cg, 1);
   assert(src_is_null(cg, 2));
}

/**
 * Codegen an instruction in 3-src format.
 */
static void
codegen_inst_3src(const struct codegen *cg, uint32_t *code)
{
   const struct codegen_dst *dst = &cg->dst;
   uint32_t dw0, dw1, dw_src[3];
   int i;

   dw0 = translate_inst(cg);

   /*
    * 3-src instruction restrictions
    *
    *  - align16 with direct addressing
    *  - GRF or MRF dst
    *  - GRF src
    *  - sub_reg_num is DWORD aligned
    *  - no regioning except replication control
    *    (vert_stride == 0 && horz_stride == 0)
    */
   assert(cg->inst->access_mode == BRW_ALIGN_16);

   assert(!dst->indirect);
   assert((dst->file == BRW_GENERAL_REGISTER_FILE &&
           CG_REG_NUM(dst->origin) < 128) ||
          (dst->file == BRW_MESSAGE_REGISTER_FILE &&
           CG_REG_NUM(dst->origin) < 16));
   assert(!(dst->origin & 0x3));
   assert(dst->horz_stride == BRW_HORIZONTAL_STRIDE_1);

   dw1 = dst->origin << 19 |
         dst->writemask << 17 |
         cg->src[2].negate << 9 |
         cg->src[2].absolute << 8 |
         cg->src[1].negate << 7 |
         cg->src[1].absolute << 6 |
         cg->src[0].negate << 5 |
         cg->src[0].absolute << 4 |
         cg->flag_sub_reg_num << 1 |
         (dst->file == BRW_MESSAGE_REGISTER_FILE);

   for (i = 0; i < 3; i++) {
      const struct codegen_src *src = &cg->src[i];

      assert(!src->indirect);
      assert(src->file == BRW_GENERAL_REGISTER_FILE &&
             CG_REG_NUM(src->origin) < 128);
      assert(!(src->origin & 0x3));

      assert((src->vert_stride == BRW_VERTICAL_STRIDE_4 &&
              src->horz_stride == BRW_HORIZONTAL_STRIDE_1) ||
             (src->vert_stride == BRW_VERTICAL_STRIDE_0 &&
              src->horz_stride == BRW_HORIZONTAL_STRIDE_0));
      assert(src->width == BRW_WIDTH_4);

      dw_src[i] = src->origin << 7 |
                  src->swizzle[3] << 7 |
                  src->swizzle[2] << 5 |
                  src->swizzle[1] << 3 |
                  src->swizzle[0] << 1 |
                  (src->vert_stride == BRW_VERTICAL_STRIDE_0 &&
                   src->horz_stride == BRW_HORIZONTAL_STRIDE_0);

      /* only the lower 20 bits are used */
      assert((dw_src[i] & 0xfffff) == dw_src[i]);
   }

   code[0] = dw0;
   code[1] = dw1;
   /* concatenate the bits of dw_src */
   code[2] = (dw_src[1] & 0x7ff ) << 21 | dw_src[0];
   code[3] = dw_src[2] << 10 | (dw_src[1] >> 11);
}

/**
 * Sanity check the region parameters of the operands.
 */
static void
codegen_validate_region_restrictions(const struct codegen *cg)
{
   const int exec_size_map[] = {
      [BRW_EXECUTE_1] = 1,
      [BRW_EXECUTE_2] = 2,
      [BRW_EXECUTE_4] = 4,
      [BRW_EXECUTE_8] = 8,
      [BRW_EXECUTE_16] = 16,
      [BRW_EXECUTE_32] = 32,
   };
   const int width_map[] = {
      [BRW_WIDTH_1] = 1,
      [BRW_WIDTH_2] = 2,
      [BRW_WIDTH_4] = 4,
      [BRW_WIDTH_8] = 8,
      [BRW_WIDTH_16] = 16,
   };
   const int horz_stride_map[] = {
      [BRW_HORIZONTAL_STRIDE_0] = 0,
      [BRW_HORIZONTAL_STRIDE_1] = 1,
      [BRW_HORIZONTAL_STRIDE_2] = 2,
      [BRW_HORIZONTAL_STRIDE_4] = 4,
   };
   const int vert_stride_map[] = {
      [BRW_VERTICAL_STRIDE_0] = 0,
      [BRW_VERTICAL_STRIDE_1] = 1,
      [BRW_VERTICAL_STRIDE_2] = 2,
      [BRW_VERTICAL_STRIDE_4] = 4,
      [BRW_VERTICAL_STRIDE_8] = 8,
      [BRW_VERTICAL_STRIDE_16] = 16,
      [BRW_VERTICAL_STRIDE_32] = 32,
      [BRW_VERTICAL_STRIDE_64] = 64,
      [BRW_VERTICAL_STRIDE_128] = 128,
      [BRW_VERTICAL_STRIDE_256] = 256,
      [BRW_VERTICAL_STRIDE_ONE_DIMENSIONAL] = 0,
   };
   const int exec_size = exec_size_map[cg->inst->exec_size];
   int i;

   /* Sandy Bridge PRM, volume 4 part 2, page 94 */

   /* 1. (we don't do 32 anyway) */
   assert(exec_size <= 16);

   for (i = 0; i < Elements(cg->src); i++) {
      const int width = width_map[cg->src[i].width];
      const int horz_stride = horz_stride_map[cg->src[i].horz_stride];
      const int vert_stride = vert_stride_map[cg->src[i].vert_stride];

      if (src_is_null(cg, i))
         break;

      /* 3. */
      assert(exec_size >= width);

      if (exec_size == width) {
         /* 4. & 5. */
         if (horz_stride)
            assert(vert_stride == width * horz_stride);
      }

      if (width == 1) {
         /* 6. */
         assert(horz_stride == 0);

         /* 7. */
         if (exec_size == 1)
            assert(vert_stride == 0);
      }

      /* 8. */
      if (!vert_stride && !horz_stride)
         assert(width == 1);
   }

   /* derived from 10.1.2. & 10.2. */
   assert(cg->dst.horz_stride != BRW_HORIZONTAL_STRIDE_0);
}

static unsigned
translate_vfile(enum toy_file file)
{
   switch (file) {
   case TOY_FILE_ARF:   return BRW_ARCHITECTURE_REGISTER_FILE;
   case TOY_FILE_GRF:   return BRW_GENERAL_REGISTER_FILE;
   case TOY_FILE_MRF:   return BRW_MESSAGE_REGISTER_FILE;
   case TOY_FILE_IMM:   return BRW_IMMEDIATE_VALUE;
   default:
      assert(!"unhandled toy file");
      return BRW_GENERAL_REGISTER_FILE;
   }
}

static unsigned
translate_vtype(enum toy_type type)
{
   switch (type) {
   case TOY_TYPE_F:     return BRW_REGISTER_TYPE_F;
   case TOY_TYPE_D:     return BRW_REGISTER_TYPE_D;
   case TOY_TYPE_UD:    return BRW_REGISTER_TYPE_UD;
   case TOY_TYPE_W:     return BRW_REGISTER_TYPE_W;
   case TOY_TYPE_UW:    return BRW_REGISTER_TYPE_UW;
   case TOY_TYPE_V:     return BRW_REGISTER_TYPE_V;
   default:
      assert(!"unhandled toy type");
      return BRW_REGISTER_TYPE_F;
   }
}

static unsigned
translate_writemask(enum toy_writemask writemask)
{
   /* TOY_WRITEMASK_* are compatible with the hardware definitions */
   assert(writemask <= 0xf);
   return writemask;
}

static unsigned
translate_swizzle(enum toy_swizzle swizzle)
{
   /* TOY_SWIZZLE_* are compatible with the hardware definitions */
   assert(swizzle <= 3);
   return swizzle;
}

/**
 * Prepare for generating an instruction.
 */
static void
codegen_prepare(struct codegen *cg, const struct toy_inst *inst,
                int pc, int rect_linear_width)
{
   int i;

   cg->inst = inst;
   cg->pc = pc;

   cg->flag_sub_reg_num = 0;

   cg->dst.file = translate_vfile(inst->dst.file);
   cg->dst.type = translate_vtype(inst->dst.type);
   cg->dst.indirect = inst->dst.indirect;
   cg->dst.indirect_subreg = inst->dst.indirect_subreg;
   cg->dst.origin = inst->dst.val32;

   /*
    * From the Sandy Bridge PRM, volume 4 part 2, page 81:
    *
    *     "For a word or an unsigned word immediate data, software must
    *      replicate the same 16-bit immediate value to both the lower word
    *      and the high word of the 32-bit immediate field in an instruction."
    */
   if (inst->dst.file == TOY_FILE_IMM) {
      switch (inst->dst.type) {
      case TOY_TYPE_W:
      case TOY_TYPE_UW:
         cg->dst.origin &= 0xffff;
         cg->dst.origin |= cg->dst.origin << 16;
         break;
      default:
         break;
      }
   }

   cg->dst.writemask = translate_writemask(inst->dst.writemask);

   switch (inst->dst.rect) {
   case TOY_RECT_LINEAR:
      cg->dst.horz_stride = BRW_HORIZONTAL_STRIDE_1;
      break;
   default:
      assert(!"unsupported dst region");
      cg->dst.horz_stride = BRW_HORIZONTAL_STRIDE_1;
      break;
   }

   for (i = 0; i < Elements(cg->src); i++) {
      struct codegen_src *src = &cg->src[i];

      src->file = translate_vfile(inst->src[i].file);
      src->type = translate_vtype(inst->src[i].type);
      src->indirect = inst->src[i].indirect;
      src->indirect_subreg = inst->src[i].indirect_subreg;
      src->origin = inst->src[i].val32;

      /* do the same for src */
      if (inst->dst.file == TOY_FILE_IMM) {
         switch (inst->src[i].type) {
         case TOY_TYPE_W:
         case TOY_TYPE_UW:
            src->origin &= 0xffff;
            src->origin |= src->origin << 16;
            break;
         default:
            break;
         }
      }

      src->swizzle[0] = translate_swizzle(inst->src[i].swizzle_x);
      src->swizzle[1] = translate_swizzle(inst->src[i].swizzle_y);
      src->swizzle[2] = translate_swizzle(inst->src[i].swizzle_z);
      src->swizzle[3] = translate_swizzle(inst->src[i].swizzle_w);
      src->absolute = inst->src[i].absolute;
      src->negate = inst->src[i].negate;

      switch (inst->src[i].rect) {
      case TOY_RECT_LINEAR:
         switch (rect_linear_width) {
         case 1:
            src->vert_stride = BRW_VERTICAL_STRIDE_1;
            src->width = BRW_WIDTH_1;
            break;
         case 2:
            src->vert_stride = BRW_VERTICAL_STRIDE_2;
            src->width = BRW_WIDTH_2;
            break;
         case 4:
            src->vert_stride = BRW_VERTICAL_STRIDE_4;
            src->width = BRW_WIDTH_4;
            break;
         case 8:
            src->vert_stride = BRW_VERTICAL_STRIDE_8;
            src->width = BRW_WIDTH_8;
            break;
         case 16:
            src->vert_stride = BRW_VERTICAL_STRIDE_16;
            src->width = BRW_WIDTH_16;
            break;
         default:
            assert(!"unsupported TOY_RECT_LINEAR width");
            src->vert_stride = BRW_VERTICAL_STRIDE_1;
            src->width = BRW_WIDTH_1;
            break;
         }
         src->horz_stride = BRW_HORIZONTAL_STRIDE_1;
         break;
      case TOY_RECT_041:
         src->vert_stride = BRW_VERTICAL_STRIDE_0;
         src->width = BRW_WIDTH_4;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_1;
         break;
      case TOY_RECT_010:
         src->vert_stride = BRW_VERTICAL_STRIDE_0;
         src->width = BRW_WIDTH_1;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_0;
         break;
      case TOY_RECT_220:
         src->vert_stride = BRW_VERTICAL_STRIDE_2;
         src->width = BRW_WIDTH_2;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_0;
         break;
      case TOY_RECT_440:
         src->vert_stride = BRW_VERTICAL_STRIDE_4;
         src->width = BRW_WIDTH_4;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_0;
         break;
      case TOY_RECT_240:
         src->vert_stride = BRW_VERTICAL_STRIDE_2;
         src->width = BRW_WIDTH_4;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_0;
         break;
      default:
         assert(!"unsupported src region");
         src->vert_stride = BRW_VERTICAL_STRIDE_1;
         src->width = BRW_WIDTH_1;
         src->horz_stride = BRW_HORIZONTAL_STRIDE_1;
         break;
      }
   }
}

/**
 * Generate HW shader code.  The instructions should have been legalized.
 */
void *
toy_compiler_assemble(struct toy_compiler *tc, int *size)
{
   const struct toy_inst *inst;
   uint32_t *code;
   int pc;

   code = MALLOC(tc->num_instructions * 4 * sizeof(uint32_t));
   if (!code)
      return NULL;

   pc = 0;
   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      uint32_t *dw = &code[pc * 4];
      struct codegen cg;

      if (pc >= tc->num_instructions) {
         tc_fail(tc, "wrong instructoun count");
         break;
      }

      codegen_prepare(&cg, inst, pc, tc->rect_linear_width);
      codegen_validate_region_restrictions(&cg);

      switch (inst->opcode) {
      case BRW_OPCODE_MAD:
         codegen_inst_3src(&cg, dw);
         break;
      default:
         codegen_inst(&cg, dw);
         break;
      }

      pc++;
   }

   /* never return an invalid kernel */
   if (tc->fail) {
      FREE(code);
      return NULL;
   }

   if (size)
      *size = pc * 4 * sizeof(uint32_t);

   return code;
}
