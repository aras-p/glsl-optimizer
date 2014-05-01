/**************************************************************************
 *
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef EGLCOMPILER_INCLUDED
#define EGLCOMPILER_INCLUDED


#include "c99_compat.h" /* inline, __func__, etc. */


/**
 * Get standard integer types
 */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
    (defined(_MSC_VER) && _MSC_VER >= 1600)
#  include <stdint.h>
#elif defined(_MSC_VER)
   typedef __int8             int8_t;
   typedef unsigned __int8    uint8_t;
   typedef __int16            int16_t;
   typedef unsigned __int16   uint16_t;
   typedef __int32            int32_t;
   typedef unsigned __int32   uint32_t;
   typedef __int64            int64_t;
   typedef unsigned __int64   uint64_t;

#  if defined(_WIN64)
     typedef __int64            intptr_t;
     typedef unsigned __int64   uintptr_t;
#  else
     typedef __int32            intptr_t;
     typedef unsigned __int32   uintptr_t;
#  endif

#  define INT64_C(__val) __val##i64
#  define UINT64_C(__val) __val##ui64
#else
/* hope the best instead of adding a bunch of ifdef's */
#  include <stdint.h>
#endif


/* XXX: Use standard `inline` keyword instead */
#ifndef INLINE
#  define INLINE inline
#endif


/**
 * Function visibility
 */
#ifndef PUBLIC
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PUBLIC __attribute__((visibility("default")))
#  elif defined(_MSC_VER)
#    define PUBLIC __declspec(dllexport)
#  else
#    define PUBLIC
#  endif
#endif

/* XXX: Use standard `__func__` instead */
#ifndef __FUNCTION__
#  define __FUNCTION__ __func__
#endif

#define STATIC_ASSERT(COND) \
   do { \
      (void) sizeof(char [1 - 2*!(COND)]); \
   } while (0)

#endif /* EGLCOMPILER_INCLUDED */
