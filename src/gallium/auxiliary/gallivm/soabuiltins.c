/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * This file is compiled with clang into the LLVM bitcode
  *
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */
typedef __attribute__(( ext_vector_type(4) )) float float4;


extern float fabsf(float val);

void abs(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w)
{
   res[0].x = fabsf(tmp0x.x);
   res[0].y = fabsf(tmp0x.y);
   res[0].z = fabsf(tmp0x.z);
   res[0].w = fabsf(tmp0x.w);

   res[1].x = fabsf(tmp0y.x);
   res[1].y = fabsf(tmp0y.y);
   res[1].z = fabsf(tmp0y.z);
   res[1].w = fabsf(tmp0y.w);

   res[2].x = fabsf(tmp0z.x);
   res[2].y = fabsf(tmp0z.y);
   res[2].z = fabsf(tmp0z.z);
   res[2].w = fabsf(tmp0z.w);

   res[3].x = fabsf(tmp0w.x);
   res[3].y = fabsf(tmp0w.y);
   res[3].z = fabsf(tmp0w.z);
   res[3].w = fabsf(tmp0w.w);
}

void dp3(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   float4 dot = (tmp0x * tmp1x) + (tmp0y * tmp1y) +
                (tmp0z * tmp1z);

   res[0] = dot;
   res[1] = dot;
   res[2] = dot;
   res[3] = dot;
}


void dp4(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   float4 dot = (tmp0x * tmp1x) + (tmp0y * tmp1y) +
                (tmp0z * tmp1z) + (tmp0w * tmp1w);

   res[0] = dot;
   res[1] = dot;
   res[2] = dot;
   res[3] = dot;
}

extern float powf(float num, float p);

float4 powvec(float4 vec, float4 q)
{
   float4 p;
   p.x = powf(vec.x, q.x);
   p.y = powf(vec.y, q.y);
   p.z = powf(vec.z, q.z);
   p.w = powf(vec.w, q.w);
   return p;
}

void pow(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   res[0] = powvec(tmp0x, tmp1x);
   res[1] = res[0];
   res[2] = res[0];
   res[3] = res[0];
}

void lit(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   const float4 zerovec = (float4) {0, 0, 0, 0};
   float4 tmpx = tmp0x;
   float4 tmpy = tmp0y;

   res[0] = (float4){1.0, 1.0, 1.0, 1.0};
   res[1] = tmpx;
   res[2] = tmpy;
   res[3] = (float4){1.0, 1.0, 1.0, 1.0};
}

float4 minvec(float4 a, float4 b)
{
   return (float4){(a.x < b.x) ? a.x : b.x,
         (a.y < b.y) ? a.y : b.y,
         (a.z < b.z) ? a.z : b.z,
         (a.w < b.w) ? a.w : b.w};
}

void min(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   res[0] = minvec(tmp0x, tmp1x);
   res[1] = minvec(tmp0y, tmp1y);
   res[2] = minvec(tmp0z, tmp1z);
   res[3] = minvec(tmp0w, tmp1w);
}


float4 maxvec(float4 a, float4 b)
{
   return (float4){(a.x > b.x) ? a.x : b.x,
         (a.y > b.y) ? a.y : b.y,
         (a.z > b.z) ? a.z : b.z,
         (a.w > b.w) ? a.w : b.w};
}

void max(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   res[0] = maxvec(tmp0x, tmp1x);
   res[1] = maxvec(tmp0y, tmp1y);
   res[2] = maxvec(tmp0z, tmp1z);
   res[3] = maxvec(tmp0w, tmp1w);
}
