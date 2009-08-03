/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * Blend LLVM IR generation.
 *
 * This code is generic -- it should be able to cope both with floating point
 * and integer inputs in AOS form.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "pipe/p_state.h"

#include "lp_bld.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"


/**
 * We may the same values several times, so we keep them here to avoid
 * recomputing them. Also reusing the values allows us to do simplifications
 * that LLVM optimization passes wouldn't normally be able to do.
 */
struct lp_build_blend_context
{
   struct lp_build_context base;
   
   LLVMValueRef src;
   LLVMValueRef dst;
   LLVMValueRef const_;

   LLVMValueRef inv_src;
   LLVMValueRef inv_dst;
   LLVMValueRef inv_const;
   LLVMValueRef saturate;

   LLVMValueRef rgb_src_factor;
   LLVMValueRef alpha_src_factor;
   LLVMValueRef rgb_dst_factor;
   LLVMValueRef alpha_dst_factor;
};


static LLVMValueRef
lp_build_blend_factor_unswizzled(struct lp_build_blend_context *bld,
                                 unsigned factor,
                                 boolean alpha)
{
   switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:
      return bld->base.zero;
   case PIPE_BLENDFACTOR_ONE:
      return bld->base.one;
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      return bld->src;
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA:
      return bld->dst;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      if(alpha)
         return bld->base.one;
      else {
         if(!bld->inv_dst)
            bld->inv_dst = lp_build_comp(&bld->base, bld->dst);
         if(!bld->saturate)
            bld->saturate = lp_build_min(&bld->base, bld->src, bld->inv_dst);
         return bld->saturate;
      }
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      return bld->const_;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      /* TODO */
      assert(0);
      return bld->base.zero;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      if(!bld->inv_src)
         bld->inv_src = lp_build_comp(&bld->base, bld->src);
      return bld->inv_src;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      if(!bld->inv_dst)
         bld->inv_dst = lp_build_comp(&bld->base, bld->dst);
      return bld->inv_dst;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      if(!bld->inv_const)
         bld->inv_const = lp_build_comp(&bld->base, bld->const_);
      return bld->inv_const;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      /* TODO */
      assert(0);
      return bld->base.zero;
   default:
      assert(0);
      return bld->base.zero;
   }
}


enum lp_build_blend_swizzle {
   LP_BUILD_BLEND_SWIZZLE_RGBA = 0,
   LP_BUILD_BLEND_SWIZZLE_AAAA = 1,
};


/**
 * How should we shuffle the base factor.
 */
static enum lp_build_blend_swizzle
lp_build_blend_factor_swizzle(unsigned factor)
{
   switch (factor) {
   case PIPE_BLENDFACTOR_ONE:
   case PIPE_BLENDFACTOR_ZERO:
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      return LP_BUILD_BLEND_SWIZZLE_RGBA;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
   case PIPE_BLENDFACTOR_DST_ALPHA:
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      return LP_BUILD_BLEND_SWIZZLE_AAAA;
   default:
      assert(0);
      return LP_BUILD_BLEND_SWIZZLE_RGBA;
   }
}


