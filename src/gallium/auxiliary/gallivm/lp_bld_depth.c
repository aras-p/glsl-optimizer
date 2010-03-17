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
 * Depth/stencil testing to LLVM IR translation.
 *
 * To be done accurately/efficiently the depth/stencil test must be done with
 * the same type/format of the depth/stencil buffer, which implies massaging
 * the incoming depths to fit into place. Using a more straightforward
 * type/format for depth/stencil values internally and only convert when
 * flushing would avoid this, but it would most likely result in depth fighting
 * artifacts.
 *
 * We are free to use a different pixel layout though. Since our basic
 * processing unit is a quad (2x2 pixel block) we store the depth/stencil
 * values tiled, a quad at time. That is, a depth buffer containing 
 *
 *  Z11 Z12 Z13 Z14 ...
 *  Z21 Z22 Z23 Z24 ...
 *  Z31 Z32 Z33 Z34 ...
 *  Z41 Z42 Z43 Z44 ...
 *  ... ... ... ... ...
 *
 * will actually be stored in memory as
 *
 *  Z11 Z12 Z21 Z22 Z13 Z14 Z23 Z24 ...
 *  Z31 Z32 Z41 Z42 Z33 Z34 Z43 Z44 ...
 *  ... ... ... ... ... ... ... ... ...
 *
 * FIXME: Code generate stencil test
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_state.h"
#include "util/u_format.h"

#include "lp_bld_type.h"
#include "lp_bld_arit.h"
#include "lp_bld_const.h"
#include "lp_bld_logic.h"
#include "lp_bld_flow.h"
#include "lp_bld_debug.h"
#include "lp_bld_depth.h"
#include "lp_bld_swizzle.h"



/**
 * Do the stencil test comparison (compare fb Z values against ref value.
 * \param stencilVals  vector of stencil values from framebuffer
 * \param stencilRef  the stencil reference value, replicated as a vector
 * \return mask of pass/fail values
 */
static LLVMValueRef
lp_build_stencil_test(struct lp_build_context *bld,
                      const struct pipe_stencil_state *stencil,
                      LLVMValueRef stencilVals,
                      LLVMValueRef stencilRef)
{
   const unsigned stencilMax = 255; /* XXX fix */
   struct lp_type type = bld->type;
   LLVMValueRef res;

   assert(stencil->enabled);

   if (stencil->valuemask != stencilMax) {
      /* compute stencilRef = stencilRef & valuemask */
      LLVMValueRef valuemask = lp_build_const_int_vec(type, stencil->valuemask);
      stencilRef = LLVMBuildAnd(bld->builder, stencilRef, valuemask, "");
      /* compute stencilVals = stencilVals & valuemask */
      stencilVals = LLVMBuildAnd(bld->builder, stencilVals, valuemask, "");
   }

   res = lp_build_compare(bld->builder, bld->type, stencil->func,
                          stencilVals, stencilRef);

   return res;
}


/**
 * Apply the stencil operator (add/sub/keep/etc) to the given vector
 * of stencil values.
 * \return  new stencil values vector
 */
static LLVMValueRef
lp_build_stencil_op(struct lp_build_context *bld,
                    unsigned stencil_op,
                    LLVMValueRef stencilRef,
                    const struct pipe_stencil_state *stencil,
                    LLVMValueRef stencilVals)

{
   const unsigned stencilMax = 255; /* XXX fix */
   struct lp_type type = bld->type;
   LLVMValueRef res;
   LLVMValueRef max = lp_build_const_int_vec(type, stencilMax);

   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:
      res = stencilVals;
   case PIPE_STENCIL_OP_ZERO:
      res = bld->zero;
   case PIPE_STENCIL_OP_REPLACE:
      res = lp_build_broadcast_scalar(bld, stencilRef);
   case PIPE_STENCIL_OP_INCR:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = lp_build_min(bld, res, max);
   case PIPE_STENCIL_OP_DECR:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = lp_build_max(bld, res, bld->zero);
   case PIPE_STENCIL_OP_INCR_WRAP:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(bld->builder, res, max, "");
   case PIPE_STENCIL_OP_DECR_WRAP:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(bld->builder, res, max, "");
   case PIPE_STENCIL_OP_INVERT:
      res = LLVMBuildNot(bld->builder, stencilVals, "");
   default:
      assert(0 && "bad stencil op mode");
      res = NULL;
   }

   if (stencil->writemask != stencilMax) {
      /* compute res = (res & mask) | (stencilVals & ~mask) */
      LLVMValueRef mask = lp_build_const_int_vec(type, stencil->writemask);
      LLVMValueRef cmask = LLVMBuildNot(bld->builder, mask, "notWritemask");
      LLVMValueRef t1 = LLVMBuildAnd(bld->builder, res, mask, "t1");
      LLVMValueRef t2 = LLVMBuildAnd(bld->builder, stencilVals, cmask, "t2");
      res = LLVMBuildOr(bld->builder, t1, t2, "t1_or_t2");
   }

   return res;
}


/**
 * Return a type appropriate for depth/stencil testing.
 */
