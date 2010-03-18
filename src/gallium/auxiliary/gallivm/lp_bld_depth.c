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
 *
 * Stencil test:
 * Two-sided stencil test is supported but probably not as efficient as
 * it could be.  Currently, we use if/then/else constructs to do the
 * operations for front vs. back-facing polygons.  We could probably do
 * both the front and back arithmetic then use a Select() instruction to
 * choose the result depending on polyon orientation.  We'd have to
 * measure performance both ways and see which is better.
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


/** Used to select fields from pipe_stencil_state */
enum stencil_op {
   S_FAIL_OP,
   Z_FAIL_OP,
   Z_PASS_OP
};



/**
 * Do the stencil test comparison (compare FB stencil values against ref value).
 * This will be used twice when generating two-sided stencil code.
 * \param stencil  the front/back stencil state
 * \param stencilRef  the stencil reference value, replicated as a vector
 * \param stencilVals  vector of stencil values from framebuffer
 * \return vector mask of pass/fail values (~0 or 0)
 */
static LLVMValueRef
lp_build_stencil_test_single(struct lp_build_context *bld,
                             const struct pipe_stencil_state *stencil,
                             LLVMValueRef stencilRef,
                             LLVMValueRef stencilVals)
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

   res = lp_build_cmp(bld, stencil->func, stencilVals, stencilRef);

   return res;
}


/**
 * Do the one or two-sided stencil test comparison.
 * \sa lp_build_stencil_test_single
 * \param face  an integer indicating front (+) or back (-) facing polygon.
 *              If NULL, assume front-facing.
 */
static LLVMValueRef
lp_build_stencil_test(struct lp_build_context *bld,
                      const struct pipe_stencil_state stencil[2],
                      LLVMValueRef stencilRefs[2],
                      LLVMValueRef stencilVals,
                      LLVMValueRef face)
{
   LLVMValueRef res;

   assert(stencil[0].enabled);

   if (stencil[1].enabled && face) {
      /* do two-sided test */
      struct lp_build_flow_context *flow_ctx;
      struct lp_build_if_state if_ctx;
      LLVMValueRef front_facing;
      LLVMValueRef zero = LLVMConstReal(LLVMFloatType(), 0.0);
      LLVMValueRef result = NULL;

      flow_ctx = lp_build_flow_create(bld->builder);
      lp_build_flow_scope_begin(flow_ctx);

      lp_build_flow_scope_declare(flow_ctx, &result);

      /* front_facing = face > 0.0 */
      front_facing = lp_build_cmp(bld, PIPE_FUNC_GREATER, face, zero);

      lp_build_if(&if_ctx, flow_ctx, bld->builder, front_facing);
      {
         result = lp_build_stencil_test_single(bld, &stencil[0],
                                               stencilRefs[0], stencilVals);
      }
      lp_build_else(&if_ctx);
      {
         result = lp_build_stencil_test_single(bld, &stencil[1],
                                               stencilRefs[1], stencilVals);
      }
      lp_build_endif(&if_ctx);

      lp_build_flow_scope_end(flow_ctx);
      lp_build_flow_destroy(flow_ctx);

      res = result;
   }
   else {
      /* do single-side test */
      res = lp_build_stencil_test_single(bld, &stencil[0],
                                         stencilRefs[0], stencilVals);
   }

   return res;
}


/**
 * Apply the stencil operator (add/sub/keep/etc) to the given vector
 * of stencil values.
 * \return  new stencil values vector
 */
static LLVMValueRef
lp_build_stencil_op_single(struct lp_build_context *bld,
                           const struct pipe_stencil_state *stencil,
                           enum stencil_op op,
                           LLVMValueRef stencilRef,
                           LLVMValueRef stencilVals,
                           LLVMValueRef mask)

