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


#include "util/u_format.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_format.h"


/**
 * Gather elements from scatter positions in memory into a single vector.
 *
 * @param src_width src element width
 * @param dst_width result element width (source will be expanded to fit)
 * @param length length of the offsets,
 * @param base_ptr base pointer, should be a i8 pointer type.
 * @param offsets vector with offsets
 */
LLVMValueRef
lp_build_gather(LLVMBuilderRef builder,
                unsigned length,
                unsigned src_width,
                unsigned dst_width,
                LLVMValueRef base_ptr,
                LLVMValueRef offsets)
{
   LLVMTypeRef src_type = LLVMIntType(src_width);
   LLVMTypeRef src_ptr_type = LLVMPointerType(src_type, 0);
   LLVMTypeRef dst_elem_type = LLVMIntType(dst_width);
   LLVMTypeRef dst_vec_type = LLVMVectorType(dst_elem_type, length);
   LLVMValueRef res;
   unsigned i;

   res = LLVMGetUndef(dst_vec_type);
   for(i = 0; i < length; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef elem_offset;
      LLVMValueRef elem_ptr;
      LLVMValueRef elem;

      elem_offset = LLVMBuildExtractElement(builder, offsets, index, "");
      elem_ptr = LLVMBuildGEP(builder, base_ptr, &elem_offset, 1, "");
      elem_ptr = LLVMBuildBitCast(builder, elem_ptr, src_ptr_type, "");
      elem = LLVMBuildLoad(builder, elem_ptr, "");

      assert(src_width <= dst_width);
      if(src_width > dst_width)
         elem = LLVMBuildTrunc(builder, elem, dst_elem_type, "");
      if(src_width < dst_width)
         elem = LLVMBuildZExt(builder, elem, dst_elem_type, "");

      res = LLVMBuildInsertElement(builder, res, elem, index, "");
   }

   return res;
}


static LLVMValueRef
lp_build_format_swizzle(union lp_type type,
                        const LLVMValueRef *inputs,
                        enum util_format_swizzle swizzle)
{
   switch (swizzle) {
   case UTIL_FORMAT_SWIZZLE_X:
   case UTIL_FORMAT_SWIZZLE_Y:
   case UTIL_FORMAT_SWIZZLE_Z:
   case UTIL_FORMAT_SWIZZLE_W:
      return inputs[swizzle];
   case UTIL_FORMAT_SWIZZLE_0:
      return lp_build_zero(type);
   case UTIL_FORMAT_SWIZZLE_1:
      return lp_build_one(type);
   case UTIL_FORMAT_SWIZZLE_NONE:
      return lp_build_undef(type);
   default:
      assert(0);
      return lp_build_undef(type);
   }
}


void
lp_build_unpack_rgba_soa(LLVMBuilderRef builder,
                         const struct util_format_description *format_desc,
                         union lp_type type,
                         LLVMValueRef packed,
                         LLVMValueRef *rgba)
{
   LLVMValueRef inputs[4];
   unsigned start;
   unsigned chan;

   /* FIXME: Support more formats */
   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_ARITH);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);
   assert(format_desc->block.bits <= 32);

   /* Decode the input vector components */
   start = 0;
   for (chan = 0; chan < 4; ++chan) {
      unsigned width = format_desc->channel[chan].size;
      unsigned stop = start + width;
      LLVMValueRef input;

      input = packed;

      switch(format_desc->channel[chan].type) {
      case UTIL_FORMAT_TYPE_VOID:
         input = NULL;
         break;

      case UTIL_FORMAT_TYPE_UNSIGNED:
         if(type.floating) {
            if(start)
               input = LLVMBuildLShr(builder, input, lp_build_int_const_scalar(type, start), "");
            if(stop < format_desc->block.bits) {
               unsigned mask = ((unsigned long long)1 << width) - 1;
               input = LLVMBuildAnd(builder, input, lp_build_int_const_scalar(type, mask), "");
            }

            if(format_desc->channel[chan].normalized)
               input = lp_build_unsigned_norm_to_float(builder, width, type, input);
            else
               input = LLVMBuildFPToSI(builder, input, lp_build_vec_type(type), "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(type);
         }
         break;

      default:
         /* fall through */
         input = lp_build_undef(type);
         break;
      }

      inputs[chan] = input;

      start = stop;
   }

   if(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      enum util_format_swizzle swizzle = format_desc->swizzle[0];
      LLVMValueRef depth = lp_build_format_swizzle(type, inputs, swizzle);
      rgba[2] = rgba[1] = rgba[0] = depth;
      rgba[3] = lp_build_one(type);
   }
   else {
      for (chan = 0; chan < 4; ++chan) {
         enum util_format_swizzle swizzle = format_desc->swizzle[chan];
         rgba[chan] = lp_build_format_swizzle(type, inputs, swizzle);
      }
   }
}


void
lp_build_load_rgba_soa(LLVMBuilderRef builder,
                       const struct util_format_description *format_desc,
                       union lp_type type,
                       LLVMValueRef base_ptr,
                       LLVMValueRef offsets,
                       LLVMValueRef *rgba)
{
   LLVMValueRef packed;

   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_ARITH);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);
   assert(format_desc->block.bits <= 32);

   packed = lp_build_gather(builder,
                            type.length, format_desc->block.bits, type.width,
                            base_ptr, offsets);

   lp_build_unpack_rgba_soa(builder, format_desc, type, packed, rgba);
}
