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
   return m[0].x * m[1].y - m[1].x * m[0].y;
}

float determinant(mat3 m)
{
   return (+ m[0].x * (m[1].y * m[2].z - m[1].z * m[2].y)
           - m[0].y * (m[1].x * m[2].z - m[1].z * m[2].x)
           + m[0].z * (m[1].x * m[2].y - m[1].y * m[2].x));
}

float determinant(mat4 m)
{
   float SubFactor00 = m[2].z * m[3].w - m[3].z * m[2].w;
   float SubFactor01 = m[2].y * m[3].w - m[3].y * m[2].w;
   float SubFactor02 = m[2].y * m[3].z - m[3].y * m[2].z;
   float SubFactor03 = m[2].x * m[3].w - m[3].x * m[2].w;
   float SubFactor04 = m[2].x * m[3].z - m[3].x * m[2].z;
   float SubFactor05 = m[2].x * m[3].y - m[3].x * m[2].y;
   float SubFactor06 = m[1].z * m[3].w - m[3].z * m[1].w;
   float SubFactor07 = m[1].y * m[3].w - m[3].y * m[1].w;
   float SubFactor08 = m[1].y * m[3].z - m[3].y * m[1].z;
   float SubFactor09 = m[1].x * m[3].w - m[3].x * m[1].w;
   float SubFactor10 = m[1].x * m[3].z - m[3].x * m[1].z;
   float SubFactor11 = m[1].y * m[3].w - m[3].y * m[1].w;
   float SubFactor12 = m[1].x * m[3].y - m[3].x * m[1].y;
   float SubFactor13 = m[1].z * m[2].w - m[2].z * m[1].w;
   float SubFactor14 = m[1].y * m[2].w - m[2].y * m[1].w;
   float SubFactor15 = m[1].y * m[2].z - m[2].y * m[1].z;
   float SubFactor16 = m[1].x * m[2].w - m[2].x * m[1].w;
   float SubFactor17 = m[1].x * m[2].z - m[2].x * m[1].z;
   float SubFactor18 = m[1].x * m[2].y - m[2].x * m[1].y;

   vec4 adj_0;

   adj_0.x = + (m[1].y * SubFactor00 - m[1].z * SubFactor01 + m[1].w * SubFactor02);
   adj_0.y = - (m[1].x * SubFactor00 - m[1].z * SubFactor03 + m[1].w * SubFactor04);
   adj_0.z = + (m[1].x * SubFactor01 - m[1].y * SubFactor03 + m[1].w * SubFactor05);
   adj_0.w = - (m[1].x * SubFactor02 - m[1].y * SubFactor04 + m[1].z * SubFactor05);

   return dot(m[0], adj_0);
}
