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
#ifndef R600_PIPE_H
#define R600_PIPE_H

#include "util/u_blitter.h"
#include "util/u_slab.h"
#include "util/u_suballoc.h"
#include "r600.h"
#include "r600_llvm.h"
#include "r600_public.h"
#include "r600_resource.h"
#include "evergreen_compute.h"

#define R600_NUM_ATOMS 37

#define R600_TRACE_CS 0

#define R600_MAX_USER_CONST_BUFFERS 13
#define R600_MAX_DRIVER_CONST_BUFFERS 3
#define R600_MAX_CONST_BUFFERS (R600_MAX_USER_CONST_BUFFERS + R600_MAX_DRIVER_CONST_BUFFERS)

/* start driver buffers after user buffers */
#define R600_UCP_CONST_BUFFER (R600_MAX_USER_CONST_BUFFERS)
#define R600_TXQ_CONST_BUFFER (R600_MAX_USER_CONST_BUFFERS + 1)
#define R600_BUFFER_INFO_CONST_BUFFER (R600_MAX_USER_CONST_BUFFERS + 2)

#define R600_MAX_CONST_BUFFER_SIZE 4096

#ifdef PIPE_ARCH_BIG_ENDIAN
#define R600_BIG_ENDIAN 1
#else
#define R600_BIG_ENDIAN 0
#endif

#define R600_MAP_BUFFER_ALIGNMENT 64

struct r600_bytecode;
struct r600_shader_key;

/* This encapsulates a state or an operation which can emitted into the GPU
 * command stream. It's not limited to states only, it can be used for anything
 * that wants to write commands into the CS (e.g. cache flushes). */
struct r600_atom {
	void (*emit)(struct r600_context *ctx, struct r600_atom *state);
	unsigned		id;
	unsigned		num_dw;
	bool			dirty;
};

/* This is an atom containing GPU commands that never change.
 * This is supposed to be copied directly into the CS. */
struct r600_command_buffer {
	uint32_t *buf;
	unsigned num_dw;
	unsigned max_num_dw;
	unsigned pkt_flags;
};

struct r600_db_state {
	struct r600_atom		atom;
	struct r600_surface		*rsurf;
};

struct r600_db_misc_state {
	struct r600_atom		atom;
	bool				occlusion_query_enabled;
	bool				flush_depthstencil_through_cb;
	bool				flush_depthstencil_in_place;
	bool				copy_depth, copy_stencil;
	unsigned			copy_sample;
	unsigned			log_samples;
	unsigned			db_shader_control;
	bool				htile_clear;
};

struct r600_cb_misc_state {
	struct r600_atom atom;
	unsigned cb_color_control; /* this comes from blend state */
	unsigned blend_colormask; /* 8*4 bits for 8 RGBA colorbuffers */
	unsigned nr_cbufs;
	unsigned nr_ps_color_outputs;
	bool multiwrite;
	bool dual_src_blend;
};

struct r600_clip_misc_state {
	struct r600_atom atom;
	unsigned pa_cl_clip_cntl;   /* from rasterizer    */
	unsigned pa_cl_vs_out_cntl; /* from vertex shader */
	unsigned clip_plane_enable; /* from rasterizer    */
	unsigned clip_dist_write;   /* from vertex shader */
};

struct r600_alphatest_state {
	struct r600_atom atom;
	unsigned sx_alpha_test_control; /* this comes from dsa state */
	unsigned sx_alpha_ref; /* this comes from dsa state */
	bool bypass;
	bool cb0_export_16bpc; /* from set_framebuffer_state */
};

struct r600_vgt_state {
	struct r600_atom atom;
	uint32_t vgt_multi_prim_ib_reset_en;
	uint32_t vgt_multi_prim_ib_reset_indx;
};

struct r600_vgt2_state {
	struct r600_atom atom;
	uint32_t vgt_indx_offset;
};

struct r600_blend_color {
	struct r600_atom atom;
	struct pipe_blend_color state;
};

struct r600_clip_state {
	struct r600_atom atom;
	struct pipe_clip_state state;
};

struct r600_cs_shader_state {
	struct r600_atom atom;
	unsigned kernel_index;
	struct r600_pipe_compute *shader;
};

struct r600_framebuffer {
	struct r600_atom atom;
	struct pipe_framebuffer_state state;
	unsigned compressed_cb_mask;
	unsigned nr_samples;
	bool export_16bpc;
	bool cb0_is_integer;
	bool is_msaa_resolve;
};

struct r600_sample_mask {
	struct r600_atom atom;
	uint16_t sample_mask; /* there are only 8 bits on EG, 16 bits on Cayman */
};

