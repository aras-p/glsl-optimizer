/*
 * Copyright © 2025 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include "u_math.h"

/* When converting a Float NaN value to BFloat16 it is possible that the
 * significand bits that make the value a NaN will be rounded/truncated off
 * so ensure at least one significand bit is set.
 */
static inline uint16_t
_mesa_float_nan_to_bfloat_bits(union fi x)
{
   assert(isnan(x.f));
   return x.ui >> 16 | 1 << 6;
}

/* Round-towards-zero. */
static inline uint16_t
_mesa_float_to_bfloat16_bits_rtz(float f)
{
   union fi x;
   x.f = f;

   if (isnan(f))
      return _mesa_float_nan_to_bfloat_bits(x);

   return x.ui >> 16;
}

/* Round-to-nearest-even. */
static inline uint16_t
_mesa_float_to_bfloat16_bits_rte(float f)
{
   union fi x;
   x.f = f;

   if (isnan(f))
      return _mesa_float_nan_to_bfloat_bits(x);

   /* Use the tail part that is discarded to decide rounding,
    * break the tie with the nearest even.
    *
    * Overflow of the significand value will turn to zero and
    * increment the exponent.  If exponent reaches 0xff, the
    * value will correctly end up as +/- Inf.
    */
   uint32_t result = x.ui >> 16;
   const uint32_t tail = x.ui & 0xffff;
   if (tail > 0x8000 || (tail == 0x8000 && (result & 1) == 1))
      result++;

   return result;
}

static inline float
_mesa_bfloat16_bits_to_float(uint16_t bf)
{
   union fi x;
   x.ui = (uint32_t)bf << 16;

   return x.f;
}
