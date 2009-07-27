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


void
lp_build_pack_rgba(LLVMBuilderRef builder,
                   enum pipe_format format,
                   LLVMValueRef ptr,
                   LLVMValueRef rgba)
{
   const struct util_format_description *desc;
   LLVMTypeRef type;
   LLVMValueRef packed = NULL;
   unsigned shift = 0;
   unsigned i, j;

   desc = util_format_description(format);

   assert(desc->layout == UTIL_FORMAT_LAYOUT_RGBA);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);

   type = LLVMIntType(desc->block.bits);

   LLVMValueRef swizzles[4];
   LLVMValueRef shifted, casted, scaled, unswizzled;


   /* Unswizzle the color components into the source vector. */
   for (i = 0; i < 4; ++i) {
      for (j = 0; j < 4; ++j) {
         if (desc->swizzle[j] == i)
            break;
      }
      if (j < 4)
         swizzles[i] = LLVMConstInt(LLVMInt32Type(), j, 0);
      else
         swizzles[i] = LLVMGetUndef(LLVMInt32Type());
   }

   unswizzled = LLVMBuildShuffleVector(builder, rgba,
                                       LLVMGetUndef(LLVMVectorType(LLVMFloatType(), 4)),
                                       LLVMConstVector(swizzles, 4), "");

   LLVMValueRef shifts[4];
   LLVMValueRef scales[4];
   bool normalized = FALSE;

   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;

      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
         shifts[i] = LLVMGetUndef(LLVMInt32Type());
         scales[i] =  LLVMGetUndef(LLVMFloatType());
      }
      else {
         unsigned mask = (1 << bits) - 1;

         assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED);
         assert(bits < 32);

         shifts[i] = LLVMConstInt(LLVMInt32Type(), shift, 0);

         if (desc->channel[i].normalized) {
            scales[i] = LLVMConstReal(LLVMFloatType(), mask);
            normalized = TRUE;
         }
         else
            scales[i] =  LLVMConstReal(LLVMFloatType(), 1.0);
      }

      shift += bits;
   }

   if (normalized)
      scaled = LLVMBuildMul(builder, unswizzled, LLVMConstVector(scales, 4), "");
   else
      scaled = unswizzled;

   casted = LLVMBuildFPToSI(builder, scaled, LLVMVectorType(LLVMInt32Type(), 4), "");

   shifted = LLVMBuildShl(builder, casted, LLVMConstVector(shifts, 4), "");
   
   /* Bitwise or all components */
   for (i = 0; i < 4; ++i) {
      if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
         LLVMValueRef component = LLVMBuildExtractElement(builder, shifted, LLVMConstInt(LLVMInt32Type(), i, 0), "");
         if (packed)
            packed = LLVMBuildOr(builder, packed, component, "");
         else
            packed = component;
      }
   }

   if (packed) {

      if (desc->block.bits < 32)
         packed = LLVMBuildTrunc(builder, packed, type, "");

      LLVMBuildStore(builder, packed, LLVMBuildBitCast(builder, ptr, LLVMPointerType(type, 0), ""));
   }
}

