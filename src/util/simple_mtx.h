/*
 * Copyright © 2015 Intel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef _SIMPLE_MTX_H
#define _SIMPLE_MTX_H

/*
 * glsl-optimizer change: minimal simple_mtx. Mesa takes these only around
 * shared caches (GLSL types, builtin functions) and debug output, so they all
 * map to one process-wide recursive mutex (c11/threads.h) instead of futexes
 * or C11 mutexes. Recursive, since a cache lock can be taken while another is
 * held.
 */

#include <stdbool.h>
#include <stdint.h>
#include "c11/threads.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
   int unused;
} simple_mtx_t;

#define SIMPLE_MTX_INITIALIZER { 0 }

static inline void simple_mtx_init(simple_mtx_t *mtx, int type) { (void)mtx; (void)type; }
static inline void simple_mtx_destroy(simple_mtx_t *mtx) { (void)mtx; }
static inline void simple_mtx_lock(simple_mtx_t *mtx) { (void)mtx; glslopt_global_lock(); }
static inline void simple_mtx_unlock(simple_mtx_t *mtx) { (void)mtx; glslopt_global_unlock(); }
static inline void simple_mtx_assert_locked(simple_mtx_t *mtx) { (void)mtx; }

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_MTX_H */
