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

#include "p_config.h"
#include "p_compiler.h"
#include "p_debug.h"
#include "p_pointer.h"
#include <math.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) && defined(DEBUG) 

/* memory debugging */

#include "p_debug.h"

#define MALLOC( _size ) \
   debug_malloc( __FILE__, __LINE__, __FUNCTION__, _size )
#define CALLOC( _count, _size ) \
   debug_calloc(__FILE__, __LINE__, __FUNCTION__, _count, _size )
#define FREE( _ptr ) \
   debug_free( __FILE__, __LINE__, __FUNCTION__,  _ptr )
#define REALLOC( _ptr, _old_size, _size ) \
   debug_realloc( __FILE__, __LINE__, __FUNCTION__,  _ptr, _old_size, _size )

#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

void * __stdcall
EngAllocMem(
    unsigned long Flags,
    unsigned long MemSize,
    unsigned long Tag );

void __stdcall
EngFreeMem(
    void *Mem );

#define MALLOC( _size ) EngAllocMem( 0, _size, 'D3AG' )
#define _FREE( _ptr ) EngFreeMem( _ptr )

#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)

void *
ExAllocatePool(
    unsigned long PoolType, 
    size_t NumberOfBytes);

void 
ExFreePool(void *P);

#define MALLOC(_size) ExAllocatePool(0, _size)
#define _FREE(_ptr) ExFreePool(_ptr)

#else

#define MALLOC( SIZE )  malloc( SIZE )
#define CALLOC( COUNT, SIZE )   calloc( COUNT, SIZE )
#define FREE( PTR )  free( PTR )
#define REALLOC( OLDPTR, OLDSIZE, NEWSIZE )  realloc( OLDPTR, NEWSIZE )

#endif


#ifndef CALLOC
static INLINE void *
CALLOC( unsigned count, unsigned size )
{
   void *ptr = MALLOC( count * size );
   if( ptr ) {
      memset( ptr, 0, count * size );
   }
   return ptr;
}
#endif /* !CALLOC */

#ifndef FREE
static INLINE void
FREE( void *ptr )
{
   if( ptr ) {
      _FREE( ptr );
   }
}
#endif /* !FREE */

#ifndef REALLOC
static INLINE void *
REALLOC( void *old_ptr, unsigned old_size, unsigned new_size )
{
   void *new_ptr = NULL;

   if (new_size != 0) {
      unsigned copy_size = old_size < new_size ? old_size : new_size;
      new_ptr = MALLOC( new_size );
      if (new_ptr && old_ptr && copy_size) {
         memcpy( new_ptr, old_ptr, copy_size );
      }
   }

   FREE( old_ptr );
   return new_ptr;
}
#endif /* !REALLOC */


#define MALLOC_STRUCT(T)   (struct T *) MALLOC(sizeof(struct T))

#define CALLOC_STRUCT(T)   (struct T *) CALLOC(1, sizeof(struct T))

#define GETENV( X ) debug_get_option( X, NULL )


/**
 * Return memory on given byte alignment
 */
static INLINE void *
align_malloc(size_t bytes, uint alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;
   (void) posix_memalign(& mem, alignment, bytes);
   return mem;
#else
   char *ptr, *buf;

   assert( alignment > 0 );

   ptr = (char *) MALLOC(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (char *) align_pointer( ptr + sizeof(void *), alignment );
   *(char **)(buf - sizeof(void *)) = ptr;

   return buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Free memory returned by align_malloc().
 */
static INLINE void
align_free(void *ptr)
{
#if defined(HAVE_POSIX_MEMALIGN)
   FREE(ptr);
#else
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   FREE(realAddr);
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}



/**
 * Duplicate a block of memory.
 */
static INLINE void *
mem_dup(const void *src, uint size)
{
   void *dup = MALLOC(size);
   if (dup)
      memcpy(dup, src, size);
   return dup;
}



#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

#ifndef Elements
#define Elements(x) (sizeof(x)/sizeof((x)[0]))
#endif
#define Offset(TYPE, MEMBER) ((unsigned)&(((TYPE *)NULL)->MEMBER))

/**
 * Return a pointer aligned to next multiple of 16 bytes.
 */
static INLINE void *
align16( void *unaligned )
{
   return align_pointer( unaligned, 16 );
}


static INLINE int align_int(int x, int align)
{
   return (x + align - 1) & ~(align - 1);
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
					 float c, 
					 float d )
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

static INLINE int align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}


/* util/p_util.c
 */
extern void pipe_copy_rect(ubyte * dst, unsigned cpp, unsigned dst_pitch,
                           unsigned dst_x, unsigned dst_y, unsigned width,
                           unsigned height, const ubyte * src,
                           int src_pitch, unsigned src_x, int src_y);



#if !defined(_MSC_VER) && _MSC_VER < 0x800
#if !defined(_INC_MATH) || !defined(__cplusplus)

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
   return (float) cos( (double) f );
}

#endif  /* _INC_MATH */
#endif


#ifdef __cplusplus
}
#endif

#endif
