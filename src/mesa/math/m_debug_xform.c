/* $Id: m_debug_xform.c,v 1.1 2000/11/16 21:05:41 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * Updated for P6 architecture by Gareth Hughes.
 */

#include "glheader.h"
#include "context.h"
#include "mem.h"

#include "m_debug_xform.h"
#include "m_matrix.h"
#include "m_xform.h"


#ifdef DEBUG  /* This code only used for debugging */


/* Comment this out to deactivate the cycle counter.
 * NOTE: it works only on CPUs which know the 'rdtsc' command (586 or higher)
 * (hope, you don't try to debug Mesa on a 386 ;)
 */
#if defined(__GNUC__) && defined(__i386__) && defined(USE_X86_ASM)
#define  RUN_XFORM_BENCHMARK
#endif

#define TEST_COUNT		128	/* size of the tested vector array   */

#define REQUIRED_PRECISION	10	/* allow 4 bits to miss              */
#define MAX_PRECISION		24	/* max. precision possible           */


#ifdef  RUN_XFORM_BENCHMARK
/* Overhead of profiling counter in cycles.  Automatically adjusted to
 * your machine at run time - counter initialization should give very
 * consistent results.
 */
static int need_counter = 1;
static long counter_overhead = 0;

/* Modify the the number of tests if you like.
 * We take the minimum of all results, because every error should be
 * positive (time used by other processes, task switches etc).
 * It is assumed that all calculations are done in the cache.
 */

#if 1 /* PPro, PII, PIII version */

/* Profiling on the P6 architecture requires a little more work, due to
 * the internal out-of-order execution.  We must perform a serializing
 * 'cpuid' instruction before and after the 'rdtsc' instructions to make
 * sure no other uops are executed when we sample the timestamp counter.
 */
#define  INIT_COUNTER()							\
   do {									\
      int cycle_i;							\
      counter_overhead = LONG_MAX;					\
      for ( cycle_i = 0 ; cycle_i < 4 ; cycle_i++ ) {			\
	 long cycle_tmp1 = 0, cycle_tmp2 = 0;				\
	 __asm__ ( "push %%ebx       \n"				\
		   "xor %%eax, %%eax \n"				\
		   "cpuid            \n"				\
		   "rdtsc            \n"				\
		   "mov %%eax, %0    \n"				\
		   "xor %%eax, %%eax \n"				\
		   "cpuid            \n"				\
		   "pop %%ebx        \n"				\
		   "push %%ebx       \n"				\
		   "xor %%eax, %%eax \n"				\
		   "cpuid            \n"				\
		   "rdtsc            \n"				\
		   "mov %%eax, %1    \n"				\
		   "xor %%eax, %%eax \n"				\
		   "cpuid            \n"				\
		   "pop %%ebx        \n"				\
		   : "=m" (cycle_tmp1), "=m" (cycle_tmp2)		\
		   : : "eax", "ecx", "edx" );				\
	 if ( counter_overhead > (cycle_tmp2 - cycle_tmp1) ) {		\
	    counter_overhead = cycle_tmp2 - cycle_tmp1;			\
	 }								\
      }									\
   } while (0)

#define  BEGIN_RACE(x)							\
   x = LONG_MAX;							\
   for ( cycle_i = 0 ; cycle_i < 10 ; cycle_i++ ) {			\
      long cycle_tmp1 = 0, cycle_tmp2 = 0;				\
      __asm__ ( "push %%ebx       \n"					\
		"xor %%eax, %%eax \n"					\
		"cpuid            \n"					\
		"rdtsc            \n"					\
		"mov %%eax, %0    \n"					\
		"xor %%eax, %%eax \n"					\
		"cpuid            \n"					\
		"pop %%ebx        \n"					\
		: "=m" (cycle_tmp1)					\
		: : "eax", "ecx", "edx" );

#define END_RACE(x)							\
      __asm__ ( "push %%ebx       \n"					\
		"xor %%eax, %%eax \n"					\
		"cpuid            \n"					\
		"rdtsc            \n"					\
		"mov %%eax, %0    \n"					\
		"xor %%eax, %%eax \n"					\
		"cpuid            \n"					\
		"pop %%ebx        \n"					\
		: "=m" (cycle_tmp2)					\
		: : "eax", "ecx", "edx" );				\
      if ( x > (cycle_tmp2 - cycle_tmp1) ) {				\
	 x = cycle_tmp2 - cycle_tmp1;					\
      }									\
   }									\
   x -= counter_overhead;

