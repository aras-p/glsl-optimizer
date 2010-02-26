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
#include "util/u_debug.h"


#ifdef __cplusplus
extern "C" {
#endif


#if defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
__inline double ceil(double val)
{
   double ceil_val;

   if ((val - (long) val) == 0) {
      ceil_val = val;
   }
   else {
      if (val > 0) {
         ceil_val = (long) val + 1;
      }
      else {
         ceil_val = (long) val;
      }
   }

   return ceil_val;
}

#ifndef PIPE_SUBSYSTEM_WINDOWS_CE_OGL
__inline double floor(double val)
{
   double floor_val;

   if ((val - (long) val) == 0) {
      floor_val = val;
   }
   else {
      if (val > 0) {
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

static INLINE double log2( double x )
{
   const double invln2 = 1.442695041;
   return log( x ) * invln2;
}

#endif /* _MSC_VER */





#define POW2_TABLE_SIZE_LOG2 9
#define POW2_TABLE_SIZE (1 << POW2_TABLE_SIZE_LOG2)
#define POW2_TABLE_OFFSET (POW2_TABLE_SIZE/2)
#define POW2_TABLE_SCALE ((float)(POW2_TABLE_SIZE/2))
extern float pow2_table[POW2_TABLE_SIZE];


/**
 * Initialize math module.  This should be called before using any
 * other functions in this module.
 */
extern void
util_init_math(void);


union fi {
   float f;
   int32_t i;
   uint32_t ui;
};


/**
 * Fast version of 2^x
 * Identity: exp2(a + b) = exp2(a) * exp2(b)
 * Let ipart = int(x)
 * Let fpart = x - ipart;
 * So, exp2(x) = exp2(ipart) * exp2(fpart)
 * Compute exp2(ipart) with i << ipart
 * Compute exp2(fpart) with lookup table.
 */
static INLINE float
util_fast_exp2(float x)
{
   int32_t ipart;
   float fpart, mpart;
   union fi epart;

   if(x > 129.00000f)
      return 3.402823466e+38f;

   if (x < -126.99999f)
      return 0.0f;

   ipart = (int32_t) x;
   fpart = x - (float) ipart;

   /* same as
    *   epart.f = (float) (1 << ipart)
    * but faster and without integer overflow for ipart > 31
    */
   epart.i = (ipart + 127 ) << 23;

   mpart = pow2_table[POW2_TABLE_OFFSET + (int)(fpart * POW2_TABLE_SCALE)];

   return epart.f * mpart;
}


/**
 * Fast approximation to exp(x).
 */
static INLINE float
util_fast_exp(float x)
{
   const float k = 1.44269f; /* = log2(e) */
   return util_fast_exp2(k * x);
}


#define LOG2_TABLE_SIZE_LOG2 16
#define LOG2_TABLE_SCALE (1 << LOG2_TABLE_SIZE_LOG2)
#define LOG2_TABLE_SIZE (LOG2_TABLE_SCALE + 1)
extern float log2_table[LOG2_TABLE_SIZE];


/**
 * Fast approximation to log2(x).
 */
static INLINE float
util_fast_log2(float x)
{
   union fi num;
   float epart, mpart;
   num.f = x;
   epart = (float)(((num.i & 0x7f800000) >> 23) - 127);
   /* mpart = log2_table[mantissa*LOG2_TABLE_SCALE + 0.5] */
   mpart = log2_table[((num.i & 0x007fffff) + (1 << (22 - LOG2_TABLE_SIZE_LOG2))) >> (23 - LOG2_TABLE_SIZE_LOG2)];
   return epart + mpart;
}


/**
 * Fast approximation to x^y.
 */
static INLINE float
util_fast_pow(float x, float y)
{
   return util_fast_exp2(util_fast_log2(x) * y);
}

/* Note that this counts zero as a power of two.
 */
static INLINE boolean
util_is_power_of_two( unsigned v )
{
   return (v & (v-1)) == 0;
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
   af = (3 << 22) + 0.5 + (double) f;
   bf = (3 << 22) + 0.5 - (double) f;
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



/**
 * Test if x is NaN or +/- infinity.
 */
static INLINE boolean
util_is_inf_or_nan(float x)
{
   union fi tmp;
   tmp.f = x;
   return !(int)((unsigned int)((tmp.i & 0x7fffffff)-0x7f800000) >> 31);
}


/**
 * Test whether x is a power of two.
 */
static INLINE boolean
util_is_pot(unsigned x)
{
   return (x & (x - 1)) == 0;
}


/**
 * Find first bit set in word.  Least significant bit is 1.
 * Return 0 if no bits set.
 */
#if defined(_MSC_VER) && _MSC_VER >= 1300 && (_M_IX86 || _M_AMD64 || _M_IA64)
unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask);
#pragma intrinsic(_BitScanForward)
static INLINE
unsigned long ffs( unsigned long u )
{
   unsigned long i;
   if (_BitScanForward(&i, u))
      return i + 1;
   else
      return 0;
}
#elif defined(PIPE_CC_MSVC) && defined(PIPE_ARCH_X86)
static INLINE
unsigned ffs( unsigned u )
{
   unsigned i;

   if (u == 0) {
      return 0;
   }

   __asm bsf eax, [u]
   __asm inc eax
   __asm mov [i], eax

   return i;
}
#elif defined(__MINGW32__)
#define ffs __builtin_ffs
#endif

#ifdef __MINGW32__
#define ffs __builtin_ffs
#endif


/* Could also binary search for the highest bit.
 */
static INLINE unsigned
util_unsigned_logbase2(unsigned n)
{
   unsigned log2 = 0;
   while (n >>= 1)
      ++log2;
   return log2;
}


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


/**
 * Convert ubyte to float in [0, 1].
 * XXX a 256-entry lookup table would be slightly faster.
 */
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


/**
 * Calc log base 2
 */
static INLINE unsigned
util_logbase2(unsigned n)
{
   unsigned log2 = 0;
   while (n >>= 1)
      ++log2;
   return log2;
}


/**
 * Returns the smallest power of two >= x
 */
static INLINE unsigned
util_next_power_of_two(unsigned x)
{
   unsigned i;

   if (x == 0)
      return 1;

   --x;

   for (i = 1; i < sizeof(unsigned) * 8; i <<= 1)
      x |= x >> i;

   return x + 1;
}


/**
 * Return number of bits set in n.
 */
static INLINE unsigned
util_bitcount(unsigned n)
{
#if defined(PIPE_CC_GCC)
   return __builtin_popcount(n);
#else
   /* K&R classic bitcount.
    *
    * For each iteration, clear the LSB from the bitfield.
    * Requires only one iteration per set bit, instead of
    * one iteration per bit less than highest set bit.
    */
   unsigned bits = 0;
   for (bits; n; bits++) {
      n &= n - 1;
   }
   return bits;
#endif
}


/**
 * Reverse byte order of a 32 bit word.
 */
static INLINE uint32_t
util_bswap32(uint32_t n)
{
#if defined(PIPE_CC_GCC) && (PIPE_CC_GCC_VERSION >= 403)
   return __builtin_bswap32(n);
#else
   return (n >> 24) |
          ((n >> 8) & 0x0000ff00) |
          ((n << 8) & 0x00ff0000) |
          (n << 24);
#endif
}


/**
 * Reverse byte order of a 16 bit word.
 */
static INLINE uint16_t
util_bswap16(uint16_t n)
{
   return (n >> 8) |
          (n << 8);
}


/**
 * Clamp X to [MIN, MAX].
 * This is a macro to allow float, int, uint, etc. types.
 */
#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

#define MIN3( A, B, C ) MIN2( MIN2( A, B ), C )
#define MAX3( A, B, C ) MAX2( MAX2( A, B ), C )


static INLINE int
align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

static INLINE unsigned
u_minify(unsigned value, unsigned levels)
{
    return MAX2(1, value >> levels);
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


static INLINE uint32_t util_unsigned_fixed(float value, unsigned frac_bits)
{
   return value < 0 ? 0 : (uint32_t)(value * (1<<frac_bits));
}

static INLINE int32_t util_signed_fixed(float value, unsigned frac_bits)
{
   return (int32_t)(value * (1<<frac_bits));
}



#ifdef __cplusplus
}
#endif

#endif /* U_MATH_H */
