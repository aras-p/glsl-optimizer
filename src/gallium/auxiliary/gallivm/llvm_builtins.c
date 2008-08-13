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
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */
typedef __attribute__(( ext_vector_type(4) )) float float4;

extern float powf(float a, float b);

inline float approx(float a, float b)
{
    if (b < -128.0f) b = -128.0f;
    if (b > 128.0f)   b = 128.0f;
    if (a < 0) a = 0;
    return powf(a, b);
}

inline float4 lit(float4 tmp)
{
    float4 result;
    result.x = 1.0;
    result.w = 1.0;
    if (tmp.x > 0) {
        result.y = tmp.x;
        result.z = approx(tmp.y, tmp.w);
    } else {
        result.y = 0;
        result.z = 0;
    }
    return result;
}

inline float4 cmp(float4 tmp0, float4 tmp1, float4 tmp2)
{
   float4 result;

   result.x = (tmp0.x < 0.0) ? tmp1.x : tmp2.x;
   result.y = (tmp0.y < 0.0) ? tmp1.y : tmp2.y;
   result.z = (tmp0.z < 0.0) ? tmp1.z : tmp2.z;
   result.w = (tmp0.w < 0.0) ? tmp1.w : tmp2.w;

   return result;
}

extern float cosf(float  val);
extern float sinf(float  val);

inline float4 vcos(float4 val)
{
   float4 result;
   printf("VEC IN   is %f %f %f %f\n", val.x, val.y, val.z, val.w);
   result.x = cosf(val.x);
   result.y = cosf(val.x);
   result.z = cosf(val.x);
   result.w = cosf(val.x);
   printf("VEC OUT  is %f %f %f %f\n", result.x, result.y, result.z, result.w);
   return result;
}

inline float4 scs(float4 val)
{
   float4 result;
   float tmp = val.x;
   result.x = cosf(tmp);
   result.y = sinf(tmp);
   return result;
}


inline float4 vsin(float4 val)
{
   float4 result;
   float tmp = val.x;
   float res = sinf(tmp);
   result.x = res;
   result.y = res;
   result.z = res;
   result.w = res;
   return result;
}

inline int kil(float4 val)
{
   if (val.x < 0 || val.y < 0 || val.z < 0 || val.w < 0)
      return 1;
   else
      return 0;
}
