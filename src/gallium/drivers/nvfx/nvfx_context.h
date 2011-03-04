#ifndef __NVFX_CONTEXT_H__
#define __NVFX_CONTEXT_H__

#include <stdio.h>

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_compiler.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_double_list.h"

#include "draw/draw_vertex.h"
#include "util/u_blitter.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"
#include "nouveau/nouveau_resource.h"
#include "nv30-40_3d.xml.h"
#include "nvfx_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#include "nvfx_screen.h"

#define NVFX_NEW_BLEND		(1 <<  0)
#define NVFX_NEW_RAST		(1 <<  1)
#define NVFX_NEW_ZSA		(1 <<  2)
#define NVFX_NEW_SAMPLER	(1 <<  3)
#define NVFX_NEW_FB		(1 <<  4)
#define NVFX_NEW_STIPPLE	(1 <<  5)
#define NVFX_NEW_SCISSOR	(1 <<  6)
#define NVFX_NEW_VIEWPORT	(1 <<  7)
#define NVFX_NEW_BCOL		(1 <<  8)
#define NVFX_NEW_VERTPROG	(1 <<  9)
#define NVFX_NEW_FRAGPROG	(1 << 10)
#define NVFX_NEW_ARRAYS		(1 << 11)
#define NVFX_NEW_UCP		(1 << 12)
#define NVFX_NEW_SR		(1 << 13)
#define NVFX_NEW_VERTCONST	(1 << 14)
#define NVFX_NEW_FRAGCONST	(1 << 15)
#define NVFX_NEW_INDEX	(1 << 16)
#define NVFX_NEW_SPRITE  (1 << 17)

#define NVFX_RELOCATE_FRAMEBUFFER (1 << 0)
#define NVFX_RELOCATE_FRAGTEX (1 << 1)
#define NVFX_RELOCATE_FRAGPROG (1 << 2)
#define NVFX_RELOCATE_VTXBUF (1 << 3)
#define NVFX_RELOCATE_IDXBUF (1 << 4)
#define NVFX_RELOCATE_ALL 0x1f

struct nvfx_rasterizer_state {
	struct pipe_rasterizer_state pipe;
	unsigned sb_len;
	uint32_t sb[34];
};

struct nvfx_zsa_state {
	struct pipe_depth_stencil_alpha_state pipe;
	unsigned sb_len;
	uint32_t sb[24];
};

struct nvfx_blend_state {
	struct pipe_blend_state pipe;
	unsigned sb_len;
	uint32_t sb[13];
};


struct nvfx_state {
	unsigned scissor_enabled;
	unsigned fp_samplers;
	unsigned render_temps;
};

struct nvfx_per_vertex_element {
	unsigned idx;
        unsigned vertex_buffer_index;
        unsigned src_offset;
};

struct nvfx_low_frequency_element {
	unsigned idx;
	unsigned vertex_buffer_index;
	unsigned src_offset;
        void (*fetch_rgba_float)(float *dst, const uint8_t *src, unsigned i, unsigned j);
        unsigned ncomp;
};

struct nvfx_per_instance_element {
	struct nvfx_low_frequency_element base;
	unsigned instance_divisor;
};

struct nvfx_per_vertex_buffer_info
{
	unsigned vertex_buffer_index;
	unsigned per_vertex_size;
};

struct nvfx_vtxelt_state {
	struct pipe_vertex_element pipe[16];
	unsigned num_elements;
	unsigned vtxfmt[16];

	unsigned num_per_vertex_buffer_infos;
	struct nvfx_per_vertex_buffer_info per_vertex_buffer_info[16];

	unsigned num_per_vertex;
	struct nvfx_per_vertex_element per_vertex[16];

	unsigned num_per_instance;
	struct nvfx_per_instance_element per_instance[16];

	unsigned num_constant;
	struct nvfx_low_frequency_element constant[16];

	boolean needs_translate;
	struct translate* translate;

	unsigned vertex_length;
	unsigned max_vertices_per_packet;
};

struct nvfx_render_target {
	struct nouveau_bo* bo;
	unsigned offset;
	unsigned pitch;
};

