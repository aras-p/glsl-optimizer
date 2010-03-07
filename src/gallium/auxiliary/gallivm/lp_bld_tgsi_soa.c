/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * TGSI to LLVM IR translation -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Based on tgsi_sse2.c code written by Michal Krol, Keith Whitwell,
 * Brian Paul, and others.
 */

#include "pipe/p_config.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_exec.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_debug.h"


#define LP_MAX_TEMPS 256
#define LP_MAX_IMMEDIATES 256


#define FOR_EACH_CHANNEL( CHAN )\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)

#define IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   ((INST)->Dst[0].Register.WriteMask & (1 << (CHAN)))

#define IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   if (IS_DST0_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_DST0_ENABLED_CHANNEL( INST, CHAN )\
   FOR_EACH_CHANNEL( CHAN )\
      IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )

#define CHAN_X 0
#define CHAN_Y 1
#define CHAN_Z 2
#define CHAN_W 3

#define QUAD_TOP_LEFT     0
#define QUAD_TOP_RIGHT    1
#define QUAD_BOTTOM_LEFT  2
#define QUAD_BOTTOM_RIGHT 3

#define LP_TGSI_MAX_NESTING 16

struct lp_exec_mask {
   struct lp_build_context *bld;

   boolean has_mask;

   LLVMTypeRef int_vec_type;

   LLVMValueRef cond_stack[LP_TGSI_MAX_NESTING];
   int cond_stack_size;
   LLVMValueRef cond_mask;

   LLVMValueRef exec_mask;
};

struct lp_build_tgsi_soa_context
{
   struct lp_build_context base;

   LLVMValueRef consts_ptr;
   const LLVMValueRef *pos;
   const LLVMValueRef (*inputs)[NUM_CHANNELS];
   LLVMValueRef (*outputs)[NUM_CHANNELS];

   struct lp_build_sampler_soa *sampler;

   LLVMValueRef immediates[LP_MAX_IMMEDIATES][NUM_CHANNELS];
   LLVMValueRef temps[LP_MAX_TEMPS][NUM_CHANNELS];

   struct lp_build_mask_context *mask;
   struct lp_exec_mask exec_mask;
};

static const unsigned char
swizzle_left[4] = {
   QUAD_TOP_LEFT,     QUAD_TOP_LEFT,
   QUAD_BOTTOM_LEFT,  QUAD_BOTTOM_LEFT
};

static const unsigned char
swizzle_right[4] = {
   QUAD_TOP_RIGHT,    QUAD_TOP_RIGHT,
   QUAD_BOTTOM_RIGHT, QUAD_BOTTOM_RIGHT
};

static const unsigned char
swizzle_top[4] = {
   QUAD_TOP_LEFT,     QUAD_TOP_RIGHT,
   QUAD_TOP_LEFT,     QUAD_TOP_RIGHT
};

static const unsigned char
swizzle_bottom[4] = {
   QUAD_BOTTOM_LEFT,  QUAD_BOTTOM_RIGHT,
   QUAD_BOTTOM_LEFT,  QUAD_BOTTOM_RIGHT
};

static void lp_exec_mask_init(struct lp_exec_mask *mask, struct lp_build_context *bld)
{
   mask->bld = bld;
   mask->has_mask = FALSE;
   mask->cond_stack_size = 0;

   mask->int_vec_type = lp_build_int_vec_type(mask->bld->type);
}

static void lp_exec_mask_update(struct lp_exec_mask *mask)
{
   mask->exec_mask = mask->cond_mask;
   if (mask->cond_stack_size > 0)
      mask->has_mask = TRUE;
}