#else /* PPlain, PMMX version */

/* To ensure accurate results, we stall the pipelines with the
 * non-pairable 'cdq' instruction.  This ensures all the code being
 * profiled is complete when the 'rdtsc' instruction executes.
 */
#define  INIT_COUNTER(x)						\
   do {									\
      int cycle_i;							\
      x = LONG_MAX;							\
      for ( cycle_i = 0 ; cycle_i < 32 ; cycle_i++ ) {			\
	 long cycle_tmp1, cycle_tmp2, dummy;				\
	 __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp1) );		\
	 __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp2) );		\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "rdtsc" : "=a" (cycle_tmp1), "=d" (dummy) );		\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "cdq" );						\
	 __asm__ ( "rdtsc" : "=a" (cycle_tmp2), "=d" (dummy) );		\
	 if ( x > (cycle_tmp2 - cycle_tmp1) )				\
	    x = cycle_tmp2 - cycle_tmp1;				\
      }									\
   } while (0)

#define  BEGIN_RACE(x)							\
   x = LONG_MAX;							\
   for ( cycle_i = 0 ; cycle_i < 16 ; cycle_i++ ) {			\
      long cycle_tmp1, cycle_tmp2, dummy;				\
      __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp1) );			\
      __asm__ ( "mov %%eax, %0" : "=a" (cycle_tmp2) );			\
      __asm__ ( "cdq" );						\
      __asm__ ( "cdq" );						\
      __asm__ ( "rdtsc" : "=a" (cycle_tmp1), "=d" (dummy) );


#define END_RACE(x)							\
      __asm__ ( "cdq" );						\
      __asm__ ( "cdq" );						\
      __asm__ ( "rdtsc" : "=a" (cycle_tmp2), "=d" (dummy) );		\
      if ( x > (cycle_tmp2 - cycle_tmp1) )				\
	 x = cycle_tmp2 - cycle_tmp1;					\
   }									\
   x -= counter_overhead;

#endif

#else

#define BEGIN_RACE(x)
#define END_RACE(x)

#endif


static char *mesa_profile = NULL;


enum { NIL=0, ONE=1, NEG=-1, VAR=2 };

