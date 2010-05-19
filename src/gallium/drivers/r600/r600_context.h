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
#ifndef R600_CONTEXT_H
#define R600_CONTEXT_H

#include <pipe/p_state.h>
#include <pipe/p_context.h>
#include <tgsi/tgsi_scan.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_util.h>
#include <util/u_blitter.h>
#include "radeon.h"
#include "r600_shader.h"

struct r600_state;
typedef void (*r600_state_destroy_t)(struct r600_state *rstate);

/* XXX move this to a more appropriate place */
struct r600_vertex_elements_state
{
	unsigned count;
	struct pipe_vertex_element elements[32];
};

struct r600_state {
	unsigned		type;
	struct r600_atom	*atom;
	void			*state;
	unsigned		nbuffers;
	struct pipe_buffer	*buffer[256];
	unsigned		nsurfaces;
	struct pipe_surface	*surface[256];
	r600_state_destroy_t	destroy;
};

struct r600_pipe_shader {
	unsigned				type;
	struct r600_shader			shader;
	struct radeon_bo			*bo;
	struct radeon_state			*state;
};

struct r600_context {
	struct pipe_context		context;
	struct radeon_ctx		*ctx;
	struct radeon_state		*cb_cntl;
	struct radeon_state		*db;
	struct radeon_state		*config;
	struct r600_pipe_shader		*ps_shader;
	struct r600_pipe_shader		*vs_shader;
	unsigned			flat_shade;
	unsigned			nvertex_buffer;
	struct r600_vertex_elements_state *vertex_elements;
	struct pipe_vertex_buffer	vertex_buffer[PIPE_MAX_ATTRIBS];
	struct blitter_context		*blitter;
	struct pipe_stencil_ref		stencil_ref;
	struct pipe_framebuffer_state	fb_state;
	struct radeon_draw		*draw;
	struct pipe_viewport_state *viewport;
};

void r600_draw_arrays(struct pipe_context *ctx, unsigned mode,
			unsigned start, unsigned count);
void r600_draw_elements(struct pipe_context *ctx,
		struct pipe_buffer *index_buffer,
		unsigned index_size, unsigned index_bias, unsigned mode,
		unsigned start, unsigned count);
void r600_draw_range_elements(struct pipe_context *ctx,
		struct pipe_buffer *index_buffer,
		unsigned index_size, unsigned index_bias, unsigned min_index,
		unsigned max_index, unsigned mode,
		unsigned start, unsigned count);

void r600_state_destroy_common(struct r600_state *state);
struct r600_state *r600_state_new(r600_state_destroy_t destroy);
struct r600_state *r600_state_destroy(struct r600_state *state);

void r600_init_state_functions(struct r600_context *rctx);
void r600_init_query_functions(struct r600_context* rctx);
struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv);

void r600_pipe_shader_destroy(struct pipe_context *ctx, struct r600_pipe_shader *rpshader);
struct r600_pipe_shader *r600_pipe_shader_create(struct pipe_context *ctx,
						unsigned type,
						const struct tgsi_token *tokens);
int r600_pipe_shader_update(struct pipe_context *ctx, struct r600_pipe_shader *rpshader);

#endif
