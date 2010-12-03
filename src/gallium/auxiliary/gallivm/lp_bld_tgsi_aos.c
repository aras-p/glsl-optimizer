/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * TGSI to LLVM IR translation -- AoS.
 *
 * FIXME:
 * - No control flow support: the existing control flow code should be factored
 * out into from the SoA code into a common module and shared.
 * - No derivatives. Derivate logic should be pluggable, just like the samplers.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_config.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_scan.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_quad.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_limits.h"
#include "lp_bld_debug.h"


#define LP_MAX_INSTRUCTIONS 256


struct lp_build_tgsi_aos_context
{
   struct lp_build_context base;

   /* Builder for integer masks and indices */
   struct lp_build_context int_bld;

   /*
    * AoS swizzle used:
    * - swizzles[0] = red index
    * - swizzles[1] = green index
    * - swizzles[2] = blue index
    * - swizzles[3] = alpha index
    */
   unsigned char swizzles[4];
   unsigned char inv_swizzles[4];

   LLVMValueRef consts_ptr;
   const LLVMValueRef *inputs;
   LLVMValueRef *outputs;

   struct lp_build_sampler_aos *sampler;

   LLVMValueRef immediates[LP_MAX_TGSI_IMMEDIATES];
   LLVMValueRef temps[LP_MAX_TGSI_TEMPS];
   LLVMValueRef addr[LP_MAX_TGSI_ADDRS];
   LLVMValueRef preds[LP_MAX_TGSI_PREDS];

   /* We allocate/use this array of temps if (1 << TGSI_FILE_TEMPORARY) is
    * set in the indirect_files field.
    * The temps[] array above is unused then.
    */
   LLVMValueRef temps_array;

   /** bitmask indicating which register files are accessed indirectly */
   unsigned indirect_files;

   struct tgsi_full_instruction *instructions;
   uint max_instructions;
};


/**
 * Wrapper around lp_build_swizzle_aos which translates swizzles to another 
 * ordering.
 */
static LLVMValueRef
swizzle_aos(struct lp_build_tgsi_aos_context *bld,
            LLVMValueRef a,
            unsigned swizzle_x,
            unsigned swizzle_y,
            unsigned swizzle_z,
            unsigned swizzle_w)
{
   unsigned char swizzles[4];

   assert(swizzle_x < 4);
   assert(swizzle_y < 4);
   assert(swizzle_z < 4);
   assert(swizzle_w < 4);

   swizzles[bld->inv_swizzles[0]] = bld->swizzles[swizzle_x];
   swizzles[bld->inv_swizzles[1]] = bld->swizzles[swizzle_y];
   swizzles[bld->inv_swizzles[2]] = bld->swizzles[swizzle_z];
   swizzles[bld->inv_swizzles[3]] = bld->swizzles[swizzle_w];

   return lp_build_swizzle_aos(&bld->base, a, swizzles);
}


static LLVMValueRef
swizzle_scalar_aos(struct lp_build_tgsi_aos_context *bld,
                   LLVMValueRef a,
                   unsigned chan)
{
   chan = bld->swizzles[chan];
   return lp_build_swizzle_scalar_aos(&bld->base, a, chan);
}


/**
 * Register fetch.
 */
static LLVMValueRef
emit_fetch(
   struct lp_build_tgsi_aos_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned src_op)
{
   LLVMBuilderRef builder = bld->base.gallivm->builder;
   struct lp_type type = bld->base.type;
   const struct tgsi_full_src_register *reg = &inst->Src[src_op];
   LLVMValueRef res;
   unsigned chan;

   assert(!reg->Register.Indirect);

   /*
    * Fetch the from the register file.
    */

