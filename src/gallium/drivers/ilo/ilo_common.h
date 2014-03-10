/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_COMMON_H
#define ILO_COMMON_H

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"
#include "pipe/p_format.h"

#include "util/u_debug.h"
#include "util/u_double_list.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_pointer.h"

#define ILO_GEN(gen) ((int) (gen * 100))
#define ILO_GEN_GET_MAJOR(gen) (gen / 100)

/* enable debug flags affecting hot pathes only with debug builds */
#ifdef DEBUG
#define ILO_DEBUG_HOT 1
#else
#define ILO_DEBUG_HOT 0
#endif

enum ilo_debug {
   ILO_DEBUG_3D        = 1 << 0,
   ILO_DEBUG_VS        = 1 << 1,
   ILO_DEBUG_GS        = 1 << 2,
   ILO_DEBUG_FS        = 1 << 3,
   ILO_DEBUG_CS        = 1 << 4,
   ILO_DEBUG_DRAW      = ILO_DEBUG_HOT << 5,
   ILO_DEBUG_FLUSH     = 1 << 6,

   /* flags that affect the behaviors of the driver */
   ILO_DEBUG_NOHW      = 1 << 20,
   ILO_DEBUG_NOCACHE   = 1 << 21,
   ILO_DEBUG_NOHIZ     = 1 << 22,
};

struct ilo_dev_info {
   /* these mirror intel_winsys_info */
   int devid;
   int max_batch_size;
   bool has_llc;
   bool has_address_swizzling;
   bool has_logical_context;
   bool has_ppgtt;
   bool has_timestamp;
   bool has_gen7_sol_reset;

   int gen;
   int gt;
   int urb_size;
};

extern int ilo_debug;

/**
 * Print a message, for dumping or debugging.
 */
static inline void _util_printf_format(1, 2)
ilo_printf(const char *format, ...)
{
   va_list ap;

   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
}

/**
 * Print a critical error.
 */
static inline void _util_printf_format(1, 2)
ilo_err(const char *format, ...)
{
   va_list ap;

   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
}

/**
 * Print a warning, silenced for release builds.
 */
static inline void _util_printf_format(1, 2)
ilo_warn(const char *format, ...)
{
#ifdef DEBUG
   va_list ap;

   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
#else
#endif
}

#endif /* ILO_COMMON_H */
