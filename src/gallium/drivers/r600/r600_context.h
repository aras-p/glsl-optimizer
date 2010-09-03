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

#include <stdio.h>
#include <pipe/p_state.h>
#include <pipe/p_context.h>
#include <tgsi/tgsi_scan.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_util.h>
#include <util/u_blitter.h>
#include <util/u_double_list.h>
#include "radeon.h"
#include "r600_shader.h"

#define R600_QUERY_STATE_STARTED	(1 << 0)
#define R600_QUERY_STATE_ENDED		(1 << 1)
#define R600_QUERY_STATE_SUSPENDED	(1 << 2)

struct r600_query {
	u64					result;
	/* The kind of query. Currently only OQ is supported. */
	unsigned				type;
	/* How many results have been written, in dwords. It's incremented
	 * after end_query and flush. */
	unsigned				num_results;
	/* if we've flushed the query */
	boolean					flushed;
	unsigned				state;
	/* The buffer where query results are stored. */
	struct radeon_bo			*buffer;
	unsigned				buffer_size;
	/* linked list of queries */
	struct list_head			list;
	struct radeon_state			rstate;
};

/* XXX move this to a more appropriate place */
union pipe_states {
	struct pipe_rasterizer_state		rasterizer;
	struct pipe_poly_stipple		poly_stipple;
	struct pipe_scissor_state		scissor;
	struct pipe_clip_state			clip;
	struct pipe_shader_state		shader;
	struct pipe_depth_state			depth;
	struct pipe_stencil_state		stencil;
	struct pipe_alpha_state			alpha;
	struct pipe_depth_stencil_alpha_state	dsa;
	struct pipe_blend_state			blend;
	struct pipe_blend_color			blend_color;
	struct pipe_stencil_ref			stencil_ref;
	struct pipe_framebuffer_state		framebuffer;
	struct pipe_sampler_state		sampler;
	struct pipe_sampler_view		sampler_view;
	struct pipe_viewport_state		viewport;
};

enum pipe_state_type {
	pipe_rasterizer_type = 1,
	pipe_poly_stipple_type,
	pipe_scissor_type,
	pipe_clip_type,
	pipe_shader_type,
	pipe_depth_type,
	pipe_stencil_type,
	pipe_alpha_type,
	pipe_dsa_type,
	pipe_blend_type,
	pipe_stencil_ref_type,
	pipe_framebuffer_type,
	pipe_sampler_type,
	pipe_sampler_view_type,
	pipe_viewport_type,
	pipe_type_count
};

#define R600_MAX_RSTATE		16

struct r600_context_state {
	union pipe_states		state;
	unsigned			refcount;
	unsigned			type;
	struct radeon_state		rstate[R600_MAX_RSTATE];
	struct r600_shader		shader;
	struct radeon_bo		*bo;
	unsigned			nrstate;
};

struct r600_vertex_element
{
	unsigned			refcount;
	unsigned			count;
	struct pipe_vertex_element	elements[32];
};

struct r600_context_hw_states {
	struct radeon_state	rasterizer;
	struct radeon_state	scissor;
	struct radeon_state	dsa;
	struct radeon_state	cb_cntl;

	struct radeon_state	db_flush;
	struct radeon_state	cb_flush;
};

#define R600_MAX_CONSTANT 256 /* magic */
#define R600_MAX_RESOURCE 160 /* magic */

struct r600_shader_sampler_states {
	unsigned			nsampler;
	unsigned			nview;
	unsigned			nborder;
	struct radeon_state		*sampler[PIPE_MAX_ATTRIBS];
	struct radeon_state		*view[PIPE_MAX_ATTRIBS];
	struct radeon_state		*border[PIPE_MAX_ATTRIBS];
};

struct r600_context;
struct r600_resource;

struct r600_context_hw_state_vtbl {
	void (*blend)(struct r600_context *rctx,
		      struct radeon_state *rstate,
		      const struct pipe_blend_state *state);
	void (*ucp)(struct r600_context *rctx, struct radeon_state *rstate,
		    const struct pipe_clip_state *state);
	void (*cb)(struct r600_context *rctx, struct radeon_state *rstate,
		   const struct pipe_framebuffer_state *state, int cb);
	void (*db)(struct r600_context *rctx, struct radeon_state *rstate,
		   const struct pipe_framebuffer_state *state);
	void (*rasterizer)(struct r600_context *rctx, struct radeon_state *rstate);
	void (*scissor)(struct r600_context *rctx, struct radeon_state *rstate);
	void (*viewport)(struct r600_context *rctx, struct radeon_state *rstate, const struct pipe_viewport_state *state);
	void (*dsa)(struct r600_context *rctx, struct radeon_state *rstate);
	void (*sampler_border)(struct r600_context *rctx, struct radeon_state *rstate,
			       const struct pipe_sampler_state *state, unsigned id);
	void (*sampler)(struct r600_context *rctx, struct radeon_state *rstate,
			const struct pipe_sampler_state *state, unsigned id);
	void (*resource)(struct pipe_context *ctx, struct radeon_state *rstate,
			 const struct pipe_sampler_view *view, unsigned id);
	void (*cb_cntl)(struct r600_context *rctx, struct radeon_state *rstate);
	int (*vs_resource)(struct r600_context *rctx, int id, struct r600_resource *rbuffer, uint32_t offset,
			   uint32_t stride, uint32_t format);
	int (*vgt_init)(struct r600_context *rctx, struct radeon_state *draw,
			struct r600_resource *rbuffer,
			uint32_t count, int vgt_draw_initiator);
	int (*vgt_prim)(struct r600_context *rctx, struct radeon_state *vgt,
			uint32_t prim, uint32_t start, uint32_t vgt_dma_index_type);