struct r600_config_state {
	struct r600_atom atom;
	unsigned sq_gpr_resource_mgmt_1;
};

struct r600_stencil_ref
{
	ubyte ref_value[2];
	ubyte valuemask[2];
	ubyte writemask[2];
};

struct r600_stencil_ref_state {
	struct r600_atom atom;
	struct r600_stencil_ref state;
	struct pipe_stencil_ref pipe_state;
};

struct r600_viewport_state {
	struct r600_atom atom;
	struct pipe_viewport_state state;
};

struct compute_memory_pool;
void compute_memory_pool_delete(struct compute_memory_pool* pool);
struct compute_memory_pool* compute_memory_pool_new(
	struct r600_screen *rscreen);

struct r600_pipe_fences {
	struct r600_resource		*bo;
	unsigned			*data;
	unsigned			next_index;
	/* linked list of preallocated blocks */
	struct list_head		blocks;
	/* linked list of freed fences */
	struct list_head		pool;
	pipe_mutex			mutex;
};

enum r600_msaa_texture_mode {
	/* If the hw can fetch the first sample only (no decompression available).
	 * This means MSAA texturing is not fully implemented. */
	MSAA_TEXTURE_SAMPLE_ZERO,

	/* If the hw can fetch decompressed MSAA textures.
	 * Supported families: R600, R700, Evergreen.
	 * Cayman cannot use this, because it cannot do the decompression. */
	MSAA_TEXTURE_DECOMPRESSED,

	/* If the hw can fetch compressed MSAA textures, which means shaders can
	 * read resolved FMASK. This yields the best performance.
	 * Supported families: Evergreen, Cayman. */
	MSAA_TEXTURE_COMPRESSED
};

typedef boolean (*r600g_dma_blit_t)(struct pipe_context *ctx,
				struct pipe_resource *dst,
				unsigned dst_level,
				unsigned dst_x, unsigned dst_y, unsigned dst_z,
				struct pipe_resource *src,
				unsigned src_level,
				const struct pipe_box *src_box);

struct r600_screen {
	struct pipe_screen		screen;
	struct radeon_winsys		*ws;
	unsigned			family;
	enum chip_class			chip_class;
	struct radeon_info		info;
	bool				has_streamout;
	bool				has_msaa;
	enum r600_msaa_texture_mode	msaa_texture_support;
	bool				use_hyperz;
	struct r600_tiling_info		tiling_info;
	struct r600_pipe_fences		fences;

	/*for compute global memory binding, we allocate stuff here, instead of
	 * buffers.
	 * XXX: Not sure if this is the best place for global_pool.  Also,
	 * it's not thread safe, so it won't work with multiple contexts. */
	struct compute_memory_pool *global_pool;
#if R600_TRACE_CS
	struct r600_resource		*trace_bo;
	uint32_t			*trace_ptr;
	unsigned			cs_count;
#endif
	r600g_dma_blit_t		dma_blit;
};

struct r600_pipe_sampler_view {
	struct pipe_sampler_view	base;
	struct r600_resource		*tex_resource;
	uint32_t			tex_resource_words[8];
	bool				skip_mip_address_reloc;
};

struct r600_rasterizer_state {
	struct r600_command_buffer	buffer;
	boolean				flatshade;
	boolean				two_side;
	unsigned			sprite_coord_enable;
	unsigned                        clip_plane_enable;
	unsigned			pa_sc_line_stipple;
	unsigned			pa_cl_clip_cntl;
	float				offset_units;
	float				offset_scale;
	bool				offset_enable;
	bool				scissor_enable;
	bool				multisample_enable;
};

struct r600_poly_offset_state {
	struct r600_atom		atom;
	enum pipe_format		zs_format;
	float				offset_units;
	float				offset_scale;
};

struct r600_blend_state {
	struct r600_command_buffer	buffer;
	struct r600_command_buffer	buffer_no_blend;
	unsigned			cb_target_mask;
	unsigned			cb_color_control;
	unsigned			cb_color_control_no_blend;
	bool				dual_src_blend;
	bool				alpha_to_one;
};

struct r600_dsa_state {
	struct r600_command_buffer	buffer;
	unsigned			alpha_ref;
	ubyte				valuemask[2];
	ubyte				writemask[2];
	unsigned                        sx_alpha_test_control;
};

struct r600_pipe_shader;

struct r600_pipe_shader_selector {
	struct r600_pipe_shader *current;

	struct tgsi_token       *tokens;
	struct pipe_stream_output_info  so;

	unsigned	num_shaders;

