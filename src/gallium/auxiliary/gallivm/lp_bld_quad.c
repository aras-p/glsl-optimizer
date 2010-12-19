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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#include "lp_bld_type.h"
#include "lp_bld_arit.h"
#include "lp_bld_const.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_quad.h"


static const unsigned char
swizzle_left[4] = {
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_LEFT,
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_LEFT
};

static const unsigned char
swizzle_right[4] = {
   LP_BLD_QUAD_TOP_RIGHT,    LP_BLD_QUAD_TOP_RIGHT,
   LP_BLD_QUAD_BOTTOM_RIGHT, LP_BLD_QUAD_BOTTOM_RIGHT
};

static const unsigned char
swizzle_top[4] = {
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_RIGHT,
   LP_BLD_QUAD_TOP_LEFT,     LP_BLD_QUAD_TOP_RIGHT
};

static const unsigned char
swizzle_bottom[4] = {
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_RIGHT,
   LP_BLD_QUAD_BOTTOM_LEFT,  LP_BLD_QUAD_BOTTOM_RIGHT
};


LLVMValueRef
lp_build_ddx(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMValueRef a_left  = lp_build_swizzle_aos(bld, a, swizzle_left);
   LLVMValueRef a_right = lp_build_swizzle_aos(bld, a, swizzle_right);
   return lp_build_sub(bld, a_right, a_left);
}


LLVMValueRef
lp_build_ddy(struct lp_build_context *bld,
             LLVMValueRef a)
{
   LLVMValueRef a_top    = lp_build_swizzle_aos(bld, a, swizzle_top);
   LLVMValueRef a_bottom = lp_build_swizzle_aos(bld, a, swizzle_bottom);
   return lp_build_sub(bld, a_bottom, a_top);
}


LLVMValueRef
lp_build_scalar_ddx(struct lp_build_context *bld,
                    LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef idx_left  = lp_build_const_int32(bld->gallivm, LP_BLD_QUAD_TOP_LEFT);
   LLVMValueRef idx_right = lp_build_const_int32(bld->gallivm, LP_BLD_QUAD_TOP_RIGHT);
   LLVMValueRef a_left  = LLVMBuildExtractElement(builder, a, idx_left, "left");
   LLVMValueRef a_right = LLVMBuildExtractElement(builder, a, idx_right, "right");
   if (bld->type.floating)
      return LLVMBuildFSub(builder, a_right, a_left, "ddx");
   else
      return LLVMBuildSub(builder, a_right, a_left, "ddx");
}


LLVMValueRef
lp_build_scalar_ddy(struct lp_build_context *bld,
                    LLVMValueRef a)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef idx_top    = lp_build_const_int32(bld->gallivm, LP_BLD_QUAD_TOP_LEFT);
   LLVMValueRef idx_bottom = lp_build_const_int32(bld->gallivm, LP_BLD_QUAD_BOTTOM_LEFT);
   LLVMValueRef a_top    = LLVMBuildExtractElement(builder, a, idx_top, "top");
   LLVMValueRef a_bottom = LLVMBuildExtractElement(builder, a, idx_bottom, "bottom");
   if (bld->type.floating)
      return LLVMBuildFSub(builder, a_bottom, a_top, "ddy");
   else
      return LLVMBuildSub(builder, a_bottom, a_top, "ddy");
}
