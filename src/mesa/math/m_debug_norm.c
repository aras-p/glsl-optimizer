/* $Id: m_debug_norm.c,v 1.7 2001/03/30 14:44:43 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"

#include "m_matrix.h"
#include "m_xform.h"

#include "m_debug.h"
#include "m_debug_util.h"


#ifdef DEBUG  /* This code only used for debugging */


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


/* =============================================================
 * Reference transformations
 */

static void ref_norm_transform_rescale( const GLmatrix *mat,
					GLfloat scale,
					const GLvector3f *in,
					const GLfloat *lengths,
					GLvector3f *dest )
{
   GLuint i;
   const GLfloat *s = in->start;
   const GLfloat *m = mat->inv;
   GLfloat (*out)[3] = (GLfloat (*)[3])dest->start;

   (void) lengths;

   for ( i = 0 ; i < in->count ; i++ ) {
      GLfloat t[3];

      TRANSFORM_NORMAL( t, s, m );
      SCALE_SCALAR_3V( out[i], scale, t );

      s = (GLfloat *)((char *)s + in->stride);
   }
}

static void ref_norm_transform_normalize( const GLmatrix *mat,
					  GLfloat scale,
					  const GLvector3f *in,
					  const GLfloat *lengths,
					  GLvector3f *dest )
{
   GLuint i;
   const GLfloat *s = in->start;
   const GLfloat *m = mat->inv;
   GLfloat (*out)[3] = (GLfloat (*)[3])dest->start;

   for ( i = 0 ; i < in->count ; i++ ) {
      GLfloat t[3];

      TRANSFORM_NORMAL( t, s, m );

      if ( !lengths ) {
         GLfloat len = LEN_SQUARED_3FV( t );
         if ( len > 1e-20 ) {
	    /* Hmmm, don't know how we could test the precalculated
	     * length case...
	     */
            scale = 1.0 / sqrt( len );
	    SCALE_SCALAR_3V( out[i], scale, t );
         } else {
            out[i][0] = out[i][1] = out[i][2] = 0;
         }
      } else {
         scale = lengths[i];;
	 SCALE_SCALAR_3V( out[i], scale, t );
      }

      s = (GLfloat *)((char *)s + in->stride);
   }
}


/* =============================================================
 * Normal transformation tests
 */

static int test_norm_function( normal_func func, int mtype, long *cycles )
{
   GLvector3f source[1], dest[1], dest2[1], ref[1], ref2[1];
   GLmatrix mat[1];
   GLfloat s[TEST_COUNT][5], d[TEST_COUNT][3], r[TEST_COUNT][3];
   GLfloat d2[TEST_COUNT][3], r2[TEST_COUNT][3], length[TEST_COUNT];
   GLfloat scale;
   GLfloat *m;
   int i, j;
#ifdef  RUN_DEBUG_BENCHMARK
   int cycle_i;		/* the counter for the benchmarks we run */
#endif

   (void) cycles;

   mat->m = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
   mat->inv = m = mat->m;

   init_matrix( m );

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
      ASSIGN_3V( d[i],  0.0, 0.0, 0.0 );
      ASSIGN_3V( s[i],  0.0, 0.0, 0.0 );
      ASSIGN_3V( d2[i], 0.0, 0.0, 0.0 );
      for ( j = 0 ; j < 3 ; j++ )
         s[i][j] = rnd();
      length[i] = 1 / sqrt( LEN_SQUARED_3FV( s[i] ) );
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
      ref_norm_transform_rescale( mat, scale, source, NULL, ref );
   } else {
      ref_norm_transform_normalize( mat, scale, source, NULL, ref );
      ref_norm_transform_normalize( mat, scale, source, length, ref2 );
   }

   if ( mesa_profile ) {
      BEGIN_RACE( *cycles );
      func( mat, scale, source, NULL, dest );
      END_RACE( *cycles );
      func( mat, scale, source, length, dest2 );
   } else {
      func( mat, scale, source, NULL, dest );
      func( mat, scale, source, length, dest2 );
   }

   for ( i = 0 ; i < TEST_COUNT ; i++ ) {
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

void _math_test_all_normal_transform_functions( char *description )
{
   int mtype;
   long benchmark_tab[0xf];
   static int first_time = 1;

   if ( first_time ) {
      first_time = 0;
      mesa_profile = getenv( "MESA_PROFILE" );
   }

#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      if ( !counter_overhead ) {
	 INIT_COUNTER();
	 printf( "counter overhead: %ld cycles\n\n", counter_overhead );
      }
      printf( "normal transform results after hooking in %s functions:\n",
	      description );
      printf( "\n-------------------------------------------------------\n" );
   }
#endif

   for ( mtype = 0 ; mtype < 8 ; mtype++ ) {
      normal_func func = _mesa_normal_tab[norm_types[mtype]];
      long *cycles = &benchmark_tab[mtype];

      if ( test_norm_function( func, mtype, cycles ) == 0 ) {
	 char buf[100];
	 sprintf( buf, "_mesa_normal_tab[0][%s] failed test (%s)",
		  norm_strings[mtype], description );
	 _mesa_problem( NULL, buf );
      }

#ifdef RUN_DEBUG_BENCHMARK
      if ( mesa_profile ) {
	 printf( " %li\t", benchmark_tab[mtype] );
	 printf( " | [%s]\n", norm_strings[mtype] );
      }
#endif
   }
#ifdef RUN_DEBUG_BENCHMARK
   if ( mesa_profile ) {
      printf( "\n" );
      fflush( stdout );
   }
#endif
}


#endif /* DEBUG */
