#ifndef __NV40_CONTEXT_H__
#define __NV40_CONTEXT_H__

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
#include "nouveau/nouveau_context.h"
#include "nouveau/nouveau_stateobj.h"

#include "nv40_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

enum nv40_state_index {
	NV40_STATE_FB = 0,
	NV40_STATE_VIEWPORT = 1,
	NV40_STATE_BLEND = 2,
	NV40_STATE_RAST = 3,
	NV40_STATE_ZSA = 4,
	NV40_STATE_BCOL = 5,
	NV40_STATE_CLIP = 6,
	NV40_STATE_SCISSOR = 7,
	NV40_STATE_STIPPLE = 8,
	NV40_STATE_FRAGPROG = 9,
	NV40_STATE_VERTPROG = 10,
	NV40_STATE_FRAGTEX0 = 11,
	NV40_STATE_FRAGTEX1 = 12,
	NV40_STATE_FRAGTEX2 = 13,
	NV40_STATE_FRAGTEX3 = 14,
	NV40_STATE_FRAGTEX4 = 15,
	NV40_STATE_FRAGTEX5 = 16,
	NV40_STATE_FRAGTEX6 = 17,
	NV40_STATE_FRAGTEX7 = 18,
	NV40_STATE_FRAGTEX8 = 19,
	NV40_STATE_FRAGTEX9 = 20,
	NV40_STATE_FRAGTEX10 = 21,
	NV40_STATE_FRAGTEX11 = 22,
	NV40_STATE_FRAGTEX12 = 23,
	NV40_STATE_FRAGTEX13 = 24,
	NV40_STATE_FRAGTEX14 = 25,
	NV40_STATE_FRAGTEX15 = 26,
	NV40_STATE_VERTTEX0 = 27,
	NV40_STATE_VERTTEX1 = 28,
	NV40_STATE_VERTTEX2 = 29,
	NV40_STATE_VERTTEX3 = 30,
	NV40_STATE_VTXBUF = 31,
	NV40_STATE_VTXFMT = 32,
	NV40_STATE_VTXATTR = 33,
	NV40_STATE_SR = 34,
	NV40_STATE_MAX = 35
};

#include "nv40_screen.h"

#define NV40_NEW_BLEND		(1 <<  0)
#define NV40_NEW_RAST		(1 <<  1)
#define NV40_NEW_ZSA		(1 <<  2)
#define NV40_NEW_SAMPLER	(1 <<  3)
#define NV40_NEW_FB		(1 <<  4)
#define NV40_NEW_STIPPLE	(1 <<  5)
#define NV40_NEW_SCISSOR	(1 <<  6)
#define NV40_NEW_VIEWPORT	(1 <<  7)
#define NV40_NEW_BCOL		(1 <<  8)
#define NV40_NEW_VERTPROG	(1 <<  9)
#define NV40_NEW_FRAGPROG	(1 << 10)
#define NV40_NEW_ARRAYS		(1 << 11)
#define NV40_NEW_UCP		(1 << 12)
#define NV40_NEW_SR		(1 << 13)

struct nv40_rasterizer_state {
	struct pipe_rasterizer_state pipe;
	struct nouveau_stateobj *so;
};

struct nv40_zsa_state {
	struct pipe_depth_stencil_alpha_state pipe;
	struct nouveau_stateobj *so;
};

struct nv40_blend_state {
	struct pipe_blend_state pipe;
	struct nouveau_stateobj *so;
};


struct nv40_state {
	unsigned scissor_enabled;
	unsigned stipple_enabled;
	unsigned fp_samplers;

	uint64_t dirty;
	struct nouveau_stateobj *hw[NV40_STATE_MAX];
};

