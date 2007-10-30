/*clang --emit-llvm llvm_builtins.c |llvm-as|opt -std-compile-opts|llvm2cpp -gen-contents -o=gallivm_builtins.cpp -f -for=shader -funcname=createGallivmBuiltins*/
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
typedef __attribute__(( ocu_vector_type(4) )) float float4;


extern float powf(float a, float b);

inline float approx(float a, float b)
{
    if (b < -128.0f) b = -128.0f;
    if (b > 128.0f)   b = 128.0f;
    if (a < 0) a = 0;
    return powf(a, b);
}

float4 lit(float4 tmp)
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
