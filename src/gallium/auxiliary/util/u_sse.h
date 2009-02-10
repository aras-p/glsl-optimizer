/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * @file
 * SSE intrinsics portability header.
 * 
 * Although the SSE intrinsics are support by all modern x86 and x86-64 
 * compilers, there are some intrisincs missing in some implementations 
 * (especially older MSVC versions). This header abstracts that away.
 */

#ifndef U_SSE_H_
#define U_SSE_H_

#include "pipe/p_config.h"

#if defined(PIPE_ARCH_SSE)

#include <xmmintrin.h>
#include <emmintrin.h>


/* MSVC before VC8 does not support the _mm_castxxx_yyy */
#if defined(_MSC_VER) && _MSC_VER < 1500

union __declspec(align(16)) m128_types {
   __m128 m128;
   __m128i m128i;
   __m128d m128d;
};

static __inline __m128
_mm_castsi128_ps(__m128i a)
{
   union m128_types u;
   u.m128i = a;
   return u.m128;
}

static __inline __m128i
_mm_castps_si128(__m128 a)
{
   union m128_types u;
   u.m128 = a;
   return u.m128i;
}

#endif /* defined(_MSC_VER) && _MSC_VER < 1500 */

#endif /* PIPE_ARCH_X86 || PIPE_ARCH_X86_64 */

#endif /* U_SSE_H_ */
