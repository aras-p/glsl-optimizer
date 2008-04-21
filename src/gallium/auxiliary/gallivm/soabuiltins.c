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

void pow(float4 *res,
         float4 tmp0x, float4 tmp0y, float4 tmp0z, float4 tmp0w,
         float4 tmp1x, float4 tmp1y, float4 tmp1z, float4 tmp1w)
{
   float4 p;
   p.x = powf(tmp0x.x, tmp1x.x);
   p.y = powf(tmp0x.y, tmp1x.y);
   p.z = powf(tmp0x.z, tmp1x.z);
   p.w = powf(tmp0x.w, tmp1x.w);

   res[0] = p;
   res[1] = p;
   res[2] = p;
   res[3] = p;
}

#if 0
void yo(float4 *out, float4 *in)
{
   float4 res[4];

   dp3(res, in[0], in[1], in[2], in[3],
       in[4], in[5], in[6], in[7]);
   out[1] = res[1];
}
#endif
