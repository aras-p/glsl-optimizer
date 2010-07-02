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

#include "lp_bld_arit.h"
#include "lp_bld_init.h"
#include "lp_bld_type.h"
#include "lp_bld_flow.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"


/**
 * Basic swizzling.  Rearrange the order of the unswizzled array elements
 * according to the format description.  PIPE_SWIZZLE_ZERO/ONE are supported
 * too.
 * Ex: if unswizzled[4] = {B, G, R, x}, then swizzled_out[4] = {R, G, B, 1}.
 */
LLVMValueRef
lp_build_format_swizzle_aos(const struct util_format_description *desc,
                            struct lp_build_context *bld,
                            LLVMValueRef unswizzled)
{
   unsigned char swizzles[4];
   unsigned chan;

   assert(bld->type.length % 4 == 0);

   for (chan = 0; chan < 4; ++chan) {
      enum util_format_swizzle swizzle;

      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
         /*
          * For ZS formats do RGBA = ZZZ1
          */
         if (chan == 3) {
            swizzle = UTIL_FORMAT_SWIZZLE_1;
         } else if (desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_NONE) {
            swizzle = UTIL_FORMAT_SWIZZLE_0;
         } else {
            swizzle = desc->swizzle[0];
         }
      } else {
         swizzle = desc->swizzle[chan];
      }
      swizzles[chan] = swizzle;
   }

   return lp_build_swizzle_aos(bld, unswizzled, swizzles);
}


/**
 * Whether the format matches the vector type, apart of swizzles.
 */
static INLINE boolean
format_matches_type(const struct util_format_description *desc,
                    struct lp_type type)
{
   enum util_format_type chan_type;
   unsigned chan;

   assert(type.length % 4 == 0);

   if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
       desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB) {
      return FALSE;
   }

   if (type.floating) {
      chan_type = UTIL_FORMAT_TYPE_FLOAT;
   } else if (type.fixed) {
      chan_type = UTIL_FORMAT_TYPE_FIXED;
   } else if (type.sign) {
      chan_type = UTIL_FORMAT_TYPE_SIGNED;
   } else {
      chan_type = UTIL_FORMAT_TYPE_UNSIGNED;
   }

   for (chan = 0; chan < desc->nr_channels; ++chan) {
      if (desc->channel[chan].size != type.width) {
         return FALSE;
      }

      if (desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID) {
         if (desc->channel[chan].type != chan_type ||
             desc->channel[chan].normalized != type.norm) {
            return FALSE;
         }
      }
   }

   return TRUE;
}


/**
 * Unpack a single pixel into its RGBA components.
 *
 * @param desc  the pixel format for the packed pixel value
 * @param type  the desired return type (float[4] vs. ubyte[4])
 * @param packed integer pixel in a format such as PIPE_FORMAT_B8G8R8A8_UNORM
 *
 * @return RGBA in a float[4] or ubyte[4] or ushort[4] vector.
 */
static INLINE LLVMValueRef
lp_build_unpack_rgba_aos(const struct util_format_description *desc,
                         struct lp_build_context *bld,
                         LLVMValueRef packed)
{
   LLVMBuilderRef builder = bld->builder;
   struct lp_type type = bld->type;
   LLVMValueRef shifted, casted, scaled, masked;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];

   boolean normalized;
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

   /* Broadcast the packed value to all four channels
    * before: packed = BGRA
    * after: packed = {BGRA, BGRA, BGRA, BGRA}
    */
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
   shift = 0;

   /* Loop over 4 color components */
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;

      if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
         shifts[i] = LLVMGetUndef(LLVMInt32Type());
         masks[i] = LLVMConstNull(LLVMInt32Type());
         scales[i] =  LLVMConstNull(LLVMFloatType());
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

   /* Ex: convert packed = {BGRA, BGRA, BGRA, BGRA}
    * into masked = {B, G, R, A}
    */
   shifted = LLVMBuildLShr(builder, packed, LLVMConstVector(shifts, 4), "");
   masked = LLVMBuildAnd(builder, shifted, LLVMConstVector(masks, 4), "");


   if (!needs_uitofp) {
      /* UIToFP can't be expressed in SSE2 */
      casted = LLVMBuildSIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   } else {
      casted = LLVMBuildUIToFP(builder, masked, LLVMVectorType(LLVMFloatType(), 4), "");
   }

   /* At this point 'casted' may be a vector of floats such as
    * {255.0, 255.0, 255.0, 255.0}.  Next, if the pixel values are normalized
    * we'll scale this to {1.0, 1.0, 1.0, 1.0}.
    */

   if (normalized)
      scaled = LLVMBuildMul(builder, casted, LLVMConstVector(scales, 4), "");
   else
      scaled = casted;

   /*
    * Type conversion.
    *
    * TODO: We could avoid floating conversion for integer to
    * integer conversions.
    */

   lp_build_conv(builder,
                 lp_float32_vec4_type(),
                 type,
                 &scaled, 1, &scaled, 1);

   scaled = lp_build_format_swizzle_aos(desc, bld, scaled);

   return scaled;
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
 * \param format_desc  describes format of the image we're fetching from
 * \param ptr  address of the pixel block (or the texel if uncompressed)
 * \param i, j  the sub-block pixel coordinates.  For non-compressed formats
 *              these will always be (0, 0).
 * \return  a 4 element vector with the pixel's RGBA values.
 */
