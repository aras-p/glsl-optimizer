/* $Id: t_imm_elt.c,v 1.11 2001/05/11 08:11:31 keithw Exp $ */

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
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "colormac.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "math/m_translate.h"

#include "t_context.h"
#include "t_imm_elt.h"



typedef void (*trans_elt_1f_func)(GLfloat *to,
				  CONST void *ptr,
				  GLuint stride,
				  GLuint *flags,
				  GLuint *elts,
				  GLuint match,
				  GLuint start,
				  GLuint n );

typedef void (*trans_elt_1ui_func)(GLuint *to,
				   CONST void *ptr,
				   GLuint stride,
				   GLuint *flags,
				   GLuint *elts,
				   GLuint match,
				   GLuint start,
				   GLuint n );

typedef void (*trans_elt_1ub_func)(GLubyte *to,
				   CONST void *ptr,
				   GLuint stride,
				   GLuint *flags,
				   GLuint *elts,
				   GLuint match,
				   GLuint start,
				   GLuint n );

typedef void (*trans_elt_4ub_func)(GLubyte (*to)[4],
                                   CONST void *ptr,
                                   GLuint stride,
                                   GLuint *flags,
                                   GLuint *elts,
                                   GLuint match,
                                   GLuint start,
                                   GLuint n );

typedef void (*trans_elt_4us_func)(GLushort (*to)[4],
                                   CONST void *ptr,
                                   GLuint stride,
                                   GLuint *flags,
                                   GLuint *elts,
                                   GLuint match,
                                   GLuint start,
                                   GLuint n );

typedef void (*trans_elt_4f_func)(GLfloat (*to)[4],
				  CONST void *ptr,
				  GLuint stride,
				  GLuint *flags,
				  GLuint *elts,
				  GLuint match,
				  GLuint start,
				  GLuint n );

typedef void (*trans_elt_3f_func)(GLfloat (*to)[3],
				  CONST void *ptr,
				  GLuint stride,
				  GLuint *flags,
				  GLuint *elts,
				  GLuint match,
				  GLuint start,
				  GLuint n );




static trans_elt_1f_func _tnl_trans_elt_1f_tab[MAX_TYPES];
static trans_elt_1ui_func _tnl_trans_elt_1ui_tab[MAX_TYPES];
static trans_elt_1ub_func _tnl_trans_elt_1ub_tab[MAX_TYPES];
static trans_elt_3f_func  _tnl_trans_elt_3f_tab[MAX_TYPES];
static trans_elt_4ub_func _tnl_trans_elt_4ub_tab[5][MAX_TYPES];
static trans_elt_4us_func _tnl_trans_elt_4us_tab[5][MAX_TYPES];
static trans_elt_4f_func  _tnl_trans_elt_4f_tab[5][MAX_TYPES];


#define PTR_ELT(ptr, elt) (((SRC *)ptr)[elt])





/* Code specific to array element implementation.  There is a small
 * subtlety in the bits CHECK() tests, and the way bits are set in
 * glArrayElement which ensures that if, eg, in the case that the
 * vertex array is disabled and normal array is enabled, and we get
 * either sequence:
 *
 * ArrayElement()    OR   Normal()
 * Normal()               ArrayElement()
 * Vertex()               Vertex()
 *
 * That the correct value for normal is used.
 */
#define TAB(x) _tnl_trans_elt##x##_tab
#define ARGS   GLuint *flags, GLuint *elts, GLuint match, \
               GLuint start, GLuint n
#define SRC_START  0
#define DST_START  start
#define CHECK  if ((flags[i]&match) == VERT_ELT)
#define NEXT_F  (void)1
#define NEXT_F2 f = first + elts[i] * stride;


/* GL_BYTE
 */
#define SRC GLbyte
#define SRC_IDX TYPE_IDX(GL_BYTE)
#define TRX_3F(f,n)   BYTE_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   BYTE_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = BYTE_TO_UBYTE( PTR_ELT(f,n) )
#define TRX_US(us, f,n)  us = BYTE_TO_USHORT( PTR_ELT(f,n) )
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ 4
#define INIT init_trans_4_GLbyte_elt
#define DEST_4F trans_4_GLbyte_4f_elt
#define DEST_4UB trans_4_GLbyte_4ub_elt
#define DEST_4US trans_4_GLbyte_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLbyte_elt
#define DEST_4F trans_3_GLbyte_4f_elt
#define DEST_4UB trans_3_GLbyte_4ub_elt
#define DEST_4US trans_3_GLbyte_4us_elt
#define DEST_3F trans_3_GLbyte_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLbyte_elt
#define DEST_4F trans_2_GLbyte_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLbyte_elt
#define DEST_4F trans_1_GLbyte_4f_elt
#define DEST_1UB trans_1_GLbyte_1ub_elt
#define DEST_1UI trans_1_GLbyte_1ui_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI
#undef SRC_IDX