static LLVMValueRef
lp_build_blend_swizzle(struct lp_build_blend_context *bld,
                       LLVMValueRef rgb, 
                       LLVMValueRef alpha, 
                       enum lp_build_blend_swizzle rgb_swizzle,
                       unsigned alpha_swizzle)
{
   const unsigned n = bld->base.type.length;
   LLVMValueRef swizzles[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   if(rgb == alpha) {
      if(rgb_swizzle == LP_BUILD_BLEND_SWIZZLE_RGBA)
         return rgb;

      alpha = bld->base.undef;
   }

   if(rgb_swizzle == LP_BUILD_BLEND_SWIZZLE_RGBA &&
      !bld->base.type.floating) {
#if 0
      /* Use a select */
      /* FIXME: Unfortunetaly select of vectors do not work */

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            swizzles[j + i] = LLVMConstInt(LLVMInt1Type(), i == alpha_swizzle ? 0 : 1, 0);

      return LLVMBuildSelect(bld->base.builder, LLVMConstVector(swizzles, n), rgb, alpha, "");
#else
      /* XXX: Use a bitmask, as byte shuffles often end up being translated
       * into many PEXTRB. Ideally LLVM X86 code generation should pick this
       * automatically for us. */

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            swizzles[j + i] = LLVMConstInt(LLVMIntType(bld->base.type.width), i == alpha_swizzle ? 0 : ~0, 0);

      /* TODO: Unfortunately constant propagation prevents from using PANDN. And
       * on SSE4 we have even better -- PBLENDVB */
      return LLVMBuildOr(bld->base.builder,
                         LLVMBuildAnd(bld->base.builder, rgb,   LLVMConstVector(swizzles, n), ""),
                         LLVMBuildAnd(bld->base.builder, alpha, LLVMBuildNot(bld->base.builder, LLVMConstVector(swizzles, n), ""), ""),
                         "");
#endif
   }

   for(j = 0; j < n; j += 4) {
      for(i = 0; i < 4; ++i) {
         unsigned swizzle;

         if(i == alpha_swizzle && alpha != bld->base.undef) {
            /* Take the alpha from the second shuffle argument */
            swizzle = n + j + alpha_swizzle;
         }
         else if (rgb_swizzle == LP_BUILD_BLEND_SWIZZLE_AAAA) {
            /* Take the alpha from the first shuffle argument */
            swizzle = j + alpha_swizzle;
         }
         else {
            swizzle = j + i;
         }

         swizzles[j + i] = LLVMConstInt(LLVMInt32Type(), swizzle, 0);
      }
   }

   return LLVMBuildShuffleVector(bld->base.builder, rgb, alpha, LLVMConstVector(swizzles, n), "");
}


/**
 * @sa http://www.opengl.org/sdk/docs/man/xhtml/glBlendFuncSeparate.xml
 */
static LLVMValueRef
lp_build_blend_factor(struct lp_build_blend_context *bld,
                      LLVMValueRef factor1,
                      unsigned rgb_factor,
                      unsigned alpha_factor,
                      unsigned alpha_swizzle)
{
   LLVMValueRef rgb_factor_;
   LLVMValueRef alpha_factor_;
   LLVMValueRef factor2;
   enum lp_build_blend_swizzle rgb_swizzle;

   rgb_factor_   = lp_build_blend_factor_unswizzled(bld, rgb_factor,   FALSE);
   alpha_factor_ = lp_build_blend_factor_unswizzled(bld, alpha_factor, TRUE);

   rgb_swizzle = lp_build_blend_factor_swizzle(rgb_factor);

   factor2 = lp_build_blend_swizzle(bld, rgb_factor_, alpha_factor_, rgb_swizzle, alpha_swizzle);

   return lp_build_mul(&bld->base, factor1, factor2);
}


/**
 * @sa http://www.opengl.org/sdk/docs/man/xhtml/glBlendEquationSeparate.xml
 */
static LLVMValueRef
lp_build_blend_func(struct lp_build_blend_context *bld,
                    unsigned func,
                    LLVMValueRef term1, 
                    LLVMValueRef term2)
{
   switch (func) {
   case PIPE_BLEND_ADD:
      return lp_build_add(&bld->base, term1, term2);
      break;
   case PIPE_BLEND_SUBTRACT:
      return lp_build_sub(&bld->base, term1, term2);
   case PIPE_BLEND_REVERSE_SUBTRACT:
      return lp_build_sub(&bld->base, term2, term1);
   case PIPE_BLEND_MIN:
      return lp_build_min(&bld->base, term1, term2);
   case PIPE_BLEND_MAX:
      return lp_build_max(&bld->base, term1, term2);
   default:
      assert(0);
      return bld->base.zero;
   }
}


LLVMValueRef
lp_build_blend(LLVMBuilderRef builder,
               const struct pipe_blend_state *blend,
               union lp_type type,
               LLVMValueRef src,
               LLVMValueRef dst,
               LLVMValueRef const_,
               unsigned alpha_swizzle)
{
   struct lp_build_blend_context bld;
   LLVMValueRef src_term;
   LLVMValueRef dst_term;

   /* It makes no sense to blend unless values are normalized */
   assert(type.norm);

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   bld.base.builder = builder;
   bld.base.type = type;
   bld.base.undef = lp_build_undef(type);
   bld.base.zero = lp_build_zero(type);
   bld.base.one = lp_build_one(type);
   bld.src = src;
   bld.dst = dst;
   bld.const_ = const_;

   /* TODO: There are still a few optimization oportunities here. For certain
    * combinations it is possible to reorder the operations and therefor saving
    * some instructions. */

   src_term = lp_build_blend_factor(&bld, src, blend->rgb_src_factor, blend->alpha_src_factor, alpha_swizzle);
   dst_term = lp_build_blend_factor(&bld, dst, blend->rgb_dst_factor, blend->alpha_dst_factor, alpha_swizzle);

#ifdef DEBUG
   LLVMSetValueName(src_term, "src_term");
   LLVMSetValueName(dst_term, "dst_term");
#endif

   if(blend->rgb_func == blend->alpha_func) {
      return lp_build_blend_func(&bld, blend->rgb_func, src_term, dst_term);
   }
   else {
      /* Seperate RGB / A functions */

      LLVMValueRef rgb;
      LLVMValueRef alpha;

      rgb   = lp_build_blend_func(&bld, blend->rgb_func,   src_term, dst_term);
      alpha = lp_build_blend_func(&bld, blend->alpha_func, src_term, dst_term);

      return lp_build_blend_swizzle(&bld, rgb, alpha, LP_BUILD_BLEND_SWIZZLE_RGBA, alpha_swizzle);
   }
}
