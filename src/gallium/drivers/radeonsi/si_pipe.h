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
 *
 * Authors:
 *      Jerome Glisse
 */
#ifndef SI_PIPE_H
#define SI_PIPE_H

#include "si_state.h"

#ifdef PIPE_ARCH_BIG_ENDIAN
#define SI_BIG_ENDIAN 1
#else
#define SI_BIG_ENDIAN 0
#endif

#define SI_TRACE_CS 0
#define SI_TRACE_CS_DWORDS		6

#define SI_MAX_DRAW_CS_DWORDS 18

#define LLVM_SUPPORTS_GEOM_SHADERS \
	((HAVE_LLVM >= 0x0305) || \
	(HAVE_LLVM == 0x0304 && LLVM_VERSION_PATCH >= 1))

struct si_pipe_compute;

struct si_screen {
	struct r600_common_screen	b;
};

struct si_pipe_sampler_view {
	struct pipe_sampler_view	base;
	struct r600_resource		*resource;
	uint32_t			state[8];
	uint32_t			fmask_state[8];
};

struct si_pipe_sampler_state {
	uint32_t			val[4];
	uint32_t			border_color[4];
};

struct si_cs_shader_state {
	struct si_pipe_compute		*program;
};

struct si_textures_info {
	struct si_sampler_views		views;
	struct si_pipe_sampler_state	*samplers[NUM_TEX_UNITS];
	unsigned			n_views;
	uint32_t			depth_texture_mask; /* which textures are depth */
	uint32_t			compressed_colortex_mask;
	unsigned			n_samplers;
};

struct si_framebuffer {
	struct r600_atom		atom;
	struct pipe_framebuffer_state	state;
	unsigned			nr_samples;
	unsigned			log_samples;
	unsigned			cb0_is_integer;
	unsigned			compressed_cb_mask;
	unsigned			export_16bpc;
};

#define SI_NUM_ATOMS(sctx) (sizeof((sctx)->atoms)/sizeof((sctx)->atoms.array[0]))

#define SI_NUM_SHADERS (PIPE_SHADER_GEOMETRY+1)

#define SI_RW_SO 2 /* Streamout buffer descriptors after ring buffers */

struct si_context {
	struct r600_common_context	b;
	struct blitter_context		*blitter;
	void				*custom_dsa_flush_depth_stencil[8];
	void				*custom_dsa_flush_depth[8];
	void				*custom_dsa_flush_stencil[8];
	void				*custom_dsa_flush_inplace;
	void				*custom_blend_resolve;
	void				*custom_blend_decompress;
	void				*custom_blend_fastclear;
	struct si_screen		*screen;

	union {
		struct {
			/* The order matters. */
			struct r600_atom *const_buffers[SI_NUM_SHADERS];
			struct r600_atom *rw_buffers[SI_NUM_SHADERS];
			struct r600_atom *sampler_views[SI_NUM_SHADERS];
			/* Caches must be flushed after resource descriptors are
			 * updated in memory. */
			struct r600_atom *cache_flush;
			struct r600_atom *streamout_begin;
			struct r600_atom *streamout_enable; /* must be after streamout_begin */
			struct r600_atom *framebuffer;
		} s;
		struct r600_atom *array[0];
	} atoms;

	struct si_framebuffer		framebuffer;
	struct si_vertex_element	*vertex_elements;
	unsigned			pa_sc_line_stipple;
	unsigned			pa_su_sc_mode_cntl;
	/* for saving when using blitter */
	struct pipe_stencil_ref		stencil_ref;
	struct si_pipe_shader_selector	*ps_shader;
	struct si_pipe_shader_selector	*gs_shader;
	struct si_pipe_shader_selector	*vs_shader;
	struct si_cs_shader_state	cs_shader_state;
	/* shader information */
	unsigned			sprite_coord_enable;
	struct si_buffer_resources	const_buffers[SI_NUM_SHADERS];
	struct si_buffer_resources	rw_buffers[SI_NUM_SHADERS];
	struct si_textures_info	samplers[SI_NUM_SHADERS];
	struct r600_resource		*border_color_table;
	unsigned			border_color_offset;

	unsigned default_ps_gprs, default_vs_gprs;

	/* Below are variables from the old r600_context.
	 */
	unsigned		pm4_dirty_cdwords;

	/* Vertex and index buffers. */
	bool			vertex_buffers_dirty;
	struct pipe_index_buffer index_buffer;
	struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
	unsigned		nr_vertex_buffers;

	/* With rasterizer discard, there doesn't have to be a pixel shader.
	 * In that case, we bind this one: */
	void			*dummy_pixel_shader;
	struct si_pm4_state	*gs_on;
	struct si_pm4_state	*gs_off;
	struct si_pm4_state	*gs_rings;
	struct r600_atom	cache_flush;
	struct pipe_constant_buffer null_const_buf; /* used for set_constant_buffer(NULL) on CIK */
	struct pipe_constant_buffer esgs_ring;
	struct pipe_constant_buffer gsvs_ring;

	/* SI state handling */
	union si_state	queued;
	union si_state	emitted;
};

/* si_blit.c */
void si_init_blit_functions(struct si_context *sctx);
void si_flush_depth_textures(struct si_context *sctx,
			     struct si_textures_info *textures);
void si_decompress_color_textures(struct si_context *sctx,
				  struct si_textures_info *textures);

/* si_dma.c */
void si_dma_copy(struct pipe_context *ctx,
		 struct pipe_resource *dst,
		 unsigned dst_level,
		 unsigned dstx, unsigned dsty, unsigned dstz,
		 struct pipe_resource *src,
		 unsigned src_level,
		 const struct pipe_box *src_box);

/* si_hw_context.c */
void si_context_gfx_flush(void *context, unsigned flags,
			  struct pipe_fence_handle **fence);
void si_begin_new_cs(struct si_context *ctx);
void si_need_cs_space(struct si_context *ctx, unsigned num_dw, boolean count_draw_in);

#if SI_TRACE_CS
void si_trace_emit(struct si_context *sctx);
#endif

/* si_compute.c */
void si_init_compute_functions(struct si_context *sctx);

/* si_uvd.c */
struct pipe_video_codec *si_uvd_create_decoder(struct pipe_context *context,
					       const struct pipe_video_codec *templ);

struct pipe_video_buffer *si_video_buffer_create(struct pipe_context *pipe,
						 const struct pipe_video_buffer *tmpl);

/*
 * common helpers
 */

static INLINE struct r600_resource *
si_resource_create_custom(struct pipe_screen *screen,
			  unsigned usage, unsigned size)
{
	assert(size);
	return r600_resource(pipe_buffer_create(screen,
		PIPE_BIND_CUSTOM, usage, size));
}

#endif
