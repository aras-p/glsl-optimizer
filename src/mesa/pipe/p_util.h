/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef P_UTIL_H
#define P_UTIL_H

#include "p_compiler.h"
#include <math.h>

#ifdef WIN32

#ifdef __cplusplus
extern "C"
{
#endif

void * __stdcall
EngAllocMem(
    unsigned long Flags,
    unsigned long MemSize,
    unsigned long Tag );

void __stdcall
EngFreeMem(
    void *Mem );

#ifdef __cplusplus
}
#endif

static INLINE void *
MALLOC( unsigned size )
{
   return EngAllocMem( 0, size, 'D3AG' );
}

static INLINE void *
CALLOC( unsigned count, unsigned size )
{
   void *ptr = MALLOC( count * size );
   if( ptr ) {
      memset( ptr, 0, count * size );
   }
   return ptr;
}

static INLINE void
FREE( void *ptr )
{
   if( ptr ) {
      EngFreeMem( ptr );
   }
}

static INLINE void *
REALLOC( void *old_ptr, unsigned old_size, unsigned new_size )
{
   void *new_ptr;
   if( new_size <= old_size ) {
      return old_ptr;
   }
   new_ptr = MALLOC( new_size );
   if( new_ptr ) {
      memcpy( new_ptr, old_ptr, old_size );
   }
   FREE( old_ptr );
   return new_ptr;
}

#define GETENV( X )  NULL

#else // WIN32

#define MALLOC( SIZE )  malloc( SIZE )

#define CALLOC( COUNT, SIZE )   calloc( COUNT, SIZE )

#define FREE( PTR )  free( PTR )

#define REALLOC( OLDPTR, OLDSIZE, NEWSIZE )  realloc( OLDPTR, NEWSIZE )

#define GETENV( X )  getenv( X )

#endif // WIN32

#define CALLOC_STRUCT(T)   (struct T *) CALLOC(1, sizeof(struct T))

#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

#define Elements(x) sizeof(x)/sizeof(*(x))

/**
 * Return a pointer aligned to next multiple of 16 bytes.
 */
static INLINE void *
align16( void *unaligned )
{
   union {
      void *p;
      uint64 u;
   } pu;
   pu.p = unaligned;
   pu.u = (pu.u + 15) & ~15;
   return pu.p;
}


#if defined(__MSC__) && defined(__WIN32__)
static INLINE unsigned ffs( unsigned u )
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

union fi {
   float f;
   int i;
   unsigned ui;
};

#define UBYTE_TO_FLOAT( ub ) ((float)(ub) / 255.0F)

#define IEEE_0996 0x3f7f0000	/* 0.996 or so */

/* This function/macro is sensitive to precision.  Test very carefully
 * if you change it!
 */
#define UNCLAMPED_FLOAT_TO_UBYTE(UB, F)					\
        do {								\
           union fi __tmp;						\
           __tmp.f = (F);						\
           if (__tmp.i < 0)						\
              UB = (ubyte) 0;						\
           else if (__tmp.i >= IEEE_0996)				\
              UB = (ubyte) 255;					\
           else {							\
              __tmp.f = __tmp.f * (255.0f/256.0f) + 32768.0f;		\
              UB = (ubyte) __tmp.i;					\
           }								\
        } while (0)



static INLINE unsigned pack_ub4( unsigned char b0,
				 unsigned char b1,
				 unsigned char b2,
				 unsigned char b3 )
{
   return ((((unsigned int)b0) << 0) |
	   (((unsigned int)b1) << 8) |
	   (((unsigned int)b2) << 16) |
	   (((unsigned int)b3) << 24));
}

static INLINE unsigned fui( float f )
{
   union fi fi;
   fi.f = f;
   return fi.ui;
}

static INLINE unsigned char float_to_ubyte( float f )
{
   unsigned char ub;
   UNCLAMPED_FLOAT_TO_UBYTE(ub, f);
   return ub;
}

static INLINE unsigned pack_ui32_float4( float a,
					 float b, 
					 float d, 
					 float c )
{
   return pack_ub4( float_to_ubyte(a),
		    float_to_ubyte(b),
		    float_to_ubyte(c),
		    float_to_ubyte(d) );
}

#define COPY_4V( DST, SRC )         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
   (DST)[3] = (SRC)[3];             \
} while (0)


#define COPY_4FV( DST, SRC )  COPY_4V(DST, SRC)


#define ASSIGN_4V( DST, V0, V1, V2, V3 ) \
do {                                     \
   (DST)[0] = (V0);                      \
   (DST)[1] = (V1);                      \
   (DST)[2] = (V2);                      \
   (DST)[3] = (V3);                      \
} while (0)


static INLINE int ifloor(float f)
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


#if defined(__GNUC__) && defined(__i386__) 
static INLINE int iround(float f)
{
   int r;
   __asm__ ("fistpl %0" : "=m" (r) : "t" (f) : "st");
   return r;
}
#elif defined(__MSC__) && defined(__WIN32__)
static INLINE int iround(float f)
{
   int r;
   _asm {
	 fld f
	 fistp r
	}
   return r;
}
#else
#define IROUND(f)  ((int) (((f) >= 0.0F) ? ((f) + 0.5F) : ((f) - 0.5F)))
#endif


/* Could maybe have an inline version of this?
 */
#if defined(__GNUC__)
#define FABSF(x)   fabsf(x)
#else
#define FABSF(x)   ((float) fabs(x))
#endif

/* Pretty fast, and accurate.
 * Based on code from http://www.flipcode.com/totd/
 */
static INLINE float LOG2(float val)
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

#if defined(__GNUC__)
#define CEILF(x)   ceilf(x)
#else
#define CEILF(x)   ((float) ceil(x))
#endif

#endif