   switch (reg->Register.File) {
   case TGSI_FILE_CONSTANT:
      /*
       * Get the constants components
       */

      res = bld->base.undef;
      for (chan = 0; chan < 4; ++chan) {
         LLVMValueRef index;
         LLVMValueRef scalar_ptr;
         LLVMValueRef scalar;
         LLVMValueRef swizzle;

         index = lp_build_const_int32(bld->base.gallivm, reg->Register.Index * 4 + chan);

         scalar_ptr = LLVMBuildGEP(builder, bld->consts_ptr,
                                   &index, 1, "");

         scalar = LLVMBuildLoad(builder, scalar_ptr, "");

         lp_build_name(scalar, "const[%u].%c", reg->Register.Index, "xyzw"[chan]);

         /*
          * NOTE: constants array is always assumed to be RGBA
          */

         swizzle = lp_build_const_int32(bld->base.gallivm, chan);

         res = LLVMBuildInsertElement(builder, res, scalar, swizzle, "");
      }

      /*
       * Broadcast the first quaternion to all others.
       *
       * XXX: could be factored into a reusable function.
       */

      if (type.length > 4) {
         LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
         unsigned i;

         for (chan = 0; chan < 4; ++chan) {
            shuffles[chan] = lp_build_const_int32(bld->base.gallivm, chan);
         }

         for (i = 4; i < type.length; ++i) {
            shuffles[i] = shuffles[i % 4];
         }

         res = LLVMBuildShuffleVector(builder,
                                      res, bld->base.undef,
                                      LLVMConstVector(shuffles, type.length),
                                      "");
      }
      break;

   case TGSI_FILE_IMMEDIATE:
      res = bld->immediates[reg->Register.Index];
      assert(res);
      break;

   case TGSI_FILE_INPUT:
      res = bld->inputs[reg->Register.Index];
      assert(res);
      break;

   case TGSI_FILE_TEMPORARY:
      {
         LLVMValueRef temp_ptr;
         temp_ptr = bld->temps[reg->Register.Index];
         res = LLVMBuildLoad(builder, temp_ptr, "");
         if (!res)
            return bld->base.undef;
      }
      break;

   default:
      assert(0 && "invalid src register in emit_fetch()");
      return bld->base.undef;
   }

   /*
    * Apply sign modifier.
    */

   if (reg->Register.Absolute) {
      res = lp_build_abs(&bld->base, res);
   }

   if(reg->Register.Negate) {
      res = lp_build_negate(&bld->base, res);
   }

   /*
    * Swizzle the argument
    */

   res = swizzle_aos(bld, res,
                     reg->Register.SwizzleX,
                     reg->Register.SwizzleY,
                     reg->Register.SwizzleZ,
                     reg->Register.SwizzleW);

   return res;
}


/**
 * Register store.
 */
static void
emit_store(
   struct lp_build_tgsi_aos_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   LLVMValueRef value)
{
   LLVMBuilderRef builder = bld->base.gallivm->builder;
   const struct tgsi_full_dst_register *reg = &inst->Dst[index];
   LLVMValueRef mask = NULL;
   LLVMValueRef ptr;

   /*
    * Saturate the value
    */

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      value = lp_build_max(&bld->base, value, bld->base.zero);
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      value = lp_build_max(&bld->base, value, lp_build_const_vec(bld->base.gallivm, bld->base.type, -1.0));
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   default:
      assert(0);
   }

   /*
    * Translate the register file
    */

   assert(!reg->Register.Indirect);

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      ptr = bld->outputs[reg->Register.Index];
      break;

   case TGSI_FILE_TEMPORARY:
      ptr = bld->temps[reg->Register.Index];
      break;

   case TGSI_FILE_ADDRESS:
      ptr = bld->addr[reg->Indirect.Index];
      break;

   case TGSI_FILE_PREDICATE:
      ptr = bld->preds[reg->Register.Index];
      break;

   default:
      assert(0);
      return;
   }

   /*
    * Predicate
    */

   if (inst->Instruction.Predicate) {
      LLVMValueRef pred;

      assert(inst->Predicate.Index < LP_MAX_TGSI_PREDS);

      pred = LLVMBuildLoad(builder,
                           bld->preds[inst->Predicate.Index], "");

      /*
       * Convert the value to an integer mask.
       */
      pred = lp_build_compare(bld->base.gallivm,
                               bld->base.type,
                               PIPE_FUNC_NOTEQUAL,
                               pred,
                               bld->base.zero);

      if (inst->Predicate.Negate) {
         pred = LLVMBuildNot(builder, pred, "");
      }

      pred = swizzle_aos(bld, pred,
                         inst->Predicate.SwizzleX,
                         inst->Predicate.SwizzleY,
                         inst->Predicate.SwizzleZ,
                         inst->Predicate.SwizzleW);

      if (mask) {
         mask = LLVMBuildAnd(builder, mask, pred, "");
      } else {
         mask = pred;
      }
   }

   /*
    * Writemask
    */

   if (reg->Register.WriteMask != TGSI_WRITEMASK_XYZW) {
      LLVMValueRef writemask;

      writemask = lp_build_const_mask_aos(bld->base.gallivm, bld->base.type,
                                          reg->Register.WriteMask);

      if (mask) {
         mask = LLVMBuildAnd(builder, mask, writemask, "");
      } else {
         mask = writemask;
      }
   }

   if (mask) {
      LLVMValueRef orig_value;

      orig_value = LLVMBuildLoad(builder, ptr, "");
      value = lp_build_select(&bld->base,
                              mask, value, orig_value);
   }

   LLVMBuildStore(builder, value, ptr);
}


