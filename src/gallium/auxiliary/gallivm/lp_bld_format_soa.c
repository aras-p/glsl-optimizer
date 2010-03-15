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


static LLVMValueRef
lp_build_format_swizzle_chan_soa(struct lp_type type,
                                 const LLVMValueRef *unswizzled,
                                 enum util_format_swizzle swizzle)
{
   switch (swizzle) {
   case UTIL_FORMAT_SWIZZLE_X:
   case UTIL_FORMAT_SWIZZLE_Y:
   case UTIL_FORMAT_SWIZZLE_Z:
   case UTIL_FORMAT_SWIZZLE_W:
      return unswizzled[swizzle];
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
lp_build_format_swizzle_soa(const struct util_format_description *format_desc,
                            struct lp_type type,
                            const LLVMValueRef *unswizzled,
                            LLVMValueRef *swizzled)
{
   if(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      enum util_format_swizzle swizzle = format_desc->swizzle[0];
      LLVMValueRef depth = lp_build_format_swizzle_chan_soa(type, unswizzled, swizzle);
      swizzled[2] = swizzled[1] = swizzled[0] = depth;
      swizzled[3] = lp_build_one(type);
   }
   else {
      unsigned chan;
      for (chan = 0; chan < 4; ++chan) {
         enum util_format_swizzle swizzle = format_desc->swizzle[chan];
         swizzled[chan] = lp_build_format_swizzle_chan_soa(type, unswizzled, swizzle);
      }
   }
}


void
lp_build_unpack_rgba_soa(LLVMBuilderRef builder,
                         const struct util_format_description *format_desc,
                         struct lp_type type,
                         LLVMValueRef packed,
                         LLVMValueRef *rgba)
{
   LLVMValueRef inputs[4];
   unsigned start;
   unsigned chan;

   /* FIXME: Support more formats */
   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
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

   lp_build_format_swizzle_soa(format_desc, type, inputs, rgba);
}
