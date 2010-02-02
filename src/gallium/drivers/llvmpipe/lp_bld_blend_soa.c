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
 * Blend LLVM IR generation -- SoA layout.
 *
 * Blending in SoA is much faster than AoS, especially when separate rgb/alpha
 * factors/functions are used, since no channel masking/shuffling is necessary
 * and we can achieve the full throughput of the SIMD operations. Furthermore
 * the fragment shader output is also in SoA, so it fits nicely with the rest of
 * the fragment pipeline.
 *
 * The drawback is that to be displayed the color buffer needs to be in AoS
 * layout, so we need to tile/untile the color buffer before/after rendering.
 * A color buffer like
 *
 *  R11 G11 B11 A11 R12 G12 B12 A12  R13 G13 B13 A13 R14 G14 B14 A14  ...
 *  R21 G21 B21 A21 R22 G22 B22 A22  R23 G23 B23 A23 R24 G24 B24 A24  ...
 *
 *  R31 G31 B31 A31 R32 G32 B32 A32  R33 G33 B33 A33 R34 G34 B34 A34  ...
 *  R41 G41 B41 A41 R42 G42 B42 A42  R43 G43 B43 A43 R44 G44 B44 A44  ...
 *
 *  ... ... ... ... ... ... ... ...  ... ... ... ... ... ... ... ...  ...
 *
 * will actually be stored in memory as
 *
 *  R11 R12 R21 R22 R13 R14 R23 R24 ... G11 G12 G21 G22 G13 G14 G23 G24 ... B11 B12 B21 B22 B13 B14 B23 B24 ... A11 A12 A21 A22 A13 A14 A23 A24 ...
 *  R31 R32 R41 R42 R33 R34 R43 R44 ... G31 G32 G41 G42 G33 G34 G43 G44 ... B31 B32 B41 B42 B33 B34 B43 B44 ... A31 A32 A41 A42 A33 A34 A43 A44 ...
 *  ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ...
 *
 * NOTE: Run lp_blend_test after any change to this file.
 *
 * You can also run lp_blend_test to obtain AoS vs SoA benchmarks. Invoking it
 * as:
 *
 *  lp_blend_test -o blend.tsv
 *
 * will generate a tab-seperated-file with the test results and performance
 * measurements.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "pipe/p_state.h"
#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_arit.h"
#include "lp_bld_blend.h"


/**
 * We may the same values several times, so we keep them here to avoid
 * recomputing them. Also reusing the values allows us to do simplifications
 * that LLVM optimization passes wouldn't normally be able to do.
 */
struct lp_build_blend_soa_context
{
   struct lp_build_context base;

   LLVMValueRef src[4];
   LLVMValueRef dst[4];
   LLVMValueRef con[4];

   LLVMValueRef inv_src[4];
   LLVMValueRef inv_dst[4];
   LLVMValueRef inv_con[4];

   LLVMValueRef src_alpha_saturate;

   /**
    * We store all factors in a table in order to eliminate redundant
    * multiplications later.
    */
   LLVMValueRef factor[2][2][4];

   /**
    * Table with all terms.
    */
   LLVMValueRef term[2][4];
};


static LLVMValueRef
lp_build_blend_soa_factor(struct lp_build_blend_soa_context *bld,
                          unsigned factor, unsigned i)
{
   /*
    * Compute src/first term RGB
    */
   switch (factor) {
   case PIPE_BLENDFACTOR_ONE:
      return bld->base.one;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      return bld->src[i];
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      return bld->src[3];
   case PIPE_BLENDFACTOR_DST_COLOR:
      return bld->dst[i];
   case PIPE_BLENDFACTOR_DST_ALPHA:
      return bld->dst[3];
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      if(i == 3)
         return bld->base.one;
      else {
         if(!bld->inv_dst[3])
            bld->inv_dst[3] = lp_build_comp(&bld->base, bld->dst[3]);
         if(!bld->src_alpha_saturate)
            bld->src_alpha_saturate = lp_build_min(&bld->base, bld->src[3], bld->inv_dst[3]);
         return bld->src_alpha_saturate;
      }
   case PIPE_BLENDFACTOR_CONST_COLOR:
      return bld->con[i];
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      return bld->con[3];
   case PIPE_BLENDFACTOR_SRC1_COLOR:
      /* TODO */
      assert(0);
      return bld->base.zero;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      /* TODO */
      assert(0);
      return bld->base.zero;
   case PIPE_BLENDFACTOR_ZERO:
      return bld->base.zero;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      if(!bld->inv_src[i])
         bld->inv_src[i] = lp_build_comp(&bld->base, bld->src[i]);
      return bld->inv_src[i];
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      if(!bld->inv_src[3])
         bld->inv_src[3] = lp_build_comp(&bld->base, bld->src[3]);
      return bld->inv_src[3];
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      if(!bld->inv_dst[i])
         bld->inv_dst[i] = lp_build_comp(&bld->base, bld->dst[i]);
      return bld->inv_dst[i];
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      if(!bld->inv_dst[3])
         bld->inv_dst[3] = lp_build_comp(&bld->base, bld->dst[3]);
      return bld->inv_dst[3];
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      if(!bld->inv_con[i])
         bld->inv_con[i] = lp_build_comp(&bld->base, bld->con[i]);
      return bld->inv_con[i];
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      if(!bld->inv_con[3])
         bld->inv_con[3] = lp_build_comp(&bld->base, bld->con[3]);
      return bld->inv_con[3];
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      /* TODO */
      assert(0);
      return bld->base.zero;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      /* TODO */
      assert(0);
      return bld->base.zero;
   default:
      assert(0);
      return bld->base.zero;
   }
}


