/* $Id: m_translate.h,v 1.1 2000/11/16 21:05:41 keithw Exp $ */

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


#ifndef _M_TRANSLATE_H_
#define _M_TRANSLATE_H_


typedef void (*trans_1f_func)(GLfloat *to,
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start, 
			      GLuint n );

typedef void (*trans_1ui_func)(GLuint *to,
			       CONST void *ptr,
			       GLuint stride,
			       GLuint start, 
			       GLuint n );

typedef void (*trans_1ub_func)(GLubyte *to,
			       CONST void *ptr,
			       GLuint stride,
			       GLuint start,
			       GLuint n );

typedef void (*trans_4ub_func)(GLubyte (*to)[4],
			       CONST void *ptr,
			       GLuint stride,
			       GLuint start,
			       GLuint n );

typedef void (*trans_4f_func)(GLfloat (*to)[4],
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start, 
			      GLuint n );

typedef void (*trans_3f_func)(GLfloat (*to)[3],
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start, 
			      GLuint n );




/* Translate GL_UNSIGNED_BYTE, etc to the indexes used in the arrays
 * below.
 */
#define TYPE_IDX(t) ((t) & 0xf)

#define MAX_TYPES TYPE_IDX(GL_DOUBLE)+1      /* 0xa + 1 */

/* Only useful combinations are defined, thus there is no function to
 * translate eg, ubyte->float or ubyte->ubyte, which are never used.  
 */
extern trans_1f_func gl_trans_1f_tab[MAX_TYPES];
extern trans_1ui_func gl_trans_1ui_tab[MAX_TYPES];
extern trans_1ub_func gl_trans_1ub_tab[MAX_TYPES];
extern trans_3f_func  gl_trans_3f_tab[MAX_TYPES];
extern trans_4ub_func gl_trans_4ub_tab[5][MAX_TYPES];
extern trans_4f_func  gl_trans_4f_tab[5][MAX_TYPES];


extern void gl_init_translate( void );


#endif
