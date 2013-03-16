/* Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#version 120
mat2 inverse(mat2 m)
{
   mat2 adj;
   adj[0].x = m[1].y;
   adj[0].y = -m[0].y;
   adj[1].x = -m[1].x;
   adj[1].y = m[0].x;
   float det = m[0].x * m[1].y - m[1].x * m[0].y;
   return adj / det;
}

mat3 inverse(mat3 m)
{
   mat3 adj;
   adj[0].x = + (m[1].y * m[2].z - m[2].y * m[1].z);
   adj[1].x = - (m[1].x * m[2].z - m[2].x * m[1].z);
   adj[2].x = + (m[1].x * m[2].y - m[2].x * m[1].y);
   adj[0].y = - (m[0].y * m[2].z - m[2].y * m[0].z);
   adj[1].y = + (m[0].x * m[2].z - m[2].x * m[0].z);
   adj[2].y = - (m[0].x * m[2].y - m[2].x * m[0].y);
   adj[0].z = + (m[0].y * m[1].z - m[1].y * m[0].z);
   adj[1].z = - (m[0].x * m[1].z - m[1].x * m[0].z);
   adj[2].z = + (m[0].x * m[1].y - m[1].x * m[0].y);

   float det = (+ m[0].x * (m[1].y * m[2].z - m[1].z * m[2].y)
		- m[0].y * (m[1].x * m[2].z - m[1].z * m[2].x)
		+ m[0].z * (m[1].x * m[2].y - m[1].y * m[2].x));

   return adj / det;
}

mat4 inverse(mat4 m)
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

   mat4 adj;

   adj[0].x = + (m[1].y * SubFactor00 - m[1].z * SubFactor01 + m[1].w * SubFactor02);
   adj[1].x = - (m[1].x * SubFactor00 - m[1].z * SubFactor03 + m[1].w * SubFactor04);
   adj[2].x = + (m[1].x * SubFactor01 - m[1].y * SubFactor03 + m[1].w * SubFactor05);
   adj[3].x = - (m[1].x * SubFactor02 - m[1].y * SubFactor04 + m[1].z * SubFactor05);

   adj[0].y = - (m[0].y * SubFactor00 - m[0].z * SubFactor01 + m[0].w * SubFactor02);
   adj[1].y = + (m[0].x * SubFactor00 - m[0].z * SubFactor03 + m[0].w * SubFactor04);
   adj[2].y = - (m[0].x * SubFactor01 - m[0].y * SubFactor03 + m[0].w * SubFactor05);
   adj[3].y = + (m[0].x * SubFactor02 - m[0].y * SubFactor04 + m[0].z * SubFactor05);

   adj[0].z = + (m[0].y * SubFactor06 - m[0].z * SubFactor07 + m[0].w * SubFactor08);
   adj[1].z = - (m[0].x * SubFactor06 - m[0].z * SubFactor09 + m[0].w * SubFactor10);
   adj[2].z = + (m[0].x * SubFactor11 - m[0].y * SubFactor09 + m[0].w * SubFactor12);
   adj[3].z = - (m[0].x * SubFactor08 - m[0].y * SubFactor10 + m[0].z * SubFactor12);

   adj[0].w = - (m[0].y * SubFactor13 - m[0].z * SubFactor14 + m[0].w * SubFactor15);
   adj[1].w = + (m[0].x * SubFactor13 - m[0].z * SubFactor16 + m[0].w * SubFactor17);
   adj[2].w = - (m[0].x * SubFactor14 - m[0].y * SubFactor16 + m[0].w * SubFactor18);
   adj[3].w = + (m[0].x * SubFactor15 - m[0].y * SubFactor17 + m[0].z * SubFactor18);

   float det = (+ m[0].x * adj[0].x
		+ m[0].y * adj[1].x
		+ m[0].z * adj[2].x
		+ m[0].w * adj[3].x);

   return adj / det;
}

