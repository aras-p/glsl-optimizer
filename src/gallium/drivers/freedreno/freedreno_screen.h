/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FREEDRENO_SCREEN_H_
#define FREEDRENO_SCREEN_H_

#include <freedreno_drmif.h>
#include <freedreno_ringbuffer.h>

#include "pipe/p_screen.h"
#include "util/u_memory.h"

typedef uint32_t u32;

struct fd_bo;

struct fd_screen {
	struct pipe_screen base;

	uint32_t gmemsize_bytes;
	uint32_t device_id;
	uint32_t gpu_id;         /* 220, 305, etc */
	uint32_t chip_id;        /* coreid:8 majorrev:8 minorrev:8 patch:8 */

	struct fd_device *dev;
	struct fd_pipe *pipe;

	int64_t cpu_gpu_time_delta;
};

static INLINE struct fd_screen *
fd_screen(struct pipe_screen *pscreen)
{
	return (struct fd_screen *)pscreen;
}

boolean fd_screen_bo_get_handle(struct pipe_screen *pscreen,
		struct fd_bo *bo,
		unsigned stride,
		struct winsys_handle *whandle);
struct fd_bo * fd_screen_bo_from_handle(struct pipe_screen *pscreen,
		struct winsys_handle *whandle,
		unsigned *out_stride);

struct pipe_screen * fd_screen_create(struct fd_device *dev);

/* is a3xx patch revision 0? */
static inline boolean
is_a3xx_p0(struct fd_screen *screen)
{
	return (screen->gpu_id & 0xff0000ff) == 0x03000000;
}

#endif /* FREEDRENO_SCREEN_H_ */
