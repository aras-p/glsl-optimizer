/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _HALF_FLOAT_H_
#define _HALF_FLOAT_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "util/detect_arch.h"
#include "util/detect_cc.h"
/* glsl-optimizer change: no runtime CPU feature detection (F16C path) */
#include "util/u_math.h"

#if DETECT_ARCH_X86_64
#include <xmmintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FP16_ONE     ((uint16_t) 0x3c00)
#define FP16_ZERO    ((uint16_t) 0)
#define FP16_MAX_F   65504.0

uint16_t _mesa_float_to_half_slow(float val);
float _mesa_half_to_float_slow(uint16_t val);
uint16_t _mesa_uint16_div_64k_to_half(uint16_t v);

/*
 * _mesa_float_to_float16_rtz_slow is no more than a wrapper to the counterpart
 * softfloat.h call. Still, softfloat.h conversion API is meant to be kept
 * private. In other words, only use the API published here, instead of
 * calling directly the softfloat.h one.
 */
uint16_t _mesa_float_to_float16_rtz_slow(float val);

static inline uint16_t
_mesa_float_to_half(float val)
{
#if 0 /* glsl-optimizer change: was DETECT_ARCH_X86_64 && DETECT_CC_GCC (F16C) */
   if (util_get_cpu_caps()->has_f16c) {
      __m128 in = {val};
      __m128i out;

      /* $0 = round to nearest */
      __asm volatile("vcvtps2ph $0, %1, %0" : "=v"(out) : "v"(in));
      return out[0];
   }
#endif
   return _mesa_float_to_half_slow(val);
}

static inline float
_mesa_half_to_float(uint16_t val)
{
#if 0 /* glsl-optimizer change: was DETECT_ARCH_X86_64 && DETECT_CC_GCC (F16C) */
   if (util_get_cpu_caps()->has_f16c) {
      __m128i in = {val};
      __m128 out;

      __asm volatile("vcvtph2ps %1, %0" : "=v"(out) : "v"(in));
      return out[0];
   }
#elif DETECT_ARCH_AARCH64 && DETECT_CC_GCC
   float result;
   uint16_t in = val;

   __asm volatile(
     "fcvt %s0, %h1\n"
     : "=w"(result)
     : "w"(in)
   );
   return result;
#endif
   return _mesa_half_to_float_slow(val);
}

static inline uint16_t
_mesa_float_to_float16_rtz(float val)
{
#if 0 /* glsl-optimizer change: was DETECT_ARCH_X86_64 && DETECT_CC_GCC (F16C) */
   if (util_get_cpu_caps()->has_f16c) {
      __m128 in = {val};
      __m128i out;

      /* $3 = round towards zero (truncate) */
      __asm volatile("vcvtps2ph $3, %1, %0" : "=v"(out) : "v"(in));
      return out[0];
   }
#endif
   return _mesa_float_to_float16_rtz_slow(val);
}

uint16_t _mesa_float_to_float16_ru(float val);
uint16_t _mesa_float_to_float16_rd(float val);

static inline uint16_t
_mesa_float_to_float16_rtne(float val)
{
   return _mesa_float_to_half(val);
}

static inline bool
_mesa_half_is_negative(uint16_t h)
{
   return !!(h & 0x8000);
}

static inline bool
_mesa_float_is_half(double val)
{
   /* val parameter is double to prevent implicit double->float cast.  We have
    * to cast to float because that's what _mesa_float_to_half expects and we
    * don't have any readily available _double_to_half function.  This may
    * introduce double-rounding errors, however this is ok because the final
    * check is done at double precision, any rounding will fail to produce the
    * original value.
    */
   uint16_t fp16_val = _mesa_float_to_half((float) val);
   bool is_denorm = (fp16_val & 0x7fff) != 0 && (fp16_val & 0x7fff) <= 0x3ff;
   return val == (double) _mesa_half_to_float(fp16_val) && !is_denorm;
}