static int m_general[16] = {
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR
};
static int m_identity[16] = {
   ONE, NIL, NIL, NIL,
   NIL, ONE, NIL, NIL,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int  m_2d[16]  = {
   VAR, VAR, NIL, VAR,
   VAR, VAR, NIL, VAR,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int m_2d_no_rot[16] = {
   VAR, NIL, NIL, VAR,
   NIL, VAR, NIL, VAR,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, ONE
};
static int m_3d[16] = {
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   VAR, VAR, VAR, VAR,
   NIL, NIL, NIL, ONE
};
static int m_3d_no_rot[16] = {
   VAR, NIL, NIL, VAR,
   NIL, VAR, NIL, VAR,
   NIL, NIL, VAR, VAR,
   NIL, NIL, NIL, ONE
};
static int m_perspective[16] = {
   VAR, NIL, VAR, NIL,
   NIL, VAR, VAR, NIL,
   NIL, NIL, VAR, VAR,
   NIL, NIL, NEG, NIL
};
static int *templates[7] = {
   m_general,
   m_identity,
   m_3d_no_rot,
   m_perspective,
   m_2d,
   m_2d_no_rot,
   m_3d
};
static int mtypes[7] = {
   MATRIX_GENERAL,
   MATRIX_IDENTITY,
   MATRIX_3D_NO_ROT,
   MATRIX_PERSPECTIVE,
   MATRIX_2D,
   MATRIX_2D_NO_ROT,
   MATRIX_3D
};
static char *mstrings[7] = {
   "MATRIX_GENERAL",
   "MATRIX_IDENTITY",
   "MATRIX_3D_NO_ROT",
   "MATRIX_PERSPECTIVE",
   "MATRIX_2D",
   "MATRIX_2D_NO_ROT",
   "MATRIX_3D"
};



static int m_norm_identity[16] = {
   ONE, NIL, NIL, NIL,
   NIL, ONE, NIL, NIL,
   NIL, NIL, ONE, NIL,
   NIL, NIL, NIL, NIL
};
static int m_norm_general[16] = {
   VAR, VAR, VAR, NIL,
   VAR, VAR, VAR, NIL,
   VAR, VAR, VAR, NIL,
   NIL, NIL, NIL, NIL
};
static int m_norm_no_rot[16] = {
   VAR, NIL, NIL, NIL,
   NIL, VAR, NIL, NIL,
   NIL, NIL, VAR, NIL,
   NIL, NIL, NIL, NIL
};
static int *norm_templates[8] = {
   m_norm_no_rot,
   m_norm_no_rot,
   m_norm_no_rot,
   m_norm_general,
   m_norm_general,
   m_norm_general,
   m_norm_identity,
   m_norm_identity
};
static int norm_types[8] = {
   NORM_TRANSFORM_NO_ROT,
   NORM_TRANSFORM_NO_ROT | NORM_RESCALE,
   NORM_TRANSFORM_NO_ROT | NORM_NORMALIZE,
   NORM_TRANSFORM,
   NORM_TRANSFORM | NORM_RESCALE,
   NORM_TRANSFORM | NORM_NORMALIZE,
   NORM_RESCALE,
   NORM_NORMALIZE
};
static int norm_scale_types[8] = {               /*  rescale factor          */
   NIL,                                          /*  NIL disables rescaling  */
   VAR,
   NIL,
   NIL,
   VAR,
   NIL,
   VAR,
   NIL
};
static int norm_normalize_types[8] = {           /*  normalizing ?? (no = 0) */
   0,
   0,
   1,
   0,
   0,
   1,
   0,
   1
};
static char *norm_strings[8] = {
   "NORM_TRANSFORM_NO_ROT",
   "NORM_TRANSFORM_NO_ROT | NORM_RESCALE",
   "NORM_TRANSFORM_NO_ROT | NORM_NORMALIZE",
   "NORM_TRANSFORM",
   "NORM_TRANSFORM | NORM_RESCALE",
   "NORM_TRANSFORM | NORM_NORMALIZE",
   "NORM_RESCALE",
   "NORM_NORMALIZE"
};



/* ================================================================
 * Helper functions
 */

static GLfloat rnd( void )
{
   GLfloat f = (GLfloat)rand() / (GLfloat)RAND_MAX;
   GLfloat gran = (GLfloat)(1 << 13);

   f = (GLfloat)(GLint)(f * gran) / gran;

   return f * 2.0 - 1.0;
}

static int significand_match( GLfloat a, GLfloat b )
{
   GLfloat d = a - b;
   int a_ex, b_ex, d_ex;

   if ( d == 0.0F ) {
      return MAX_PRECISION;   /* Exact match */
   }

   if ( a == 0.0F || b == 0.0F ) {
      /* It would probably be better to check if the
       * non-zero number is denormalized and return
       * the index of the highest set bit here.
       */
      return 0;
   }

   frexp( a, &a_ex );
   frexp( b, &b_ex );
   frexp( d, &d_ex );

   if ( a_ex < b_ex )
      return a_ex - d_ex;
   else
      return b_ex - d_ex;
}



/* ================================================================
 * Reference transformations
 */

static void ref_transform( GLvector4f *dst,
                           const GLmatrix *mat,
                           const GLvector4f *src,
                           const GLubyte *clipmask,
                           const GLubyte flag )
{
   int i;
   GLfloat *s = (GLfloat *)src->start;
   GLfloat (*d)[4] = (GLfloat (*)[4])dst->start;
   const GLfloat *m = mat->m;

   (void) clipmask;
   (void) flag;

   for ( i = 0 ; i < src->count ; i++ ) {
      GLfloat x = s[0], y = s[1], z = s[2], w = s[3];
      d[i][0] = m[0]*x + m[4]*y + m[ 8]*z + m[12]*w;
      d[i][1] = m[1]*x + m[5]*y + m[ 9]*z + m[13]*w;
      d[i][2] = m[2]*x + m[6]*y + m[10]*z + m[14]*w;
      d[i][3] = m[3]*x + m[7]*y + m[11]*z + m[15]*w;
      s = (GLfloat *)((char *)s + src->stride);
   }
}

static void ref_norm_transform_rescale( const GLmatrix *mat,
					GLfloat scale,
					const GLvector3f *in,
					const GLfloat *lengths,
					const GLubyte mask[],
					GLvector3f *dest )
{
   int i;
   const GLfloat *s = in->start;
   const GLfloat *m = mat->inv;
   GLfloat (*out)[3] = (GLfloat (*)[3])dest->start;

   (void) mask;
   (void) lengths;

   for ( i = 0 ; i < in->count ; i++ ) {
      GLfloat x = s[0], y = s[1], z = s[2] ;
      GLfloat tx = m[0]*x + m[1]*y + m[ 2]*z ;
      GLfloat ty = m[4]*x + m[5]*y + m[ 6]*z ;
      GLfloat tz = m[8]*x + m[9]*y + m[10]*z ;

      out[i][0] = tx * scale;
      out[i][1] = ty * scale;
      out[i][2] = tz * scale;

      s = (GLfloat *)((char *)s + in->stride);
   }
}

static void ref_norm_transform_normalize( const GLmatrix *mat,
					  GLfloat scale,
					  const GLvector3f *in,
					  const GLfloat *lengths,
					  const GLubyte mask[],
					  GLvector3f *dest )
{
   int i;
   const GLfloat *s = in->start;
   const GLfloat *m = mat->inv;
   GLfloat (*out)[3] = (GLfloat (*)[3])dest->start;

   (void) mask;

   for ( i = 0 ; i < in->count ; i++ ) {
      GLfloat x = s[0], y = s[1], z = s[2] ;
      GLfloat tx = m[0]*x + m[1]*y + m[ 2]*z ;
      GLfloat ty = m[4]*x + m[5]*y + m[ 6]*z ;
      GLfloat tz = m[8]*x + m[9]*y + m[10]*z ;

      if ( !lengths ) {
         GLfloat len = tx*tx + ty*ty + tz*tz;
         if ( len > 1e-20 ) {
	    /* Hmmm, don't know how we could test the precalculated
	     * length case...
	     */
            scale = 1.0 / sqrt( len );
            out[i][0] = tx * scale;
            out[i][1] = ty * scale;
            out[i][2] = tz * scale;
         } else {
            out[i][0] = out[i][1] = out[i][2] = 0;
         }
      } else {
         scale = lengths[i];;
         out[i][0] = tx * scale;
         out[i][1] = ty * scale;
         out[i][2] = tz * scale;
      }

      s = (GLfloat *)((char *)s + in->stride);
   }
}



/* ================================================================
 * Vertex transformation tests
 */

/* Ensure our arrays are correctly aligned.
 */
#if defined(__GNUC__)
#define ALIGN16(x)		x __attribute__ ((aligned (16)))
#else
#define ALIGN16(x)		x
#endif
static GLfloat ALIGN16(s[TEST_COUNT][5]);
static GLfloat ALIGN16(d[TEST_COUNT][4]);
static GLfloat ALIGN16(r[TEST_COUNT][4]);

static int test_transform_function( transform_func func, int psize, int mtype,
                                    int masked, long *cycles )
{
   GLvector4f source[1], dest[1], ref[1];
   GLmatrix mat[1];
   GLfloat *m;
   GLubyte mask[TEST_COUNT];
   int i, j;
#ifdef  RUN_XFORM_BENCHMARK
   int cycle_i;                /* the counter for the benchmarks we run */
#endif

   (void) cycles;

   if ( psize > 4 ) {
      gl_problem( NULL, "test_transform_function called with psize > 4\n" );
      return 0;
   }

   mat->m = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
   mat->type = mtypes[mtype];

   m = mat->m;

   m[0] = 63.0; m[4] = 43.0; m[ 8] = 29.0; m[12] = 43.0;
   m[1] = 55.0; m[5] = 17.0; m[ 9] = 31.0; m[13] =  7.0;
   m[2] = 44.0; m[6] =  9.0; m[10] =  7.0; m[14] =  3.0;
   m[3] = 11.0; m[7] = 23.0; m[11] = 91.0; m[15] =  9.0;

   for ( i = 0 ; i < 4 ; i++ ) {
      for ( j = 0 ; j < 4 ; j++ ) {
         switch ( templates[mtype][i * 4 + j] ) {
         case NIL:
            m[j * 4 + i] = 0.0;
            break;
         case ONE:
            m[j * 4 + i] = 1.0;
            break;
         case NEG:
            m[j * 4 + i] = -1.0;
            break;
         case VAR:
            break;
         default:
            abort();
         }
      }
   }

   for ( i = 0 ; i < TEST_COUNT ; i++) {
      mask[i] = i % 2;				/* mask every 2nd element */
      d[i][0] = s[i][0] = 0.0;
      d[i][1] = s[i][1] = 0.0;
      d[i][2] = s[i][2] = 0.0;
      d[i][3] = s[i][3] = 1.0;
      for ( j = 0 ; j < psize ; j++ )
         s[i][j] = rnd();
   }

   source->data = (GLfloat(*)[4])s;
   source->start = (GLfloat *)s;
   source->count = TEST_COUNT;
   source->stride = sizeof(s[0]);
   source->size = 4;
   source->flags = 0;

   dest->data = (GLfloat(*)[4])d;
   dest->start = (GLfloat *)d;
   dest->count = TEST_COUNT;
   dest->stride = sizeof(float[4]);
   dest->size = 0;
   dest->flags = 0;

   ref->data = (GLfloat(*)[4])r;
   ref->start = (GLfloat *)r;
   ref->count = TEST_COUNT;
   ref->stride = sizeof(float[4]);
   ref->size = 0;
   ref->flags = 0;

   ref_transform( ref, mat, source, NULL, 0 );

   if ( mesa_profile ) {
      if ( masked ) {
         BEGIN_RACE( *cycles );
         func( dest, mat->m, source, mask, 1 );
         END_RACE( *cycles );
      } else {
         BEGIN_RACE( *cycles );
         func( dest, mat->m, source, NULL, 0 );
         END_RACE( *cycles );
     }
   }
   else {
      if ( masked ) {
         func( dest, mat->m, source, mask, 1 );
      } else {
         func( dest, mat->m, source, NULL, 0 );
      }
   }

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      if ( masked && (mask[i] & 1) )
         continue;

      for ( j = 0 ; j < 4 ; j++ ) {
         if ( significand_match( d[i][j], r[i][j] ) < REQUIRED_PRECISION ) {
            printf( "-----------------------------\n" );
            printf( "(i = %i, j = %i)\n", i, j );
            printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][0], r[i][0], r[i][0]-d[i][0],
		    MAX_PRECISION - significand_match( d[i][0], r[i][0] ) );
            printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][1], r[i][1], r[i][1]-d[i][1],
		    MAX_PRECISION - significand_match( d[i][1], r[i][1] ) );
            printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][2], r[i][2], r[i][2]-d[i][2],
		    MAX_PRECISION - significand_match( d[i][2], r[i][2] ) );
            printf( "%f \t %f \t [diff = %e - %i bit missed]\n",
		    d[i][3], r[i][3], r[i][3]-d[i][3],
		    MAX_PRECISION - significand_match( d[i][3], r[i][3] ) );
            return 0;
         }
      }
   }

   ALIGN_FREE( mat->m );
   return 1;
}

