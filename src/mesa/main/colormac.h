/* $Id: colormac.h,v 1.1 2000/10/28 20:41:13 brianp Exp $ */

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
 * Color-related macros
 */

#ifndef COLORMAC_H
#define COLORMAC_H


#include "glheader.h"
#include "config.h"
#include "macros.h"
#include "mmath.h"



/*
 * Integer / float conversion for colors, normals, etc.
 */

#define BYTE_TO_UBYTE(b)   (b < 0 ? 0 : (GLubyte) b)
#define SHORT_TO_UBYTE(s)  (s < 0 ? 0 : (GLubyte) (s >> 7))
#define USHORT_TO_UBYTE(s)              (GLubyte) (s >> 8)
#define INT_TO_UBYTE(i)    (i < 0 ? 0 : (GLubyte) (i >> 23))
#define UINT_TO_UBYTE(i)                (GLubyte) (i >> 24)

/* Convert GLfloat in [0.0,1.0] to GLubyte in [0,255] */
#define FLOAT_TO_UBYTE(X)	((GLubyte) (GLint) (((X)) * 255.0F))


/* Convert GLbyte in [-128,127] to GLfloat in [-1.0,1.0] */
#define BYTE_TO_FLOAT(B)	((2.0F * (B) + 1.0F) * (1.0F/255.0F))

/* Convert GLfloat in [-1.0,1.0] to GLbyte in [-128,127] */
#define FLOAT_TO_BYTE(X)	( (((GLint) (255.0F * (X))) - 1) / 2 )


/* Convert GLushort in [0,65536] to GLfloat in [0.0,1.0] */
#define USHORT_TO_FLOAT(S)	((GLfloat) (S) * (1.0F / 65535.0F))

/* Convert GLfloat in [0.0,1.0] to GLushort in [0,65536] */
#define FLOAT_TO_USHORT(X)	((GLushort) (GLint) ((X) * 65535.0F))


/* Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)	((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

/* Convert GLfloat in [0.0,1.0] to GLshort in [-32768,32767] */
#define FLOAT_TO_SHORT(X)	( (((GLint) (65535.0F * (X))) - 1) / 2 )


/* Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
#define UINT_TO_FLOAT(U)	((GLfloat) (U) * (1.0F / 4294967295.0F))

/* Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
#define FLOAT_TO_UINT(X)	((GLuint) ((X) * 4294967295.0))


/* Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)		((2.0F * (I) + 1.0F) * (1.0F/4294967294.0F))

/* Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
/* causes overflow:
#define FLOAT_TO_INT(X)		( (((GLint) (4294967294.0F * (X))) - 1) / 2 )
*/
/* a close approximation: */
#define FLOAT_TO_INT(X)		( (GLint) (2147483647.0 * (X)) )



#if CHAN_BITS == 8

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (GLchan) (b))
#define UBYTE_TO_CHAN(b)  (b)
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) ((s) >> 7))
#define USHORT_TO_CHAN(s) ((GLchan) ((s) >> 8))
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 23))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 24))

#define CHAN_TO_FLOAT(c)  UBYTE_TO_FLOAT(c)

#define FLOAT_COLOR_TO_CHAN(c, f) FLOAT_COLOR_TO_UBYTE_COLOR(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4UBV(DST, SRC)

#define CHAN_PRODUCT(a, b)  ( (GLubyte) (((GLint)(a) * ((GLint)(b) + 1)) >> 8) )

#elif CHAN_BITS == 16

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (GLchan) ((b) * 516))
#define UBYTE_TO_CHAN(b)  ((GLchan) (((b) << 8) | (b)))
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) (s))
#define USHORT_TO_CHAN(s) (s)
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 15))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 16))

#define CHAN_TO_FLOAT(c)  ((GLfloat) ((c) * (1.0 / CHAN_MAXF)))

#define FLOAT_COLOR_TO_CHAN(c, f) \
	c = ((GLchan) FloatToInt(CLAMP(f, 0.0F, 1.0F) * CHAN_MAXF + 0.5F))

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b)  ( (GLchan) ((((GLint) (a)) * ((GLint) (b))) / 65535) )

#elif CHAN_BITS == 32

/* XXX floating-point color channels not fully thought-out */
#define BYTE_TO_CHAN(b)   ((GLfloat) ((b) * (1.0F / 127.0F)))
#define UBYTE_TO_CHAN(b)  ((GLfloat) ((b) * (1.0F / 255.0F)))
#define SHORT_TO_CHAN(s)  ((GLfloat) ((s) * (1.0F / 32767.0F)))
#define USHORT_TO_CHAN(s) ((GLfloat) ((s) * (1.0F / 65535.0F)))
#define INT_TO_CHAN(i)    ((GLfloat) ((i) * (1.0F / 2147483647.0F)))
#define UINT_TO_CHAN(i)   ((GLfloat) ((i) * (1.0F / 4294967295.0F)))

#define CHAN_TO_FLOAT(c)  (c)

#define FLOAT_COLOR_TO_CHAN(c, f)  c = f

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b)    ((a) * (b))

#else 

#error unexpected CHAN_BITS size

#endif


#define FLOAT_RGB_TO_CHAN_RGB(dst, f)		\
do {						\
   FLOAT_COLOR_TO_CHAN(dst[0], f[0]);		\
   FLOAT_COLOR_TO_CHAN(dst[1], f[1]);		\
   FLOAT_COLOR_TO_CHAN(dst[2], f[2]);		\
} while(0)

#define FLOAT_RGBA_TO_CHAN_RGBA(c, f)		\
do {						\
   FLOAT_COLOR_TO_CHAN(c[0], f[0]);		\
   FLOAT_COLOR_TO_CHAN(c[1], f[1]);		\
   FLOAT_COLOR_TO_CHAN(c[2], f[2]);		\
   FLOAT_COLOR_TO_CHAN(c[3], f[3]);		\
} while(0)


#if CHAN_BITS == 32

#define FLOAT_TO_CHAN(f)   (f)
#define DOUBLE_TO_CHAN(f)  ((GLfloat) (f))

#else

#define FLOAT_TO_CHAN(f)   ( (GLchan) FloatToInt((f) * CHAN_MAXF + 0.5F) )
#define DOUBLE_TO_CHAN(f)  ( (GLchan) FloatToInt((f) * CHAN_MAXF + 0.5F) )

#endif


#endif /* COLORMAC_H */