struct nv40_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nv40_screen *screen;

	struct draw_context *draw;

	/* HW state derived from pipe states */
	struct nv40_state state;
	struct {
		struct nv40_vertex_program *vertprog;

		unsigned nr_attribs;
		unsigned hw[PIPE_MAX_SHADER_INPUTS];
		unsigned draw[PIPE_MAX_SHADER_INPUTS];
		unsigned emit[PIPE_MAX_SHADER_INPUTS];
	} swtnl;

	enum {
		HW, SWTNL, SWRAST
	} render_mode;
	unsigned fallback_swtnl;
	unsigned fallback_swrast;

	/* Context state */
	unsigned dirty, draw_dirty;
	struct pipe_scissor_state scissor;
	unsigned stipple[32];
	struct pipe_clip_state clip;
	struct nv40_vertex_program *vertprog;
	struct nv40_fragment_program *fragprog;
	struct pipe_buffer *constbuf[PIPE_SHADER_TYPES];
	unsigned constbuf_nr[PIPE_SHADER_TYPES];
	struct nv40_rasterizer_state *rasterizer;
	struct nv40_zsa_state *zsa;
	struct nv40_blend_state *blend;
	struct pipe_blend_color blend_colour;
	struct pipe_stencil_ref stencil_ref;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_buffer *idxbuf;
	unsigned idxbuf_format;
	struct nv40_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv40_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned nr_samplers;
	unsigned nr_textures;
	unsigned dirty_samplers;
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct pipe_vertex_element vtxelt[PIPE_MAX_ATTRIBS];
	unsigned vtxelt_nr;
};

static INLINE struct nv40_context *
nv40_context(struct pipe_context *pipe)
{
	return (struct nv40_context *)pipe;
}

struct nv40_state_entry {
	boolean (*validate)(struct nv40_context *nv40);
	struct {
		unsigned pipe;
		unsigned hw;
	} dirty;
};

extern void nv40_init_state_functions(struct nv40_context *nv40);
extern void nv40_init_surface_functions(struct nv40_context *nv40);
extern void nv40_init_query_functions(struct nv40_context *nv40);

extern void nv40_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv40_draw.c */
extern struct draw_stage *nv40_draw_render_stage(struct nv40_context *nv40);
extern void nv40_draw_elements_swtnl(struct pipe_context *pipe,
					struct pipe_buffer *idxbuf,
					unsigned ib_size, unsigned mode,
					unsigned start, unsigned count);

/* nv40_vertprog.c */
extern void nv40_vertprog_destroy(struct nv40_context *,
				  struct nv40_vertex_program *);

/* nv40_fragprog.c */
extern void nv40_fragprog_destroy(struct nv40_context *,
				  struct nv40_fragment_program *);

/* nv40_fragtex.c */
extern void nv40_fragtex_bind(struct nv40_context *);

/* nv40_state.c and friends */
extern boolean nv40_state_validate(struct nv40_context *nv40);
extern boolean nv40_state_validate_swtnl(struct nv40_context *nv40);
extern void nv40_state_emit(struct nv40_context *nv40);
extern void nv40_state_flush_notify(struct nouveau_channel *chan);
extern struct nv40_state_entry nv40_state_rasterizer;
extern struct nv40_state_entry nv40_state_scissor;
extern struct nv40_state_entry nv40_state_stipple;
extern struct nv40_state_entry nv40_state_fragprog;
extern struct nv40_state_entry nv40_state_vertprog;
extern struct nv40_state_entry nv40_state_blend;
extern struct nv40_state_entry nv40_state_blend_colour;
extern struct nv40_state_entry nv40_state_zsa;
extern struct nv40_state_entry nv40_state_viewport;
extern struct nv40_state_entry nv40_state_framebuffer;
extern struct nv40_state_entry nv40_state_fragtex;
extern struct nv40_state_entry nv40_state_vbo;
extern struct nv40_state_entry nv40_state_vtxfmt;
extern struct nv40_state_entry nv40_state_sr;

/* nv40_vbo.c */
extern void nv40_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern void nv40_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);

/* nv40_clear.c */
extern void nv40_clear(struct pipe_context *pipe, unsigned buffers,
		       const float *rgba, double depth, unsigned stencil);

/* nv40_context.c */
struct pipe_context *
nv40_create(struct pipe_screen *pscreen, void *priv);

#endif