static void lp_exec_mask_cond_push(struct lp_exec_mask *mask,
                                   LLVMValueRef val)
{
   mask->cond_stack[mask->cond_stack_size++] = mask->cond_mask;
   mask->cond_mask = LLVMBuildBitCast(mask->bld->builder, val,
                                      mask->int_vec_type, "");

   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_invert(struct lp_exec_mask *mask)
{
   LLVMValueRef prev_mask = mask->cond_stack[mask->cond_stack_size - 1];
   LLVMValueRef inv_mask = LLVMBuildNot(mask->bld->builder,
                                        mask->cond_mask, "");

   /* means that we didn't have any mask before and that
    * we were fully enabled */
   if (mask->cond_stack_size <= 1) {
      prev_mask = LLVMConstAllOnes(mask->int_vec_type);
   }

   mask->cond_mask = LLVMBuildAnd(mask->bld->builder,
                                  inv_mask,
                                  prev_mask, "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_pop(struct lp_exec_mask *mask)
{
   mask->cond_mask = mask->cond_stack[--mask->cond_stack_size];
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_store(struct lp_exec_mask *mask,
                               LLVMValueRef val,
                               LLVMValueRef dst)
{
   if (mask->has_mask) {
      LLVMValueRef real_val, dst_val;

      dst_val = LLVMBuildLoad(mask->bld->builder, dst, "");
      real_val = lp_build_select(mask->bld,
                                 mask->exec_mask,
                                 val, dst_val);

      LLVMBuildStore(mask->bld->builder, real_val, dst);
   } else
      LLVMBuildStore(mask->bld->builder, val, dst);
}


static LLVMValueRef
emit_ddx(struct lp_build_tgsi_soa_context *bld,
         LLVMValueRef src)
{
   LLVMValueRef src_left  = lp_build_swizzle1_aos(&bld->base, src, swizzle_left);
   LLVMValueRef src_right = lp_build_swizzle1_aos(&bld->base, src, swizzle_right);
   return lp_build_sub(&bld->base, src_right, src_left);
}


static LLVMValueRef
emit_ddy(struct lp_build_tgsi_soa_context *bld,
         LLVMValueRef src)
{
   LLVMValueRef src_top    = lp_build_swizzle1_aos(&bld->base, src, swizzle_top);
   LLVMValueRef src_bottom = lp_build_swizzle1_aos(&bld->base, src, swizzle_bottom);
   return lp_build_sub(&bld->base, src_top, src_bottom);
}


/**
 * Register fetch.
 */
static LLVMValueRef
emit_fetch(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   const unsigned chan_index )
{
   const struct tgsi_full_src_register *reg = &inst->Src[index];
   unsigned swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );
   LLVMValueRef res;

   switch (swizzle) {
   case TGSI_SWIZZLE_X:
   case TGSI_SWIZZLE_Y:
   case TGSI_SWIZZLE_Z:
   case TGSI_SWIZZLE_W:

      switch (reg->Register.File) {
      case TGSI_FILE_CONSTANT: {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), reg->Register.Index*4 + swizzle, 0);
         LLVMValueRef scalar_ptr = LLVMBuildGEP(bld->base.builder, bld->consts_ptr, &index, 1, "");
         LLVMValueRef scalar = LLVMBuildLoad(bld->base.builder, scalar_ptr, "");
         res = lp_build_broadcast_scalar(&bld->base, scalar);
         break;
      }

      case TGSI_FILE_IMMEDIATE:
         res = bld->immediates[reg->Register.Index][swizzle];
         assert(res);
         break;

      case TGSI_FILE_INPUT:
         res = bld->inputs[reg->Register.Index][swizzle];
         assert(res);
         break;

      case TGSI_FILE_TEMPORARY:
         res = LLVMBuildLoad(bld->base.builder, bld->temps[reg->Register.Index][swizzle], "");
         if(!res)
            return bld->base.undef;
         break;

      default:
         assert( 0 );
         return bld->base.undef;
      }
      break;

   default:
      assert( 0 );
      return bld->base.undef;
   }

   switch( tgsi_util_get_full_src_register_sign_mode( reg, chan_index ) ) {
   case TGSI_UTIL_SIGN_CLEAR:
      res = lp_build_abs( &bld->base, res );
      break;

   case TGSI_UTIL_SIGN_SET:
      /* TODO: Use bitwese OR for floating point */
      res = lp_build_abs( &bld->base, res );
      res = LLVMBuildNeg( bld->base.builder, res, "" );
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      res = LLVMBuildNeg( bld->base.builder, res, "" );
      break;

   case TGSI_UTIL_SIGN_KEEP:
      break;
   }

   return res;
}


/**
 * Register fetch with derivatives.
 */
static void
emit_fetch_deriv(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   const unsigned chan_index,
   LLVMValueRef *res,
   LLVMValueRef *ddx,
   LLVMValueRef *ddy)
{
   LLVMValueRef src;

   src = emit_fetch(bld, inst, index, chan_index);

   if(res)
      *res = src;

   /* TODO: use interpolation coeffs for inputs */

   if(ddx)
      *ddx = emit_ddx(bld, src);

   if(ddy)
      *ddy = emit_ddy(bld, src);
}


/**
 * Register store.
 */
static void
emit_store(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   unsigned chan_index,
   LLVMValueRef value)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[index];

   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      value = lp_build_max(&bld->base, value, bld->base.zero);
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      value = lp_build_max(&bld->base, value, lp_build_const_scalar(bld->base.type, -1.0));
      value = lp_build_min(&bld->base, value, bld->base.one);
      break;

   default:
      assert(0);
   }

   switch( reg->Register.File ) {
   case TGSI_FILE_OUTPUT:
      lp_exec_mask_store(&bld->exec_mask, value,
                         bld->outputs[reg->Register.Index][chan_index]);
      break;

   case TGSI_FILE_TEMPORARY:
      lp_exec_mask_store(&bld->exec_mask, value,
                         bld->temps[reg->Register.Index][chan_index]);
      break;

   case TGSI_FILE_ADDRESS:
      /* FIXME */
      assert(0);
      break;

   default:
      assert( 0 );
   }
}


