/*
 * Copyright 2003 VMware, Inc.
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

/**
 * \file intel_debug.h
 *
 * Basic INTEL_DEBUG environment variable handling.  This file defines the
 * list of debugging flags, as well as some macros for handling them.
 */

extern uint64_t INTEL_DEBUG;

#define DEBUG_TEXTURE	  0x1
#define DEBUG_STATE	  0x2
#define DEBUG_BLIT	  0x8
#define DEBUG_MIPTREE     0x10
#define DEBUG_PERF	  0x20
#define DEBUG_PERFMON     0x40
#define DEBUG_BATCH       0x80
#define DEBUG_PIXEL       0x100
#define DEBUG_BUFMGR      0x200
#define DEBUG_FBO         0x800
#define DEBUG_GS          0x1000
#define DEBUG_SYNC	  0x2000
#define DEBUG_PRIMS	  0x4000
#define DEBUG_VERTS	  0x8000
#define DEBUG_DRI         0x10000
#define DEBUG_SF          0x20000
#define DEBUG_STATS       0x100000
#define DEBUG_WM          0x400000
#define DEBUG_URB         0x800000
#define DEBUG_VS          0x1000000
#define DEBUG_CLIP        0x2000000
#define DEBUG_AUB         0x4000000
#define DEBUG_SHADER_TIME 0x8000000
#define DEBUG_BLORP       0x10000000
#define DEBUG_NO16        0x20000000
#define DEBUG_VUE         0x40000000
#define DEBUG_NO_DUAL_OBJECT_GS 0x80000000

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "INTEL-MESA"
#include <cutils/log.h>
#ifndef ALOGW
#define ALOGW LOGW
#endif
#define dbg_printf(...)	ALOGW(__VA_ARGS__)
#else
#define dbg_printf(...)	fprintf(stderr, __VA_ARGS__)
#endif /* HAVE_ANDROID_PLATFORM */

#define DBG(...) do {						\
	if (unlikely(INTEL_DEBUG & FILE_DEBUG_FLAG))		\
		dbg_printf(__VA_ARGS__);			\
} while(0)

#define perf_debug(...) do {					\
   static GLuint msg_id = 0;                                    \
   if (unlikely(INTEL_DEBUG & DEBUG_PERF))                      \
      dbg_printf(__VA_ARGS__);                                  \
   if (brw->perf_debug)                                         \
      _mesa_gl_debug(&brw->ctx, &msg_id,                        \
                     MESA_DEBUG_TYPE_PERFORMANCE,               \
                     MESA_DEBUG_SEVERITY_MEDIUM,                \
                     __VA_ARGS__);                              \
} while(0)

#define WARN_ONCE(cond, fmt...) do {                            \
   if (unlikely(cond)) {                                        \
      static bool _warned = false;                              \
      static GLuint msg_id = 0;                                 \
      if (!_warned) {                                           \
         fprintf(stderr, "WARNING: ");                          \
         fprintf(stderr, fmt);                                  \
         _warned = true;                                        \
                                                                \
         _mesa_gl_debug(ctx, &msg_id,                           \
                        MESA_DEBUG_TYPE_OTHER,                  \
                        MESA_DEBUG_SEVERITY_HIGH, fmt);         \
      }                                                         \
   }                                                            \
} while (0)

struct brw_context;

extern void brw_process_intel_debug_variable(struct brw_context *brw);
