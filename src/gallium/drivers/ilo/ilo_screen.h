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

#ifndef ILO_SCREEN_H
#define ILO_SCREEN_H

#include "pipe/p_screen.h"

#include "ilo_common.h"

struct intel_winsys;
struct intel_bo;

struct ilo_fence;

struct ilo_screen {
   struct pipe_screen base;

   struct intel_winsys *winsys;
   struct ilo_dev_info dev;
};

static inline struct ilo_screen *
ilo_screen(struct pipe_screen *screen)
{
   return (struct ilo_screen *) screen;
}

static inline struct ilo_fence *
ilo_fence(struct pipe_fence_handle *fence)
{
   return (struct ilo_fence *) fence;
}

struct ilo_fence *
ilo_fence_create(struct pipe_screen *screen, struct intel_bo *bo);

#endif /* ILO_SCREEN_H */
