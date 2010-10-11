/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/

#ifndef _LP_DEBUG_INTRIN_H_
#define _LP_DEBUG_INTRIN_H_

#include "pipe/p_config.h"

#if defined(PIPE_ARCH_SSE)

#include <emmintrin.h>

static INLINE void print_epi8(const char *name, __m128i r)
{
   union { __m128i m; ubyte ub[16]; } u;
   u.m = r;

   debug_printf("%s: "
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x\n",
                name,
                u.ub[0],  u.ub[1],  u.ub[2],  u.ub[3],
                u.ub[4],  u.ub[5],  u.ub[6],  u.ub[7],
                u.ub[8],  u.ub[9],  u.ub[10], u.ub[11],
                u.ub[12], u.ub[13], u.ub[14], u.ub[15]);
}

static INLINE void print_epi16(const char *name, __m128i r)
{
   union { __m128i m; ushort us[8]; } u;
   u.m = r;

   debug_printf("%s: "
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x\n",
                name,
                u.us[0],  u.us[1],  u.us[2],  u.us[3],
                u.us[4],  u.us[5],  u.us[6],  u.us[7]);
}

static INLINE void print_epi32(const char *name, __m128i r)
{
   union { __m128i m; uint ui[4]; } u;
   u.m = r;

   debug_printf("%s: "
                "%08x/"
                "%08x/"
                "%08x/"
                "%08x\n",
                name,
                u.ui[0],  u.ui[1],  u.ui[2],  u.ui[3]);
}

static INLINE void print_ps(const char *name, __m128 r)
{
   union { __m128 m; float f[4]; } u;
   u.m = r;

   debug_printf("%s: "
                "%f/"
                "%f/"
                "%f/"
                "%f\n",
                name,
                u.f[0],  u.f[1],  u.f[2],  u.f[3]);
}

 
#endif
#endif
