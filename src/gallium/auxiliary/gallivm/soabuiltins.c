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

float4 absvec(float4 vec)
{
   float4 res;
   res.x = fabsf(vec.x);
   res.y = fabsf(vec.y);
   res.z = fabsf(vec.z);
   res.w = fabsf(vec.w);

   return res;
}

void abs(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w)
{
   res[0] = absvec(tmp0x);
   res[1] = absvec(tmp0y);
   res[2] = absvec(tmp0z);
   res[3] = absvec(tmp0w);
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
extern float sqrtf(float x);

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


void lit(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w)
{
   const float4 zerovec = (float4) {0.0, 0.0, 0.0, 0.0};
   const float4 min128 = (float4) {-128.f, -128.f, -128.f, -128.f};
   const float4 plus128 = (float4) {128.f,  128.f,  128.f,  128.f};

   res[0] = (float4){1.0, 1.0, 1.0, 1.0};
   if (tmp0x.x > 0) {
      float4 tmpy = maxvec(tmp0y, zerovec);
      float4 tmpw = minvec(tmp0w, plus128);
      tmpw = maxvec(tmpw, min128);
      res[1] = tmp0x;
      res[2] = powvec(tmpy, tmpw);
   } else {
      res[1] = zerovec;
      res[2] = zerovec;
   }
   res[3] = (float4){1.0, 1.0, 1.0, 1.0};
}


float4 sqrtvec(float4 vec)
{
   float4 p;
   p.x = sqrtf(vec.x);
   p.y = sqrtf(vec.y);
   p.z = sqrtf(vec.z);
   p.w = sqrtf(vec.w);
   return p;
}

void rsq(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w)
{
   const float4 onevec = (float4) {1., 1., 1., 1.};
   res[0] = onevec/sqrtvec(absvec(tmp0x));
   res[1] = onevec/sqrtvec(absvec(tmp0y));
   res[2] = onevec/sqrtvec(absvec(tmp0z));
   res[3] = onevec/sqrtvec(absvec(tmp0w));
}