/**
 * High-level instruction translators.
 */


static void
emit_tex( struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst,
          boolean apply_lodbias,
          boolean projected,
          LLVMValueRef *texel)
{
   const uint unit = inst->Src[1].Register.Index;
   LLVMValueRef lodbias;
   LLVMValueRef oow = NULL;
   LLVMValueRef coords[3];
   unsigned num_coords;
   unsigned i;

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      num_coords = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      num_coords = 2;
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      num_coords = 3;
      break;
   default:
      assert(0);
      return;
   }

   if(apply_lodbias)
      lodbias = emit_fetch( bld, inst, 0, 3 );
   else
      lodbias = bld->base.zero;

   if (projected) {
      oow = emit_fetch( bld, inst, 0, 3 );
      oow = lp_build_rcp(&bld->base, oow);
   }

   for (i = 0; i < num_coords; i++) {
      coords[i] = emit_fetch( bld, inst, 0, i );
      if (projected)
         coords[i] = lp_build_mul(&bld->base, coords[i], oow);
   }
   for (i = num_coords; i < 3; i++) {
      coords[i] = bld->base.undef;
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->base.builder,
                                  bld->base.type,
                                  unit, num_coords, coords, lodbias,
                                  texel);
}


static void
emit_kil(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst )
{
   const struct tgsi_full_src_register *reg = &inst->Src[0];
   LLVMValueRef terms[NUM_CHANNELS];
   LLVMValueRef mask;
   unsigned chan_index;

   memset(&terms, 0, sizeof terms);

   FOR_EACH_CHANNEL( chan_index ) {
      unsigned swizzle;

      /* Unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );

      /* Check if the component has not been already tested. */
      assert(swizzle < NUM_CHANNELS);
      if( !terms[swizzle] )
         /* TODO: change the comparison operator instead of setting the sign */
         terms[swizzle] =  emit_fetch(bld, inst, 0, chan_index );
   }

   mask = NULL;
   FOR_EACH_CHANNEL( chan_index ) {
      if(terms[chan_index]) {
         LLVMValueRef chan_mask;

         chan_mask = lp_build_cmp(&bld->base, PIPE_FUNC_GEQUAL, terms[chan_index], bld->base.zero);

         if(mask)
            mask = LLVMBuildAnd(bld->base.builder, mask, chan_mask, "");
         else
            mask = chan_mask;
      }
   }

   if(mask)
      lp_build_mask_update(bld->mask, mask);
}


/**
 * Check if inst src/dest regs use indirect addressing into temporary
 * register file.
 */