struct lp_type
lp_depth_type(const struct util_format_description *format_desc,
              unsigned length)
{
   struct lp_type type;
   unsigned swizzle;

   assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);

   swizzle = format_desc->swizzle[0];
   assert(swizzle < 4);

   memset(&type, 0, sizeof type);
   type.width = format_desc->block.bits;

   if(format_desc->channel[swizzle].type == UTIL_FORMAT_TYPE_FLOAT) {
      type.floating = TRUE;
      assert(swizzle == 0);
      assert(format_desc->channel[swizzle].size == format_desc->block.bits);
   }
   else if(format_desc->channel[swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED) {
      assert(format_desc->block.bits <= 32);
      if(format_desc->channel[swizzle].normalized)
         type.norm = TRUE;
   }
   else
      assert(0);

   assert(type.width <= length);
   type.length = length / type.width;

   return type;
}


/**
 * Generate code for performing depth and/or stencil tests.
 * We operate on a vector of values (typically a 2x2 quad).
 *
 * \param type  the data type of the fragment depth/stencil values
 * \param format_desc  description of the depth/stencil surface
 * \param mask  the alive/dead pixel mask for the quad
 * \param src  the incoming depth/stencil values (a 2x2 quad)
 * \param dst_ptr  the outgoing/updated depth/stencil values
 */
void
lp_build_depth_stencil_test(LLVMBuilderRef builder,
                            const struct pipe_depth_state *depth,
                            const struct pipe_stencil_state stencil[2],
                            struct lp_type type,
                            const struct util_format_description *format_desc,
                            struct lp_build_mask_context *mask,
                            LLVMValueRef stencil_refs,
                            LLVMValueRef z_src,
                            LLVMValueRef zs_dst_ptr)
{
   struct lp_build_context bld;
   unsigned z_swizzle, s_swizzle;
   LLVMValueRef zs_dst;
   LLVMValueRef z_bitmask = NULL;
   LLVMValueRef z_pass;

   (void) lp_build_stencil_test;
   (void) lp_build_stencil_op;

   assert(depth->enabled || stencil[0].enabled);

   assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);

   z_swizzle = format_desc->swizzle[0];
   s_swizzle = format_desc->swizzle[1];

   assert(z_swizzle != UTIL_FORMAT_SWIZZLE_NONE ||
          s_swizzle != UTIL_FORMAT_SWIZZLE_NONE);

   /* Sanity checking */
   assert(z_swizzle < 4);
   assert(format_desc->block.bits == type.width);
   if(type.floating) {
      assert(z_swizzle == 0);
      assert(format_desc->channel[z_swizzle].type == UTIL_FORMAT_TYPE_FLOAT);
      assert(format_desc->channel[z_swizzle].size == format_desc->block.bits);
   }
   else {
      assert(format_desc->channel[z_swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED);
      assert(format_desc->channel[z_swizzle].normalized);
      assert(!type.fixed);
      assert(!type.sign);
      assert(type.norm);
   }

   /* Setup build context */
   lp_build_context_init(&bld, builder, type);

   zs_dst = LLVMBuildLoad(builder, zs_dst_ptr, "");

   lp_build_name(zs_dst, "zsbuf");

   /* Align the source depth bits with the destination's, and mask out any
    * stencil or padding bits from both */
   if(format_desc->channel[z_swizzle].size == format_desc->block.bits) {
      assert(z_swizzle == 0);
      /* nothing to do */
   }
   else {
      /* shift/mask bits to right-justify the Z bits */
      unsigned padding_left;
      unsigned padding_right;
      unsigned chan;

      assert(format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
      assert(format_desc->channel[z_swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED);
      assert(format_desc->channel[z_swizzle].size <= format_desc->block.bits);
      assert(format_desc->channel[z_swizzle].normalized);

      padding_right = 0;
      for(chan = 0; chan < z_swizzle; ++chan)
         padding_right += format_desc->channel[chan].size;
      padding_left = format_desc->block.bits -
                     (padding_right + format_desc->channel[z_swizzle].size);

      if(padding_left || padding_right) {
         const unsigned long long mask_left = (1ULL << (format_desc->block.bits - padding_left)) - 1;
         const unsigned long long mask_right = (1ULL << (padding_right)) - 1;
         z_bitmask = lp_build_const_int_vec(type, mask_left ^ mask_right);
      }

      if(padding_left)
         z_src = LLVMBuildLShr(builder, z_src,
                                lp_build_const_int_vec(type, padding_left), "");
      if(padding_right)
         z_src = LLVMBuildAnd(builder, z_src, z_bitmask, "");
      if(padding_left || padding_right)
         zs_dst = LLVMBuildAnd(builder, zs_dst, z_bitmask, "");
   }

   lp_build_name(zs_dst, "zsbuf.z");

   /* compare src Z to dst Z, returning 'pass' mask */
   z_pass = lp_build_cmp(&bld, depth->func, z_src, zs_dst);
   lp_build_mask_update(mask, z_pass);

   if (depth->writemask) {
      if(z_bitmask)
         z_bitmask = LLVMBuildAnd(builder, mask->value, z_bitmask, "");
      else
         z_bitmask = mask->value;

      zs_dst = lp_build_select(&bld, z_bitmask, z_src, zs_dst);
      LLVMBuildStore(builder, zs_dst, zs_dst_ptr);
   }
}
