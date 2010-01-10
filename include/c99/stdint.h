/**************************************************************************
 *
 * Copyright 2007-2010 VMware, Inc.
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

/*
 * stdint.h --
 *
 *    Portable subset of C99's stdint.h.
 *
 *    At the moment it only supports MSVC, given all other mainstream compilers
 *    already support C99. If this is necessary for other compilers then it
 *    might be worth to replace this with
 *    http://www.azillionmonkeys.com/qed/pstdint.h.
 */

#ifndef _STDINT_H_
#define _STDINT_H_


#ifndef INT8_MAX
#define INT8_MAX           127
#endif
#ifndef INT8_MIN
#define INT8_MIN           -128
#endif
#ifndef UINT8_MAX
#define UINT8_MAX          255
#endif
#ifndef INT16_MAX
#define INT16_MAX          32767
#endif
#ifndef INT16_MIN
#define INT16_MIN          -32768
#endif
#ifndef UINT16_MAX
#define UINT16_MAX         65535
#endif
#ifndef INT32_MAX
#define INT32_MAX          2147483647
#endif
#ifndef INT32_MIN
#define INT32_MIN          -2147483648
#endif
#ifndef UINT32_MAX
#define UINT32_MAX         4294967295U
#endif

#ifndef INT8_C
#define INT8_C(__val)     __val
#endif
#ifndef UINT8_C
#define UINT8_C(__val)    __val
#endif
#ifndef INT16_C
#define INT16_C(__val)    __val
#endif
#ifndef UINT16_C
#define UINT16_C(__val)   __val
#endif
#ifndef INT32_C
#define INT32_C(__val)    __val
#endif
#ifndef UINT32_C
#define UINT32_C(__val)   __val##U
#endif


#if defined(_MSC_VER)

typedef __int8             int8_t;
typedef unsigned __int8    uint8_t;
typedef __int16            int16_t;
typedef unsigned __int16   uint16_t;
#ifndef __eglplatform_h_
typedef __int32            int32_t;
#endif
typedef unsigned __int32   uint32_t;
typedef __int64            int64_t;
typedef unsigned __int64   uint64_t;

#if defined(_WIN64)
typedef __int64            intptr_t;
typedef unsigned __int64   uintptr_t;
#else
typedef __int32            intptr_t;
typedef unsigned __int32   uintptr_t;
#endif

#define INT64_C(__val)    __val##i64
#define UINT64_C(__val)   __val##ui64

#else
#error "Unsupported compiler"
#endif

#endif /* _STDINT_H_ */