static boolean
indirect_temp_reference(const struct tgsi_full_instruction *inst)
{
   uint i;
   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      const struct tgsi_full_src_register *reg = &inst->Src[i];
      if (reg->Register.File == TGSI_FILE_TEMPORARY &&
          reg->Register.Indirect)
         return TRUE;
   }
   for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
      const struct tgsi_full_dst_register *reg = &inst->Dst[i];
      if (reg->Register.File == TGSI_FILE_TEMPORARY &&
          reg->Register.Indirect)
         return TRUE;
   }
   return FALSE;
}

static int
emit_declaration(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_declaration *decl)
{
   unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
   unsigned idx, i;

   for (idx = first; idx <= last; ++idx) {
      boolean ok;

      switch (decl->Declaration.File) {
      case TGSI_FILE_TEMPORARY:
         for (i = 0; i < NUM_CHANNELS; i++)
            bld->temps[idx][i] = lp_build_alloca(&bld->base);
         ok = TRUE;
         break;

      case TGSI_FILE_OUTPUT:
         for (i = 0; i < NUM_CHANNELS; i++)
            bld->outputs[idx][i] = lp_build_alloca(&bld->base);
         ok = TRUE;
         break;

      default:
         /* don't need to declare other vars */
         ok = TRUE;
      }

      if (!ok)
         return FALSE;
   }

   return TRUE;
}

static int
emit_instruction(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   const struct tgsi_opcode_info *info)
{
   unsigned chan_index;
   LLVMValueRef src0, src1, src2;
   LLVMValueRef tmp0, tmp1, tmp2;
   LLVMValueRef tmp3 = NULL;
   LLVMValueRef tmp4 = NULL;
   LLVMValueRef tmp5 = NULL;
   LLVMValueRef tmp6 = NULL;
   LLVMValueRef tmp7 = NULL;
   LLVMValueRef res;
   LLVMValueRef dst0[NUM_CHANNELS];

   /* we can't handle indirect addressing into temp register file yet */
   if (indirect_temp_reference(inst))
      return FALSE;

   assert(info->num_dst <= 1);
   if(info->num_dst) {
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.undef;
      }
   }

   switch (inst->Instruction.Opcode) {
#if 0
   case TGSI_OPCODE_ARL:
      /* FIXME */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         emit_flr(bld, 0, 0);
         emit_f2it( bld, 0 );
         dst0[chan_index] = tmp0;
      }
      break;