struct nvfx_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nvfx_screen *screen;

	unsigned is_nv4x; /* either 0 or ~0 */
	unsigned use_nv4x; /* either 0 or ~0 */
	boolean use_vp_clipping;

	struct draw_context *draw;
	/* one is for user-requested operations, the other is for temporary copying inside them */
	struct blitter_context* blitter[2];
	unsigned blitters_in_use;
	struct list_head render_cache;

	/* HW state derived from pipe states */
	struct nvfx_state state;

	enum {
		HW, SWTNL, SWRAST
	} render_mode;
	unsigned fallback_swtnl;

	/* Context state */
	unsigned dirty, draw_dirty;
	struct pipe_scissor_state scissor;
	unsigned stipple[32];
	struct pipe_clip_state clip;
	struct nvfx_pipe_vertex_program *vertprog;
	struct nvfx_pipe_fragment_program *fragprog;
	struct pipe_resource *constbuf[PIPE_SHADER_TYPES];
	unsigned constbuf_nr[PIPE_SHADER_TYPES];
	struct nvfx_rasterizer_state *rasterizer;
	struct nvfx_zsa_state *zsa;
	struct nvfx_blend_state *blend;
	struct pipe_blend_color blend_colour;
	struct pipe_stencil_ref stencil_ref;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_index_buffer idxbuf;
	struct nvfx_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct pipe_sampler_view *fragment_sampler_views[PIPE_MAX_SAMPLERS];
	struct nvfx_pipe_fragment_program* dummy_fs;
	struct pipe_query* query;

	unsigned nr_samplers;
	unsigned nr_textures;
	unsigned dirty_samplers;
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct nvfx_vtxelt_state *vtxelt;
	int base_vertex;
	boolean use_index_buffer;
	/* -1 = hardware input setup is outdated
	 * 0 = hardware input setup is for inline vertices
	 * 1 = hardware input setup is for hardware vertices
	 */
	int use_vertex_buffers;

	unsigned hw_vtxelt_nr;
	unsigned hw_samplers;
	uint32_t hw_txf[16];
	struct nvfx_render_target hw_rt[4];
	struct nvfx_render_target hw_zeta;
	int hw_pointsprite_control;
	int hw_vp_output;
	struct nvfx_fragment_program* hw_fragprog;
	struct nvfx_vertex_program* hw_vertprog;

	unsigned relocs_needed;
};

static INLINE struct nvfx_context *
nvfx_context(struct pipe_context *pipe)
{
	return (struct nvfx_context *)pipe;
}

extern struct nvfx_state_entry nvfx_state_blend;
extern struct nvfx_state_entry nvfx_state_blend_colour;
extern struct nvfx_state_entry nvfx_state_fragprog;
extern struct nvfx_state_entry nvfx_state_fragtex;
extern struct nvfx_state_entry nvfx_state_framebuffer;
extern struct nvfx_state_entry nvfx_state_rasterizer;
extern struct nvfx_state_entry nvfx_state_scissor;
extern struct nvfx_state_entry nvfx_state_sr;
extern struct nvfx_state_entry nvfx_state_stipple;
extern struct nvfx_state_entry nvfx_state_vbo;
extern struct nvfx_state_entry nvfx_state_vertprog;
extern struct nvfx_state_entry nvfx_state_viewport;
extern struct nvfx_state_entry nvfx_state_vtxfmt;
extern struct nvfx_state_entry nvfx_state_zsa;

extern void nvfx_init_query_functions(struct nvfx_context *nvfx);
extern void nvfx_init_surface_functions(struct nvfx_context *nvfx);

/* nvfx_context.c */
struct pipe_context *
nvfx_create(struct pipe_screen *pscreen, void *priv);

/* nvfx_clear.c */
extern void nvfx_clear(struct pipe_context *pipe, unsigned buffers,
		       const float *rgba, double depth, unsigned stencil);

/* nvfx_draw.c */
extern struct draw_stage *nvfx_draw_render_stage(struct nvfx_context *nvfx);
extern void nvfx_draw_vbo_swtnl(struct pipe_context *pipe, const struct pipe_draw_info* info);
extern void nvfx_vtxfmt_validate(struct nvfx_context *nvfx);

/* nvfx_fb.c */
extern int nvfx_framebuffer_prepare(struct nvfx_context *nvfx);
extern void nvfx_framebuffer_validate(struct nvfx_context *nvfx, unsigned prepare_result);
void
nvfx_framebuffer_relocate(struct nvfx_context *nvfx);

/* nvfx_fragprog.c */
extern void nvfx_fragprog_destroy(struct nvfx_context *,
				    struct nvfx_fragment_program *);
extern void nvfx_fragprog_validate(struct nvfx_context *nvfx);
extern void nvfx_fragprog_relocate(struct nvfx_context *nvfx);
extern void nvfx_init_fragprog_functions(struct nvfx_context *nvfx);

/* nvfx_fragtex.c */
extern void nvfx_init_sampling_functions(struct nvfx_context *nvfx);
extern void nvfx_fragtex_validate(struct nvfx_context *nvfx);
extern void nvfx_fragtex_relocate(struct nvfx_context *nvfx);

struct nvfx_sampler_view;