	/* PIPE_SHADER_[VERTEX|FRAGMENT|...] */
	unsigned	type;

	unsigned	nr_ps_max_color_exports;
};

struct r600_pipe_sampler_state {
	uint32_t			tex_sampler_words[3];
	union pipe_color_union		border_color;
	bool				border_color_use;
	bool				seamless_cube_map;
};

/* needed for blitter save */
#define NUM_TEX_UNITS 16

struct r600_seamless_cube_map {
	struct r600_atom		atom;
	bool				enabled;
};

struct r600_samplerview_state {
	struct r600_atom		atom;
	struct r600_pipe_sampler_view	*views[NUM_TEX_UNITS];
	uint32_t			enabled_mask;
	uint32_t			dirty_mask;
	uint32_t			compressed_depthtex_mask; /* which textures are depth */
	uint32_t			compressed_colortex_mask;
	boolean                         dirty_txq_constants;
	boolean				dirty_buffer_constants;
};

struct r600_sampler_states {
	struct r600_atom		atom;
	struct r600_pipe_sampler_state	*states[NUM_TEX_UNITS];
	uint32_t			enabled_mask;
	uint32_t			dirty_mask;
	uint32_t			has_bordercolor_mask; /* which states contain the border color */
};

struct r600_textures_info {
	struct r600_samplerview_state	views;
	struct r600_sampler_states	states;
	bool				is_array_sampler[NUM_TEX_UNITS];

	/* cube array txq workaround */
	uint32_t			*txq_constants;
	/* buffer related workarounds */
	uint32_t			*buffer_constants;
};

struct r600_fence {
	struct pipe_reference		reference;
	unsigned			index; /* in the shared bo */
	struct r600_resource		*sleep_bo;
	struct list_head		head;
};

#define FENCE_BLOCK_SIZE 16

struct r600_fence_block {
	struct r600_fence		fences[FENCE_BLOCK_SIZE];
	struct list_head		head;
};

#define R600_CONSTANT_ARRAY_SIZE 256
#define R600_RESOURCE_ARRAY_SIZE 160

struct r600_constbuf_state
{
	struct r600_atom		atom;
	struct pipe_constant_buffer	cb[PIPE_MAX_CONSTANT_BUFFERS];
	uint32_t			enabled_mask;
	uint32_t			dirty_mask;
};

struct r600_vertexbuf_state
{
	struct r600_atom		atom;
	struct pipe_vertex_buffer	vb[PIPE_MAX_ATTRIBS];
	uint32_t			enabled_mask; /* non-NULL buffers */
	uint32_t			dirty_mask;
};

/* CSO (constant state object, in other words, immutable state). */
struct r600_cso_state
{
	struct r600_atom atom;
	void *cso; /* e.g. r600_blend_state */
	struct r600_command_buffer *cb;
};

struct r600_scissor_state
{
	struct r600_atom		atom;
	struct pipe_scissor_state	scissor;
	bool				enable; /* r6xx only */
};

struct r600_fetch_shader {
	struct r600_resource		*buffer;
	unsigned			offset;
};

struct r600_ring {
	struct radeon_winsys_cs		*cs;
	bool				flushing;
	void (*flush)(void *ctx, unsigned flags);
};

struct r600_rings {
	struct r600_ring		gfx;
	struct r600_ring		dma;
};

struct r600_context {
	struct pipe_context		context;
	struct r600_screen		*screen;
	struct radeon_winsys		*ws;
	struct r600_rings		rings;
	struct blitter_context		*blitter;
	struct u_upload_mgr		*uploader;
	struct u_suballocator		*allocator_so_filled_size;
	struct u_suballocator		*allocator_fetch_shader;
	struct util_slab_mempool	pool_transfers;

	/* Hardware info. */
	enum radeon_family		family;
	enum chip_class			chip_class;
	boolean				has_vertex_cache;
	boolean				keep_tiling_flags;
	unsigned			default_ps_gprs, default_vs_gprs;
	unsigned			r6xx_num_clause_temp_gprs;
	unsigned			backend_mask;
	unsigned			max_db; /* for OQ */

	/* Miscellaneous state objects. */
	void				*custom_dsa_flush;
	void				*custom_blend_resolve;
	void				*custom_blend_decompress;
	void				*custom_blend_fmask_decompress;
	/* With rasterizer discard, there doesn't have to be a pixel shader.
	 * In that case, we bind this one: */
	void				*dummy_pixel_shader;
	/* These dummy CMASK and FMASK buffers are used to get around the R6xx hardware
	 * bug where valid CMASK and FMASK are required to be present to avoid
	 * a hardlock in certain operations but aren't actually used
	 * for anything useful. */
	struct r600_resource		*dummy_fmask;
	struct r600_resource		*dummy_cmask;