/* GL_UNSIGNED_BYTE
 */
#define SRC GLubyte
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_BYTE)
#define TRX_3F(f,n)	     UBYTE_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)	     UBYTE_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)	     ub = PTR_ELT(f,n)
#define TRX_US(us, f,n)	     us = PTR_ELT(f,n)
#define TRX_UI(f,n)          (GLuint)PTR_ELT(f,n)

/* 4ub->4ub handled in special case below.
 */
#define SZ 4
#define INIT init_trans_4_GLubyte_elt
#define DEST_4F trans_4_GLubyte_4f_elt
#define DEST_4US trans_4_GLubyte_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLubyte_elt
#define DEST_4F trans_3_GLubyte_4f_elt
#define DEST_3F trans_3_GLubyte_3f_elt
#define DEST_4UB trans_3_GLubyte_4ub_elt
#define DEST_4US trans_3_GLubyte_4us_elt
#include "math/m_trans_tmp.h"


#define SZ 1
#define INIT init_trans_1_GLubyte_elt
#define DEST_1UI trans_1_GLubyte_1ui_elt
#define DEST_1UB trans_1_GLubyte_1ub_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_SHORT
 */
#define SRC GLshort
#define SRC_IDX TYPE_IDX(GL_SHORT)
#define TRX_3F(f,n)   SHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = SHORT_TO_UBYTE(PTR_ELT(f,n))
#define TRX_US(us, f,n)  us = SHORT_TO_USHORT(PTR_ELT(f,n))
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ  4
#define INIT init_trans_4_GLshort_elt
#define DEST_4F trans_4_GLshort_4f_elt
#define DEST_4UB trans_4_GLshort_4ub_elt
#define DEST_4US trans_4_GLshort_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLshort_elt
#define DEST_4F trans_3_GLshort_4f_elt
#define DEST_4UB trans_3_GLshort_4ub_elt
#define DEST_4US trans_3_GLshort_4us_elt
#define DEST_3F trans_3_GLshort_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLshort_elt
#define DEST_4F trans_2_GLshort_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLshort_elt
#define DEST_4F trans_1_GLshort_4f_elt
#define DEST_1UB trans_1_GLshort_1ub_elt
#define DEST_1UI trans_1_GLshort_1ui_elt
#include "math/m_trans_tmp.h"


#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_UNSIGNED_SHORT
 */
#define SRC GLushort
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_SHORT)
#define TRX_3F(f,n)   USHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_UB(ub,f,n)  ub = (GLubyte) (PTR_ELT(f,n) >> 8)
#define TRX_US(us,f,n)  us = PTR_ELT(f,n)
#define TRX_UI(f,n)  (GLuint)   PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLushort_elt
#define DEST_4F trans_4_GLushort_4f_elt
#define DEST_4UB trans_4_GLushort_4ub_elt
#define DEST_4US trans_4_GLushort_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLushort_elt
#define DEST_4F trans_3_GLushort_4f_elt
#define DEST_4UB trans_3_GLushort_4ub_elt
#define DEST_4US trans_3_GLushort_4us_elt
#define DEST_3F trans_3_GLushort_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLushort_elt
#define DEST_4F trans_2_GLushort_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLushort_elt
#define DEST_4F trans_1_GLushort_4f_elt
#define DEST_1UB trans_1_GLushort_1ub_elt
#define DEST_1UI trans_1_GLushort_1ui_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_INT
 */
#define SRC GLint
#define SRC_IDX TYPE_IDX(GL_INT)
#define TRX_3F(f,n)   INT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = INT_TO_UBYTE(PTR_ELT(f,n))
#define TRX_US(us, f,n)  us = INT_TO_USHORT(PTR_ELT(f,n))
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ 4
#define INIT init_trans_4_GLint_elt
#define DEST_4F trans_4_GLint_4f_elt
#define DEST_4UB trans_4_GLint_4ub_elt
#define DEST_4US trans_4_GLint_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLint_elt
#define DEST_4F trans_3_GLint_4f_elt
#define DEST_4UB trans_3_GLint_4ub_elt
#define DEST_4US trans_3_GLint_4us_elt
#define DEST_3F trans_3_GLint_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLint_elt
#define DEST_4F trans_2_GLint_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLint_elt
#define DEST_4F trans_1_GLint_4f_elt
#define DEST_1UB trans_1_GLint_1ub_elt
#define DEST_1UI trans_1_GLint_1ui_elt
#include "math/m_trans_tmp.h"


