/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef R600_SCREEN_H
#define R600_SCREEN_H

#include <pipe/p_state.h>
#include <pipe/p_screen.h>
#include <pipebuffer/pb_buffer.h>
#include <xf86drm.h>
#include <radeon_drm.h>
#include "radeon.h"
#include "util/u_transfer.h"

#define r600_screen(s) ((struct r600_screen*)s)

/* Texture transfer. */
struct r600_transfer {
	/* Base class. */
	struct pipe_transfer		transfer;
	/* Buffer transfer. */
	struct pipe_transfer		*buffer_transfer;
	unsigned			offset;
};

struct r600_buffer {
	struct u_resource		b;
	struct radeon_bo		*bo;
	u32				domain;
	u32				flink;
	struct pb_buffer		*pb;
};

struct r600_screen {
	struct pipe_screen		screen;
	struct radeon			*rw;
};

/* Buffer functions. */
struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ);
struct pipe_resource *r600_user_buffer_create(struct pipe_screen *screen,
					      void *ptr, unsigned bytes,
					      unsigned bind);
unsigned r600_buffer_is_referenced_by_cs(struct pipe_context *context,
					 struct pipe_resource *buf,
					 unsigned face, unsigned level);
struct pipe_resource *r600_buffer_from_handle(struct pipe_screen *screen,
					      struct winsys_handle *whandle);

/* Texture transfer functions. */
struct pipe_transfer* r600_texture_get_transfer(struct pipe_context *ctx,
						struct pipe_resource *texture,
						struct pipe_subresource sr,
						unsigned usage,
						const struct pipe_box *box);
void r600_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *trans);
void* r600_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer* transfer);
void r600_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer* transfer);


/* Blit functions. */
void r600_clear(struct pipe_context *ctx,
		unsigned buffers,
		const float *rgba,
		double depth,
		unsigned stencil);
void r600_surface_copy(struct pipe_context *ctx,
			struct pipe_surface *dst,
			unsigned dstx, unsigned dsty,
			struct pipe_surface *src,
			unsigned srcx, unsigned srcy,
			unsigned width, unsigned height);
void r600_surface_fill(struct pipe_context *ctx,
			struct pipe_surface *dst,
			unsigned dstx, unsigned dsty,
			unsigned width, unsigned height,
			unsigned value);

/* helpers */
int r600_conv_pipe_format(unsigned pformat, unsigned *format);
int r600_conv_pipe_prim(unsigned pprim, unsigned *prim);

union r600_float_to_u32_u {
	u32	u;
	float	f;
};

static inline u32 r600_float_to_u32(float f)
{
	union r600_float_to_u32_u c;

	c.f = f;
	return c.u;
}

#endif