	/* State binding slots are here. */
	struct r600_atom		*atoms[R600_NUM_ATOMS];
	/* States for CS initialization. */
	struct r600_command_buffer	start_cs_cmd; /* invariant state mostly */
	/** Compute specific registers initializations.  The start_cs_cmd atom
	 *  must be emitted before start_compute_cs_cmd. */
	struct r600_command_buffer      start_compute_cs_cmd;
	/* Register states. */
	struct r600_alphatest_state	alphatest_state;
	struct r600_cso_state		blend_state;
	struct r600_blend_color		blend_color;
	struct r600_cb_misc_state	cb_misc_state;
	struct r600_clip_misc_state	clip_misc_state;
	struct r600_clip_state		clip_state;
	struct r600_db_misc_state	db_misc_state;
	struct r600_db_state		db_state;
	struct r600_cso_state		dsa_state;
	struct r600_framebuffer		framebuffer;
	struct r600_poly_offset_state	poly_offset_state;
	struct r600_cso_state		rasterizer_state;
	struct r600_sample_mask		sample_mask;
	struct r600_scissor_state	scissor;
	struct r600_seamless_cube_map	seamless_cube_map;
	struct r600_config_state	config_state;
	struct r600_stencil_ref_state	stencil_ref;
	struct r600_vgt_state		vgt_state;
	struct r600_vgt2_state		vgt2_state;
	struct r600_viewport_state	viewport;
	/* Shaders and shader resources. */
	struct r600_cso_state		vertex_fetch_shader;
	struct r600_cs_shader_state	cs_shader_state;
	struct r600_constbuf_state	constbuf_state[PIPE_SHADER_TYPES];
	struct r600_textures_info	samplers[PIPE_SHADER_TYPES];
	/** Vertex buffers for fetch shaders */
	struct r600_vertexbuf_state	vertex_buffer_state;
	/** Vertex buffers for compute shaders */
	struct r600_vertexbuf_state	cs_vertex_buffer_state;

	/* Additional context states. */
	unsigned			flags;
	unsigned			compute_cb_target_mask;
	struct r600_pipe_shader_selector 	*ps_shader;
	struct r600_pipe_shader_selector 	*vs_shader;
	struct r600_rasterizer_state	*rasterizer;
	bool				alpha_to_one;
	bool				force_blend_disable;
	boolean				dual_src_blend;

	/* Index buffer. */
	struct pipe_index_buffer	index_buffer;

	/* Last draw state (-1 = unset). */
	int				last_primitive_type; /* Last primitive type used in draw_vbo. */
	int				last_start_instance;

	/* Queries. */
	/* The list of active queries. Only one query of each type can be active. */
	int				num_occlusion_queries;
	/* Keep track of non-timer queries, because they should be suspended
	 * during context flushing.
	 * The timer queries (TIME_ELAPSED) shouldn't be suspended. */
	struct list_head		active_nontimer_queries;
	unsigned			num_cs_dw_nontimer_queries_suspend;
	/* If queries have been suspended. */
	bool				nontimer_queries_suspended;

	/* Render condition. */
	struct pipe_query		*current_render_cond;
	unsigned			current_render_cond_mode;
	boolean				predicate_drawing;

	/* Streamout state. */
	unsigned			num_cs_dw_streamout_end;
	unsigned			num_so_targets;
	struct r600_so_target		*so_targets[PIPE_MAX_SO_BUFFERS];
	boolean				streamout_start;
	unsigned			streamout_append_bitmask;
	bool				streamout_suspended;

	/* Deprecated state management. */
	struct r600_range		*range;
	unsigned			nblocks;
	struct r600_block		**blocks;
	struct list_head		dirty;
	struct list_head		enable_list;
	unsigned			pm4_dirty_cdwords;
};

static INLINE void r600_emit_command_buffer(struct radeon_winsys_cs *cs,
					    struct r600_command_buffer *cb)
{
	assert(cs->cdw + cb->num_dw <= RADEON_MAX_CMDBUF_DWORDS);
	memcpy(cs->buf + cs->cdw, cb->buf, 4 * cb->num_dw);
	cs->cdw += cb->num_dw;
}

#if R600_TRACE_CS
void r600_trace_emit(struct r600_context *rctx);
#endif