/* nv30_fragtex.c */
extern void
nv30_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso);
extern void
nv30_sampler_view_init(struct pipe_context *pipe,
			  struct nvfx_sampler_view *sv);
extern void nv30_fragtex_set(struct nvfx_context *nvfx, int unit);

/* nv40_fragtex.c */
extern void
nv40_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso);
extern void
nv40_sampler_view_init(struct pipe_context *pipe,
			  struct nvfx_sampler_view *sv);
extern void nv40_fragtex_set(struct nvfx_context *nvfx, int unit);

/* nvfx_state.c */
extern void nvfx_init_state_functions(struct nvfx_context *nvfx);
extern void nvfx_state_scissor_validate(struct nvfx_context *nvfx);
extern void nvfx_state_stipple_validate(struct nvfx_context *nvfx);
extern void nvfx_state_blend_validate(struct nvfx_context *nvfx);
extern void nvfx_state_blend_colour_validate(struct nvfx_context *nvfx);
extern void nvfx_state_viewport_validate(struct nvfx_context *nvfx);
extern void nvfx_state_rasterizer_validate(struct nvfx_context *nvfx);
extern void nvfx_state_sr_validate(struct nvfx_context *nvfx);
extern void nvfx_state_zsa_validate(struct nvfx_context *nvfx);

/* nvfx_state_emit.c */
extern void nvfx_state_relocate(struct nvfx_context *nvfx, unsigned relocs);
extern boolean nvfx_state_validate(struct nvfx_context *nvfx);
extern boolean nvfx_state_validate_swtnl(struct nvfx_context *nvfx);

static inline void
nvfx_state_emit(struct nvfx_context *nvfx)
{
        unsigned relocs = NVFX_RELOCATE_FRAMEBUFFER | NVFX_RELOCATE_FRAGTEX | NVFX_RELOCATE_FRAGPROG;
        if (nvfx->render_mode == HW)
        {
                relocs |= NVFX_RELOCATE_VTXBUF;
                if(nvfx->use_index_buffer)
                        relocs |= NVFX_RELOCATE_IDXBUF;
        }

        relocs &= nvfx->relocs_needed;
        if(relocs)
                nvfx_state_relocate(nvfx, relocs);
}

/* nvfx_transfer.c */
extern void nvfx_init_transfer_functions(struct pipe_context *pipe);

/* nvfx_vbo.c */
extern boolean nvfx_vbo_validate(struct nvfx_context *nvfx);
extern void nvfx_vbo_swtnl_validate(struct nvfx_context *nvfx);
extern void nvfx_vbo_relocate(struct nvfx_context *nvfx);
extern void nvfx_idxbuf_validate(struct nvfx_context* nvfx);
extern void nvfx_idxbuf_relocate(struct nvfx_context* nvfx);
extern void nvfx_draw_vbo(struct pipe_context *pipe,
                          const struct pipe_draw_info *info);
extern void nvfx_init_vbo_functions(struct nvfx_context *nvfx);
extern unsigned nvfx_vertex_formats[];

/* nvfx_vertprog.c */
extern boolean nvfx_vertprog_validate(struct nvfx_context *nvfx);
extern void nvfx_vertprog_destroy(struct nvfx_context *,
				  struct nvfx_vertex_program *);
extern void nvfx_init_vertprog_functions(struct nvfx_context *nvfx);

/* nvfx_push.c */
extern void nvfx_push_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info);

static inline void nvfx_emit_vtx_attr(struct nouveau_channel* chan,
		struct nouveau_grobj *eng3d, unsigned attrib, const float* v,
		unsigned ncomp)
{
	switch (ncomp) {
	case 4:
		BEGIN_RING(chan, eng3d, NV30_3D_VTX_ATTR_4F_X(attrib), 4);
		OUT_RING(chan, fui(v[0]));
		OUT_RING(chan, fui(v[1]));
		OUT_RING(chan,  fui(v[2]));
		OUT_RING(chan,  fui(v[3]));
		break;
	case 3:
		BEGIN_RING(chan, eng3d, NV30_3D_VTX_ATTR_3F_X(attrib), 3);
		OUT_RING(chan,  fui(v[0]));
		OUT_RING(chan,  fui(v[1]));
		OUT_RING(chan,  fui(v[2]));
		break;
	case 2:
		BEGIN_RING(chan, eng3d, NV30_3D_VTX_ATTR_2F_X(attrib), 2);
		OUT_RING(chan,  fui(v[0]));
		OUT_RING(chan,  fui(v[1]));
		break;
	case 1:
		BEGIN_RING(chan, eng3d, NV30_3D_VTX_ATTR_1F(attrib), 1);
		OUT_RING(chan,  fui(v[0]));
		break;
	}
}

#endif
