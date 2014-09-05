/*
 * Copyright © 2014 Broadcom
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

#ifndef VC4_SCREEN_H
#define VC4_SCREEN_H

#include "pipe/p_screen.h"
#include "state_tracker/drm_driver.h"

struct vc4_bo;

#define VC4_DEBUG_CL        0x0001
#define VC4_DEBUG_QPU       0x0002
#define VC4_DEBUG_QIR       0x0004
#define VC4_DEBUG_TGSI      0x0008
#define VC4_DEBUG_SHADERDB  0x0010
#define VC4_DEBUG_PERF      0x0020
#define VC4_DEBUG_NORAST    0x0040
#define VC4_DEBUG_ALWAYS_FLUSH 0x0080

#define VC4_MAX_MIP_LEVELS 12
#define VC4_MAX_TEXTURE_SAMPLERS 16

struct vc4_screen {
        struct pipe_screen base;
        int fd;

        void *simulator_mem_base;
        uint32_t simulator_mem_size;
};

static inline struct vc4_screen *
vc4_screen(struct pipe_screen *screen)
{
        return (struct vc4_screen *)screen;
}

struct pipe_screen *vc4_screen_create(int fd);
boolean vc4_screen_bo_get_handle(struct pipe_screen *pscreen,
                                 struct vc4_bo *bo,
                                 unsigned stride,
                                 struct winsys_handle *whandle);
struct vc4_bo *
vc4_screen_bo_from_handle(struct pipe_screen *pscreen,
                          struct winsys_handle *whandle,
                          unsigned *out_stride);

extern uint32_t vc4_debug;

#endif /* VC4_SCREEN_H */