static INLINE void r600_emit_atom(struct r600_context *rctx, struct r600_atom *atom)
{
	atom->emit(rctx, atom);
	atom->dirty = false;
#if R600_TRACE_CS
	if (rctx->screen->trace_bo) {
		r600_trace_emit(rctx);
	}
#endif
}

static INLINE void r600_set_cso_state(struct r600_cso_state *state, void *cso)
{
	state->cso = cso;
	state->atom.dirty = cso != NULL;
}

static INLINE void r600_set_cso_state_with_cb(struct r600_cso_state *state, void *cso,
					      struct r600_command_buffer *cb)
{
	state->cb = cb;
	state->atom.num_dw = cb->num_dw;
	r600_set_cso_state(state, cso);
}

/* evergreen_state.c */
struct pipe_sampler_view *
evergreen_create_sampler_view_custom(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     const struct pipe_sampler_view *state,
				     unsigned width0, unsigned height0);
void evergreen_init_common_regs(struct r600_command_buffer *cb,
				enum chip_class ctx_chip_class,
				enum radeon_family ctx_family,
				int ctx_drm_minor);
void cayman_init_common_regs(struct r600_command_buffer *cb,
			     enum chip_class ctx_chip_class,
			     enum radeon_family ctx_family,
			     int ctx_drm_minor);

void evergreen_init_state_functions(struct r600_context *rctx);
void evergreen_init_atom_start_cs(struct r600_context *rctx);
void evergreen_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader);
void evergreen_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader);
void *evergreen_create_db_flush_dsa(struct r600_context *rctx);
void *evergreen_create_resolve_blend(struct r600_context *rctx);
void *evergreen_create_decompress_blend(struct r600_context *rctx);
void *evergreen_create_fmask_decompress_blend(struct r600_context *rctx);
boolean evergreen_is_format_supported(struct pipe_screen *screen,
				      enum pipe_format format,
				      enum pipe_texture_target target,
				      unsigned sample_count,
				      unsigned usage);
void evergreen_init_color_surface(struct r600_context *rctx,
				  struct r600_surface *surf);
void evergreen_init_color_surface_rat(struct r600_context *rctx,
					struct r600_surface *surf);
void evergreen_update_db_shader_control(struct r600_context * rctx);

/* r600_blit.c */
void r600_copy_buffer(struct pipe_context *ctx, struct pipe_resource *dst, unsigned dstx,
		      struct pipe_resource *src, const struct pipe_box *src_box);
void r600_init_blit_functions(struct r600_context *rctx);
void r600_blit_decompress_depth(struct pipe_context *ctx,
		struct r600_texture *texture,
		struct r600_texture *staging,
		unsigned first_level, unsigned last_level,
		unsigned first_layer, unsigned last_layer,
		unsigned first_sample, unsigned last_sample);
void r600_decompress_depth_textures(struct r600_context *rctx,
				    struct r600_samplerview_state *textures);
void r600_decompress_color_textures(struct r600_context *rctx,
				    struct r600_samplerview_state *textures);

/* r600_buffer.c */
bool r600_init_resource(struct r600_screen *rscreen,
			struct r600_resource *res,
			unsigned size, unsigned alignment,
			bool use_reusable_pool, unsigned usage);
struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ,
					 unsigned alignment);

/* r600_pipe.c */
boolean r600_rings_is_buffer_referenced(struct r600_context *ctx,
					struct radeon_winsys_cs_handle *buf,
					enum radeon_bo_usage usage);
void *r600_buffer_mmap_sync_with_rings(struct r600_context *ctx,
					struct r600_resource *resource,
					unsigned usage);

/* r600_query.c */
void r600_init_query_functions(struct r600_context *rctx);
void r600_suspend_nontimer_queries(struct r600_context *ctx);
void r600_resume_nontimer_queries(struct r600_context *ctx);

/* r600_resource.c */
void r600_init_context_resource_functions(struct r600_context *r600);

/* r600_shader.c */
int r600_pipe_shader_create(struct pipe_context *ctx,
			    struct r600_pipe_shader *shader,
			    struct r600_shader_key key);
#ifdef HAVE_OPENCL
int r600_compute_shader_create(struct pipe_context * ctx,
	LLVMModuleRef mod,  struct r600_bytecode * bytecode);
#endif
void r600_pipe_shader_destroy(struct pipe_context *ctx, struct r600_pipe_shader *shader);

/* r600_state.c */
struct pipe_sampler_view *
r600_create_sampler_view_custom(struct pipe_context *ctx,
				struct pipe_resource *texture,
				const struct pipe_sampler_view *state,
				unsigned width_first_level, unsigned height_first_level);
