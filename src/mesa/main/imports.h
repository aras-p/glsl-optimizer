/* $Id: imports.h,v 1.14 2003/03/02 19:36:09 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This file provides wrappers for all the standard C library functions
 * like malloc, free, printf, getenv, etc.
 */


#ifndef IMPORTS_H
#define IMPORTS_H


/* XXX some of the stuff in glheader.h should be moved into this file.
 */
#include "glheader.h"


/**********************************************************************
 * General macros
 */

#ifndef NULL
#define NULL 0
#endif



/**********************************************************************
 * Memory macros
 */

#define MALLOC(BYTES)      _mesa_malloc(BYTES)
#define CALLOC(BYTES)      _mesa_calloc(BYTES)
#define MALLOC_STRUCT(T)   (struct T *) _mesa_malloc(sizeof(struct T))
#define CALLOC_STRUCT(T)   (struct T *) _mesa_calloc(sizeof(struct T))
#define FREE(PTR)          _mesa_free(PTR)

#define ALIGN_MALLOC(BYTES, N)     _mesa_align_malloc(BYTES, N)
#define ALIGN_CALLOC(BYTES, N)     _mesa_align_calloc(BYTES, N)
#define ALIGN_MALLOC_STRUCT(T, N)  (struct T *) _mesa_align_malloc(sizeof(struct T), N)
#define ALIGN_CALLOC_STRUCT(T, N)  (struct T *) _mesa_align_calloc(sizeof(struct T), N)
#define ALIGN_FREE(PTR)            _mesa_align_free(PTR)

#define MEMCPY( DST, SRC, BYTES)   _mesa_memcpy(DST, SRC, BYTES)
#define MEMSET( DST, VAL, N )      _mesa_memset(DST, VAL, N)

#define MEMSET16( DST, VAL, N )   _mesa_memset16( (DST), (VAL), (size_t) (N) )


/* MACs and BeOS don't support static larger than 32kb, so... */
#if defined(macintosh) && !defined(__MRC__)
/*extern char *AGLAlloc(int size);*/
/*extern void AGLFree(char* ptr);*/
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE *NAME = (TYPE*)_mesa_alloc(sizeof(TYPE)*(SIZE))
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)		TYPE (*NAME)[SIZE2] = (TYPE(*)[SIZE2])_mesa_alloc(sizeof(TYPE)*(SIZE1)*(SIZE2))
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE (*NAME)[SIZE2][SIZE3] = (TYPE(*)[SIZE2][SIZE3])_mesa_alloc(sizeof(TYPE)*(SIZE1)*(SIZE2)*(SIZE3))

#  define CHECKARRAY(NAME,CMD)				do {if (!(NAME)) {CMD;}} while (0)
#  define UNDEFARRAY(NAME)          			do {if ((NAME)) {_mesa_free((char*)NAME);}  }while (0)
#elif defined(__BEOS__)
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE *NAME = (TYPE*)_mesa_malloc(sizeof(TYPE)*(SIZE))
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)  		TYPE (*NAME)[SIZE2] = (TYPE(*)[SIZE2])_mesa_malloc(sizeof(TYPE)*(SIZE1)*(SIZE2))
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE (*NAME)[SIZE2][SIZE3] = (TYPE(*)[SIZE2][SIZE3])_mesa_malloc(sizeof(TYPE)*(SIZE1)*(SIZE2)*(SIZE3))
#  define CHECKARRAY(NAME,CMD)				do {if (!(NAME)) {CMD;}} while (0)
#  define UNDEFARRAY(NAME)          			do {if ((NAME)) {_mesa_free((char*)NAME);}  }while (0)
#else
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE NAME[SIZE]
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)		TYPE NAME[SIZE1][SIZE2]
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE NAME[SIZE1][SIZE2][SIZE3]
#  define CHECKARRAY(NAME,CMD)				do {} while(0)
#  define UNDEFARRAY(NAME)
#endif


#ifdef MESA_EXTERNAL_BUFFERALLOC
/*
 * If you want Mesa's depth/stencil/accum/etc buffers to be allocated
 * with a specialized allocator you can define MESA_EXTERNAL_BUFFERALLOC
 * and implement _ext_mesa_alloc/free_pixelbuffer() in your app.
 * Contributed by Gerk Huisma (gerk@five-d.demon.nl).
 */
extern void *_ext_mesa_alloc_pixelbuffer( unsigned int size );
extern void _ext_mesa_free_pixelbuffer( void *pb );