/**
 * Generate blend code in SOA mode.
 * \param src  src/fragment color
 * \param dst  dst/framebuffer color
 * \param con  constant blend color
 * \param res  the result/output
 */
void
lp_build_blend_soa(LLVMBuilderRef builder,
                   const struct pipe_blend_state *blend,
                   struct lp_type type,
                   LLVMValueRef src[4],
                   LLVMValueRef dst[4],
                   LLVMValueRef con[4],
                   LLVMValueRef res[4])
{
   struct lp_build_blend_soa_context bld;
   unsigned i, j, k;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.base, builder, type);
   for (i = 0; i < 4; ++i) {
      bld.src[i] = src[i];
      bld.dst[i] = dst[i];
      bld.con[i] = con[i];
   }

   for (i = 0; i < 4; ++i) {
      if (blend->rt[0].colormask & (1 << i)) {
         if (blend->logicop_enable) {
            if(!type.floating) {
               res[i] = lp_build_logicop(builder, blend->logicop_func, src[i], dst[i]);
            }
            else
               res[i] = dst[i];
         }
         else if (blend->rt[0].blend_enable) {
            unsigned src_factor = i < 3 ? blend->rt[0].rgb_src_factor : blend->rt[0].alpha_src_factor;
            unsigned dst_factor = i < 3 ? blend->rt[0].rgb_dst_factor : blend->rt[0].alpha_dst_factor;
            unsigned func = i < 3 ? blend->rt[0].rgb_func : blend->rt[0].alpha_func;
            boolean func_commutative = lp_build_blend_func_commutative(func);

            /* It makes no sense to blend unless values are normalized */
            assert(type.norm);

            /*
             * Compute src/dst factors.
             */

            bld.factor[0][0][i] = src[i];
            bld.factor[0][1][i] = lp_build_blend_soa_factor(&bld, src_factor, i);
            bld.factor[1][0][i] = dst[i];
            bld.factor[1][1][i] = lp_build_blend_soa_factor(&bld, dst_factor, i);

            /*
             * Compute src/dst terms
             */

            for(k = 0; k < 2; ++k) {
               /* See if this multiplication has been previously computed */
               for(j = 0; j < i; ++j) {
                  if((bld.factor[k][0][j] == bld.factor[k][0][i] &&
                      bld.factor[k][1][j] == bld.factor[k][1][i]) ||
                     (bld.factor[k][0][j] == bld.factor[k][1][i] &&
                      bld.factor[k][1][j] == bld.factor[k][0][i]))
                     break;
               }

               if(j < i)
                  bld.term[k][i] = bld.term[k][j];
               else
                  bld.term[k][i] = lp_build_mul(&bld.base, bld.factor[k][0][i], bld.factor[k][1][i]);
            }

            /*
             * Combine terms
             */

            /* See if this function has been previously applied */
            for(j = 0; j < i; ++j) {
               unsigned prev_func = j < 3 ? blend->rt[0].rgb_func : blend->rt[0].alpha_func;
               unsigned func_reverse = lp_build_blend_func_reverse(func, prev_func);

               if((!func_reverse &&
                   bld.term[0][j] == bld.term[0][i] &&
                   bld.term[1][j] == bld.term[1][i]) ||
                  ((func_commutative || func_reverse) &&
                   bld.term[0][j] == bld.term[1][i] &&
                   bld.term[1][j] == bld.term[0][i]))
                  break;
            }

            if(j < i)
               res[i] = res[j];
            else
               res[i] = lp_build_blend_func(&bld.base, func, bld.term[0][i], bld.term[1][i]);
         }
         else {
            res[i] = src[i];
         }
      }
      else {
         res[i] = dst[i];
      }
   }
}
