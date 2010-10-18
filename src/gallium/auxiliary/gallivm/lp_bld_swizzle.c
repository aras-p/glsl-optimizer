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
 * Helper functions for swizzling/shuffling.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"


LLVMValueRef
lp_build_broadcast(LLVMBuilderRef builder,
                   LLVMTypeRef vec_type,
                   LLVMValueRef scalar)
{
   const unsigned n = LLVMGetVectorSize(vec_type);
   LLVMValueRef res;
   unsigned i;

   res = LLVMGetUndef(vec_type);
   for(i = 0; i < n; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(builder, res, scalar, index, "");
   }

   return res;
}


/**
 * Broadcast
 */
LLVMValueRef
lp_build_broadcast_scalar(struct lp_build_context *bld,
                          LLVMValueRef scalar)
{
   const struct lp_type type = bld->type;

   assert(lp_check_elem_type(type, LLVMTypeOf(scalar)));

   if (type.length == 1) {
      return scalar;
   }
   else {
      LLVMValueRef res;

#if HAVE_LLVM >= 0x207
      /* The shuffle vector is always made of int32 elements */
      struct lp_type i32_vec_type = lp_type_int_vec(32);
      i32_vec_type.length = type.length;

      res = LLVMBuildInsertElement(bld->builder, bld->undef, scalar,
                                   LLVMConstInt(LLVMInt32Type(), 0, 0), "");
      res = LLVMBuildShuffleVector(bld->builder, res, bld->undef,
                                   lp_build_const_int_vec(i32_vec_type, 0), "");
#else
      /* XXX: The above path provokes a bug in LLVM 2.6 */
      unsigned i;
      res = bld->undef;
      for(i = 0; i < type.length; ++i) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
         res = LLVMBuildInsertElement(bld->builder, res, scalar, index, "");
      }
#endif
      return res;
   }
}


/**
 * Combined extract and broadcast (or a mere shuffle when the two types match)
 */
LLVMValueRef
lp_build_extract_broadcast(LLVMBuilderRef builder,
                           struct lp_type src_type,
                           struct lp_type dst_type,
                           LLVMValueRef vector,
                           LLVMValueRef index)
{
   LLVMTypeRef i32t = LLVMInt32Type();
   LLVMValueRef res;

   assert(src_type.floating == dst_type.floating);
   assert(src_type.width    == dst_type.width);

   assert(lp_check_value(src_type, vector));
   assert(LLVMTypeOf(index) == i32t);

   if (src_type.length == 1) {
      if (dst_type.length == 1) {
         /*
          * Trivial scalar -> scalar.
          */

         res = vector;
      }
      else {
         /*
          * Broadcast scalar -> vector.
          */

         res = lp_build_broadcast(builder,
                                  lp_build_vec_type(dst_type),
                                  vector);
      }
   }
   else {
      if (dst_type.length == src_type.length) {
         /*
          * Special shuffle of the same size.
          */

         LLVMValueRef shuffle;
         shuffle = lp_build_broadcast(builder,
                                      LLVMVectorType(i32t, dst_type.length),
                                      index);
         res = LLVMBuildShuffleVector(builder, vector,
                                      LLVMGetUndef(lp_build_vec_type(dst_type)),
                                      shuffle, "");
      }
      else {
         LLVMValueRef scalar;
         scalar = LLVMBuildExtractElement(builder, vector, index, "");
         if (dst_type.length == 1) {
            /*
             * Trivial extract scalar from vector.
             */

            res = scalar;
         }
         else {
            /*
             * General case of different sized vectors.
             */

            res = lp_build_broadcast(builder,
                                     lp_build_vec_type(dst_type),
                                     vector);
         }
      }
   }

   return res;
}


/**
 * Swizzle one channel into all other three channels.
 */
