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

/* glsl-optimizer change: Expose scalar half conversions without CPU detection or F16C dispatch.
 * Omit unused RTZ and half/UNORM interfaces and their softfloat dependencies.
 */

#ifndef _HALF_FLOAT_H_
#define _HALF_FLOAT_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#define FP16_ONE     ((uint16_t) 0x3c00)
#define FP16_ZERO    ((uint16_t) 0)

uint16_t _mesa_float_to_half_slow(float val);
float _mesa_half_to_float_slow(uint16_t val);
static inline uint16_t
_mesa_float_to_half(float val)
{
   return _mesa_float_to_half_slow(val);
}

static inline float
_mesa_half_to_float(uint16_t val)
{
   return _mesa_half_to_float_slow(val);
}

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


#ifdef __cplusplus

/* Helper class for disambiguating fp16 from uint16_t in C++ overloads */

struct float16_t {
   uint16_t bits;
   float16_t(float f) : bits(_mesa_float_to_half(f)) {}
   float16_t(double d) : bits(_mesa_float_to_half(d)) {}
   float16_t(uint16_t bits) : bits(bits) {}
   static float16_t one() { return float16_t(FP16_ONE); }
   static float16_t zero() { return float16_t(FP16_ZERO); }
};

#endif


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _HALF_FLOAT_H_ */