#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_UNSIGNED_INT
 */
#define SRC GLuint
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_INT)
#define TRX_3F(f,n)   UINT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = (GLubyte) (PTR_ELT(f,n) >> 24)
#define TRX_US(us, f,n)  us = (GLushort) (PTR_ELT(f,n) >> 16)
#define TRX_UI(f,n)		PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLuint_elt
#define DEST_4F trans_4_GLuint_4f_elt
#define DEST_4UB trans_4_GLuint_4ub_elt
#define DEST_4US trans_4_GLuint_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLuint_elt
#define DEST_4F trans_3_GLuint_4f_elt
#define DEST_4UB trans_3_GLuint_4ub_elt
#define DEST_4US trans_3_GLuint_4us_elt
#define DEST_3F trans_3_GLuint_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLuint_elt
#define DEST_4F trans_2_GLuint_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLuint_elt
#define DEST_4F trans_1_GLuint_4f_elt
#define DEST_1UB trans_1_GLuint_1ub_elt
#define DEST_1UI trans_1_GLuint_1ui_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_DOUBLE
 */
#define SRC GLdouble
#define SRC_IDX TYPE_IDX(GL_DOUBLE)
#define TRX_3F(f,n)   PTR_ELT(f,n)
#define TRX_4F(f,n)   PTR_ELT(f,n)
#define TRX_UB(ub,f,n) UNCLAMPED_FLOAT_TO_UBYTE(ub, PTR_ELT(f,n))
#define TRX_US(us,f,n) UNCLAMPED_FLOAT_TO_USHORT(us, PTR_ELT(f,n))
#define TRX_UI(f,n)  (GLuint) (GLint) PTR_ELT(f,n)
#define TRX_1F(f,n)   PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLdouble_elt
#define DEST_4F trans_4_GLdouble_4f_elt
#define DEST_4UB trans_4_GLdouble_4ub_elt
#define DEST_4US trans_4_GLdouble_4us_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLdouble_elt
#define DEST_4F trans_3_GLdouble_4f_elt
#define DEST_4UB trans_3_GLdouble_4ub_elt
#define DEST_4US trans_3_GLdouble_4us_elt
#define DEST_3F trans_3_GLdouble_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLdouble_elt
#define DEST_4F trans_2_GLdouble_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLdouble_elt
#define DEST_4F trans_1_GLdouble_4f_elt
#define DEST_1UB trans_1_GLdouble_1ub_elt
#define DEST_1UI trans_1_GLdouble_1ui_elt
#define DEST_1F trans_1_GLdouble_1f_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef SRC_IDX

/* GL_FLOAT
 */
#define SRC GLfloat
#define SRC_IDX TYPE_IDX(GL_FLOAT)
#define SZ 4
#define INIT init_trans_4_GLfloat_elt
#define DEST_4UB trans_4_GLfloat_4ub_elt
#define DEST_4US trans_4_GLfloat_4us_elt
#define DEST_4F  trans_4_GLfloat_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLfloat_elt
#define DEST_4F  trans_3_GLfloat_4f_elt
#define DEST_4UB trans_3_GLfloat_4ub_elt
#define DEST_4US trans_3_GLfloat_4us_elt
#define DEST_3F trans_3_GLfloat_3f_elt
#include "math/m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLfloat_elt
#define DEST_4F trans_2_GLfloat_4f_elt
#include "math/m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLfloat_elt
#define DEST_4F  trans_1_GLfloat_3f_elt
#define DEST_1UB trans_1_GLfloat_1ub_elt
#define DEST_1UI trans_1_GLfloat_1ui_elt
#define DEST_1F trans_1_GLfloat_1f_elt
#include "math/m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3F
#undef TRX_4F
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


static void trans_4_GLubyte_4ub(GLubyte (*t)[4],
                                CONST void *Ptr,
                                GLuint stride,
                                ARGS )
{
   const GLubyte *f = (GLubyte *) Ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) start;
   if (((((long) f | (long) stride)) & 3L) == 0L) {
      /* Aligned.
       */
      for (i = DST_START ; i < n ; i++, NEXT_F) {
	 CHECK {
	    NEXT_F2;
	    COPY_4UBV( t[i], f );
	 }
      }
   } else {
      for (i = DST_START ; i < n ; i++, NEXT_F) {
	 CHECK {
	    NEXT_F2;
	    t[i][0] = f[0];
	    t[i][1] = f[1];
	    t[i][2] = f[2];
	    t[i][3] = f[3];
	 }
      }
   }
}


