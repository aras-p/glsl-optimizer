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


#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_string.h"

#include "lp_bld_init.h"
#include "lp_bld_type.h"
#include "lp_bld_flow.h"
#include "lp_bld_format.h"


/**
 * Unpack a single pixel into its RGBA components.
 *
 * @param packed integer.
 *
 * @return RGBA in a 4 floats vector.
 */
LLVMValueRef
lp_build_unpack_rgba_aos(LLVMBuilderRef builder,
                         const struct util_format_description *desc,
                         LLVMValueRef packed)
{
   LLVMValueRef shifted, casted, scaled, masked;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];
   LLVMValueRef swizzles[4];
   LLVMValueRef aux[4];
   boolean normalized;
   int empty_channel;
   boolean needs_uitofp;
   unsigned shift;
   unsigned i;

   /* TODO: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
   assert(desc->block.bits <= 32);

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
   needs_uitofp = FALSE;
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
         unsigned long long mask = (1ULL << bits) - 1;

         assert(desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED);

         if (bits == 32) {
            needs_uitofp = TRUE;
         }

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
   if (!needs_uitofp) {
      /* UIToFP can't be expressed in SSE2 */
      casted = LLVMBuildSIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   } else {
      casted = LLVMBuildUIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   }

   if (normalized)
      scaled = LLVMBuildMul(builder, casted, LLVMConstVector(scales, 4), "");
   else
      scaled = casted;

   for (i = 0; i < 4; ++i)
      aux[i] = LLVMGetUndef(LLVMFloatType());

   for (i = 0; i < 4; ++i) {
      enum util_format_swizzle swizzle;

      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
         /*
          * For ZS formats do RGBA = ZZZ1
          */
         if (i == 3) {
            swizzle = UTIL_FORMAT_SWIZZLE_1;
         } else if (desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_NONE) {
            swizzle = UTIL_FORMAT_SWIZZLE_0;
         } else {
            swizzle = desc->swizzle[0];
         }
      } else {
         swizzle = desc->swizzle[i];
      }

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
   boolean normalized;
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


/**
 * Fetch a pixel into a 4 float AoS.
 *
 * i and j are the sub-block pixel coordinates.
 */
LLVMValueRef
lp_build_fetch_rgba_aos(LLVMBuilderRef builder,
                        const struct util_format_description *format_desc,
                        LLVMValueRef ptr,
                        LLVMValueRef i,
                        LLVMValueRef j)
{

   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
       (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) &&
       format_desc->block.width == 1 &&
       format_desc->block.height == 1 &&
       util_is_pot(format_desc->block.bits) &&
       format_desc->block.bits <= 32 &&
       format_desc->is_bitmask &&
       !format_desc->is_mixed &&
       (format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED ||
        format_desc->channel[1].type == UTIL_FORMAT_TYPE_UNSIGNED))
   {
      LLVMValueRef packed;

      ptr = LLVMBuildBitCast(builder, ptr,
                             LLVMPointerType(LLVMIntType(format_desc->block.bits), 0) ,
                             "");

      packed = LLVMBuildLoad(builder, ptr, "packed");

      return lp_build_unpack_rgba_aos(builder, format_desc, packed);
   }
   else if (format_desc->fetch_rgba_float) {
      /*
       * Fallback to calling util_format_description::fetch_rgba_float.
       *
       * This is definitely not the most efficient way of fetching pixels, as
       * we miss the opportunity to do vectorization, but this it is a
       * convenient for formats or scenarios for which there was no opportunity
       * or incentive to optimize.
       */

      LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
      char name[256];
      LLVMValueRef function;
      LLVMValueRef tmp;
      LLVMValueRef args[4];

      util_snprintf(name, sizeof name, "util_format_%s_fetch_rgba_float",
                    format_desc->short_name);

      /*
       * Declare and bind format_desc->fetch_rgba_float().
       */

      function = LLVMGetNamedFunction(module, name);
      if (!function) {
         LLVMTypeRef ret_type;
         LLVMTypeRef arg_types[4];
         LLVMTypeRef function_type;

         ret_type = LLVMVoidType();
         arg_types[0] = LLVMPointerType(LLVMFloatType(), 0);
         arg_types[1] = LLVMPointerType(LLVMInt8Type(), 0);
         arg_types[3] = arg_types[2] = LLVMIntType(sizeof(unsigned) * 8);
         function_type = LLVMFunctionType(ret_type, arg_types, Elements(arg_types), 0);
         function = LLVMAddFunction(module, name, function_type);

         LLVMSetFunctionCallConv(function, LLVMCCallConv);
         LLVMSetLinkage(function, LLVMExternalLinkage);

         assert(LLVMIsDeclaration(function));

         LLVMAddGlobalMapping(lp_build_engine, function, format_desc->fetch_rgba_float);
      }

      tmp = lp_build_alloca(builder, LLVMVectorType(LLVMFloatType(), 4), "");

      /*
       * Invoke format_desc->fetch_rgba_float() for each pixel and insert the result
       * in the SoA vectors.
       */

      args[0] = LLVMBuildBitCast(builder, tmp,
                                 LLVMPointerType(LLVMFloatType(), 0), "");
      args[1] = ptr;
      args[2] = i;
      args[3] = j;

      LLVMBuildCall(builder, function, args, Elements(args), "");

      return LLVMBuildLoad(builder, tmp, "");
   }
   else {
      assert(0);
      return LLVMGetUndef(LLVMVectorType(LLVMFloatType(), 4));
   }
}
