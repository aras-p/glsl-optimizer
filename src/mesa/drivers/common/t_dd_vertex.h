/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 * Author:
 *   Keith Whitwell <keithw@valinux.com>
 */

#if (COLOR_IS_RBGA)
typedef struct {
  GLubyte red;
  GLubyte green;
  GLubyte blue;
  GLubyte alpha;
} TAG(_color);
#else 
typedef struct {
  GLubyte blue;
  GLubyte green;
  GLubyte red;
  GLubyte alpha;
} TAG(_color);
#endif

typedef union {
   struct {
      float x, y, z, w;
      TAG(_color) color;
      TAG(_color) specular;
      float u0, v0;
      float u1, v1;
      float u2, v2;
      float u3, v3;
   } v;
   struct {
      float x, y, z, w;
      TAG(_color) color;
      TAG(_color) specular;
      float u0, v0, q0;
      float u1, v1, q1;
      float u2, v2, q2;
      float u3, v3, q3;
   } pv;
   struct {
      float x, y, z;
      TAG(_color) color;
   } tv;
   float f[16];
   unsigned int ui[16];
   unsigned char ub4[4][16];
} TAG(Vertex), *TAG(VertexPtr);
