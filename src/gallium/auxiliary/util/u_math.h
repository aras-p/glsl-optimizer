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
 *
 * "fast" is used in the names of functions which are low-precision,
 * or at least lower-precision than the normal C lib functions.
 */


#ifndef U_MATH_H
#define U_MATH_H


#include "pipe/p_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


#if defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
__inline double ceil(double val)
{
   double ceil_val;

   if((val - (long) val) == 0) {
      ceil_val = val;
   }
   else {
      if(val > 0) {
         ceil_val = (long) val + 1;
      }
      else {
         ceil_val = (long) val;
      }
   }

   return ceil_val;
}

#ifndef PIPE_SUBSYSTEM_WINDOWS_CE
__inline double floor(double val)
{
   double floor_val;

   if((val - (long) val) == 0) {
      floor_val = val;
   }
   else {
      if(val > 0) {
         floor_val = (long) val;
      }
      else {
         floor_val = (long) val - 1;
      }
   }

   return floor_val;
}
#endif

#pragma function(pow)
__inline double __cdecl pow(double val, double exponent)
{
   /* XXX */
   assert(0);
   return 0;
}

#pragma function(log)
__inline double __cdecl log(double val)
{
   /* XXX */
   assert(0);
   return 0;
}

#pragma function(atan2)
__inline double __cdecl atan2(double val)
{
   /* XXX */
   assert(0);
   return 0;
}
#else
#include <math.h>
#include <stdarg.h>
#endif


#if defined(_MSC_VER) 
#if _MSC_VER < 1400 && !defined(__cplusplus) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
 
static INLINE float cosf( float f ) 
{
   return (float) cos( (double) f );
}

static INLINE float sinf( float f ) 
{
   return (float) sin( (double) f );
}

static INLINE float ceilf( float f ) 
{
   return (float) ceil( (double) f );
}

static INLINE float floorf( float f ) 
{
   return (float) floor( (double) f );
}

static INLINE float powf( float f, float g ) 
{
   return (float) pow( (double) f, (double) g );
}

static INLINE float sqrtf( float f ) 
{
   return (float) sqrt( (double) f );
}

static INLINE float fabsf( float f ) 
{
   return (float) fabs( (double) f );
}

static INLINE float logf( float f ) 
{
   return (float) log( (double) f );
}

#else
/* Work-around an extra semi-colon in VS 2005 logf definition */
#ifdef logf
#undef logf
#define logf(x) ((float)log((double)(x)))
#endif /* logf */
#endif
#endif /* _MSC_VER */





#define POW2_TABLE_SIZE 256
#define POW2_TABLE_SCALE ((float) (POW2_TABLE_SIZE-1))
extern float pow2_table[POW2_TABLE_SIZE];



extern void
util_init_math(void);


union fi {
   float f;
   int i;
   unsigned ui;
};


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
   /* XXX these tests may need adjustment */
   if (y >= 3.0f && (-0.02f <= x && x <= 0.02f))
      return 0.0f;
   if (y >= 50.0f && (-0.9f <= x && x <= 0.9f))
      return 0.0f;
   return util_fast_exp2(util_fast_log2(x) * y);
}



/**
 * Floor(x), returned as int.
 */
static INLINE int
util_ifloor(float f)
{
   int ai, bi;
   double af, bf;
   union fi u;
   af = (3 << 22) + 0.5 + (double)f;
   bf = (3 << 22) + 0.5 - (double)f;
   u.f = (float) af;  ai = u.i;
   u.f = (float) bf;  bi = u.i;
   return (ai - bi) >> 1;
}


/**
 * Round float to nearest int.
 */
static INLINE int
util_iround(float f)
{
#if defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86) 
   int r;
   __asm__ ("fistpl %0" : "=m" (r) : "t" (f) : "st");
   return r;
#elif defined(PIPE_CC_MSVC) && defined(PIPE_ARCH_X86)
   int r;
   _asm {
	 fld f
	 fistp r
	}
   return r;
#else
   if (f >= 0.0f)
      return (int) (f + 0.5f);
   else
      return (int) (f - 0.5f);
#endif
}



#if defined(PIPE_CC_MSVC) && defined(PIPE_ARCH_X86)
/**
 * Find first bit set in word.  Least significant bit is 1.
 * Return 0 if no bits set.
 */
static INLINE
unsigned ffs( unsigned u )
{
   unsigned i;

   if( u == 0 ) {
      return 0;
   }

   __asm bsf eax, [u]
   __asm inc eax
   __asm mov [i], eax

   return i;
}
#endif


/**
 * Return float bits.
 */
static INLINE unsigned
fui( float f )
{
   union fi fi;
   fi.f = f;
   return fi.ui;
}



static INLINE float
ubyte_to_float(ubyte ub)
{
   return (float) ub * (1.0f / 255.0f);
}


/**
 * Convert float in [0,1] to ubyte in [0,255] with clamping.
 */
static INLINE ubyte
float_to_ubyte(float f)
{
   const int ieee_0996 = 0x3f7f0000;   /* 0.996 or so */
   union fi tmp;

   tmp.f = f;
   if (tmp.i < 0) {
      return (ubyte) 0;
   }
   else if (tmp.i >= ieee_0996) {
      return (ubyte) 255;
   }
   else {
      tmp.f = tmp.f * (255.0f/256.0f) + 32768.0f;
      return (ubyte) tmp.i;
   }
}



#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )


static INLINE int
align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}


#ifndef COPY_4V
#define COPY_4V( DST, SRC )         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
   (DST)[3] = (SRC)[3];             \
} while (0)
#endif


#ifndef COPY_4FV
#define COPY_4FV( DST, SRC )  COPY_4V(DST, SRC)
#endif


#ifndef ASSIGN_4V
#define ASSIGN_4V( DST, V0, V1, V2, V3 ) \
do {                                     \
   (DST)[0] = (V0);                      \
   (DST)[1] = (V1);                      \
   (DST)[2] = (V2);                      \
   (DST)[3] = (V3);                      \
} while (0)
#endif


#ifdef __cplusplus
}
#endif

#endif /* U_MATH_H */