#define MESA_PBUFFER_ALLOC(BYTES)  (void *) _ext_mesa_alloc_pixelbuffer(BYTES)
#define MESA_PBUFFER_FREE(PTR)     _ext_mesa_free_pixelbuffer(PTR)
#else
/* Default buffer allocation uses the aligned allocation routines: */
#define MESA_PBUFFER_ALLOC(BYTES)  (void *) _mesa_align_malloc(BYTES, 512)
#define MESA_PBUFFER_FREE(PTR)     _mesa_align_free(PTR)
#endif



/**********************************************************************
 * Math macros
 */

#define MAX_GLUSHORT	0xffff
#define MAX_GLUINT	0xffffffff

#ifndef M_PI
#define M_PI (3.1415926)
#endif

/* Degrees to radians conversion: */
#define DEG2RAD (M_PI/180.0)


/***
 *** USE_IEEE: Determine if we're using IEEE floating point
 ***/
#if defined(__i386__) || defined(__sparc__) || defined(__s390x__) || \
    defined(__powerpc__) || \
    ( defined(__alpha__) && ( defined(__IEEE_FLOAT) || !defined(VMS) ) )
#define USE_IEEE
#define IEEE_ONE 0x3f800000
#endif


/***
 *** SQRTF: single-precision square root
 ***/
#ifdef DEBUG
#  define SQRTF(X)  ((float)_mesa_sqrtd((float) X))
#elif defined(__WATCOMC__) && defined(USE_X86_ASM)
float asm_sqrt (float x);
#pragma aux asm_sqrt =                      \
	"fsqrt"                             \
	parm [8087]                         \
	value [8087]                        \
	modify exact [];
#  define SQRTF(X)  asm_sqrt(X)
#else
#  define SQRTF(X)  _mesa_sqrtf(X)
#endif


/***
 *** LOG2: Log base 2 of float
 ***/
#ifdef USE_IEEE
#if 0
/* This is pretty fast, but not accurate enough (only 2 fractional bits).
 * Based on code from http://www.stereopsis.com/log2.html
 */
static INLINE GLfloat LOG2(GLfloat x)
{
   const GLfloat y = x * x * x * x;
   const GLuint ix = *((GLuint *) &y);
   const GLuint exp = (ix >> 23) & 0xFF;
   const GLint log2 = ((GLint) exp) - 127;
   return (GLfloat) log2 * (1.0 / 4.0);  /* 4, because of x^4 above */
}
#endif
/* Pretty fast, and accurate.
 * Based on code from http://www.flipcode.com/totd/
 */
static INLINE GLfloat LOG2(GLfloat val)
{
   GLint *exp_ptr = (GLint *) &val;
   GLint x = *exp_ptr;
   const GLint log_2 = ((x >> 23) & 255) - 128;
   x &= ~(255 << 23);
   x += 127 << 23;
   *exp_ptr = x;
   val = ((-1.0f/3) * val + 2) * val - 2.0f/3;
   return val + log_2;
}
#elif defined(XFree86LOADER) && defined(IN_MODULE)
#define LOG2(x) ((GLfloat) (xf86log(x) * 1.442695))
#else
/*
 * NOTE: log_base_2(x) = log(x) / log(2)
 * NOTE: 1.442695 = 1/log(2).
 */
#define LOG2(x)  ((GLfloat) (log(x) * 1.442695F))
#endif


/***
 *** IS_INF_OR_NAN: test if float is infinite or NaN
 ***/
#ifdef USE_IEEE
static INLINE int IS_INF_OR_NAN( float x )
{
   fi_type tmp;
   tmp.f = x;
   return !(int)((unsigned int)((tmp.i & 0x7fffffff)-0x7f800000) >> 31);
}
#elif defined(isfinite)
#define IS_INF_OR_NAN(x)        (!isfinite(x))
#elif defined(finite)
#define IS_INF_OR_NAN(x)        (!finite(x))
#elif __VMS
#define IS_INF_OR_NAN(x)        (!finite(x))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IS_INF_OR_NAN(x)        (!isfinite(x))
#else
#define IS_INF_OR_NAN(x)        (!finite(x))
#endif 


/***
 *** IS_NEGATIVE: test if float is negative
 ***/
#if defined(USE_IEEE)
#define GET_FLOAT_BITS(x) ((fi_type *) &(x))->i
#define IS_NEGATIVE(x) (GET_FLOAT_BITS(x) & (1<<31))
#else
#define IS_NEGATIVE(x) (x < 0.0F)
#endif