LLVMValueRef
lp_build_fetch_rgba_aos(LLVMBuilderRef builder,
                        const struct util_format_description *format_desc,
                        struct lp_type type,
                        LLVMValueRef ptr,
                        LLVMValueRef i,
                        LLVMValueRef j)
{
   struct lp_build_context bld;

   /* XXX: For now we only support one pixel at a time */
   assert(type.length == 4);

   lp_build_context_init(&bld, builder, type);

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

      if (format_matches_type(format_desc, type)) {
         /*
          * The format matches the type (apart of a swizzle) so no need for
          * scaling or converting.
          */

         assert(format_desc->block.bits <= type.width * type.length);
         if (format_desc->block.bits < type.width * type.length) {
            packed = LLVMBuildZExt(builder, packed,
                                   LLVMIntType(type.width * type.length), "");
         }

         packed = LLVMBuildBitCast(builder, packed, lp_build_vec_type(type), "");

         return lp_build_format_swizzle_aos(format_desc, &bld, packed);
      } else {
         return lp_build_unpack_rgba_aos(format_desc, &bld, packed);
      }
   }
   else if (format_desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      LLVMValueRef packed;
      LLVMValueRef rgba;

      ptr = LLVMBuildBitCast(builder, ptr,
                             LLVMPointerType(LLVMInt32Type(), 0),
                             "packed_ptr");

      packed = LLVMBuildLoad(builder, ptr, "packed");

      rgba = lp_build_unpack_subsampled_to_rgba_aos(builder, format_desc,
                                                    1, packed, i, j);

      lp_build_conv(builder,
                    lp_unorm8_vec4_type(),
                    type,
                    &rgba, 1, &rgba, 1);

      return rgba;
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
      LLVMTypeRef f32t = LLVMFloatType();
      LLVMTypeRef f32x4t = LLVMVectorType(f32t, 4);
      LLVMTypeRef pf32t = LLVMPointerType(f32t, 0);
      LLVMValueRef function;
      LLVMValueRef tmp_ptr;
      LLVMValueRef tmp_val;
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
         arg_types[0] = pf32t;
         arg_types[1] = LLVMPointerType(LLVMInt8Type(), 0);
         arg_types[3] = arg_types[2] = LLVMIntType(sizeof(unsigned) * 8);
         function_type = LLVMFunctionType(ret_type, arg_types, Elements(arg_types), 0);
         function = LLVMAddFunction(module, name, function_type);

         LLVMSetFunctionCallConv(function, LLVMCCallConv);
         LLVMSetLinkage(function, LLVMExternalLinkage);

         assert(LLVMIsDeclaration(function));

         LLVMAddGlobalMapping(lp_build_engine, function,
                              func_to_pointer((func_pointer)format_desc->fetch_rgba_float));
      }

      tmp_ptr = lp_build_alloca(builder, f32x4t, "");

      /*
       * Invoke format_desc->fetch_rgba_float() for each pixel and insert the result
       * in the SoA vectors.
       */

      args[0] = LLVMBuildBitCast(builder, tmp_ptr, pf32t, "");
      args[1] = ptr;
      args[2] = i;
      args[3] = j;

      LLVMBuildCall(builder, function, args, Elements(args), "");

      tmp_val = LLVMBuildLoad(builder, tmp_ptr, "");

      if (type.floating) {
         /* No further conversion necessary */
      } else {
         lp_build_conv(builder,
                       lp_float32_vec4_type(),
                       type,
                       &tmp_val, 1, &tmp_val, 1);
      }

      return tmp_val;
   }
   else {
      assert(0);
      return lp_build_undef(type);
   }
}