void r600_init_state_functions(struct r600_context *rctx);
void r600_init_atom_start_cs(struct r600_context *rctx);
void r600_pipe_shader_ps(struct pipe_context *ctx, struct r600_pipe_shader *shader);
void r600_pipe_shader_vs(struct pipe_context *ctx, struct r600_pipe_shader *shader);
void *r600_create_db_flush_dsa(struct r600_context *rctx);
void *r600_create_resolve_blend(struct r600_context *rctx);
void *r700_create_resolve_blend(struct r600_context *rctx);
void *r600_create_decompress_blend(struct r600_context *rctx);
bool r600_adjust_gprs(struct r600_context *rctx);
boolean r600_is_format_supported(struct pipe_screen *screen,
				 enum pipe_format format,
				 enum pipe_texture_target target,
				 unsigned sample_count,
				 unsigned usage);
void r600_update_db_shader_control(struct r600_context * rctx);

/* r600_texture.c */
void r600_init_screen_texture_functions(struct pipe_screen *screen);
void r600_init_surface_functions(struct r600_context *r600);
uint32_t r600_translate_texformat(struct pipe_screen *screen, enum pipe_format format,
				  const unsigned char *swizzle_view,
				  uint32_t *word4_p, uint32_t *yuv_format_p);
unsigned r600_texture_get_offset(struct r600_texture *rtex,
					unsigned level, unsigned layer);
struct pipe_surface *r600_create_surface_custom(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *templ,
						unsigned width, unsigned height);

unsigned r600_get_swizzle_combined(const unsigned char *swizzle_format,
				   const unsigned char *swizzle_view,
				   boolean vtx);

/* r600_state_common.c */
void r600_init_common_state_functions(struct r600_context *rctx);
void r600_emit_cso_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_alphatest_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_blend_color(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_vgt_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_vgt2_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_clip_misc_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_stencil_ref(struct r600_context *rctx, struct r600_atom *atom);
void r600_emit_viewport_state(struct r600_context *rctx, struct r600_atom *atom);
void r600_init_atom(struct r600_context *rctx, struct r600_atom *atom, unsigned id,
		    void (*emit)(struct r600_context *ctx, struct r600_atom *state),
		    unsigned num_dw);
void r600_vertex_buffers_dirty(struct r600_context *rctx);
void r600_sampler_views_dirty(struct r600_context *rctx,
			      struct r600_samplerview_state *state);
void r600_sampler_states_dirty(struct r600_context *rctx,
			       struct r600_sampler_states *state);
void r600_constant_buffers_dirty(struct r600_context *rctx, struct r600_constbuf_state *state);
void r600_draw_rectangle(struct blitter_context *blitter,
			 int x1, int y1, int x2, int y2, float depth,
			 enum blitter_attrib_type type, const union pipe_color_union *attrib);
uint32_t r600_translate_stencil_op(int s_op);
uint32_t r600_translate_fill(uint32_t func);
unsigned r600_tex_wrap(unsigned wrap);
unsigned r600_tex_filter(unsigned filter);
unsigned r600_tex_mipfilter(unsigned filter);
unsigned r600_tex_compare(unsigned compare);
bool sampler_state_needs_border_color(const struct pipe_sampler_state *state);

/*
 * Helpers for building command buffers
 */

#define PKT3_SET_CONFIG_REG	0x68
#define PKT3_SET_CONTEXT_REG	0x69
#define PKT3_SET_CTL_CONST      0x6F
#define PKT3_SET_LOOP_CONST                    0x6C

#define R600_CONFIG_REG_OFFSET	0x08000
#define R600_CONTEXT_REG_OFFSET 0x28000
#define R600_CTL_CONST_OFFSET   0x3CFF0
#define R600_LOOP_CONST_OFFSET                 0X0003E200
#define EG_LOOP_CONST_OFFSET               0x0003A200

#define PKT_TYPE_S(x)                   (((x) & 0x3) << 30)
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)
#define PKT3_IT_OPCODE_S(x)             (((x) & 0xFF) << 8)
#define PKT3_PREDICATE(x)               (((x) >> 0) & 0x1)
#define PKT3(op, count, predicate) (PKT_TYPE_S(3) | PKT_COUNT_S(count) | PKT3_IT_OPCODE_S(op) | PKT3_PREDICATE(predicate))

#define RADEON_CP_PACKET3_COMPUTE_MODE 0x00000002

/*Evergreen Compute packet3*/
#define PKT3C(op, count, predicate) (PKT_TYPE_S(3) | PKT3_IT_OPCODE_S(op) | PKT_COUNT_S(count) | PKT3_PREDICATE(predicate) | RADEON_CP_PACKET3_COMPUTE_MODE)

