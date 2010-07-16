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

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"

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

struct nvfx_rasterizer_state {
	struct pipe_rasterizer_state pipe;
	unsigned sb_len;
	uint32_t sb[32];
};

struct nvfx_zsa_state {
	struct pipe_depth_stencil_alpha_state pipe;
	unsigned sb_len;
	uint32_t sb[26];
};

struct nvfx_blend_state {
	struct pipe_blend_state pipe;
	unsigned sb_len;
	uint32_t sb[13];
};


struct nvfx_state {
	unsigned scissor_enabled;
	unsigned stipple_enabled;
	unsigned fp_samplers;
};

struct nvfx_vtxelt_state {
	struct pipe_vertex_element pipe[16];
	unsigned num_elements;
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

	struct draw_context *draw;

	/* HW state derived from pipe states */
	struct nvfx_state state;
	struct {
		struct nvfx_vertex_program *vertprog;

		unsigned nr_attribs;
		unsigned hw[PIPE_MAX_SHADER_INPUTS];
		unsigned draw[PIPE_MAX_SHADER_INPUTS];
		unsigned emit[PIPE_MAX_SHADER_INPUTS];
	} swtnl;

	enum {
		HW, SWTNL, SWRAST
	} render_mode;
	unsigned fallback_swtnl;

	/* Context state */
	unsigned dirty, draw_dirty;
	struct pipe_scissor_state scissor;
	unsigned stipple[32];
	struct pipe_clip_state clip;
	struct nvfx_vertex_program *vertprog;
	struct nvfx_fragment_program *fragprog;
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
	struct pipe_resource *idxbuf_buffer;
	unsigned idxbuf_format;
	struct nvfx_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct pipe_sampler_view *fragment_sampler_views[PIPE_MAX_SAMPLERS];
	unsigned nr_samplers;
	unsigned nr_textures;
	unsigned dirty_samplers;
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct nvfx_vtxelt_state *vtxelt;

	unsigned vbo_bo;
	unsigned hw_vtxelt_nr;
	uint8_t hw_samplers;
	uint32_t hw_txf[8];
	struct nvfx_render_target hw_rt[4];
	struct nvfx_render_target hw_zeta;
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
extern void nvfx_draw_elements_swtnl(struct pipe_context *pipe,
                                     struct pipe_resource *idxbuf,
                                     unsigned ib_size, int ib_bias,
                                     unsigned mode,
                                     unsigned start, unsigned count);
extern void nvfx_vtxfmt_validate(struct nvfx_context *nvfx);

/* nvfx_fb.c */
extern void nvfx_state_framebuffer_validate(struct nvfx_context *nvfx);
void
nvfx_framebuffer_relocate(struct nvfx_context *nvfx);

/* nvfx_fragprog.c */
extern void nvfx_fragprog_destroy(struct nvfx_context *,
				    struct nvfx_fragment_program *);
extern void nvfx_fragprog_validate(struct nvfx_context *nvfx);
extern void
nvfx_fragprog_relocate(struct nvfx_context *nvfx);

/* nvfx_fragtex.c */
extern void nvfx_fragtex_validate(struct nvfx_context *nvfx);
extern void
nvfx_fragtex_relocate(struct nvfx_context *nvfx);

/* nv30_fragtex.c */
extern void
nv30_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso);
extern void nv30_fragtex_set(struct nvfx_context *nvfx, int unit);

/* nv40_fragtex.c */
extern void
nv40_sampler_state_init(struct pipe_context *pipe,
			  struct nvfx_sampler_state *ps,
			  const struct pipe_sampler_state *cso);
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
extern void nvfx_state_relocate(struct nvfx_context *nvfx);
extern boolean nvfx_state_validate(struct nvfx_context *nvfx);
extern boolean nvfx_state_validate_swtnl(struct nvfx_context *nvfx);
extern void nvfx_state_emit(struct nvfx_context *nvfx);

/* nvfx_transfer.c */
extern void nvfx_init_transfer_functions(struct nvfx_context *nvfx);

/* nvfx_vbo.c */
extern boolean nvfx_vbo_validate(struct nvfx_context *nvfx);
extern void nvfx_vbo_relocate(struct nvfx_context *nvfx);
extern void nvfx_draw_vbo(struct pipe_context *pipe,
                          const struct pipe_draw_info *info);

/* nvfx_vertprog.c */
extern boolean nvfx_vertprog_validate(struct nvfx_context *nvfx);
extern void nvfx_vertprog_destroy(struct nvfx_context *,
				  struct nvfx_vertex_program *);

#endif
