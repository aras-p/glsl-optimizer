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
#include "util/u_memory.h"
#include "util/u_string.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_sample.h" /* for lp_build_gather */
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


/**
 * Unpack several pixels in SoA.
 *
 * It takes a vector of packed pixels:
 *
 *   packed = {P0, P1, P2, P3, ..., Pn}
 *
 * And will produce four vectors:
 *
 *   red    = {R0, R1, R2, R3, ..., Rn}
 *   green  = {G0, G1, G2, G3, ..., Gn}
 *   blue   = {B0, B1, B2, B3, ..., Bn}
 *   alpha  = {A0, A1, A2, A3, ..., An}
 *
 * It requires that a packed pixel fits into an element of the output
 * channels. The common case is when converting pixel with a depth of 32 bit or
 * less into floats.
 */
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

   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);
   assert(format_desc->block.bits <= type.width);
   /* FIXME: Support more output types */
   assert(type.floating);
   assert(type.width == 32);

   /* Decode the input vector components */
   start = 0;
   for (chan = 0; chan < format_desc->nr_channels; ++chan) {
      unsigned width = format_desc->channel[chan].size;
      unsigned stop = start + width;
      LLVMValueRef input;

      input = packed;

      switch(format_desc->channel[chan].type) {
      case UTIL_FORMAT_TYPE_VOID:
         input = lp_build_undef(type);
         break;

      case UTIL_FORMAT_TYPE_UNSIGNED:
         /*
          * Align the LSB
          */

         if (start) {
            input = LLVMBuildLShr(builder, input, lp_build_const_int_vec(type, start), "");
         }

         /*
          * Zero the MSBs
          */

         if (stop < format_desc->block.bits) {
            unsigned mask = ((unsigned long long)1 << width) - 1;
            input = LLVMBuildAnd(builder, input, lp_build_const_int_vec(type, mask), "");
         }

         /*
          * Type conversion
          */

         if (type.floating) {
            if(format_desc->channel[chan].normalized)
               input = lp_build_unsigned_norm_to_float(builder, width, type, input);
            else
               input = LLVMBuildSIToFP(builder, input, lp_build_vec_type(type), "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(type);
         }

         break;

      case UTIL_FORMAT_TYPE_SIGNED:
         /*
          * Align the sign bit first.
          */

         if (stop < type.width) {
            unsigned bits = type.width - stop;
            LLVMValueRef bits_val = lp_build_const_int_vec(type, bits);
            input = LLVMBuildShl(builder, input, bits_val, "");
         }

         /*
          * Align the LSB (with an arithmetic shift to preserve the sign)
          */

         if (format_desc->channel[chan].size < type.width) {
            unsigned bits = type.width - format_desc->channel[chan].size;
            LLVMValueRef bits_val = lp_build_const_int_vec(type, bits);
            input = LLVMBuildAShr(builder, input, bits_val, "");
         }

         /*
          * Type conversion
          */

         if (type.floating) {
            input = LLVMBuildSIToFP(builder, input, lp_build_vec_type(type), "");
            if (format_desc->channel[chan].normalized) {
               double scale = 1.0 / ((1 << (format_desc->channel[chan].size - 1)) - 1);
               LLVMValueRef scale_val = lp_build_const_vec(type, scale);
               input = LLVMBuildMul(builder, input, scale_val, "");
            }
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(type);
         }

         break;

      case UTIL_FORMAT_TYPE_FLOAT:
         if (type.floating) {
            assert(start == 0);
            assert(stop == 32);
            assert(type.width == 32);
            input = LLVMBuildBitCast(builder, input, lp_build_vec_type(type), "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(type);
         }
         break;

      case UTIL_FORMAT_TYPE_FIXED:
         if (type.floating) {
            double scale = 1.0 / ((1 << (format_desc->channel[chan].size/2)) - 1);
            LLVMValueRef scale_val = lp_build_const_vec(type, scale);
            input = LLVMBuildSIToFP(builder, input, lp_build_vec_type(type), "");
            input = LLVMBuildMul(builder, input, scale_val, "");
         }
         else {
            /* FIXME */
            assert(0);
            input = lp_build_undef(type);
         }
         break;

      default:
         assert(0);
         input = lp_build_undef(type);
         break;
      }

      inputs[chan] = input;

      start = stop;
   }

   lp_build_format_swizzle_soa(format_desc, type, inputs, rgba);
}


/**
 * Fetch a pixel into a SoA.
 *
 * i and j are the sub-block pixel coordinates.
 */
void
lp_build_fetch_rgba_soa(LLVMBuilderRef builder,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef base_ptr,
                        LLVMValueRef offset,
                        LLVMValueRef i,
                        LLVMValueRef j,
                        LLVMValueRef *rgba)
{

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
       (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) &&
       format_desc->block.width == 1 &&
       format_desc->block.height == 1 &&
       format_desc->block.bits <= type.width &&
       (format_desc->channel[0].type != UTIL_FORMAT_TYPE_FLOAT ||
        format_desc->channel[0].size == 32))
   {
      /*
       * The packed pixel fits into an element of the destination format. Put
       * the packed pixels into a vector and estract each component for all
       * vector elements in parallel.
       */

      LLVMValueRef packed;

      /*
       * gather the texels from the texture
       */
      packed = lp_build_gather(builder,
                               type.length,
                               format_desc->block.bits,
                               type.width,
                               base_ptr, offset);

      /*
       * convert texels to float rgba
       */
      lp_build_unpack_rgba_soa(builder,
                               format_desc,
                               type,
                               packed, rgba);
   }
   else {
      /*
       * Fallback to calling lp_build_fetch_rgba_aos for each pixel.
       *
       * This is not the most efficient way of fetching pixels, as
       * we miss some opportunities to do vectorization, but this it is a
       * convenient for formats or scenarios for which there was no opportunity
       * or incentive to optimize.
       */

      unsigned k, chan;

      assert(type.floating);

      for (chan = 0; chan < 4; ++chan) {
         rgba[chan] = lp_build_undef(type);
      }

      for(k = 0; k < type.length; ++k) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), k, 0);
         LLVMValueRef offset_elem;
         LLVMValueRef ptr;
         LLVMValueRef i_elem, j_elem;
         LLVMValueRef tmp;

         offset_elem = LLVMBuildExtractElement(builder, offset, index, "");
         ptr = LLVMBuildGEP(builder, base_ptr, &offset_elem, 1, "");

         i_elem = LLVMBuildExtractElement(builder, i, index, "");
         j_elem = LLVMBuildExtractElement(builder, j, index, "");

         tmp = lp_build_fetch_rgba_aos(builder, format_desc, ptr, i_elem, j_elem);

         /*
          * AoS to SoA
          */

         for (chan = 0; chan < 4; ++chan) {
            LLVMValueRef chan_val = LLVMConstInt(LLVMInt32Type(), chan, 0),
            tmp_chan = LLVMBuildExtractElement(builder, tmp, chan_val, "");
            rgba[chan] = LLVMBuildInsertElement(builder, rgba[chan], tmp_chan, index, "");
         }
      }
   }
}