static INLINE void r600_store_value(struct r600_command_buffer *cb, unsigned value)
{
	cb->buf[cb->num_dw++] = value;
}

static INLINE void r600_store_config_reg_seq(struct r600_command_buffer *cb, unsigned reg, unsigned num)
{
	assert(reg < R600_CONTEXT_REG_OFFSET);
	assert(cb->num_dw+2+num <= cb->max_num_dw);
	cb->buf[cb->num_dw++] = PKT3(PKT3_SET_CONFIG_REG, num, 0);
	cb->buf[cb->num_dw++] = (reg - R600_CONFIG_REG_OFFSET) >> 2;
}

/**
 * Needs cb->pkt_flags set to  RADEON_CP_PACKET3_COMPUTE_MODE for compute
 * shaders.
 */
static INLINE void r600_store_context_reg_seq(struct r600_command_buffer *cb, unsigned reg, unsigned num)
{
	assert(reg >= R600_CONTEXT_REG_OFFSET && reg < R600_CTL_CONST_OFFSET);
	assert(cb->num_dw+2+num <= cb->max_num_dw);
	cb->buf[cb->num_dw++] = PKT3(PKT3_SET_CONTEXT_REG, num, 0) | cb->pkt_flags;
	cb->buf[cb->num_dw++] = (reg - R600_CONTEXT_REG_OFFSET) >> 2;
}

/**
 * Needs cb->pkt_flags set to  RADEON_CP_PACKET3_COMPUTE_MODE for compute
 * shaders.
 */
static INLINE void r600_store_ctl_const_seq(struct r600_command_buffer *cb, unsigned reg, unsigned num)
{
	assert(reg >= R600_CTL_CONST_OFFSET);
	assert(cb->num_dw+2+num <= cb->max_num_dw);
	cb->buf[cb->num_dw++] = PKT3(PKT3_SET_CTL_CONST, num, 0) | cb->pkt_flags;
	cb->buf[cb->num_dw++] = (reg - R600_CTL_CONST_OFFSET) >> 2;
}

static INLINE void r600_store_loop_const_seq(struct r600_command_buffer *cb, unsigned reg, unsigned num)
{
	assert(reg >= R600_LOOP_CONST_OFFSET);
	assert(cb->num_dw+2+num <= cb->max_num_dw);
	cb->buf[cb->num_dw++] = PKT3(PKT3_SET_LOOP_CONST, num, 0);
	cb->buf[cb->num_dw++] = (reg - R600_LOOP_CONST_OFFSET) >> 2;
}

/**
 * Needs cb->pkt_flags set to  RADEON_CP_PACKET3_COMPUTE_MODE for compute
 * shaders.
 */
static INLINE void eg_store_loop_const_seq(struct r600_command_buffer *cb, unsigned reg, unsigned num)
{
	assert(reg >= EG_LOOP_CONST_OFFSET);
	assert(cb->num_dw+2+num <= cb->max_num_dw);
	cb->buf[cb->num_dw++] = PKT3(PKT3_SET_LOOP_CONST, num, 0) | cb->pkt_flags;
	cb->buf[cb->num_dw++] = (reg - EG_LOOP_CONST_OFFSET) >> 2;
}

static INLINE void r600_store_config_reg(struct r600_command_buffer *cb, unsigned reg, unsigned value)
{
	r600_store_config_reg_seq(cb, reg, 1);
	r600_store_value(cb, value);
}

static INLINE void r600_store_context_reg(struct r600_command_buffer *cb, unsigned reg, unsigned value)
{
	r600_store_context_reg_seq(cb, reg, 1);
	r600_store_value(cb, value);
}

static INLINE void r600_store_ctl_const(struct r600_command_buffer *cb, unsigned reg, unsigned value)
{
	r600_store_ctl_const_seq(cb, reg, 1);
	r600_store_value(cb, value);
}

static INLINE void r600_store_loop_const(struct r600_command_buffer *cb, unsigned reg, unsigned value)
{
	r600_store_loop_const_seq(cb, reg, 1);
	r600_store_value(cb, value);
}

static INLINE void eg_store_loop_const(struct r600_command_buffer *cb, unsigned reg, unsigned value)
{
	eg_store_loop_const_seq(cb, reg, 1);
	r600_store_value(cb, value);
}

void r600_init_command_buffer(struct r600_command_buffer *cb, unsigned num_dw);
void r600_release_command_buffer(struct r600_command_buffer *cb);

/*
 * Helpers for emitting state into a command stream directly.
 */