#endif

   case TGSI_OPCODE_MOV:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = emit_fetch( bld, inst, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LIT:
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ) {
         dst0[CHAN_X] = bld->base.one;
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ) {
         src0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_Y] = lp_build_max( &bld->base, src0, bld->base.zero);
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         /* XMM[1] = SrcReg[0].yyyy */
         tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
         /* XMM[1] = max(XMM[1], 0) */
         tmp1 = lp_build_max( &bld->base, tmp1, bld->base.zero);
         /* XMM[2] = SrcReg[0].wwww */
         tmp2 = emit_fetch( bld, inst, 0, CHAN_W );
         tmp1 = lp_build_pow( &bld->base, tmp1, tmp2);
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         tmp2 = lp_build_cmp(&bld->base, PIPE_FUNC_GREATER, tmp0, bld->base.zero);
         dst0[CHAN_Z] = lp_build_select(&bld->base, tmp2, tmp1, bld->base.zero);
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_RCP:
   /* TGSI_OPCODE_RECIP */
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      res = lp_build_rcp(&bld->base, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_RSQ:
   /* TGSI_OPCODE_RECIPSQRT */
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      src0 = lp_build_abs(&bld->base, src0);
      res = lp_build_rsqrt(&bld->base, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_EXP:
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z )) {
         LLVMValueRef *p_exp2_int_part = NULL;
         LLVMValueRef *p_frac_part = NULL;
         LLVMValueRef *p_exp2 = NULL;

         src0 = emit_fetch( bld, inst, 0, CHAN_X );

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            p_exp2_int_part = &tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            p_frac_part = &tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            p_exp2 = &tmp2;

         lp_build_exp2_approx(&bld->base, src0, p_exp2_int_part, p_frac_part, p_exp2);

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            dst0[CHAN_X] = tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            dst0[CHAN_Y] = tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            dst0[CHAN_Z] = tmp2;
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_W )) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_LOG:
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z )) {
         LLVMValueRef *p_floor_log2 = NULL;
         LLVMValueRef *p_exp = NULL;
         LLVMValueRef *p_log2 = NULL;

         src0 = emit_fetch( bld, inst, 0, CHAN_X );
         src0 = lp_build_abs( &bld->base, src0 );

         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            p_floor_log2 = &tmp0;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ))
            p_exp = &tmp1;
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            p_log2 = &tmp2;

         lp_build_log2_approx(&bld->base, src0, p_exp, p_floor_log2, p_log2);

         /* dst.x = floor(lg2(abs(src.x))) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ))
            dst0[CHAN_X] = tmp0;
         /* dst.y = abs(src)/ex2(floor(lg2(abs(src.x)))) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y )) {
            dst0[CHAN_Y] = lp_build_div( &bld->base, src0, tmp1);
         }
         /* dst.z = lg2(abs(src.x)) */
         if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ))
            dst0[CHAN_Z] = tmp2;
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( inst, CHAN_W )) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_MUL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_mul(&bld->base, src0, src1);
      }
      break;

   case TGSI_OPCODE_ADD:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_add(&bld->base, src0, src1);
      }
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DP4:
   /* TGSI_OPCODE_DOT4 */
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_W );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_W );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DST:
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         dst0[CHAN_X] = bld->base.one;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_Y );
         tmp1 = emit_fetch( bld, inst, 1, CHAN_Y );
         dst0[CHAN_Y] = lp_build_mul( &bld->base, tmp0, tmp1);
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         dst0[CHAN_Z] = emit_fetch( bld, inst, 0, CHAN_Z );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = emit_fetch( bld, inst, 1, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MIN:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_min( &bld->base, src0, src1 );
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_max( &bld->base, src0, src1 );
      }
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LESS, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_MAD:
   /* TGSI_OPCODE_MADD */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp1 = emit_fetch( bld, inst, 1, chan_index );
         tmp2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
         tmp0 = lp_build_add( &bld->base, tmp0, tmp2);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         tmp1 = emit_fetch( bld, inst, 1, chan_index );
         dst0[chan_index] = lp_build_sub( &bld->base, tmp0, tmp1);
      }
      break;

   case TGSI_OPCODE_LRP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_sub( &bld->base, src1, src2 );
         tmp0 = lp_build_mul( &bld->base, src0, tmp0 );
         dst0[chan_index] = lp_build_add( &bld->base, tmp0, src2 );
      }
      break;

   case TGSI_OPCODE_CND:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp1 = lp_build_const_scalar(bld->base.type, 0.5);
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GREATER, src2, tmp1);
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, src0, src1 );
      }
      break;

   case TGSI_OPCODE_DP2A:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );  /* xmm0 = src[0].x */
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );  /* xmm1 = src[1].x */
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 * xmm1 */
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );  /* xmm1 = src[0].y */
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );  /* xmm2 = src[1].y */
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);              /* xmm1 = xmm1 * xmm2 */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      tmp1 = emit_fetch( bld, inst, 2, CHAN_X );  /* xmm1 = src[2].x */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;  /* dest[ch] = xmm0 */
      }
      break;

   case TGSI_OPCODE_FRC:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         tmp0 = lp_build_floor(&bld->base, src0);
         tmp0 = lp_build_sub(&bld->base, src0, tmp0);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_CLAMP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_max(&bld->base, tmp0, src1);
         tmp0 = lp_build_min(&bld->base, tmp0, src2);
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_FLR:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_floor(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_ROUND:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_round(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_EX2: {
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_exp2( &bld->base, tmp0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;
   }

   case TGSI_OPCODE_LG2:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_log2( &bld->base, tmp0);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_POW:
      src0 = emit_fetch( bld, inst, 0, CHAN_X );
      src1 = emit_fetch( bld, inst, 1, CHAN_X );
      res = lp_build_pow( &bld->base, src0, src1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = res;
      }
      break;

   case TGSI_OPCODE_XPD:
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ) {
         tmp1 = emit_fetch( bld, inst, 1, CHAN_Z );
         tmp3 = emit_fetch( bld, inst, 0, CHAN_Z );
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_Y );
         tmp4 = emit_fetch( bld, inst, 1, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         tmp2 = tmp0;
         tmp2 = lp_build_mul( &bld->base, tmp2, tmp1);
         tmp5 = tmp3;
         tmp5 = lp_build_mul( &bld->base, tmp5, tmp4);
         tmp2 = lp_build_sub( &bld->base, tmp2, tmp5);
         dst0[CHAN_X] = tmp2;
      }
      if( IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) ) {
         tmp2 = emit_fetch( bld, inst, 1, CHAN_X );
         tmp5 = emit_fetch( bld, inst, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp3 = lp_build_mul( &bld->base, tmp3, tmp2);
         tmp1 = lp_build_mul( &bld->base, tmp1, tmp5);
         tmp3 = lp_build_sub( &bld->base, tmp3, tmp1);
         dst0[CHAN_Y] = tmp3;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         tmp5 = lp_build_mul( &bld->base, tmp5, tmp4);
         tmp0 = lp_build_mul( &bld->base, tmp0, tmp2);
         tmp5 = lp_build_sub( &bld->base, tmp5, tmp0);
         dst0[CHAN_Z] = tmp5;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_ABS:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_abs( &bld->base, tmp0 );
      }
      break;

   case TGSI_OPCODE_RCC:
      /* deprecated? */
      assert(0);
      return 0;

   case TGSI_OPCODE_DPH:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Z );
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Z );
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      tmp1 = emit_fetch( bld, inst, 1, CHAN_W );
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_COS:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_cos( &bld->base, tmp0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_DDX:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_fetch_deriv( bld, inst, 0, chan_index, NULL, &dst0[chan_index], NULL);
      }
      break;

   case TGSI_OPCODE_DDY:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_fetch_deriv( bld, inst, 0, chan_index, NULL, NULL, &dst0[chan_index]);
      }
      break;

   case TGSI_OPCODE_KILP:
      /* predicated kill */
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_KIL:
      /* conditional kill */
      emit_kil( bld, inst );
      break;

   case TGSI_OPCODE_PK2H:
      return 0;
      break;

   case TGSI_OPCODE_PK2US:
      return 0;
      break;

   case TGSI_OPCODE_PK4B:
      return 0;
      break;

   case TGSI_OPCODE_PK4UB:
      return 0;
      break;

   case TGSI_OPCODE_RFL:
      return 0;
      break;

   case TGSI_OPCODE_SEQ:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_EQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SFL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.zero;
      }
      break;

   case TGSI_OPCODE_SGT:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_GREATER, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SIN:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
      tmp0 = lp_build_sin( &bld->base, tmp0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;
      }
      break;

   case TGSI_OPCODE_SLE:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_SNE:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_NOTEQUAL, src0, src1 );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, bld->base.one, bld->base.zero );
      }
      break;

   case TGSI_OPCODE_STR:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_TEX:
      /* XXX what about dst0 writemask? */
      emit_tex( bld, inst, FALSE, FALSE, dst0 );
      break;

   case TGSI_OPCODE_TXD:
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_UP2H:
      /* deprecated */
      assert (0);
      return 0;
      break;

   case TGSI_OPCODE_UP2US:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_UP4B:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_UP4UB:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_X2D:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_ARA:
      /* deprecated */
      assert(0);
      return 0;
      break;

