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
#include "r600_resource.h"

/* Texture transfer. */
struct r600_transfer {
	/* Base class. */
	struct pipe_transfer		transfer;
	/* Buffer transfer. */
	struct pipe_transfer		*buffer_transfer;
	unsigned			offset;
	struct pipe_resource		*linear_texture;
};

enum chip_class {
	R600,
	R700,
	EVERGREEN,
};

struct r600_screen {
	struct pipe_screen		screen;
	struct radeon			*rw;
	enum chip_class			chip_class;
	boolean use_mem_constant;
};

static INLINE struct r600_screen *r600_screen(struct pipe_screen *screen)
{
	return (struct r600_screen*)screen;
}

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

/* r600_texture.c texture transfer functions. */
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
int r600_texture_scissor(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level);
int r600_texture_cb(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned cb, unsigned level);
int r600_texture_db(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level);
int r600_texture_from_depth(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level);
int r600_texture_viewport(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level);

/* r600_blit.c */
int r600_blit_uncompress_depth(struct pipe_context *ctx, struct r600_resource_texture *rtexture, unsigned level);

/* helpers */
int r600_conv_pipe_format(unsigned pformat, unsigned *format);
int r600_conv_pipe_prim(unsigned pprim, unsigned *prim);

void r600_init_screen_texture_functions(struct pipe_screen *screen);

#endif