static void init_translate_elt(void)
{
   MEMSET( TAB(_1ui), 0, sizeof(TAB(_1ui)) );
   MEMSET( TAB(_1ub), 0, sizeof(TAB(_1ub)) );
   MEMSET( TAB(_3f),  0, sizeof(TAB(_3f)) );
   MEMSET( TAB(_4ub), 0, sizeof(TAB(_4ub)) );
   MEMSET( TAB(_4us), 0, sizeof(TAB(_4us)) );
   MEMSET( TAB(_4f),  0, sizeof(TAB(_4f)) );

   TAB(_4ub)[4][TYPE_IDX(GL_UNSIGNED_BYTE)] = trans_4_GLubyte_4ub;

   init_trans_4_GLbyte_elt();
   init_trans_3_GLbyte_elt();
   init_trans_2_GLbyte_elt();
   init_trans_1_GLbyte_elt();
   init_trans_1_GLubyte_elt();
   init_trans_3_GLubyte_elt();
   init_trans_4_GLubyte_elt();
   init_trans_4_GLshort_elt();
   init_trans_3_GLshort_elt();
   init_trans_2_GLshort_elt();
   init_trans_1_GLshort_elt();
   init_trans_4_GLushort_elt();
   init_trans_3_GLushort_elt();
   init_trans_2_GLushort_elt();
   init_trans_1_GLushort_elt();
   init_trans_4_GLint_elt();
   init_trans_3_GLint_elt();
   init_trans_2_GLint_elt();
   init_trans_1_GLint_elt();
   init_trans_4_GLuint_elt();
   init_trans_3_GLuint_elt();
   init_trans_2_GLuint_elt();
   init_trans_1_GLuint_elt();
   init_trans_4_GLdouble_elt();
   init_trans_3_GLdouble_elt();
   init_trans_2_GLdouble_elt();
   init_trans_1_GLdouble_elt();
   init_trans_4_GLfloat_elt();
   init_trans_3_GLfloat_elt();
   init_trans_2_GLfloat_elt();
   init_trans_1_GLfloat_elt();
}


#undef TAB
#undef CLASS
#undef ARGS
#undef CHECK
#undef START




void _tnl_imm_elt_init( void )
{
   init_translate_elt();
}


static void _tnl_trans_elt_1f(GLfloat *to,
		       const struct gl_client_array *from,
		       GLuint *flags,
		       GLuint *elts,
		       GLuint match,
		       GLuint start,
		       GLuint n )
{
   _tnl_trans_elt_1f_tab[TYPE_IDX(from->Type)]( to,
					      from->Ptr,
					      from->StrideB,
					      flags,
					      elts,
					      match,
					      start,
					      n );

}

static void _tnl_trans_elt_1ui(GLuint *to,
			const struct gl_client_array *from,
			GLuint *flags,
			GLuint *elts,
			GLuint match,
			GLuint start,
			GLuint n )
{
   _tnl_trans_elt_1ui_tab[TYPE_IDX(from->Type)]( to,
					       from->Ptr,
					       from->StrideB,
					       flags,
					       elts,
					       match,
					       start,
					       n );

}


static void _tnl_trans_elt_1ub(GLubyte *to,
			const struct gl_client_array *from,
			GLuint *flags,
			GLuint *elts,
			GLuint match,
			GLuint start,
			GLuint n )
{
   _tnl_trans_elt_1ub_tab[TYPE_IDX(from->Type)]( to,
                                                 from->Ptr,
                                                 from->StrideB,
                                                 flags,
                                                 elts,
                                                 match,
                                                 start,
                                                 n );

}


#if 0
static void _tnl_trans_elt_4ub(GLubyte (*to)[4],
                               const struct gl_client_array *from,
                               GLuint *flags,
                               GLuint *elts,
                               GLuint match,
                               GLuint start,
                               GLuint n )
{
   _tnl_trans_elt_4ub_tab[from->Size][TYPE_IDX(from->Type)]( to,
                                                             from->Ptr,
                                                             from->StrideB,
                                                             flags,
                                                             elts,
                                                             match,
                                                             start,
                                                             n );

}
#endif