	int (*ps_shader)(struct r600_context *rctx, struct r600_context_state *rshader,
			 struct radeon_state *state);
	int (*vs_shader)(struct r600_context *rctx, struct r600_context_state *rpshader,
			 struct radeon_state *state);
	void (*init_config)(struct r600_context *rctx);
};
extern struct r600_context_hw_state_vtbl r600_hw_state_vtbl;
extern struct r600_context_hw_state_vtbl eg_hw_state_vtbl;

struct r600_context {
	struct pipe_context		context;
	struct r600_screen		*screen;
	struct radeon			*rw;
	struct radeon_ctx		ctx;
	struct blitter_context		*blitter;
	struct radeon_draw		draw;
	struct r600_context_hw_state_vtbl *vtbl;
	struct radeon_state		config;
	boolean use_mem_constant;
	/* FIXME get rid of those vs_resource,vs/ps_constant */
	struct radeon_state		*vs_resource;
	unsigned			vs_nresource;
	struct radeon_state		*vs_constant;
	struct radeon_state		*ps_constant;
	/* hw states */
	struct r600_context_hw_states	hw_states;
	/* pipe states */
	unsigned			flat_shade;

	unsigned			nvertex_buffer;
	struct r600_context_state	*rasterizer;
	struct r600_context_state	*poly_stipple;
	struct r600_context_state	*scissor;
	struct r600_context_state	*clip;
	struct r600_context_state	*ps_shader;
	struct r600_context_state	*vs_shader;
	struct r600_context_state	*depth;
	struct r600_context_state	*stencil;
	struct r600_context_state	*alpha;
	struct r600_context_state	*dsa;
	struct r600_context_state	*blend;
	struct r600_context_state	*stencil_ref;
	struct r600_context_state	*viewport;
	struct r600_context_state	*framebuffer;
	struct r600_shader_sampler_states vs_sampler;
	struct r600_shader_sampler_states ps_sampler;
	/* can add gs later */
	struct r600_vertex_element	*vertex_elements;
	struct pipe_vertex_buffer	vertex_buffer[PIPE_MAX_ATTRIBS];
	struct pipe_index_buffer	index_buffer;
	struct pipe_blend_color		blend_color;
	struct list_head		query_list;
};

/* Convenience cast wrapper. */
static INLINE struct r600_context *r600_context(struct pipe_context *pipe)
{
    return (struct r600_context*)pipe;
}

static INLINE struct r600_query* r600_query(struct pipe_query* q)
{
    return (struct r600_query*)q;
}

struct r600_context_state *r600_context_state_incref(struct r600_context_state *rstate);
struct r600_context_state *r600_context_state_decref(struct r600_context_state *rstate);
void r600_flush(struct pipe_context *ctx, unsigned flags,
			struct pipe_fence_handle **fence);

int r600_context_hw_states(struct pipe_context *ctx);

void r600_draw_vbo(struct pipe_context *ctx,
                   const struct pipe_draw_info *info);

void r600_init_blit_functions(struct r600_context *rctx);
void r600_init_state_functions(struct r600_context *rctx);
void r600_init_query_functions(struct r600_context* rctx);
struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv);

extern int r600_pipe_shader_create(struct pipe_context *ctx,
			struct r600_context_state *rstate,
			const struct tgsi_token *tokens);
extern int r600_pipe_shader_update(struct pipe_context *ctx,
				struct r600_context_state *rstate);

#define R600_ERR(fmt, args...) \
	fprintf(stderr, "EE %s/%s:%d - "fmt, __FILE__, __func__, __LINE__, ##args)

uint32_t r600_translate_texformat(enum pipe_format format,
				  const unsigned char *swizzle_view, 
				  uint32_t *word4_p, uint32_t *yuv_format_p);

/* query */
extern void r600_queries_resume(struct pipe_context *ctx);
extern void r600_queries_suspend(struct pipe_context *ctx);

int eg_bc_cf_build(struct r600_bc *bc, struct r600_bc_cf *cf);

void r600_set_constant_buffer_file(struct pipe_context *ctx,
				   uint shader, uint index,
				   struct pipe_resource *buffer);
void r600_set_constant_buffer_mem(struct pipe_context *ctx,
				  uint shader, uint index,
				  struct pipe_resource *buffer);
void eg_set_constant_buffer(struct pipe_context *ctx,
			    uint shader, uint index,
			    struct pipe_resource *buffer);

#endif
