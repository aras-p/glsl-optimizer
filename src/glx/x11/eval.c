/* $XFree86$ */
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#include "packrender.h"

/*
** Routines to pack evaluator maps into the transport buffer.  Maps are
** allowed to have extra arbitrary data, so these routines extract just
** the information that the GL needs.
*/

void __glFillMap1f(GLint k, GLint order, GLint stride, 
		   const GLfloat *points, GLubyte *pc)
{
    if (stride == k) {
	/* Just copy the data */
	__GLX_PUT_FLOAT_ARRAY(0, points, order * k);
    } else {
	GLint i;

	for (i = 0; i < order; i++) {
	    __GLX_PUT_FLOAT_ARRAY(0, points, k);
	    points += stride;
	    pc += k * __GLX_SIZE_FLOAT32;
	}
    }
}

void __glFillMap1d(GLint k, GLint order, GLint stride, 
		   const GLdouble *points, GLubyte *pc)
{
    if (stride == k) {
	/* Just copy the data */
	__GLX_PUT_DOUBLE_ARRAY(0, points, order * k);
    } else {
	GLint i;
	for (i = 0; i < order; i++) {
            __GLX_PUT_DOUBLE_ARRAY(0, points, k);
	    points += stride;
	    pc += k * __GLX_SIZE_FLOAT64;
	}
    }
}

void __glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder, 
		   GLint majorStride, GLint minorStride,
		   const GLfloat *points, GLfloat *data)
{
    GLint i, j, x;

    if ((minorStride == k) && (majorStride == minorOrder*k)) {
	/* Just copy the data */
	__GLX_MEM_COPY(data, points, majorOrder * majorStride *
		       __GLX_SIZE_FLOAT32);
	return;
    }
    for (i = 0; i < majorOrder; i++) {
	for (j = 0; j < minorOrder; j++) {
	    for (x = 0; x < k; x++) {
		data[x] = points[x];
	    }
	    points += minorStride;
	    data += k;
	}
	points += majorStride - minorStride * minorOrder;
    }
}

void __glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder, 
		   GLint majorStride, GLint minorStride,
		   const GLdouble *points, GLdouble *data)
{
    int i,j,x;

    if ((minorStride == k) && (majorStride == minorOrder*k)) {
	/* Just copy the data */
	__GLX_MEM_COPY(data, points, majorOrder * majorStride *
		       __GLX_SIZE_FLOAT64);
	return;
    }

#ifdef __GLX_ALIGN64
    x = k * __GLX_SIZE_FLOAT64;
#endif
    for (i = 0; i<majorOrder; i++) {
	for (j = 0; j<minorOrder; j++) {
#ifdef __GLX_ALIGN64
	    __GLX_MEM_COPY(data, points, x);
#else
	    for (x = 0; x<k; x++) {
		data[x] = points[x];
	    }
#endif
	    points += minorStride;
	    data += k;
	}
	points += majorStride - minorStride * minorOrder;
    }
}