/**
 * High-level instruction translators.
 */

static LLVMValueRef
emit_tex(struct lp_build_tgsi_aos_context *bld,
         const struct tgsi_full_instruction *inst,
         enum lp_build_tex_modifier modifier)
{
   unsigned target;
   unsigned unit;
   LLVMValueRef coords;
   LLVMValueRef ddx;
   LLVMValueRef ddy;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      return bld->base.undef;
   }

   target = inst->Texture.Texture;

   coords = emit_fetch( bld, inst, 0 );

   if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      ddx = emit_fetch( bld, inst, 1 );
      ddy = emit_fetch( bld, inst, 2 );
      unit = inst->Src[3].Register.Index;
   }  else {
#if 0
      ddx = lp_build_ddx( &bld->base, coords );
      ddy = lp_build_ddy( &bld->base, coords );
#else
      /* TODO */
      ddx = bld->base.one;
      ddy = bld->base.one;
#endif
      unit = inst->Src[1].Register.Index;
   }

   return bld->sampler->emit_fetch_texel(bld->sampler,
                                         &bld->base,
                                         target, unit,
                                         coords, ddx, ddy,
                                         modifier);
}


static void
emit_declaration(
   struct lp_build_tgsi_aos_context *bld,
   const struct tgsi_full_declaration *decl)
{
   struct gallivm_state *gallivm = bld->base.gallivm;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->base.gallivm, bld->base.type);

   unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
   unsigned idx;

   for (idx = first; idx <= last; ++idx) {
      switch (decl->Declaration.File) {
      case TGSI_FILE_TEMPORARY:
         assert(idx < LP_MAX_TGSI_TEMPS);
         if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
            LLVMValueRef array_size = lp_build_const_int32(gallivm, last + 1);
            bld->temps_array = lp_build_array_alloca(bld->base.gallivm,
                                                     vec_type, array_size, "");
         } else {
            bld->temps[idx] = lp_build_alloca(gallivm, vec_type, "");
         }
         break;

      case TGSI_FILE_OUTPUT:
         bld->outputs[idx] = lp_build_alloca(gallivm, vec_type, "");
         break;

      case TGSI_FILE_ADDRESS:
         assert(idx < LP_MAX_TGSI_ADDRS);
         bld->addr[idx] = lp_build_alloca(gallivm, vec_type, "");
         break;

      case TGSI_FILE_PREDICATE:
         assert(idx < LP_MAX_TGSI_PREDS);
         bld->preds[idx] = lp_build_alloca(gallivm, vec_type, "");
         break;

      default:
         /* don't need to declare other vars */
         break;
      }
   }
}


/**
 * Emit LLVM for one TGSI instruction.
 * \param return TRUE for success, FALSE otherwise
 */