#if 0
   case TGSI_OPCODE_ARR:
      /* FIXME */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         emit_rnd( bld, 0, 0 );
         emit_f2it( bld, 0 );
         dst0[chan_index] = tmp0;
      }
      break;
#endif

   case TGSI_OPCODE_BRA:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_CAL:
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_RET:
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_END:
      break;

   case TGSI_OPCODE_SSG:
   /* TGSI_OPCODE_SGN */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_sgn( &bld->base, tmp0 );
      }
      break;

   case TGSI_OPCODE_CMP:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         src0 = emit_fetch( bld, inst, 0, chan_index );
         src1 = emit_fetch( bld, inst, 1, chan_index );
         src2 = emit_fetch( bld, inst, 2, chan_index );
         tmp0 = lp_build_cmp( &bld->base, PIPE_FUNC_LESS, src0, bld->base.zero );
         dst0[chan_index] = lp_build_select( &bld->base, tmp0, src1, src2);
      }
      break;

   case TGSI_OPCODE_SCS:
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_X ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_X] = lp_build_cos( &bld->base, tmp0 );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Y ) {
         tmp0 = emit_fetch( bld, inst, 0, CHAN_X );
         dst0[CHAN_Y] = lp_build_sin( &bld->base, tmp0 );
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_Z ) {
         dst0[CHAN_Z] = bld->base.zero;
      }
      IF_IS_DST0_CHANNEL_ENABLED( inst, CHAN_W ) {
         dst0[CHAN_W] = bld->base.one;
      }
      break;

   case TGSI_OPCODE_TXB:
      emit_tex( bld, inst, TRUE, FALSE, dst0 );
      break;

   case TGSI_OPCODE_NRM:
      /* fall-through */
   case TGSI_OPCODE_NRM4:
      /* 3 or 4-component normalization */
      {
         uint dims = (inst->Instruction.Opcode == TGSI_OPCODE_NRM) ? 3 : 4;

         if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X) ||
             IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y) ||
             IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z) ||
             (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W) && dims == 4)) {

            /* NOTE: Cannot use xmm regs 2/3 here (see emit_rsqrt() above). */

            /* xmm4 = src.x */
            /* xmm0 = src.x * src.x */
            tmp0 = emit_fetch(bld, inst, 0, CHAN_X);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X)) {
               tmp4 = tmp0;
            }
            tmp0 = lp_build_mul( &bld->base, tmp0, tmp0);

            /* xmm5 = src.y */
            /* xmm0 = xmm0 + src.y * src.y */
            tmp1 = emit_fetch(bld, inst, 0, CHAN_Y);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y)) {
               tmp5 = tmp1;
            }
            tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
            tmp0 = lp_build_add( &bld->base, tmp0, tmp1);

            /* xmm6 = src.z */
            /* xmm0 = xmm0 + src.z * src.z */
            tmp1 = emit_fetch(bld, inst, 0, CHAN_Z);
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z)) {
               tmp6 = tmp1;
            }
            tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
            tmp0 = lp_build_add( &bld->base, tmp0, tmp1);

            if (dims == 4) {
               /* xmm7 = src.w */
               /* xmm0 = xmm0 + src.w * src.w */
               tmp1 = emit_fetch(bld, inst, 0, CHAN_W);
               if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W)) {
                  tmp7 = tmp1;
               }
               tmp1 = lp_build_mul( &bld->base, tmp1, tmp1);
               tmp0 = lp_build_add( &bld->base, tmp0, tmp1);
            }

            /* xmm1 = 1 / sqrt(xmm0) */
            tmp1 = lp_build_rsqrt( &bld->base, tmp0);

            /* dst.x = xmm1 * src.x */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X)) {
               dst0[CHAN_X] = lp_build_mul( &bld->base, tmp4, tmp1);
            }

            /* dst.y = xmm1 * src.y */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Y)) {
               dst0[CHAN_Y] = lp_build_mul( &bld->base, tmp5, tmp1);
            }

            /* dst.z = xmm1 * src.z */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_Z)) {
               dst0[CHAN_Z] = lp_build_mul( &bld->base, tmp6, tmp1);
            }

            /* dst.w = xmm1 * src.w */
            if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_X) && dims == 4) {
               dst0[CHAN_W] = lp_build_mul( &bld->base, tmp7, tmp1);
            }
         }

         /* dst.w = 1.0 */
         if (IS_DST0_CHANNEL_ENABLED(inst, CHAN_W) && dims == 3) {
            dst0[CHAN_W] = bld->base.one;
         }
      }
      break;

   case TGSI_OPCODE_DIV:
      /* deprecated */
      assert( 0 );
      return 0;
      break;

   case TGSI_OPCODE_DP2:
      tmp0 = emit_fetch( bld, inst, 0, CHAN_X );  /* xmm0 = src[0].x */
      tmp1 = emit_fetch( bld, inst, 1, CHAN_X );  /* xmm1 = src[1].x */
      tmp0 = lp_build_mul( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 * xmm1 */
      tmp1 = emit_fetch( bld, inst, 0, CHAN_Y );  /* xmm1 = src[0].y */
      tmp2 = emit_fetch( bld, inst, 1, CHAN_Y );  /* xmm2 = src[1].y */
      tmp1 = lp_build_mul( &bld->base, tmp1, tmp2);              /* xmm1 = xmm1 * xmm2 */
      tmp0 = lp_build_add( &bld->base, tmp0, tmp1);              /* xmm0 = xmm0 + xmm1 */
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         dst0[chan_index] = tmp0;  /* dest[ch] = xmm0 */
      }
      break;

   case TGSI_OPCODE_TXL:
      emit_tex( bld, inst, TRUE, FALSE, dst0 );
      break;

   case TGSI_OPCODE_TXP:
      emit_tex( bld, inst, FALSE, TRUE, dst0 );
      break;
      
   case TGSI_OPCODE_BRK:
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_IF:
      tmp0 = emit_fetch(bld, inst, 0, CHAN_X);
      lp_exec_mask_cond_push(&bld->exec_mask, tmp0);
      break;

   case TGSI_OPCODE_BGNFOR:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_REP:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_ELSE:
      lp_exec_mask_cond_invert(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ENDIF:
      lp_exec_mask_cond_pop(&bld->exec_mask);
      break;

   case TGSI_OPCODE_ENDFOR:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_ENDREP:
      /* deprecated */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_PUSHA:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_POPA:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_CEIL:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_ceil(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_I2F:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_NOT:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_TRUNC:
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         tmp0 = emit_fetch( bld, inst, 0, chan_index );
         dst0[chan_index] = lp_build_trunc(&bld->base, tmp0);
      }
      break;

   case TGSI_OPCODE_SHL:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_ISHR:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_AND:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_OR:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_MOD:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_XOR:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_SAD:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_TXF:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_TXQ:
      /* deprecated? */
      assert(0);
      return 0;
      break;

   case TGSI_OPCODE_CONT:
      /* FIXME */
      return 0;
      break;

   case TGSI_OPCODE_EMIT:
      return 0;
      break;

   case TGSI_OPCODE_ENDPRIM:
      return 0;
      break;

   case TGSI_OPCODE_NOP:
      break;

   default:
      return 0;
   }
   
   if(info->num_dst) {
      FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_store( bld, inst, 0, chan_index, dst0[chan_index]);
      }
   }

   return 1;
}


