/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#version 120

// Forward declaration because builtins don't know about other builtins.
float dot(vec4, vec4);

float determinant(mat2 m)
{
   return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

float determinant(mat3 m)
{
   return (+ m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
           - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
           + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));
}

float determinant(mat4 m)
{
   float SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
   float SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
   float SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
   float SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
   float SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
   float SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
   float SubFactor06 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
   float SubFactor07 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
   float SubFactor08 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
   float SubFactor09 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
   float SubFactor10 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
   float SubFactor11 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
   float SubFactor12 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
   float SubFactor13 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
   float SubFactor14 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
   float SubFactor15 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
   float SubFactor16 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
   float SubFactor17 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
   float SubFactor18 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

   vec4 adj_0;

   adj_0[0] = + (m[1][1] * SubFactor00 - m[1][2] * SubFactor01 + m[1][3] * SubFactor02);
   adj_0[1] = - (m[1][0] * SubFactor00 - m[1][2] * SubFactor03 + m[1][3] * SubFactor04);
   adj_0[2] = + (m[1][0] * SubFactor01 - m[1][1] * SubFactor03 + m[1][3] * SubFactor05);
   adj_0[3] = - (m[1][0] * SubFactor02 - m[1][1] * SubFactor04 + m[1][2] * SubFactor05);

   return dot(m[0], adj_0);
}