static boolean
emit_instruction(
   struct lp_build_tgsi_aos_context *bld,
   const struct tgsi_full_instruction *inst,
   const struct tgsi_opcode_info *info,
   int *pc)
{
   LLVMValueRef src0, src1, src2;
   LLVMValueRef tmp0, tmp1;
   LLVMValueRef dst0 = NULL;

   /*
    * Stores and write masks are handled in a general fashion after the long
    * instruction opcode switch statement.
    *
    * Although not stricitly necessary, we avoid generating instructions for
    * channels which won't be stored, in cases where's that easy. For some
    * complex instructions, like texture sampling, it is more convenient to
    * assume a full writemask and then let LLVM optimization passes eliminate
    * redundant code.
    */

   (*pc)++;

   assert(info->num_dst <= 1);
   if (info->num_dst) {
      dst0 = bld->base.undef;
   }

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_floor(&bld->base, src0);
      break;

   case TGSI_OPCODE_MOV:
      dst0 = emit_fetch(bld, inst, 0);
      break;

   case TGSI_OPCODE_LIT:
      return FALSE;

   case TGSI_OPCODE_RCP:
   /* TGSI_OPCODE_RECIP */
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_rcp(&bld->base, src0);
      break;

   case TGSI_OPCODE_RSQ:
   /* TGSI_OPCODE_RECIPSQRT */
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = lp_build_abs(&bld->base, src0);
      dst0 = lp_build_rsqrt(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_EXP:
      return FALSE;

   case TGSI_OPCODE_LOG:
      return FALSE;

   case TGSI_OPCODE_MUL:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      dst0 = lp_build_mul(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_ADD:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      dst0 = lp_build_add(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      return FALSE;

   case TGSI_OPCODE_DP4:
   /* TGSI_OPCODE_DOT4 */
      return FALSE;

   case TGSI_OPCODE_DST:
      return FALSE;

   case TGSI_OPCODE_MIN:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      dst0 = lp_build_max(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_MAX:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      dst0 = lp_build_max(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_LESS, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_GEQUAL, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_MAD:
   /* TGSI_OPCODE_MADD */
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      src2 = emit_fetch(bld, inst, 2);
      tmp0 = lp_build_mul(&bld->base, src0, src1);
      dst0 = lp_build_add(&bld->base, tmp0, src2);
      break;

   case TGSI_OPCODE_SUB:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      dst0 = lp_build_sub(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_LRP:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      src2 = emit_fetch(bld, inst, 2);
      tmp0 = lp_build_sub(&bld->base, src1, src2);
      tmp0 = lp_build_mul(&bld->base, src0, tmp0);
      dst0 = lp_build_add(&bld->base, tmp0, src2);
      break;

   case TGSI_OPCODE_CND:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      src2 = emit_fetch(bld, inst, 2);
      tmp1 = lp_build_const_vec(bld->base.gallivm, bld->base.type, 0.5);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_GREATER, src2, tmp1);
      dst0 = lp_build_select(&bld->base, tmp0, src0, src1);
      break;

   case TGSI_OPCODE_DP2A:
      return FALSE;

   case TGSI_OPCODE_FRC:
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = lp_build_floor(&bld->base, src0);
      dst0 = lp_build_sub(&bld->base, src0, tmp0);
      break;

   case TGSI_OPCODE_CLAMP:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      src2 = emit_fetch(bld, inst, 2);
      tmp0 = lp_build_max(&bld->base, src0, src1);
      dst0 = lp_build_min(&bld->base, tmp0, src2);
      break;

   case TGSI_OPCODE_FLR:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_floor(&bld->base, src0);
      break;

   case TGSI_OPCODE_ROUND:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_round(&bld->base, src0);
      break;

   case TGSI_OPCODE_EX2:
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = lp_build_swizzle_scalar_aos(&bld->base, src0, TGSI_SWIZZLE_X);
      dst0 = lp_build_exp2(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_LG2:
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = swizzle_scalar_aos(bld, src0, TGSI_SWIZZLE_X);
      dst0 = lp_build_log2(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_POW:
      src0 = emit_fetch(bld, inst, 0);
      src0 = swizzle_scalar_aos(bld, src0, TGSI_SWIZZLE_X);
      src1 = emit_fetch(bld, inst, 1);
      src1 = swizzle_scalar_aos(bld, src1, TGSI_SWIZZLE_X);
      dst0 = lp_build_pow(&bld->base, src0, src1);
      break;

   case TGSI_OPCODE_XPD:
      return FALSE;

   case TGSI_OPCODE_ABS:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_abs(&bld->base, src0);
      break;

   case TGSI_OPCODE_RCC:
      /* deprecated? */
      assert(0);
      return FALSE;

   case TGSI_OPCODE_DPH:
      return FALSE;

   case TGSI_OPCODE_COS:
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = swizzle_scalar_aos(bld, src0, TGSI_SWIZZLE_X);
      dst0 = lp_build_cos(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_DDX:
      return FALSE;

   case TGSI_OPCODE_DDY:
      return FALSE;

   case TGSI_OPCODE_KILP:
      /* predicated kill */
      return FALSE;

   case TGSI_OPCODE_KIL:
      /* conditional kill */
      return FALSE;

   case TGSI_OPCODE_PK2H:
      return FALSE;
      break;

   case TGSI_OPCODE_PK2US:
      return FALSE;
      break;

   case TGSI_OPCODE_PK4B:
      return FALSE;
      break;

   case TGSI_OPCODE_PK4UB:
      return FALSE;

   case TGSI_OPCODE_RFL:
      return FALSE;

   case TGSI_OPCODE_SEQ:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_EQUAL, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_SFL:
      dst0 = bld->base.zero;
      break;

   case TGSI_OPCODE_SGT:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_GREATER, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_SIN:
      src0 = emit_fetch(bld, inst, 0);
      tmp0 = swizzle_scalar_aos(bld, src0, TGSI_SWIZZLE_X);
      dst0 = lp_build_sin(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_SLE:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_LEQUAL, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_SNE:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_NOTEQUAL, src0, src1);
      dst0 = lp_build_select(&bld->base, tmp0, bld->base.one, bld->base.zero);
      break;

   case TGSI_OPCODE_STR:
      dst0 = bld->base.one;
      break;

   case TGSI_OPCODE_TEX:
      dst0 = emit_tex(bld, inst, LP_BLD_TEX_MODIFIER_NONE);
      break;

   case TGSI_OPCODE_TXD:
      dst0 = emit_tex(bld, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV);
      break;

   case TGSI_OPCODE_UP2H:
      /* deprecated */
      assert (0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP2US:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP4B:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_UP4UB:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_X2D:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ARA:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ARR:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_round(&bld->base, src0);
      break;

   case TGSI_OPCODE_BRA:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CAL:
      return FALSE;

   case TGSI_OPCODE_RET:
      return FALSE;

   case TGSI_OPCODE_END:
      *pc = -1;
      break;

   case TGSI_OPCODE_SSG:
   /* TGSI_OPCODE_SGN */
      tmp0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_sgn(&bld->base, tmp0);
      break;

   case TGSI_OPCODE_CMP:
      src0 = emit_fetch(bld, inst, 0);
      src1 = emit_fetch(bld, inst, 1);
      src2 = emit_fetch(bld, inst, 2);
      tmp0 = lp_build_cmp(&bld->base, PIPE_FUNC_LESS, src0, bld->base.zero);
      dst0 = lp_build_select(&bld->base, tmp0, src1, src2);
      break;

   case TGSI_OPCODE_SCS:
      return FALSE;

   case TGSI_OPCODE_TXB:
      dst0 = emit_tex(bld, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS);
      break;

   case TGSI_OPCODE_NRM:
      /* fall-through */
   case TGSI_OPCODE_NRM4:
      return FALSE;

   case TGSI_OPCODE_DIV:
      /* deprecated */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_DP2:
      return FALSE;

   case TGSI_OPCODE_TXL:
      dst0 = emit_tex(bld, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD);
      break;

   case TGSI_OPCODE_TXP:
      dst0 = emit_tex(bld, inst, LP_BLD_TEX_MODIFIER_PROJECTED);
      break;

   case TGSI_OPCODE_BRK:
      return FALSE;

   case TGSI_OPCODE_IF:
      return FALSE;

   case TGSI_OPCODE_BGNLOOP:
      return FALSE;

   case TGSI_OPCODE_BGNSUB:
      return FALSE;

   case TGSI_OPCODE_ELSE:
      return FALSE;

   case TGSI_OPCODE_ENDIF:
      return FALSE;

   case TGSI_OPCODE_ENDLOOP:
      return FALSE;

   case TGSI_OPCODE_ENDSUB:
      return FALSE;

   case TGSI_OPCODE_PUSHA:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_POPA:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CEIL:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_ceil(&bld->base, src0);
      break;

   case TGSI_OPCODE_I2F:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_NOT:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TRUNC:
      src0 = emit_fetch(bld, inst, 0);
      dst0 = lp_build_trunc(&bld->base, src0);
      break;

   case TGSI_OPCODE_SHL:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_ISHR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_AND:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_OR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_MOD:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_XOR:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_SAD:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TXF:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_TXQ:
      /* deprecated? */
      assert(0);
      return FALSE;
      break;

   case TGSI_OPCODE_CONT:
      return FALSE;

   case TGSI_OPCODE_EMIT:
      return FALSE;
      break;

   case TGSI_OPCODE_ENDPRIM:
      return FALSE;
      break;

   case TGSI_OPCODE_NOP:
      break;

   default:
      return FALSE;
   }
   
   if (info->num_dst) {
      emit_store(bld, inst, 0, dst0);
   }

   return TRUE;
}


void
lp_build_tgsi_aos(struct gallivm_state *gallivm,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  const unsigned char swizzles[4],
                  LLVMValueRef consts_ptr,
                  const LLVMValueRef *inputs,
                  LLVMValueRef *outputs,
                  struct lp_build_sampler_aos *sampler,
                  const struct tgsi_shader_info *info)
{
   struct lp_build_tgsi_aos_context bld;
   struct tgsi_parse_context parse;
   uint num_immediates = 0;
   uint num_instructions = 0;
   unsigned chan;
   int pc = 0;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.base, gallivm, type);
   lp_build_context_init(&bld.int_bld, gallivm, lp_int_type(type));

   for (chan = 0; chan < 4; ++chan) {
      bld.swizzles[chan] = swizzles[chan];
      bld.inv_swizzles[swizzles[chan]] = chan;
   }

   bld.inputs = inputs;
   bld.outputs = outputs;
   bld.consts_ptr = consts_ptr;
   bld.sampler = sampler;
   bld.indirect_files = info->indirect_files;
   bld.instructions = (struct tgsi_full_instruction *)
                      MALLOC(LP_MAX_INSTRUCTIONS * sizeof(struct tgsi_full_instruction));
   bld.max_instructions = LP_MAX_INSTRUCTIONS;

   if (!bld.instructions) {
      return;
   }

   tgsi_parse_init(&parse, tokens);

   while (!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch(parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* Inputs already interpolated */
         emit_declaration(&bld, &parse.FullToken.FullDeclaration);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            /* save expanded instruction */
            if (num_instructions == bld.max_instructions) {
               struct tgsi_full_instruction *instructions;
               instructions = REALLOC(bld.instructions,
                                      bld.max_instructions
                                      * sizeof(struct tgsi_full_instruction),
                                      (bld.max_instructions + LP_MAX_INSTRUCTIONS)
                                      * sizeof(struct tgsi_full_instruction));
               if (!instructions) {
                  break;
               }
               bld.instructions = instructions;
               bld.max_instructions += LP_MAX_INSTRUCTIONS;
            }

            memcpy(bld.instructions + num_instructions,
                   &parse.FullToken.FullInstruction,
                   sizeof(bld.instructions[0]));

            num_instructions++;
         }

         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* simply copy the immediate values into the next immediates[] slot */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            float imm[4];
            assert(size <= 4);
            assert(num_immediates < LP_MAX_TGSI_IMMEDIATES);
            for (chan = 0; chan < 4; ++chan) {
               imm[chan] = 0.0f;
            }
            for (chan = 0; chan < size; ++chan) {
               unsigned swizzle = bld.swizzles[chan];
               imm[swizzle] = parse.FullToken.FullImmediate.u[chan].Float;
            }
            bld.immediates[num_immediates] =
                     lp_build_const_aos(gallivm, type,
                                        imm[0], imm[1], imm[2], imm[3],
                                        NULL);
            num_immediates++;
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert(0);
      }
   }

   while (pc != -1) {
      struct tgsi_full_instruction *instr = bld.instructions + pc;
      const struct tgsi_opcode_info *opcode_info =
         tgsi_get_opcode_info(instr->Instruction.Opcode);
      if (!emit_instruction(&bld, instr, opcode_info, &pc))
         _debug_printf("warning: failed to translate tgsi opcode %s to LLVM\n",
                       opcode_info->mnemonic);
   }

   if (0) {
      LLVMBasicBlockRef block = LLVMGetInsertBlock(gallivm->builder);
      LLVMValueRef function = LLVMGetBasicBlockParent(block);
      debug_printf("11111111111111111111111111111 \n");
      tgsi_dump(tokens, 0);
      lp_debug_dump_value(function);
      debug_printf("2222222222222222222222222222 \n");
   }
   tgsi_parse_free(&parse);

   if (0) {
      LLVMModuleRef module = LLVMGetGlobalParent(
         LLVMGetBasicBlockParent(LLVMGetInsertBlock(gallivm->builder)));
      LLVMDumpModule(module);
   }

   FREE(bld.instructions);
}

