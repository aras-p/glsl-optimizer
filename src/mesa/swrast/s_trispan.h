/* $Id: s_trispan.h,v 1.1 2001/05/14 16:23:04 brianp Exp $ */

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


#ifndef S_TRISPAN_H
#define S_TRISPAN_H


/*
 * The triangle_span structure is used by the triangle template code in
 * s_tritemp.h.  It describes how colors, Z, texcoords, etc are to be
 * interpolated across each scanline of triangle.
 * With this structure it's easy to hand-off span rasterization to a
 * subroutine instead of doing it all inline like we used to do.
 * It also cleans up the local variable namespace a great deal.
 *
 * It would be interesting to experiment with multiprocessor rasterization
 * with this structure.  The triangle rasterizer could simply emit a
 * stream of these structures which would be consumed by one or more
 * span-processing threads which could run in parallel.
 */


#define SPAN_RGBA         0x01
#define SPAN_SPEC         0x02
#define SPAN_INDEX        0x04
#define SPAN_Z            0x08
#define SPAN_FOG          0x10
#define SPAN_TEXTURE      0x20
#define SPAN_INT_TEXTURE  0x40
#define SPAN_LAMBDA       0x80


struct triangle_span {
   GLint x, y;
   GLuint count;
   GLuint activeMask;  /* OR of the SPAN_* flags */
   GLfixed red, redStep;
   GLfixed green, greenStep;
   GLfixed blue, blueStep;
   GLfixed alpha, alphaStep;
   GLfixed specRed, specRedStep;
   GLfixed specGreen, specGreenStep;
   GLfixed specBlue, specBlueStep;
   GLfixed index, indexStep;
   GLfixed z, zStep;
   GLfloat fog, fogStep;
   GLfloat tex[MAX_TEXTURE_UNITS][4], texStep[MAX_TEXTURE_UNITS][4];
   GLfixed intTex[2], intTexStep[2];
   /* Needed for texture lambda (LOD) computation */
   GLfloat rho[MAX_TEXTURE_UNITS];
   GLfloat texWidth[MAX_TEXTURE_UNITS], texHeight[MAX_TEXTURE_UNITS];
};


#endif /* S_TRISPAN_H */
