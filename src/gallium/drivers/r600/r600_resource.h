/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com
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
#ifndef R600_RESOURCE_H
#define R600_RESOURCE_H

#include "util/u_transfer.h"

/* Texture transfer. */
struct r600_transfer {
	/* Base class. */
	struct pipe_transfer		transfer;
	/* Buffer transfer. */
	struct pipe_transfer		*buffer_transfer;
	unsigned			offset;
	struct pipe_resource		*linear_texture;
};

/* This gets further specialized into either buffer or texture
 * structures. Use the vtbl struct to choose between the two
 * underlying implementations.
 */
struct r600_resource {
	struct u_resource		base;
	struct r600_bo			*bo;
	u32				domain;
	u32				flink;
	u32				size;
};

struct r600_resource_texture {
	struct r600_resource		resource;
	unsigned			offset[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			pitch_in_bytes[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			pitch_in_pixels[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			layer_size[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			array_mode[PIPE_MAX_TEXTURE_LEVELS];
	unsigned			pitch_override;
	unsigned			size;
	unsigned			tiled;
	unsigned			tile_type;
	unsigned			depth;
	unsigned			dirty;
	struct r600_resource_texture    *flushed_depth_texture;
};

void r600_init_screen_resource_functions(struct pipe_screen *screen);

/* r600_buffer */
u32 r600_domain_from_usage(unsigned usage);

/* r600_texture */
struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					const struct pipe_resource *templ);
struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
						const struct pipe_resource *base,
						struct winsys_handle *whandle);

#define R600_BUFFER_MAGIC 0xabcd1600
#define R600_BUFFER_MAX_RANGES 32

struct r600_buffer_range {
	uint32_t start;
	uint32_t end;
};

struct r600_resource_buffer {
	struct r600_resource r;
	uint32_t magic;
	void *user_buffer;
	struct r600_buffer_range ranges[R600_BUFFER_MAX_RANGES];
	unsigned num_ranges;
};

/* r600_buffer */
static INLINE struct r600_resource_buffer *r600_buffer(struct pipe_resource *buffer)
{
	if (buffer) {
		assert(((struct r600_resource_buffer *)buffer)->magic == R600_BUFFER_MAGIC);
		return (struct r600_resource_buffer *)buffer;
    }
    return NULL;
}

static INLINE boolean r600_buffer_is_user_buffer(struct pipe_resource *buffer)
{
    return r600_buffer(buffer)->user_buffer ? TRUE : FALSE;
}

int r600_texture_depth_flush(struct pipe_context *ctx,
			     struct pipe_resource *texture);

extern int (*r600_blit_uncompress_depth_ptr)(struct pipe_context *ctx, struct r600_resource_texture *texture);

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

#endif
