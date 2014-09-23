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

#define DEBUG_TEXTURE             (1 <<  0)
#define DEBUG_STATE               (1 <<  1)
#define DEBUG_BLIT                (1 <<  2)
#define DEBUG_MIPTREE             (1 <<  3)
#define DEBUG_PERF                (1 <<  4)
#define DEBUG_PERFMON             (1 <<  5)
#define DEBUG_BATCH               (1 <<  6)
#define DEBUG_PIXEL               (1 <<  7)
#define DEBUG_BUFMGR              (1 <<  8)
#define DEBUG_FBO                 (1 <<  9)
#define DEBUG_GS                  (1 << 10)
#define DEBUG_SYNC                (1 << 11)
#define DEBUG_PRIMS               (1 << 12)
#define DEBUG_VERTS               (1 << 13)
#define DEBUG_DRI                 (1 << 14)
#define DEBUG_SF                  (1 << 15)
#define DEBUG_STATS               (1 << 16)
#define DEBUG_WM                  (1 << 17)
#define DEBUG_URB                 (1 << 18)
#define DEBUG_VS                  (1 << 19)
#define DEBUG_CLIP                (1 << 20)
#define DEBUG_AUB                 (1 << 21)
#define DEBUG_SHADER_TIME         (1 << 22)
#define DEBUG_BLORP               (1 << 23)
#define DEBUG_NO16                (1 << 24)
#define DEBUG_VUE                 (1 << 25)
#define DEBUG_NO_DUAL_OBJECT_GS   (1 << 26)
#define DEBUG_OPTIMIZER           (1 << 27)
#define DEBUG_NO_ANNOTATION       (1 << 28)
#define DEBUG_NO8                 (1 << 29)

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