LLVMValueRef
lp_build_swizzle_scalar_aos(struct lp_build_context *bld,
                            LLVMValueRef a,
                            unsigned channel)
{
   const struct lp_type type = bld->type;
   const unsigned n = type.length;
   unsigned i, j;

   if(a == bld->undef || a == bld->zero || a == bld->one)
      return a;

   /* XXX: SSE3 has PSHUFB which should be better than bitmasks, but forcing
    * using shuffles here actually causes worst results. More investigation is
    * needed. */
   if (type.width >= 16) {
      /*
       * Shuffle.
       */
      LLVMTypeRef elem_type = LLVMInt32Type();
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            shuffles[j + i] = LLVMConstInt(elem_type, j + channel, 0);

      return LLVMBuildShuffleVector(bld->builder, a, bld->undef, LLVMConstVector(shuffles, n), "");
   }
   else {
      /*
       * Bit mask and recursive shifts
       *
       *   XYZW XYZW .... XYZW  <= input
       *   0Y00 0Y00 .... 0Y00
       *   YY00 YY00 .... YY00
       *   YYYY YYYY .... YYYY  <= output
       */
      struct lp_type type4;
      const char shifts[4][2] = {
         { 1,  2},
         {-1,  2},
         { 1, -2},
         {-1, -2}
      };
      unsigned i;

      a = LLVMBuildAnd(bld->builder, a,
                       lp_build_const_mask_aos(type, 1 << channel), "");

      /*
       * Build a type where each element is an integer that cover the four
       * channels.
       */

      type4 = type;
      type4.floating = FALSE;
      type4.width *= 4;
      type4.length /= 4;

      a = LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(type4), "");

      for(i = 0; i < 2; ++i) {
         LLVMValueRef tmp = NULL;
         int shift = shifts[channel][i];

#ifdef PIPE_ARCH_LITTLE_ENDIAN
         shift = -shift;
#endif

         if(shift > 0)
            tmp = LLVMBuildLShr(bld->builder, a, lp_build_const_int_vec(type4, shift*type.width), "");
         if(shift < 0)
            tmp = LLVMBuildShl(bld->builder, a, lp_build_const_int_vec(type4, -shift*type.width), "");

         assert(tmp);
         if(tmp)
            a = LLVMBuildOr(bld->builder, a, tmp, "");
      }

      return LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(type), "");
   }
}


LLVMValueRef
lp_build_swizzle_aos(struct lp_build_context *bld,
                     LLVMValueRef a,
                     const unsigned char swizzles[4])
{
   const struct lp_type type = bld->type;
   const unsigned n = type.length;
   unsigned i, j;

   if (swizzles[0] == PIPE_SWIZZLE_RED &&
       swizzles[1] == PIPE_SWIZZLE_GREEN &&
       swizzles[2] == PIPE_SWIZZLE_BLUE &&
       swizzles[3] == PIPE_SWIZZLE_ALPHA) {
      return a;
   }

   if (swizzles[0] == swizzles[1] &&
       swizzles[1] == swizzles[2] &&
       swizzles[2] == swizzles[3]) {
      switch (swizzles[0]) {
      case PIPE_SWIZZLE_RED:
      case PIPE_SWIZZLE_GREEN:
      case PIPE_SWIZZLE_BLUE:
      case PIPE_SWIZZLE_ALPHA:
         return lp_build_swizzle_scalar_aos(bld, a, swizzles[0]);
      case PIPE_SWIZZLE_ZERO:
         return bld->zero;
      case PIPE_SWIZZLE_ONE:
         return bld->one;
      default:
         assert(0);
         return bld->undef;
      }
   }

   if (type.width >= 16) {
      /*
       * Shuffle.
       */
      LLVMValueRef undef = LLVMGetUndef(lp_build_elem_type(type));
      LLVMTypeRef i32t = LLVMInt32Type();
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef aux[LP_MAX_VECTOR_LENGTH];

      memset(aux, 0, sizeof aux);

      for(j = 0; j < n; j += 4) {
         for(i = 0; i < 4; ++i) {
            unsigned shuffle;
            switch (swizzles[i]) {
            default:
               assert(0);
               /* fall through */
            case PIPE_SWIZZLE_RED:
            case PIPE_SWIZZLE_GREEN:
            case PIPE_SWIZZLE_BLUE:
            case PIPE_SWIZZLE_ALPHA:
               shuffle = j + swizzles[i];
               break;
            case PIPE_SWIZZLE_ZERO:
               shuffle = type.length + 0;
               if (!aux[0]) {
                  aux[0] = lp_build_const_elem(type, 0.0);
               }
               break;
            case PIPE_SWIZZLE_ONE:
               shuffle = type.length + 1;
               if (!aux[1]) {
                  aux[1] = lp_build_const_elem(type, 1.0);
               }
               break;
            }
            shuffles[j + i] = LLVMConstInt(i32t, shuffle, 0);
         }
      }

      for (i = 0; i < n; ++i) {
         if (!aux[i]) {
            aux[i] = undef;
         }
      }

      return LLVMBuildShuffleVector(bld->builder, a,
                                    LLVMConstVector(aux, n),
                                    LLVMConstVector(shuffles, n), "");
   } else {
      /*
       * Bit mask and shifts.
       *
       * For example, this will convert BGRA to RGBA by doing
       *
       *   rgba = (bgra & 0x00ff0000) >> 16
       *        | (bgra & 0xff00ff00)
       *        | (bgra & 0x000000ff) << 16
       *
       * This is necessary not only for faster cause, but because X86 backend
       * will refuse shuffles of <4 x i8> vectors
       */
      LLVMValueRef res;
      struct lp_type type4;
      unsigned cond = 0;
      unsigned chan;
      int shift;

      /*
       * Start with a mixture of 1 and 0.
       */
      for (chan = 0; chan < 4; ++chan) {
         if (swizzles[chan] == PIPE_SWIZZLE_ONE) {
            cond |= 1 << chan;
         }
      }
      res = lp_build_select_aos(bld, cond, bld->one, bld->zero);

      /*
       * Build a type where each element is an integer that cover the four
       * channels.
       */
      type4 = type;
      type4.floating = FALSE;
      type4.width *= 4;
      type4.length /= 4;

      a = LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(type4), "");
      res = LLVMBuildBitCast(bld->builder, res, lp_build_vec_type(type4), "");

      /*
       * Mask and shift the channels, trying to group as many channels in the
       * same shift as possible
       */
      for (shift = -3; shift <= 3; ++shift) {
         unsigned long long mask = 0;

         assert(type4.width <= sizeof(mask)*8);

         for (chan = 0; chan < 4; ++chan) {
            /* FIXME: big endian */
            if (swizzles[chan] < 4 &&
                chan - swizzles[chan] == shift) {
               mask |= ((1ULL << type.width) - 1) << (swizzles[chan] * type.width);
            }
         }

         if (mask) {
            LLVMValueRef masked;
            LLVMValueRef shifted;

            if (0)
               debug_printf("shift = %i, mask = 0x%08llx\n", shift, mask);

            masked = LLVMBuildAnd(bld->builder, a,
                                  lp_build_const_int_vec(type4, mask), "");
            if (shift > 0) {
               shifted = LLVMBuildShl(bld->builder, masked,
                                      lp_build_const_int_vec(type4, shift*type.width), "");
            } else if (shift < 0) {
               shifted = LLVMBuildLShr(bld->builder, masked,
                                       lp_build_const_int_vec(type4, -shift*type.width), "");
            } else {
               shifted = masked;
            }

            res = LLVMBuildOr(bld->builder, res, shifted, "");
         }
      }

      return LLVMBuildBitCast(bld->builder, res, lp_build_vec_type(type), "");
   }
}


