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
 * AoS pixel format manipulation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_cpu_detect.h"
#include "util/u_format.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_format.h"


/**
 * Unpack a single pixel into its RGBA components.
 *
 * @param packed integer.
 *
 * @return RGBA in a 4 floats vector.
 *
 * XXX: This is mostly for reference and testing -- operating a single pixel at
 * a time is rarely if ever needed.
 */
LLVMValueRef
lp_build_unpack_rgba_aos(LLVMBuilderRef builder,
                         const struct util_format_description *desc,
                         LLVMValueRef packed)
{
   LLVMTypeRef type;
   LLVMValueRef shifted, casted, scaled, masked;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];
   LLVMValueRef swizzles[4];
   LLVMValueRef aux[4];
   bool normalized;
   int empty_channel;
   unsigned shift;
   unsigned i;

   /* FIXME: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

   type = LLVMIntType(desc->block.bits);

   /* Do the intermediate integer computations with 32bit integers since it
    * matches floating point size */
   if (desc->block.bits < 32)
      packed = LLVMBuildZExt(builder, packed, LLVMInt32Type(), "");

   /* Broadcast the packed value to all four channels */
   packed = LLVMBuildInsertElement(builder,
                                   LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                   packed,
                                   LLVMConstNull(LLVMInt32Type()),
                                   "");
   packed = LLVMBuildShuffleVector(builder,
                                   packed,
                                   LLVMGetUndef(LLVMVectorType(LLVMInt32Type(), 4)),
                                   LLVMConstNull(LLVMVectorType(LLVMInt32Type(), 4)),
                                   "");

   /* Initialize vector constants */
   normalized = FALSE;
   empty_channel = -1;
   shift = 0;
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

   shifted = LLVMBuildLShr(builder, packed, LLVMConstVector(shifts, 4), "");
   masked = LLVMBuildAnd(builder, shifted, LLVMConstVector(masks, 4), "");
   /* UIToFP can't be expressed in SSE2 */
   casted = LLVMBuildSIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");

   if (normalized)
      scaled = LLVMBuildMul(builder, casted, LLVMConstVector(scales, 4), "");
   else
      scaled = casted;

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

   return LLVMBuildShuffleVector(builder, scaled, LLVMConstVector(aux, 4), LLVMConstVector(swizzles, 4), "");
}


/**
 * Take a vector with packed pixels and unpack into a rgba8 vector.
 *
 * Formats with bit depth smaller than 32bits are accepted, but they must be
 * padded to 32bits.
 */
LLVMValueRef
lp_build_unpack_rgba8_aos(LLVMBuilderRef builder,
                          const struct util_format_description *desc,
                          struct lp_type type,
                          LLVMValueRef packed)
{
   struct lp_build_context bld;
   bool rgba8;
   LLVMValueRef res;
   unsigned i;

   lp_build_context_init(&bld, builder, type);

   /* FIXME: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

   assert(!type.floating);
   assert(!type.fixed);
   assert(type.norm);
   assert(type.width == 8);
   assert(type.length % 4 == 0);

   rgba8 = TRUE;
   for(i = 0; i < 4; ++i) {
      assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED ||
             desc->channel[i].type == UTIL_FORMAT_TYPE_VOID);
      if(desc->channel[0].size != 8)
         rgba8 = FALSE;
   }

   if(rgba8) {
      /*
       * The pixel is already in a rgba8 format variant. All it is necessary
       * is to swizzle the channels.
       */

      unsigned char swizzles[4];
      boolean zeros[4]; /* bitwise AND mask */
      boolean ones[4]; /* bitwise OR mask */
      boolean swizzles_needed = FALSE;
      boolean zeros_needed = FALSE;
      boolean ones_needed = FALSE;

      for(i = 0; i < 4; ++i) {
         enum util_format_swizzle swizzle = desc->swizzle[i];

         /* Initialize with the no-op case */
         swizzles[i] = util_cpu_caps.little_endian ? 3 - i : i;
         zeros[i] = TRUE;
         ones[i] = FALSE;

         switch (swizzle) {
         case UTIL_FORMAT_SWIZZLE_X:
         case UTIL_FORMAT_SWIZZLE_Y:
         case UTIL_FORMAT_SWIZZLE_Z:
         case UTIL_FORMAT_SWIZZLE_W:
            if(swizzle != swizzles[i]) {
               swizzles[i] = swizzle;
               swizzles_needed = TRUE;
            }
            break;
         case UTIL_FORMAT_SWIZZLE_0:
            zeros[i] = FALSE;
            zeros_needed = TRUE;
            break;
         case UTIL_FORMAT_SWIZZLE_1:
            ones[i] = TRUE;
            ones_needed = TRUE;
            break;
         case UTIL_FORMAT_SWIZZLE_NONE:
            assert(0);
            break;
         }
      }

      res = packed;

      if(swizzles_needed)
         res = lp_build_swizzle1_aos(&bld, res, swizzles);

      if(zeros_needed) {
         /* Mask out zero channels */
         LLVMValueRef mask = lp_build_const_mask_aos(type, zeros);
         res = LLVMBuildAnd(builder, res, mask, "");
      }

      if(ones_needed) {
         /* Or one channels */
         LLVMValueRef mask = lp_build_const_mask_aos(type, ones);
         res = LLVMBuildOr(builder, res, mask, "");
      }
   }
   else {
      /* FIXME */
      assert(0);
      res = lp_build_undef(type);
   }

   return res;
}


/**
 * Pack a single pixel.
 *
 * @param rgba 4 float vector with the unpacked components.
 *
 * XXX: This is mostly for reference and testing -- operating a single pixel at
 * a time is rarely if ever needed.
 */
LLVMValueRef
lp_build_pack_rgba_aos(LLVMBuilderRef builder,
                       const struct util_format_description *desc,
                       LLVMValueRef rgba)
{
   LLVMTypeRef type;
   LLVMValueRef packed = NULL;
   LLVMValueRef swizzles[4];
   LLVMValueRef shifted, casted, scaled, unswizzled;
   LLVMValueRef shifts[4];
   LLVMValueRef scales[4];
   bool normalized;
   unsigned shift;
   unsigned i, j;

   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);

   type = LLVMIntType(desc->block.bits);

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

   normalized = FALSE;
   shift = 0;
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

   if (!packed)
      packed = LLVMGetUndef(LLVMInt32Type());

   if (desc->block.bits < 32)
      packed = LLVMBuildTrunc(builder, packed, type, "");

   return packed;
}