static INLINE unsigned r600_context_bo_reloc(struct r600_context *ctx,
					     struct r600_ring *ring,
					     struct r600_resource *rbo,
					     enum radeon_bo_usage usage)
{
	assert(usage);
	/* make sure that all previous ring use are flushed so everything
	 * look serialized from driver pov
	 */
	if (!ring->flushing) {
		if (ring == &ctx->rings.gfx) {
			if (ctx->rings.dma.cs) {
				/* flush dma ring */
				ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC);
			}
		} else {
			/* flush gfx ring */
			ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC);
		}
	}
	return ctx->ws->cs_add_reloc(ring->cs, rbo->cs_buf, usage, rbo->domains) * 4;
}

static INLINE void r600_write_value(struct radeon_winsys_cs *cs, unsigned value)
{
	cs->buf[cs->cdw++] = value;
}

static INLINE void r600_write_array(struct radeon_winsys_cs *cs, unsigned num, unsigned *ptr)
{
	assert(cs->cdw+num <= RADEON_MAX_CMDBUF_DWORDS);
	memcpy(&cs->buf[cs->cdw], ptr, num * sizeof(ptr[0]));
	cs->cdw += num;
}

static INLINE void r600_write_config_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg < R600_CONTEXT_REG_OFFSET);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, num, 0);
	cs->buf[cs->cdw++] = (reg - R600_CONFIG_REG_OFFSET) >> 2;
}

static INLINE void r600_write_context_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg >= R600_CONTEXT_REG_OFFSET && reg < R600_CTL_CONST_OFFSET);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, num, 0);
	cs->buf[cs->cdw++] = (reg - R600_CONTEXT_REG_OFFSET) >> 2;
}

static INLINE void r600_write_compute_context_reg_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	r600_write_context_reg_seq(cs, reg, num);
	/* Set the compute bit on the packet header */
	cs->buf[cs->cdw - 2] |= RADEON_CP_PACKET3_COMPUTE_MODE;
}

static INLINE void r600_write_ctl_const_seq(struct radeon_winsys_cs *cs, unsigned reg, unsigned num)
{
	assert(reg >= R600_CTL_CONST_OFFSET);
	assert(cs->cdw+2+num <= RADEON_MAX_CMDBUF_DWORDS);
	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CTL_CONST, num, 0);
	cs->buf[cs->cdw++] = (reg - R600_CTL_CONST_OFFSET) >> 2;
}

static INLINE void r600_write_config_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_config_reg_seq(cs, reg, 1);
	r600_write_value(cs, value);
}

static INLINE void r600_write_context_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_context_reg_seq(cs, reg, 1);
	r600_write_value(cs, value);
}

static INLINE void r600_write_compute_context_reg(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_compute_context_reg_seq(cs, reg, 1);
	r600_write_value(cs, value);
}

static INLINE void r600_write_ctl_const(struct radeon_winsys_cs *cs, unsigned reg, unsigned value)
{
	r600_write_ctl_const_seq(cs, reg, 1);
	r600_write_value(cs, value);
}

/*
 * common helpers
 */
static INLINE uint32_t S_FIXED(float value, uint32_t frac_bits)
{
	return value * (1 << frac_bits);
}
#define ALIGN_DIVUP(x, y) (((x) + (y) - 1) / (y))

static inline unsigned r600_tex_aniso_filter(unsigned filter)
{
	if (filter <= 1)   return 0;
	if (filter <= 2)   return 1;
	if (filter <= 4)   return 2;
	if (filter <= 8)   return 3;
	 /* else */        return 4;
}

/* 12.4 fixed-point */
static INLINE unsigned r600_pack_float_12p4(float x)
{
	return x <= 0    ? 0 :
	       x >= 4096 ? 0xffff : x * 16;
}

static INLINE uint64_t r600_resource_va(struct pipe_screen *screen, struct pipe_resource *resource)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct r600_resource *rresource = (struct r600_resource*)resource;

	return rscreen->ws->buffer_get_virtual_address(rresource->cs_buf);
}

static INLINE unsigned u_max_layer(struct pipe_resource *r, unsigned level)
{
	switch (r->target) {
	case PIPE_TEXTURE_CUBE:
		return 6 - 1;
	case PIPE_TEXTURE_3D:
		return u_minify(r->depth0, level) - 1;
	case PIPE_TEXTURE_1D_ARRAY:
	case PIPE_TEXTURE_2D_ARRAY:
	case PIPE_TEXTURE_CUBE_ARRAY:
		return r->array_size - 1;
	default:
		return 0;
	}
}

#endif
