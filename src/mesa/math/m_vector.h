/* $Id: m_vector.h,v 1.1 2000/11/16 21:05:41 keithw Exp $ */

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
 * New (3.1) transformation code written by Keith Whitwell.
 */


#ifndef _M_VECTOR_H_
#define _M_VECTOR_H_

#include "glheader.h"


#define VEC_DIRTY_0        0x1	/* dirty flags not really used any more */
#define VEC_DIRTY_1        0x2
#define VEC_DIRTY_2        0x4
#define VEC_DIRTY_3        0x8
#define VEC_MALLOC         0x10 /* storage field points to self-allocated mem*/
#define VEC_WRITABLE       0x20	/* keep both + and - bits for easy testing */
#define VEC_NOT_WRITABLE   0x40
#define VEC_GOOD_STRIDE    0x80	
#define VEC_BAD_STRIDE     0x100

#define VEC_WRITABLE_FLAGS (VEC_WRITABLE|VEC_NOT_WRITABLE)
#define VEC_STRIDE_FLAGS   (VEC_GOOD_STRIDE|VEC_BAD_STRIDE)


#define VEC_SIZE_1   VEC_DIRTY_0
#define VEC_SIZE_2   (VEC_DIRTY_0|VEC_DIRTY_1)
#define VEC_SIZE_3   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2)
#define VEC_SIZE_4   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2|VEC_DIRTY_3)



/* Wrap all the information about vectors up in a struct.  Has
 * additional fields compared to the other vectors to help us track of
 * different vertex sizes, and whether we need to clean columns out
 * because they contain non-(0,0,0,1) values.  
 *
 * The start field is used to reserve data for copied vertices at the
 * end of gl_transform_vb, and avoids the need for a multiplication in
 * the transformation routines.
 */
typedef struct {
   GLfloat (*data)[4];	/* may be malloc'd or point to client data */
   GLfloat *start;	/* points somewhere inside of <data> */
   GLuint count;	/* size of the vector (in elements) */
   GLuint stride;	/* stride from one element to the next (in bytes) */
   GLuint size;		/* 2-4 for vertices and 1-4 for texcoords */
   GLuint flags;	/* which columns are dirty */
   void *storage;	/* self-allocated storage */
} GLvector4f;


extern void gl_vector4f_init( GLvector4f *v, GLuint flags,
			      GLfloat (*storage)[4] );
extern void gl_vector4f_alloc( GLvector4f *v, GLuint flags,
			       GLuint count, GLuint alignment );
extern void gl_vector4f_free( GLvector4f *v );
extern void gl_vector4f_print( GLvector4f *v, GLubyte *, GLboolean );
extern void gl_vector4f_clean_elem( GLvector4f *vec, GLuint nr, GLuint elt );


/* Could use a single vector type for normals and vertices, but
 * this way avoids some casts.  
 */
typedef struct {
   GLfloat (*data)[3];       
   GLfloat *start;
   GLuint count;
   GLuint stride;
   GLuint flags;
   void *storage;
} GLvector3f;

extern void gl_vector3f_init( GLvector3f *v, GLuint flags, GLfloat (*)[3] );
extern void gl_vector3f_alloc( GLvector3f *v, GLuint flags, GLuint count,
			       GLuint alignment );
extern void gl_vector3f_free( GLvector3f *v );
extern void gl_vector3f_print( GLvector3f *v, GLubyte *, GLboolean );


typedef struct {
   GLfloat *data;       
   GLfloat *start;
   GLuint count;
   GLuint stride;
   GLuint flags;
   void *storage;
} GLvector1f;

extern void gl_vector1f_free( GLvector1f *v );
extern void gl_vector1f_init( GLvector1f *v, GLuint flags, GLfloat * );
extern void gl_vector1f_alloc( GLvector1f *v, GLuint flags, GLuint count,
			       GLuint alignment );


/* For 4ub rgba values.
 */
typedef struct {
   GLubyte (*data)[4];
   GLubyte *start;
   GLuint count;
   GLuint stride;		 
   GLuint flags; 
   void *storage;
} GLvector4ub;

extern void gl_vector4ub_init( GLvector4ub *v, GLuint flags, 
			       GLubyte (*storage)[4] );
extern void gl_vector4ub_alloc( GLvector4ub *v, GLuint flags, GLuint count,
				GLuint alignment );
extern void gl_vector4ub_free( GLvector4ub * );




/* For 1ub values, eg edgeflag.
 */
typedef struct {
   GLubyte *data;
   GLubyte *start;
   GLuint count;
   GLuint stride;		 
   GLuint flags; 
   void *storage;
} GLvector1ub;

extern void gl_vector1ub_init( GLvector1ub *v, GLuint flags, GLubyte *storage);
extern void gl_vector1ub_alloc( GLvector1ub *v, GLuint flags, GLuint count,
				GLuint alignment );
extern void gl_vector1ub_free( GLvector1ub * );




/* For, eg Index, Array element.
 */
typedef struct {
   GLuint *data;
   GLuint *start;
   GLuint count;
   GLuint stride; 
   GLuint flags; 
   void *storage;
} GLvector1ui;

extern void gl_vector1ui_init( GLvector1ui *v, GLuint flags, GLuint *storage );
extern void gl_vector1ui_alloc( GLvector1ui *v, GLuint flags, GLuint count, 
				GLuint alignment );
extern void gl_vector1ui_free( GLvector1ui * );



/*
 * Given vector <v>, return a pointer (cast to <type *> to the <i>-th element.
 *
 * End up doing a lot of slow imuls if not careful.
 */
#define VEC_ELT( v, type, i ) \
       ( (type *)  ( ((GLbyte *) ((v)->data)) + (i) * (v)->stride) )


#endif