void
lp_build_tgsi_soa(LLVMBuilderRef builder,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  struct lp_build_mask_context *mask,
                  LLVMValueRef consts_ptr,
                  const LLVMValueRef *pos,
                  const LLVMValueRef (*inputs)[NUM_CHANNELS],
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  struct lp_build_sampler_soa *sampler)
{
   struct lp_build_tgsi_soa_context bld;
   struct tgsi_parse_context parse;
   uint num_immediates = 0;
   unsigned i;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.base, builder, type);
   bld.mask = mask;
   bld.pos = pos;
   bld.inputs = inputs;
   bld.outputs = outputs;
   bld.consts_ptr = consts_ptr;
   bld.sampler = sampler;

   lp_exec_mask_init(&bld.exec_mask, &bld.base);

   tgsi_parse_init( &parse, tokens );

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* Inputs already interpolated */
         {
            if (!emit_declaration( &bld, &parse.FullToken.FullDeclaration ))
               _debug_printf("warning: failed to define LLVM variable\n");
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            unsigned opcode = parse.FullToken.FullInstruction.Instruction.Opcode;
            const struct tgsi_opcode_info *info = tgsi_get_opcode_info(opcode);
            if (!emit_instruction( &bld, &parse.FullToken.FullInstruction, info ))
               _debug_printf("warning: failed to translate tgsi opcode %s to LLVM\n",
                             info ? info->mnemonic : "<invalid>");
         }

         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* simply copy the immediate values into the next immediates[] slot */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            assert(size <= 4);
            assert(num_immediates < LP_MAX_IMMEDIATES);
            for( i = 0; i < size; ++i )
               bld.immediates[num_immediates][i] =
                  lp_build_const_scalar(type, parse.FullToken.FullImmediate.u[i].Float);
            for( i = size; i < 4; ++i )
               bld.immediates[num_immediates][i] = bld.base.undef;
            num_immediates++;
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert( 0 );
      }
   }

   tgsi_parse_free( &parse );
}