/***
 *** DIFFERENT_SIGNS: test if two floats have opposite signs
 ***/
#if defined(USE_IEEE)
#define DIFFERENT_SIGNS(x,y) ((GET_FLOAT_BITS(x) ^ GET_FLOAT_BITS(y)) & (1<<31))
#else
/* Could just use (x*y<0) except for the flatshading requirements.
 * Maybe there's a better way?
 */
#define DIFFERENT_SIGNS(x,y) ((x) * (y) <= 0.0F && (x) - (y) != 0.0F)
#endif


/***
 *** CEILF: ceiling of float
 *** FLOORF: floor of float
 *** FABSF: absolute value of float
 ***/
#if defined(__sparc__) || defined(__NeXT__) /* XXX improve? */
#define CEILF(x)   ceil(x)
#define FLOORF(x)  floor(x)
#define FABSF(x)   fabs(x)
#elif defined(__WIN32__) || defined(__IBMC__) || defined(__IBMCPP__)
#define CEILF(x)   ((GLfloat) ceil(x))
#define FLOORF(x)  ((GLfloat) floor(x))
#define FABSF(x)   ((GLfloat) fabs(x))
#elif defined(XFree86LOADER) && defined(IN_MODULE)
#define CEILF(x)   ((GLfloat) xf86ceil(x))
#define FLOORF(x)  ((GLfloat) xf86floor(x))
#define FABSF(x)   ((GLfloat) xf86fabs(x))
#else
#define CEILF(x)   ceilf(x)
#define FLOORF(x)  floorf(x)
#define FABSF(x)   fabsf(x)
#endif


/***
 *** IROUND: return (as an integer) float rounded to nearest integer
 ***/
#if defined(USE_SPARC_ASM) && defined(__GNUC__) && defined(__sparc__)
static INLINE int iround(float f)
{
       int r;
       __asm__ ("fstoi %1, %0" : "=f" (r) : "f" (f));
       return r;
}
#define IROUND(x)  iround(x)
#elif defined(USE_X86_ASM) && defined(__GNUC__) && defined(__i386__)
static INLINE int iround(float f)
{
   int r;
   __asm__ ("fistpl %0" : "=m" (r) : "t" (f) : "st");
   return r;
}
#define IROUND(x)  iround(x)
#elif defined(USE_X86_ASM) && defined(__MSC__) && defined(__WIN32__)
static INLINE int iround(float f)
{
   int r;
   _asm {
	 fld f
	 fistp r
	}
   return r;
}
#define IROUND(x)  iround(x)
#elif defined(USE_X86_ASM) && defined(__WATCOMC__)
long iround(float f);
#pragma aux iround =                        \
	"push   eax"                        \
	"fistp  dword ptr [esp]"            \
	"pop    eax"                        \
	parm [8087]                         \
	value [eax]                         \
	modify exact [eax];

#define IROUND(x)  iround(x)
#else
#define IROUND(f)  ((int) (((f) >= 0.0F) ? ((f) + 0.5F) : ((f) - 0.5F)))
#endif


/***
 *** IROUND_POS: return (as an integer) positive float rounded to nearest int
 ***/
#ifdef DEBUG
#define IROUND_POS(f) (assert((f) >= 0.0F), IROUND(f))
#else
#define IROUND_POS(f) (IROUND(f))
#endif


/***
 *** IFLOOR: return (as an integer) floor of float
 ***/
#if defined(USE_X86_ASM) && defined(__GNUC__) && defined(__i386__)
/*
 * IEEE floor for computers that round to nearest or even.
 * 'f' must be between -4194304 and 4194303.
 * This floor operation is done by "(iround(f + .5) + iround(f - .5)) >> 1",
 * but uses some IEEE specific tricks for better speed.
 * Contributed by Josh Vanderhoof
 */
static INLINE int ifloor(float f)
{
   int ai, bi;
   double af, bf;
   af = (3 << 22) + 0.5 + (double)f;
   bf = (3 << 22) + 0.5 - (double)f;
   /* GCC generates an extra fstp/fld without this. */
   __asm__ ("fstps %0" : "=m" (ai) : "t" (af) : "st");
   __asm__ ("fstps %0" : "=m" (bi) : "t" (bf) : "st");
   return (ai - bi) >> 1;
}
#define IFLOOR(x)  ifloor(x)
#elif defined(USE_IEEE)
static INLINE int ifloor(float f)
{
   int ai, bi;
   double af, bf;
   fi_type u;

   af = (3 << 22) + 0.5 + (double)f;
   bf = (3 << 22) + 0.5 - (double)f;
   u.f = af; ai = u.i;
   u.f = bf; bi = u.i;
   return (ai - bi) >> 1;
}
#define IFLOOR(x)  ifloor(x)
#else
static INLINE int ifloor(float f)
{
   int i = IROUND(f);
   return (i > f) ? i - 1 : i;
}
#define IFLOOR(x)  ifloor(x)
#endif