/** Returns a "reduced" double, suitable for conversion to f16
 *
 * RTNE is tricky to get right through a double conversion.  To work around
 * this, we do a little fixup of the fp64 value first.
 *
 * For a 64-bit float, the mantissa bits are as follows:
 *
 *    HHHHHHHHHHHLTFFFFFFFFF FFFDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
 *                           |                              |
 *                           +------- bottom 32 bits -------+
 *
 * Where:
 *  - D are only used for fp64
 *  - T and F are used for fp64 and fp32
 *  - H and L are used for fp64, fp32, and fp16
 *  - L denotes the low bit of the fp16 mantissa
 *  - T is the tie bit
 *
 * The RTNE tie-breaking rules for fp64 -> fp16 can then be described as
 * follows:
 *
 *  - If any F or D bit is non-zero:
 *     - If T == 1, round up
 *     - If T == 0, round down
 *  - If all F and D bits are zero:
 *     - If T == 0, it's already fp16, do nothing
 *     - If T != 0 and L == 0, round down
 *     - If T != 0 and L != 0, round up
 *
 * What's important here is that the only way the F or D bits fit into the
 * algorithm is if any are zero or none are zero.  So we will get the same
 * result if we take all of the bits in the low dword, or them together, and
 * then or that into the low F bits of the high dword.  The result of "all F
 * and D bits are zero" will be the same.  We can also zero the low dword
 * without affecting the final result.  Doing this accomplishes two useful
 * things:
 *
 *  1. The resulting fp64 value is exactly representable as fp32 so we don't
 *     have to care about the rounding of the fp64 -> fp32 conversion.
 *
 *  2. The fp32 -> fp16 conversion will round exactly the same as a full
 *     fp64 -> fp16 conversion on the original data since it now takes all of
 *     the D bits into account as well as the F bits.
 *
 * It's also correct for NaN/INF since those are delineated by the entire
 * mantissa being either zero or non-zero.  For denorms, anything that might
 * be a denorm in fp32 or fp64 will have a sufficiently negative exponent that
 * it will flush to zero when converted to fp16, regardless of what we do
 * here.
 *
 * This same trick works for all the rounding modes.  Even though the actual
 * rounding logic is a bit different, they all treat the F and D bits together
 * based on "all F and D bits are zero" or not.
 */
static inline float
_mesa_reduce_double_for_f16(double val)
{
   union di d;
   d.d = val;
   const uint32_t u_low = (uint32_t)d.ui;
   d.ui &= 0xffffffff00000000ull;
   if (u_low)
      d.ui |= (1ull << 32);
   return (float)d.d;
}

static inline uint16_t
_mesa_double_to_float16_rtne(double val)
{
   return _mesa_float_to_float16_rtne(_mesa_reduce_double_for_f16(val));
}

static inline uint16_t
_mesa_double_to_float16_rtz(double val)
{
   return _mesa_float_to_float16_rtz(_mesa_reduce_double_for_f16(val));
}

static inline uint16_t
_mesa_double_to_float16_ru(double val)
{
   return _mesa_float_to_float16_ru(_mesa_reduce_double_for_f16(val));
}

static inline uint16_t
_mesa_double_to_float16_rd(double val)
{
   return _mesa_float_to_float16_rd(_mesa_reduce_double_for_f16(val));
}

#ifdef __cplusplus

namespace mesa
{

/* Helper class for disambiguating fp16 from uint16_t in C++ overloads */

struct float16_t {
   uint16_t bits;
   float16_t(float f) : bits(_mesa_float_to_float16_rtne(f)) {}
   float16_t(double d) : bits(_mesa_double_to_float16_rtne(d)) {}
   float16_t(uint16_t raw_bits) : bits(raw_bits) {}
   static float16_t one() { return float16_t(FP16_ONE); }
   static float16_t zero() { return float16_t(FP16_ZERO); }
};

} /* namespace mesa */

#endif


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _HALF_FLOAT_H_ */
