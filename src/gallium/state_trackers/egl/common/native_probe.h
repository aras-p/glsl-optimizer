/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#ifndef _NATIVE_PROBE_H_
#define _NATIVE_PROBE_H_

#include "EGL/egl.h"  /* for EGL native types */

/**
 * Enumerations for probe results.
 */
enum native_probe_result {
   NATIVE_PROBE_UNKNOWN,
   NATIVE_PROBE_FALLBACK,
   NATIVE_PROBE_SUPPORTED,
   NATIVE_PROBE_EXACT,
};

/**
 * A probe object for display probe.
 */
struct native_probe {
   int magic;
   EGLNativeDisplayType display;
   void *data;

   void (*destroy)(struct native_probe *nprobe);
};

/**
 * Return a probe object for the given display.
 *
 * Note that the returned object may be cached and used by different native
 * display modules.  It allows fast probing when multiple modules probe the
 * same display.
 */
struct native_probe *
native_create_probe(EGLNativeDisplayType dpy);

/**
 * Probe the probe object.
 */
enum native_probe_result
native_get_probe_result(struct native_probe *nprobe);

#endif /* _NATIVE_PROBE_H_ */