/***
 *** ICEIL: return (as an integer) ceiling of float
 ***/
#if defined(USE_X86_ASM) && defined(__GNUC__) && defined(__i386__)
/*
 * IEEE ceil for computers that round to nearest or even.
 * 'f' must be between -4194304 and 4194303.
 * This ceil operation is done by "(iround(f + .5) + iround(f - .5) + 1) >> 1",
 * but uses some IEEE specific tricks for better speed.
 * Contributed by Josh Vanderhoof
 */
static INLINE int iceil(float f)
{
   int ai, bi;
   double af, bf;
   af = (3 << 22) + 0.5 + (double)f;
   bf = (3 << 22) + 0.5 - (double)f;
   /* GCC generates an extra fstp/fld without this. */
   __asm__ ("fstps %0" : "=m" (ai) : "t" (af) : "st");
   __asm__ ("fstps %0" : "=m" (bi) : "t" (bf) : "st");
   return (ai - bi + 1) >> 1;
}
#define ICEIL(x)  iceil(x)
#elif defined(USE_IEEE)
static INLINE int iceil(float f)
{
   int ai, bi;
   double af, bf;
   fi_type u;
   af = (3 << 22) + 0.5 + (double)f;
   bf = (3 << 22) + 0.5 - (double)f;
   u.f = af; ai = u.i;
   u.f = bf; bi = u.i;
   return (ai - bi + 1) >> 1;
}
#define ICEIL(x)  iceil(x)
#else
static INLINE int iceil(float f)
{
   int i = IROUND(f);
   return (i < f) ? i + 1 : i;
}
#define ICEIL(x)  iceil(x)
#endif


/***
 *** UNCLAMPED_FLOAT_TO_UBYTE: map float from {0,1} to ubyte in [0,255]
 *** CLAMPED_FLOAT_TO_UBYTE: map float in [0,1] to ubyte in [0,255]
 ***/
#if defined(USE_IEEE) && !defined(DEBUG)
#define IEEE_0996 0x3f7f0000	/* 0.996 or so */
/* This function/macro is sensitive to precision.  Test very carefully
 * if you change it!
 */
#define UNCLAMPED_FLOAT_TO_UBYTE(UB, F)					\
        do {								\
           fi_type __tmp;						\
           __tmp.f = (F);						\
           UB = ((__tmp.i >= IEEE_0996)					\
               ? ((GLint)__tmp.i < 0) ? (GLubyte)0 : (GLubyte)255	\
               : (__tmp.f = __tmp.f*(255.0F/256.0F) + 32768.0F,		\
                  (GLubyte)__tmp.i));					\
        } while (0)
#define CLAMPED_FLOAT_TO_UBYTE(ub, f) \
        UNCLAMPED_FLOAT_TO_UBYTE(ub, f)
#else
#define UNCLAMPED_FLOAT_TO_UBYTE(ub, f) \
	ub = ((GLubyte) IROUND(CLAMP((f), 0.0F, 1.0F) * 255.0F))
#define CLAMPED_FLOAT_TO_UBYTE(ub, f) \
	ub = ((GLubyte) IROUND((f) * 255.0F))
#endif


/***
 *** COPY_FLOAT: copy a float from src to dest, avoid slow FP regs if possible
 ***/
#if defined(USE_IEEE) && !defined(DEBUG)
#define COPY_FLOAT( dst, src )					\
	((fi_type *) &(dst))->i = ((fi_type *) &(src))->i
#else
#define COPY_FLOAT( dst, src )		(dst) = (src)
#endif


/***
 *** START_FAST_MATH: Set x86 FPU to faster, 32-bit precision mode (and save
 ***                  original mode to a temporary).
 *** END_FAST_MATH: Restore x86 FPU to original mode.
 ***/
