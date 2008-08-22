/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Math utilities and approximations for common math functions.
 * Reduced precision is usually acceptable in shaders...
 */


#ifndef U_MATH_H
#define U_MATH_H


#include "pipe/p_util.h"
#include "util/u_math.h"



#define POW2_TABLE_SIZE 256
#define POW2_TABLE_SCALE ((float) (POW2_TABLE_SIZE-1))
extern float pow2_table[POW2_TABLE_SIZE];


extern void
util_init_math(void);




/**
 * Fast approximation to exp(x).
 * Compute with base 2 exponents:  exp(x) = exp2(log2(e) * x)
 * Note: log2(e) is a constant, k = 1.44269
 * So, exp(x) = exp2(k * x);
 * Identity: exp2(a + b) = exp2(a) * exp2(b)
 * Let ipart = int(k*x)
 * Let fpart = k*x - ipart;
 * So, exp2(k*x) = exp2(ipart) * exp2(fpart)
 * Compute exp2(ipart) with i << ipart
 * Compute exp2(fpart) with lookup table.
 */
static INLINE float
util_fast_exp(float x)
{
   if (x >= 0.0f) {
      float k = 1.44269f; /* = log2(e) */
      float kx = k * x;
      int ipart = (int) kx;
      float fpart = kx - (float) ipart;
      float y = (float) (1 << ipart)
         * pow2_table[(int) (fpart * POW2_TABLE_SCALE)];
      return y;
   }
   else {
      /* exp(-x) = 1.0 / exp(x) */
      float k = -1.44269f;
      float kx = k * x;
      int ipart = (int) kx;
      float fpart = kx - (float) ipart;
      float y = (float) (1 << ipart)
         * pow2_table[(int) (fpart * POW2_TABLE_SCALE)];
      return 1.0f / y;
   }
}


/**
 * Fast version of 2^x
 * XXX the above function could be implemented in terms of this one.
 */
static INLINE float
util_fast_exp2(float x)
{
   if (x >= 0.0f) {
      int ipart = (int) x;
      float fpart = x - (float) ipart;
      float y = (float) (1 << ipart)
         * pow2_table[(int) (fpart * POW2_TABLE_SCALE)];
      return y;
   }
   else {
      /* exp(-x) = 1.0 / exp(x) */
      int ipart = (int) -x;
      float fpart = -x - (float) ipart;
      float y = (float) (1 << ipart)
         * pow2_table[(int) (fpart * POW2_TABLE_SCALE)];
      return 1.0f / y;
   }
}


/**
 * Based on code from http://www.flipcode.com/totd/
 */
static INLINE float
util_fast_log2(float val)
{
   union fi num;
   int log_2;
   num.f = val;
   log_2 = ((num.i >> 23) & 255) - 128;
   num.i &= ~(255 << 23);
   num.i += 127 << 23;
   num.f = ((-1.0f/3) * num.f + 2) * num.f - 2.0f/3;
   return num.f + log_2;
}


static INLINE float
util_fast_pow(float x, float y)
{
   /* XXX this test may need adjustment */
   if (y >= 3.0 && -0.02f <= x && x <= 0.02f)
      return 0.0f;
   return util_fast_exp2(util_fast_log2(x) * y);
}


#endif /* U_MATH_H */