/**
 * Extended swizzle of a single channel of a SoA vector.
 *
 * @param bld         building context
 * @param unswizzled  array with the 4 unswizzled values
 * @param swizzle     one of the PIPE_SWIZZLE_*
 *
 * @return  the swizzled value.
 */
LLVMValueRef
lp_build_swizzle_soa_channel(struct lp_build_context *bld,
                             const LLVMValueRef *unswizzled,
                             unsigned swizzle)
{
   switch (swizzle) {
   case PIPE_SWIZZLE_RED:
   case PIPE_SWIZZLE_GREEN:
   case PIPE_SWIZZLE_BLUE:
   case PIPE_SWIZZLE_ALPHA:
      return unswizzled[swizzle];
   case PIPE_SWIZZLE_ZERO:
      return bld->zero;
   case PIPE_SWIZZLE_ONE:
      return bld->one;
   default:
      assert(0);
      return bld->undef;
   }
}


/**
 * Extended swizzle of a SoA vector.
 *
 * @param bld         building context
 * @param unswizzled  array with the 4 unswizzled values
 * @param swizzles    array of PIPE_SWIZZLE_*
 * @param swizzled    output swizzled values
 */
void
lp_build_swizzle_soa(struct lp_build_context *bld,
                     const LLVMValueRef *unswizzled,
                     const unsigned char swizzles[4],
                     LLVMValueRef *swizzled)
{
   unsigned chan;

   for (chan = 0; chan < 4; ++chan) {
      swizzled[chan] = lp_build_swizzle_soa_channel(bld, unswizzled,
                                                    swizzles[chan]);
   }
}


/**
 * Do an extended swizzle of a SoA vector inplace.
 *
 * @param bld         building context
 * @param values      intput/output array with the 4 values
 * @param swizzles    array of PIPE_SWIZZLE_*
 */
void
lp_build_swizzle_soa_inplace(struct lp_build_context *bld,
                             LLVMValueRef *values,
                             const unsigned char swizzles[4])
{
   LLVMValueRef unswizzled[4];
   unsigned chan;

   for (chan = 0; chan < 4; ++chan) {
      unswizzled[chan] = values[chan];
   }

   lp_build_swizzle_soa(bld, unswizzled, swizzles, values);
}