#if defined(__GNUC__) && defined(__i386__)
/*
 * Set the x86 FPU control word to guarentee only 32 bits of precision
 * are stored in registers.  Allowing the FPU to store more introduces
 * differences between situations where numbers are pulled out of memory
 * vs. situations where the compiler is able to optimize register usage.
 *
 * In the worst case, we force the compiler to use a memory access to
 * truncate the float, by specifying the 'volatile' keyword.
 */
/* Hardware default: All exceptions masked, extended double precision,
 * round to nearest (IEEE compliant):
 */
#define DEFAULT_X86_FPU		0x037f
/* All exceptions masked, single precision, round to nearest:
 */
#define FAST_X86_FPU		0x003f
/* The fldcw instruction will cause any pending FP exceptions to be
 * raised prior to entering the block, and we clear any pending
 * exceptions before exiting the block.  Hence, asm code has free
 * reign over the FPU while in the fast math block.
 */
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x)						\
do {									\
   static GLuint mask = DEFAULT_X86_FPU;				\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#else
#define START_FAST_MATH(x)						\
do {									\
   static GLuint mask = FAST_X86_FPU;					\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#endif
/* Restore original FPU mode, and clear any exceptions that may have
 * occurred in the FAST_MATH block.
 */
#define END_FAST_MATH(x)						\
do {									\
   __asm__ ( "fnclex ; fldcw %0" : : "m" (*&(x)) );			\
} while (0)

#elif defined(__WATCOMC__) && !defined(NO_FAST_MATH)
void _wacom_start_fast_math(unsigned short *x);
#pragma aux _wacom_start_fast_math =    \
    "fstcw   word ptr [esi]"            \
    "or      word ptr [esi], 0x3f"      \
    "fldcw   word ptr [esi]"            \
    parm [esi]                          \
    modify exact [];
void _wacom_end_fast_math(unsigned short *x);
#pragma aux _wacom_end_fast_math =      \
    "fldcw   word ptr [esi]"            \
    parm [esi]                          \
    modify exact [];
#define START_FAST_MATH(x)  _wacom_start_fast_math(& x)
#define END_FAST_MATH(x)  _wacom_end_fast_math(& x)
#else
#define START_FAST_MATH(x)  x = 0
#define END_FAST_MATH(x)  (void)(x)
#endif



/**********************************************************************
 * Functions
 */

extern void *
_mesa_malloc( size_t bytes );

extern void *
_mesa_calloc( size_t bytes );

extern void
_mesa_free( void *ptr );

extern void *
_mesa_align_malloc( size_t bytes, unsigned long alignment );

extern void *
_mesa_align_calloc( size_t bytes, unsigned long alignment );

extern void
_mesa_align_free( void *ptr );

extern void *
_mesa_memcpy( void *dest, const void *src, size_t n );

extern void
_mesa_memset( void *dst, int val, size_t n );

extern void
_mesa_memset16( unsigned short *dst, unsigned short val, size_t n );

extern void
_mesa_bzero( void *dst, size_t n );


extern double
_mesa_sin(double a);

extern double
_mesa_cos(double a);

extern double
_mesa_sqrtd(double x);

extern float
_mesa_sqrtf(float x);

extern double
_mesa_pow(double x, double y);

extern float
_mesa_log2(float x);

extern unsigned int
_mesa_bitcount(unsigned int n);


extern char *
_mesa_getenv( const char *var );

extern char *
_mesa_strstr( const char *haystack, const char *needle );

extern char *
_mesa_strncat( char *dest, const char *src, size_t n );

extern char *
_mesa_strcpy( char *dest, const char *src );

extern char *
_mesa_strncpy( char *dest, const char *src, size_t n );

extern size_t
_mesa_strlen( const char *s );

extern int
_mesa_strcmp( const char *s1, const char *s2 );

extern int
_mesa_strncmp( const char *s1, const char *s2, size_t n );

extern char *
_mesa_strdup( const char *s );

extern int
_mesa_atoi( const char *s );

extern double
_mesa_strtod( const char *s, char **end );

extern int
_mesa_sprintf( char *str, const char *fmt, ... );

extern void
_mesa_printf( const char *fmtString, ... );


extern void
_mesa_warning( __GLcontext *gc, const char *fmtString, ... );

extern void
_mesa_problem( const __GLcontext *ctx, const char *fmtString, ... );

extern void
_mesa_error( __GLcontext *ctx, GLenum error, const char *fmtString, ... );

extern void
_mesa_debug( const __GLcontext *ctx, const char *fmtString, ... );


extern void
_mesa_init_default_imports( __GLimports *imports, void *driverCtx );


#endif /* IMPORTS_H */