#if 0
static void _tnl_trans_elt_4us(GLushort (*to)[4],
                               const struct gl_client_array *from,
                               GLuint *flags,
                               GLuint *elts,
                               GLuint match,
                               GLuint start,
                               GLuint n )
{
   _tnl_trans_elt_4us_tab[from->Size][TYPE_IDX(from->Type)]( to,
                                                             from->Ptr,
                                                             from->StrideB,
                                                             flags,
                                                             elts,
                                                             match,
                                                             start,
                                                             n );

}
#endif

static void _tnl_trans_elt_4f(GLfloat (*to)[4],
                              const struct gl_client_array *from,
                              GLuint *flags,
                              GLuint *elts,
                              GLuint match,
                              GLuint start,
                              GLuint n )
{
   _tnl_trans_elt_4f_tab[from->Size][TYPE_IDX(from->Type)]( to,
					      from->Ptr,
					      from->StrideB,
					      flags,
					      elts,
					      match,
					      start,
					      n );

}



static void _tnl_trans_elt_3f(GLfloat (*to)[3],
		       const struct gl_client_array *from,
		       GLuint *flags,
		       GLuint *elts,
		       GLuint match,
		       GLuint start,
		       GLuint n )
{
   _tnl_trans_elt_3f_tab[TYPE_IDX(from->Type)]( to,
					      from->Ptr,
					      from->StrideB,
					      flags,
					      elts,
					      match,
					      start,
					      n );
}




/* Batch function to translate away all the array elements in the
 * input buffer prior to transform.  Done only the first time a vertex
 * buffer is executed or compiled.
 *
 * KW: Have to do this after each glEnd if arrays aren't locked.
 */
void _tnl_translate_array_elts( GLcontext *ctx, struct immediate *IM,
				GLuint start, GLuint count )
{
   GLuint *flags = IM->Flag;
   GLuint *elts = IM->Elt;
   GLuint translate = ctx->Array._Enabled;
   GLuint i;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      fprintf(stderr, "exec_array_elements %d .. %d\n", start, count);

   if (translate & VERT_OBJ) {
      _tnl_trans_elt_4f( IM->Obj,
			 &ctx->Array.Vertex,
			 flags, elts, (VERT_ELT|VERT_OBJ),
			 start, count);

      if (ctx->Array.Vertex.Size == 4)
	 translate |= VERT_OBJ_234;
      else if (ctx->Array.Vertex.Size == 3)
	 translate |= VERT_OBJ_23;
   }


   if (translate & VERT_NORM)
      _tnl_trans_elt_3f( IM->Normal,
			 &ctx->Array.Normal,
			 flags, elts, (VERT_ELT|VERT_NORM),
			 start, count);

   if (translate & VERT_EDGE)
      _tnl_trans_elt_1ub( IM->EdgeFlag,
			  &ctx->Array.EdgeFlag,
			  flags, elts, (VERT_ELT|VERT_EDGE),
			  start, count);

   if (translate & VERT_RGBA) {
      _tnl_trans_elt_4f( IM->Color,
			 &ctx->Array.Color,
			 flags, elts, (VERT_ELT|VERT_RGBA),
			 start, count);
   }

   if (translate & VERT_SPEC_RGB) {
      _tnl_trans_elt_4f( IM->SecondaryColor,
			 &ctx->Array.SecondaryColor,
			 flags, elts, (VERT_ELT|VERT_SPEC_RGB),
			 start, count);
   }

   if (translate & VERT_FOG_COORD)
      _tnl_trans_elt_1f( IM->FogCoord,
			 &ctx->Array.FogCoord,
			 flags, elts, (VERT_ELT|VERT_FOG_COORD),
			 start, count);

   if (translate & VERT_INDEX)
      _tnl_trans_elt_1ui( IM->Index,
			  &ctx->Array.Index,
			  flags, elts, (VERT_ELT|VERT_INDEX),
			  start, count);

   if (translate & VERT_TEX_ANY) {
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
	 if (translate & VERT_TEX(i)) {
	    _tnl_trans_elt_4f( IM->TexCoord[i],
			       &ctx->Array.TexCoord[i],
			       flags, elts, (VERT_ELT|VERT_TEX(i)),
			       start, count);

	    if (ctx->Array.TexCoord[i].Size == 4)
	       IM->TexSize |= TEX_SIZE_4(i);
	    else if (ctx->Array.TexCoord[i].Size == 3)
	       IM->TexSize |= TEX_SIZE_3(i);
	 }
   }

   for (i = start ; i < count ; i++)
      if (flags[i] & VERT_ELT) flags[i] |= translate;

   IM->FlushElt = 0;
}
