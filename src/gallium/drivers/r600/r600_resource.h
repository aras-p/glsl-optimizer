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

struct r600_context;
struct r600_screen;

/* This gets further specialized into either buffer or texture
 * structures. Use the vtbl struct to choose between the two
 * underlying implementations.
 */
struct r600_resource {
	struct u_resource		base;
	struct radeon_bo		*bo;
	u32				domain;
	u32				flink;
	struct pb_buffer		*pb;
};

struct r600_resource_texture {
	struct r600_resource		resource;
	unsigned long			offset[PIPE_MAX_TEXTURE_LEVELS];
	unsigned long			pitch[PIPE_MAX_TEXTURE_LEVELS];
	unsigned long			width[PIPE_MAX_TEXTURE_LEVELS];
	unsigned long			height[PIPE_MAX_TEXTURE_LEVELS];
	unsigned long			layer_size[PIPE_MAX_TEXTURE_LEVELS];
	unsigned long			pitch_override;
	unsigned long			bpt;
	unsigned long			size;
	unsigned			tilled;
	unsigned			array_mode;
	unsigned			tile_type;
	unsigned			depth;
	unsigned			dirty;
	struct radeon_bo		*uncompressed;
	struct radeon_state		scissor[PIPE_MAX_TEXTURE_LEVELS];
	struct radeon_state		cb[8][PIPE_MAX_TEXTURE_LEVELS];
	struct radeon_state		db[PIPE_MAX_TEXTURE_LEVELS];
	struct radeon_state		viewport[PIPE_MAX_TEXTURE_LEVELS];
};

void r600_init_context_resource_functions(struct r600_context *r600);
void r600_init_screen_resource_functions(struct r600_screen *r600screen);

/* r600_buffer */
u32 r600_domain_from_usage(unsigned usage);

/* r600_texture */
struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					const struct pipe_resource *templ);
struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
						const struct pipe_resource *base,
						struct winsys_handle *whandle);

#endif
