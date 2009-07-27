/**************************************************************************
 *
 * Copyright 2009 Vmware, Inc.
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


#include "util/u_format.h"

#include "lp_bld.h"


LLVMValueRef
lp_build_unpack_rgba(LLVMBuilderRef builder,
                     enum pipe_format format,
                     LLVMValueRef ptr)
{
   const struct util_format_description *desc;
   LLVMTypeRef type;
   LLVMValueRef deferred;
   unsigned shift = 0;
   unsigned i;

   desc = util_format_description(format);

   /* FIXME: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_RGBA);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

   type = LLVMIntType(desc->block.bits);

   deferred = LLVMBuildLoad(builder, LLVMBuildBitCast(builder, ptr, LLVMPointerType(type, 0), ""), "");

   /* Do the intermediate integer computations with 32bit integers since it
    * matches floating point size */
   if (desc->block.bits < 32)
      deferred = LLVMBuildZExt(builder, deferred, LLVMInt32Type(), "");

   /* Broadcast the packed value to all four channels */
   deferred = LLVMBuildInsertElement(builder,
                                     LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                     deferred,
                                     LLVMConstNull(LLVMInt32Type()),
                                     "");
   deferred = LLVMBuildShuffleVector(builder,
                                     deferred,
                                     LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                     LLVMConstNull(LLVMVectorType(LLVMInt32Type(), 4)),
                                     "");

   LLVMValueRef shifted, casted, scaled, masked, swizzled;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];
   bool normalized = FALSE;
   int empty_channel = -1;

   /* Initialize vector constants */
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;

      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
         shifts[i] = LLVMGetUndef(LLVMInt32Type());
         masks[i] = LLVMConstNull(LLVMInt32Type());
         scales[i] =  LLVMConstNull(LLVMFloatType());
         empty_channel = i;
      }
      else {
         unsigned mask = (1 << bits) - 1;

         assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED);
         assert(bits < 32);

         shifts[i] = LLVMConstInt(LLVMInt32Type(), shift, 0);
         masks[i] = LLVMConstInt(LLVMInt32Type(), mask, 0);

         if (desc->channel[i].normalized) {
            scales[i] = LLVMConstReal(LLVMFloatType(), 1.0/mask);
            normalized = TRUE;
         }
         else
            scales[i] =  LLVMConstReal(LLVMFloatType(), 1.0);
      }

      shift += bits;
   }

   shifted = LLVMBuildLShr(builder, deferred, LLVMConstVector(shifts, 4), "");
   masked = LLVMBuildAnd(builder, shifted, LLVMConstVector(masks, 4), "");
   // UIToFP can't be expressed in SSE2
   casted = LLVMBuildSIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");

   if (normalized)
      scaled = LLVMBuildMul(builder, casted, LLVMConstVector(scales, 4), "");
   else
      scaled = casted;

   LLVMValueRef swizzles[4];
   LLVMValueRef aux[4];

   for (i = 0; i < 4; ++i)
      aux[i] = LLVMGetUndef(LLVMFloatType());

   for (i = 0; i < 4; ++i) {
      enum util_format_swizzle swizzle = desc->swizzle[i];

      switch (swizzle) {
      case UTIL_FORMAT_SWIZZLE_X:
      case UTIL_FORMAT_SWIZZLE_Y:
      case UTIL_FORMAT_SWIZZLE_Z:
      case UTIL_FORMAT_SWIZZLE_W:
         swizzles[i] = LLVMConstInt(LLVMInt32Type(), swizzle, 0);
         break;
      case UTIL_FORMAT_SWIZZLE_0:
         assert(empty_channel >= 0);
         swizzles[i] = LLVMConstInt(LLVMInt32Type(), empty_channel, 0);
         break;
      case UTIL_FORMAT_SWIZZLE_1:
         swizzles[i] = LLVMConstInt(LLVMInt32Type(), 4, 0);
         aux[0] = LLVMConstReal(LLVMFloatType(), 1.0);
         break;
      case UTIL_FORMAT_SWIZZLE_NONE:
         swizzles[i] = LLVMGetUndef(LLVMFloatType());
         assert(0);
         break;
      }
   }

   swizzled = LLVMBuildShuffleVector(builder, scaled, LLVMConstVector(aux, 4), LLVMConstVector(swizzles, 4), "");

   return swizzled;
}