void gl_test_all_transform_functions( char *description )
{
   int masked, psize, mtype;
   long benchmark_tab[2][4][7];
   static int first_time = 1;

   if ( first_time ) {
      first_time = 0;
      mesa_profile = getenv( "MESA_PROFILE" );
   }

#ifdef RUN_XFORM_BENCHMARK
   if ( mesa_profile ) {
      if ( need_counter ) {
	 need_counter = 0;
	 INIT_COUNTER();
	 printf( "counter overhead: %ld cycles\n\n", counter_overhead );
      }
      printf( "transform results after hooking in %s functions:\n", description );
   }
#endif

   for ( masked = 0 ; masked <= 1 ; masked++ ) {
      int cma = masked ? 1 : 0;
      char *cmastring = masked ? "CULL_MASK_ACTIVE" : "0";

#ifdef RUN_XFORM_BENCHMARK
      if ( mesa_profile ) {
         printf( "\n culling: %s \n", masked ? "CULL_MASK_ACTIVE" : "0" );
         for ( psize = 1 ; psize <= 4 ; psize++ ) {
            printf( " p%d\t", psize );
         }
         printf( "\n--------------------------------------------------------\n" );
      }
#endif

      for ( mtype = 0 ; mtype < 7 ; mtype++ ) {
         for ( psize = 1 ; psize <= 4 ; psize++ ) {
            transform_func func = gl_transform_tab[cma][psize][mtypes[mtype]];
            long *cycles = &(benchmark_tab[cma][psize-1][mtype]);

            if ( test_transform_function( func, psize, mtype,
					  masked, cycles ) == 0 ) {
               char buf[100];
               sprintf( buf, "gl_transform_tab[%s][%d][%s] failed test (%s)",
                        cmastring, psize, mstrings[mtype], description );
               gl_problem( NULL, buf );
            }
#ifdef RUN_XFORM_BENCHMARK
            if ( mesa_profile )
               printf( " %li\t", benchmark_tab[cma][psize-1][mtype] );
#endif
         }
#ifdef RUN_XFORM_BENCHMARK
         if ( mesa_profile )
            printf( " | [%s]\n", mstrings[mtype] );
#endif
      }
#ifdef RUN_XFORM_BENCHMARK
      if ( mesa_profile )
         printf( "\n" );
#endif
   }
}