{
   const unsigned stencilMax = 255; /* XXX fix */
   struct lp_type type = bld->type;
   LLVMValueRef res;
   LLVMValueRef max = lp_build_const_int_vec(type, stencilMax);
   unsigned stencil_op;

   switch (op) {
   case S_FAIL_OP:
      stencil_op = stencil->fail_op;
      break;
   case Z_FAIL_OP:
      stencil_op = stencil->zfail_op;
      break;
   case Z_PASS_OP:
      stencil_op = stencil->zpass_op;
      break;
   default:
      assert(0 && "Invalid stencil_op mode");
      stencil_op = PIPE_STENCIL_OP_KEEP;
   }

   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:
      res = stencilVals;
      /* we can return early for this case */
      return res;
   case PIPE_STENCIL_OP_ZERO:
      res = bld->zero;
      break;
   case PIPE_STENCIL_OP_REPLACE:
      res = stencilRef;
      break;
   case PIPE_STENCIL_OP_INCR:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = lp_build_min(bld, res, max);
      break;
   case PIPE_STENCIL_OP_DECR:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = lp_build_max(bld, res, bld->zero);
      break;
   case PIPE_STENCIL_OP_INCR_WRAP:
      res = lp_build_add(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(bld->builder, res, max, "");
      break;
   case PIPE_STENCIL_OP_DECR_WRAP:
      res = lp_build_sub(bld, stencilVals, bld->one);
      res = LLVMBuildAnd(bld->builder, res, max, "");
      break;
   case PIPE_STENCIL_OP_INVERT:
      res = LLVMBuildNot(bld->builder, stencilVals, "");
      break;
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

   /* only the update the vector elements enabled by 'mask' */
   res = lp_build_select(bld, mask, res, stencilVals);

   return res;
}


/**
 * Do the one or two-sided stencil test op/update.
 */
static LLVMValueRef
lp_build_stencil_op(struct lp_build_context *bld,
                    const struct pipe_stencil_state stencil[2],
                    enum stencil_op op,
                    LLVMValueRef stencilRefs[2],
                    LLVMValueRef stencilVals,
                    LLVMValueRef mask,
                    LLVMValueRef face)

{
   assert(stencil[0].enabled);

   if (stencil[1].enabled && face) {
      /* do two-sided op */
      struct lp_build_flow_context *flow_ctx;
      struct lp_build_if_state if_ctx;
      LLVMValueRef front_facing;
      LLVMValueRef zero = LLVMConstReal(LLVMFloatType(), 0.0);
      LLVMValueRef result = NULL;

      flow_ctx = lp_build_flow_create(bld->builder);
      lp_build_flow_scope_begin(flow_ctx);

      lp_build_flow_scope_declare(flow_ctx, &result);

      /* front_facing = face > 0.0 */
      front_facing = lp_build_cmp(bld, PIPE_FUNC_GREATER, face, zero);

      lp_build_if(&if_ctx, flow_ctx, bld->builder, front_facing);
      {
         result = lp_build_stencil_op_single(bld, &stencil[0], op,
                                             stencilRefs[0], stencilVals, mask);
      }
      lp_build_else(&if_ctx);
      {
         result = lp_build_stencil_op_single(bld, &stencil[1], op,
                                             stencilRefs[1], stencilVals, mask);
      }
      lp_build_endif(&if_ctx);

      lp_build_flow_scope_end(flow_ctx);
      lp_build_flow_destroy(flow_ctx);

      return result;
   }
   else {
      /* do single-sided op */
      return lp_build_stencil_op_single(bld, &stencil[0], op,
                                        stencilRefs[0], stencilVals, mask);
   }
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
                            LLVMValueRef stencil_refs[2],
                            LLVMValueRef z_src,
                            LLVMValueRef zs_dst_ptr)
{
   struct lp_build_context bld;
   unsigned z_swizzle, s_swizzle;
   LLVMValueRef zs_dst, z_dst = NULL;
   LLVMValueRef stencil_vals = NULL;
   LLVMValueRef z_bitmask = NULL, s_bitmask = NULL;
   LLVMValueRef z_pass = NULL, s_pass_mask = NULL;
   LLVMValueRef orig_mask = mask->value;
   LLVMValueRef face = NULL;

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

   /* Load current z/stencil value from z/stencil buffer */
   zs_dst = LLVMBuildLoad(builder, zs_dst_ptr, "");

   lp_build_name(zs_dst, "zsbufval");

   /* Align the source depth bits with the destination's, and mask out any
    * stencil or padding bits from both */
   if(format_desc->channel[z_swizzle].size == format_desc->block.bits) {
      assert(z_swizzle == 0);
      z_dst = zs_dst;
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

      s_bitmask = LLVMBuildNot(builder, z_bitmask, "");

      stencil_vals = LLVMBuildAnd(builder, zs_dst, s_bitmask, "");

      if(padding_left)
         z_src = LLVMBuildLShr(builder, z_src,
                                lp_build_const_int_vec(type, padding_left), "");
      if(padding_right)
         z_src = LLVMBuildAnd(builder, z_src, z_bitmask, "");
      if(padding_left || padding_right)
         z_dst = LLVMBuildAnd(builder, zs_dst, z_bitmask, "");
      else
         z_dst = zs_dst;
   }

   lp_build_name(z_dst, "zsbuf.z");

   /*
   printf("build depth %d stencil %d\n",
          depth->enabled,
          stencil[0].enabled);
   */

   if (stencil[0].enabled) {
      /* convert scalar stencil refs into vectors */
      stencil_refs[0] = lp_build_broadcast_scalar(&bld, stencil_refs[0]);
      stencil_refs[1] = lp_build_broadcast_scalar(&bld, stencil_refs[1]);


      s_pass_mask = lp_build_stencil_test(&bld, stencil,
                                          stencil_refs, stencil_vals, face);

      /* apply stencil-fail operator */
      {
         LLVMValueRef s_fail_mask = lp_build_andc(&bld, orig_mask, s_pass_mask);
         stencil_vals = lp_build_stencil_op(&bld, stencil, S_FAIL_OP,
                                            stencil_refs, stencil_vals,
                                            s_fail_mask, face);
      }
   }

   if (depth->enabled) {
      /* compare src Z to dst Z, returning 'pass' mask */
      z_pass = lp_build_cmp(&bld, depth->func, z_src, z_dst);

      if (!stencil[0].enabled) {
         /* We can potentially skip all remaining operations here, but only
          * if stencil is disabled because we still need to update the stencil
          * buffer values.  Don't need to update Z buffer values.
          */
         lp_build_mask_update(mask, z_pass);
      }

      if (depth->writemask) {
         if(z_bitmask)
            z_bitmask = LLVMBuildAnd(builder, mask->value, z_bitmask, "");
         else
            z_bitmask = mask->value;

         z_dst = lp_build_select(&bld, z_bitmask, z_src, z_dst);
      }

      if (stencil[0].enabled) {
         /* update stencil buffer values according to z pass/fail result */
         LLVMValueRef z_fail_mask, z_pass_mask;

         /* apply Z-fail operator */
         z_fail_mask = lp_build_andc(&bld, orig_mask, z_pass);
         stencil_vals = lp_build_stencil_op(&bld, stencil, Z_FAIL_OP,
                                            stencil_refs, stencil_vals,
                                            z_fail_mask, face);

         /* apply Z-pass operator */
         z_pass_mask = LLVMBuildAnd(bld.builder, orig_mask, z_pass, "");
         stencil_vals = lp_build_stencil_op(&bld, stencil, Z_PASS_OP,
                                            stencil_refs, stencil_vals,
                                            z_pass_mask, face);
      }
   }
   else {
      /* No depth test: apply Z-pass operator to stencil buffer values which
       * passed the stencil test.
       */
      s_pass_mask = LLVMBuildAnd(bld.builder, orig_mask, s_pass_mask, "");
      stencil_vals = lp_build_stencil_op(&bld, stencil, Z_PASS_OP, stencil_refs,
                                         stencil_vals, s_pass_mask, face);
   }

   /* Finally, merge/store the z/stencil values */
   if ((depth->enabled && depth->writemask) ||
       (stencil[0].enabled && stencil[0].writemask)) {

      if (z_dst && stencil_vals)
         zs_dst = LLVMBuildOr(bld.builder, z_dst, stencil_vals, "");
      else if (z_dst)
         zs_dst = z_dst;
      else
         zs_dst = stencil_vals;

      LLVMBuildStore(builder, zs_dst, zs_dst_ptr);
   }

   if (s_pass_mask)
      lp_build_mask_update(mask, s_pass_mask);

   if (depth->enabled && stencil[0].enabled)
      lp_build_mask_update(mask, z_pass);
}
