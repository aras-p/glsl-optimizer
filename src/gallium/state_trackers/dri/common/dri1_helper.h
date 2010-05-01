/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#ifndef DRI1_HELPER_H
#define DRI1_HELPER_H

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"

struct pipe_fence_handle *
dri1_swap_fences_pop_front(struct dri_drawable *draw);

void
dri1_swap_fences_push_back(struct dri_drawable *draw,
                           struct pipe_fence_handle *fence);

void
dri1_swap_fences_clear(struct dri_drawable *drawable);

struct pipe_surface *
dri1_get_pipe_surface(struct dri_drawable *drawable, struct pipe_resource *ptex);

void
dri1_destroy_pipe_surface(struct dri_drawable *drawable);

struct pipe_context *
dri1_get_pipe_context(struct dri_screen *screen);

void
dri1_destroy_pipe_context(struct dri_screen *screen);

#endif /* DRI1_HELPER_H */