/* ================================================================
 * Normal transformation tests
 */

static int test_norm_function( normal_func func, int mtype,
			       int masked, long *cycles )
{
   GLvector3f source[1], dest[1], dest2[1], ref[1], ref2[1];
   GLmatrix mat[1];
   GLfloat s[TEST_COUNT][5], d[TEST_COUNT][3], r[TEST_COUNT][3];
   GLfloat d2[TEST_COUNT][3], r2[TEST_COUNT][3], length[TEST_COUNT];
   GLfloat scale;
   GLfloat *m;
   GLubyte mask[TEST_COUNT];
   int i, j;
#ifdef  RUN_XFORM_BENCHMARK
   int cycle_i;		/* the counter for the benchmarks we run */
#endif

   (void) cycles;

   mat->m = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
   mat->inv = m = mat->m;

   m[0] = 63.0; m[4] = 43.0; m[ 8] = 29.0; m[12] = 43.0;
   m[1] = 55.0; m[5] = 17.0; m[ 9] = 31.0; m[13] =  7.0;
   m[2] = 44.0; m[6] =  9.0; m[10] =  7.0; m[14] =  3.0;
   m[3] = 11.0; m[7] = 23.0; m[11] = 91.0; m[15] =  9.0;

   scale = 1.0F + rnd () * norm_scale_types[mtype];

   for ( i = 0 ; i < 4 ; i++ ) {
      for ( j = 0 ; j < 4 ; j++ ) {
         switch ( norm_templates[mtype][i * 4 + j] ) {
         case NIL:
            m[j * 4 + i] = 0.0;
            break;
         case ONE:
            m[j * 4 + i] = 1.0;
            break;
         case NEG:
            m[j * 4 + i] = -1.0;
            break;
         case VAR:
            break;
         default:
            abort();
         }
      }
   }

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      mask[i] = i % 2;				/* mask every 2nd element */
      d[i][0] = s[i][0] = d2[i][0] = 0.0;
      d[i][1] = s[i][1] = d2[i][1] = 0.0;
      d[i][2] = s[i][2] = d2[i][2] = 0.0;
      for ( j = 0 ; j < 3 ; j++ )
         s[i][j] = rnd();
      length[i] = 1 / sqrt( s[i][0]*s[i][0] +
			    s[i][1]*s[i][1] +
			    s[i][2]*s[i][2] );
   }

   source->data = (GLfloat(*)[3])s;
   source->start = (GLfloat *)s;
   source->count = TEST_COUNT;
   source->stride = sizeof(s[0]);
   source->flags = 0;

   dest->data = (GLfloat(*)[3])d;
   dest->start = (GLfloat *)d;
   dest->count = TEST_COUNT;
   dest->stride = sizeof(float[3]);
   dest->flags = 0;

   dest2->data = (GLfloat(*)[3])d2;
   dest2->start = (GLfloat *)d2;
   dest2->count = TEST_COUNT;
   dest2->stride = sizeof(float[3]);
   dest2->flags = 0;

   ref->data = (GLfloat(*)[3])r;
   ref->start = (GLfloat *)r;
   ref->count = TEST_COUNT;
   ref->stride = sizeof(float[3]);
   ref->flags = 0;

   ref2->data = (GLfloat(*)[3])r2;
   ref2->start = (GLfloat *)r2;
   ref2->count = TEST_COUNT;
   ref2->stride = sizeof(float[3]);
   ref2->flags = 0;

   if ( norm_normalize_types[mtype] == 0 ) {
      ref_norm_transform_rescale( mat, scale, source, NULL, NULL, ref );
   } else {
      ref_norm_transform_normalize( mat, scale, source, NULL, NULL, ref );
      ref_norm_transform_normalize( mat, scale, source, length, NULL, ref2 );
   }

   if ( mesa_profile ) {
      if ( masked ) {
         BEGIN_RACE( *cycles );
         func( mat, scale, source, NULL, mask, dest );
         END_RACE( *cycles );
         func( mat, scale, source, length, mask, dest2 );
      } else {
         BEGIN_RACE( *cycles );
         func( mat, scale, source, NULL, NULL, dest );
         END_RACE( *cycles );
         func( mat, scale, source, length, NULL, dest2 );
      }
   } else {
      if ( masked ) {
         func( mat, scale, source, NULL, mask, dest );
         func( mat, scale, source, length, mask, dest2 );
      } else {
         func( mat, scale, source, NULL, NULL, dest );
         func( mat, scale, source, length, NULL, dest2 );
      }
   }

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
      if ( masked && !(mask[i] & 1) )
         continue;

      for ( j = 0 ; j < 3 ; j++ ) {
         if ( significand_match( d[i][j], r[i][j] ) < REQUIRED_PRECISION ) {
            printf( "-----------------------------\n" );
            printf( "(i = %i, j = %i)\n", i, j );
            printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		    d[i][0], r[i][0], r[i][0]/d[i][0],
		    MAX_PRECISION - significand_match( d[i][0], r[i][0] ) );
            printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		    d[i][1], r[i][1], r[i][1]/d[i][1],
		    MAX_PRECISION - significand_match( d[i][1], r[i][1] ) );
            printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		    d[i][2], r[i][2], r[i][2]/d[i][2],
		    MAX_PRECISION - significand_match( d[i][2], r[i][2] ) );
            return 0;
         }

         if ( norm_normalize_types[mtype] != 0 ) {
            if ( significand_match( d2[i][j], r2[i][j] ) < REQUIRED_PRECISION ) {
               printf( "------------------- precalculated length case ------\n" );
               printf( "(i = %i, j = %i)\n", i, j );
               printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		       d2[i][0], r2[i][0], r2[i][0]/d2[i][0],
		       MAX_PRECISION - significand_match( d2[i][0], r2[i][0] ) );
               printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		       d2[i][1], r2[i][1], r2[i][1]/d2[i][1],
		       MAX_PRECISION - significand_match( d2[i][1], r2[i][1] ) );
               printf( "%f \t %f \t [ratio = %e - %i bit missed]\n",
		       d2[i][2], r2[i][2], r2[i][2]/d2[i][2],
		       MAX_PRECISION - significand_match( d2[i][2], r2[i][2] ) );
               return 0;
            }
         }
      }
   }

   ALIGN_FREE( mat->m );
   return 1;
}

