/* $Id: colormac.h,v 1.9 2001/03/11 18:49:11 gareth Exp $ */

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
/* Do not reference mtypes.h from this file.
 */


#if CHAN_BITS == 8

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (GLchan) (b))
#define UBYTE_TO_CHAN(b)  (b)
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) ((s) >> 7))
#define USHORT_TO_CHAN(s) ((GLchan) ((s) >> 8))
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 23))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 24))

#define CHAN_TO_UBYTE(c)  (c)
#define CHAN_TO_FLOAT(c)  UBYTE_TO_FLOAT(c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)    CLAMPED_FLOAT_TO_UBYTE(c, f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)  UNCLAMPED_FLOAT_TO_UBYTE(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4UBV(DST, SRC)

#define CHAN_PRODUCT(a, b)  ((GLubyte) (((GLint)(a) * ((GLint)(b) + 1)) >> 8))


#elif CHAN_BITS == 16

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (((GLchan) (b)) * 516))
#define UBYTE_TO_CHAN(b)  ((((GLchan) (b)) << 8) | ((GLchan) (b)))
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) (s))
#define USHORT_TO_CHAN(s) (s)
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 15))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 16))

#define CHAN_TO_UBYTE(c)  ((c) >> 8)
#define CHAN_TO_FLOAT(c)  ((GLfloat) ((c) * (1.0 / CHAN_MAXF)))

#define CLAMPED_FLOAT_TO_CHAN(c, f) \
   c = ((GLchan) IROUND((f) * CHAN_MAXF))
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)   \
   c = ( (GLchan) IROUND( CLAMP(f, 0.0, 1.0) * CHAN_MAXF) )

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b) ((GLchan) ((((GLint) (a)) * ((GLint) (b))) / 65535))


#elif CHAN_BITS == 32

/* XXX floating-point color channels not fully thought-out */
#define BYTE_TO_CHAN(b)   ((GLfloat) ((b) * (1.0F / 127.0F)))
#define UBYTE_TO_CHAN(b)  ((GLfloat) ((b) * (1.0F / 255.0F)))
#define SHORT_TO_CHAN(s)  ((GLfloat) ((s) * (1.0F / 32767.0F)))
#define USHORT_TO_CHAN(s) ((GLfloat) ((s) * (1.0F / 65535.0F)))
#define INT_TO_CHAN(i)    ((GLfloat) ((i) * (1.0F / 2147483647.0F)))
#define UINT_TO_CHAN(i)   ((GLfloat) ((i) * (1.0F / 4294967295.0F)))

#define CHAN_TO_UBYTE(c)  FLOAT_TO_UBYTE(c)
#define CHAN_TO_FLOAT(c)  (c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)  c = (f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)      c = (f)

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b)    ((a) * (b))

#else

#error unexpected CHAN_BITS size

#endif



/*
 * Convert 3 channels at once.
 */
#define UNCLAMPED_FLOAT_TO_RGB_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
} while (0)


/*
 * Convert 4 channels at once.
 */
#define UNCLAMPED_FLOAT_TO_RGBA_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[3], f[3]);	\
} while (0)


#endif /* COLORMAC_H */
