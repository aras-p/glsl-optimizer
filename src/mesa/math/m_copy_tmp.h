/* $Id: m_copy_tmp.h,v 1.1 2000/11/16 21:05:41 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * New (3.1) transformation code written by Keith Whitwell.
 */


#define COPY_FUNC( BITS )						\
static void TAG2(copy, BITS)(GLvector4f *to, const GLvector4f *f,	\
			     const GLubyte mask[] )			\
{									\
   GLfloat (*t)[4] = (GLfloat (*)[4])to->start;				\
   GLfloat *from = f->start;						\
   GLuint stride = f->stride;				        	\
   GLuint count = f->count;						\
   GLuint i;								\
   (void) mask;								\
									\
   if (BITS)								\
      STRIDE_LOOP {							\
	 CULL_CHECK {							\
	    if (BITS&1) t[i][0] = from[0];				\
	    if (BITS&2) t[i][1] = from[1];				\
	    if (BITS&4) t[i][2] = from[2];				\
	    if (BITS&8) t[i][3] = from[3];				\
	 }								\
      }									\
}



/* static void TAG2(clean, BITS)(GLvector4f *to ) */
/* { */
/*    GLfloat (*t)[4] = to->data; */
/*    GLuint i; */

/*    if (BITS) */
/*       for (i = 0 ; i < VB_SIZE ; i++) { */
/*          if (BITS&1) t[i][0] = 0; */
/* 	 if (BITS&2) t[i][1] = 0; */
/* 	 if (BITS&4) t[i][2] = 0; */
/* 	 if (BITS&8) t[i][3] = 1; */
/*       } */
/*    to->flags &= ~BITS; */
/* } */


/* We got them all here:
 */
COPY_FUNC( 0x0 )		/* noop */
COPY_FUNC( 0x1 )
COPY_FUNC( 0x2 )
COPY_FUNC( 0x3 )
COPY_FUNC( 0x4 )
COPY_FUNC( 0x5 )
COPY_FUNC( 0x6 )
COPY_FUNC( 0x7 )
COPY_FUNC( 0x8 )
COPY_FUNC( 0x9 )
COPY_FUNC( 0xa )
COPY_FUNC( 0xb )
COPY_FUNC( 0xc )
COPY_FUNC( 0xd )
COPY_FUNC( 0xe )
COPY_FUNC( 0xf )

static void TAG2(init_copy, 0 ) ( void )
{
   gl_copy_tab[IDX][0x0] = TAG2(copy, 0x0);
   gl_copy_tab[IDX][0x1] = TAG2(copy, 0x1);
   gl_copy_tab[IDX][0x2] = TAG2(copy, 0x2);
   gl_copy_tab[IDX][0x3] = TAG2(copy, 0x3);
   gl_copy_tab[IDX][0x4] = TAG2(copy, 0x4);
   gl_copy_tab[IDX][0x5] = TAG2(copy, 0x5);
   gl_copy_tab[IDX][0x6] = TAG2(copy, 0x6);
   gl_copy_tab[IDX][0x7] = TAG2(copy, 0x7);
   gl_copy_tab[IDX][0x8] = TAG2(copy, 0x8);
   gl_copy_tab[IDX][0x9] = TAG2(copy, 0x9);
   gl_copy_tab[IDX][0xa] = TAG2(copy, 0xa);
   gl_copy_tab[IDX][0xb] = TAG2(copy, 0xb);
   gl_copy_tab[IDX][0xc] = TAG2(copy, 0xc);
   gl_copy_tab[IDX][0xd] = TAG2(copy, 0xd);
   gl_copy_tab[IDX][0xe] = TAG2(copy, 0xe);
   gl_copy_tab[IDX][0xf] = TAG2(copy, 0xf);

/*    gl_clean_tab[IDX][0x0] = TAG2(clean, 0x0); */
/*    gl_clean_tab[IDX][0x1] = TAG2(clean, 0x1); */
/*    gl_clean_tab[IDX][0x2] = TAG2(clean, 0x2); */
/*    gl_clean_tab[IDX][0x3] = TAG2(clean, 0x3); */
/*    gl_clean_tab[IDX][0x4] = TAG2(clean, 0x4); */
/*    gl_clean_tab[IDX][0x5] = TAG2(clean, 0x5); */
/*    gl_clean_tab[IDX][0x6] = TAG2(clean, 0x6); */
/*    gl_clean_tab[IDX][0x7] = TAG2(clean, 0x7); */
/*    gl_clean_tab[IDX][0x8] = TAG2(clean, 0x8); */
/*    gl_clean_tab[IDX][0x9] = TAG2(clean, 0x9); */
/*    gl_clean_tab[IDX][0xa] = TAG2(clean, 0xa); */
/*    gl_clean_tab[IDX][0xb] = TAG2(clean, 0xb); */
/*    gl_clean_tab[IDX][0xc] = TAG2(clean, 0xc); */
/*    gl_clean_tab[IDX][0xd] = TAG2(clean, 0xd); */
/*    gl_clean_tab[IDX][0xe] = TAG2(clean, 0xe); */
/*    gl_clean_tab[IDX][0xf] = TAG2(clean, 0xf); */
}