void gl_test_all_normal_transform_functions( char *description )
{
   int masked;
   int mtype;
   long benchmark_tab[0xf][0x4];
   static int first_time = 1;

   if ( first_time ) {
      first_time = 0;
      mesa_profile = getenv( "MESA_PROFILE" );
   }

#ifdef RUN_XFORM_BENCHMARK
   if ( mesa_profile ) {
      if ( need_counter ) {
	 need_counter = 0;
	 INIT_COUNTER();
	 printf( "counter overhead: %ld cycles\n\n", counter_overhead );
      }
      printf( "normal transform results after hooking in %s functions:\n",
	      description );
   }
#endif

   for ( masked = 0 ; masked <= 1 ; masked++ ) {
      int cma = masked ? 1 : 0;
      char *cmastring = masked ? "CULL_MASK_ACTIVE" : "0";

#ifdef RUN_XFORM_BENCHMARK
      if ( mesa_profile ) {
         printf( "\n culling: %s \n", masked ? "CULL_MASK_ACTIVE" : "0" );
         printf( "\n-------------------------------------------------------\n" );
      }
#endif

      for ( mtype = 0 ; mtype < 8 ; mtype++ ) {
         normal_func func = gl_normal_tab[norm_types[mtype]][cma];
         long *cycles = &(benchmark_tab[mtype][cma]);

         if ( test_norm_function( func, mtype, masked, cycles ) == 0 ) {
            char buf[100];
            sprintf( buf, "gl_normal_tab[%s][%s] failed test (%s)",
                     cmastring, norm_strings[mtype], description );
            gl_problem( NULL, buf );
	 }

#ifdef RUN_XFORM_BENCHMARK
         if ( mesa_profile ) {
            printf( " %li\t", benchmark_tab[mtype][cma] );
            printf( " | [%s]\n", norm_strings[mtype] );
         }
      }
      if ( mesa_profile )
	 printf( "\n" );
#else
      }
#endif
   }
#ifdef RUN_XFORM_BENCHMARK
   if ( mesa_profile )
      fflush( stdout );
#endif
}

#endif /* DEBUG */
