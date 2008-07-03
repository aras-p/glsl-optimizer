#ifndef __NV30_CONTEXT_H__
#define __NV30_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv30_screen *ctx = nv30->screen
#include "nouveau/nouveau_push.h"
#include "nouveau/nouveau_stateobj.h"

#include "nv30_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

enum nv30_state_index {
	NV30_STATE_FB = 0,
	NV30_STATE_VIEWPORT = 1,
	NV30_STATE_BLEND = 2,
	NV30_STATE_RAST = 3,
	NV30_STATE_ZSA = 4,
	NV30_STATE_BCOL = 5,
	NV30_STATE_CLIP = 6,
	NV30_STATE_SCISSOR = 7,
	NV30_STATE_STIPPLE = 8,
	NV30_STATE_FRAGPROG = 9,
	NV30_STATE_VERTPROG = 10,
	NV30_STATE_FRAGTEX0 = 11,
	NV30_STATE_FRAGTEX1 = 12,
	NV30_STATE_FRAGTEX2 = 13,
	NV30_STATE_FRAGTEX3 = 14,
	NV30_STATE_FRAGTEX4 = 15,
	NV30_STATE_FRAGTEX5 = 16,
	NV30_STATE_FRAGTEX6 = 17,
	NV30_STATE_FRAGTEX7 = 18,
	NV30_STATE_FRAGTEX8 = 19,
	NV30_STATE_FRAGTEX9 = 20,
	NV30_STATE_FRAGTEX10 = 21,
	NV30_STATE_FRAGTEX11 = 22,
	NV30_STATE_FRAGTEX12 = 23,
	NV30_STATE_FRAGTEX13 = 24,
	NV30_STATE_FRAGTEX14 = 25,
	NV30_STATE_FRAGTEX15 = 26,
	NV30_STATE_VERTTEX0 = 27,
	NV30_STATE_VERTTEX1 = 28,
	NV30_STATE_VERTTEX2 = 29,
	NV30_STATE_VERTTEX3 = 30,
	NV30_STATE_VTXBUF = 31,
	NV30_STATE_VTXFMT = 32,
	NV30_STATE_VTXATTR = 33,
	NV30_STATE_MAX = 34
};

#include "nv30_screen.h"

#define NV30_NEW_BLEND		(1 <<  0)
#define NV30_NEW_RAST		(1 <<  1)
#define NV30_NEW_ZSA		(1 <<  2)
#define NV30_NEW_SAMPLER	(1 <<  3)
#define NV30_NEW_FB		(1 <<  4)
#define NV30_NEW_STIPPLE	(1 <<  5)
#define NV30_NEW_SCISSOR	(1 <<  6)
#define NV30_NEW_VIEWPORT	(1 <<  7)
#define NV30_NEW_BCOL		(1 <<  8)
#define NV30_NEW_VERTPROG	(1 <<  9)
#define NV30_NEW_FRAGPROG	(1 << 10)
#define NV30_NEW_ARRAYS		(1 << 11)
#define NV30_NEW_UCP		(1 << 12)

struct nv30_rasterizer_state {
	struct pipe_rasterizer_state pipe;
	struct nouveau_stateobj *so;
};

struct nv30_zsa_state {
	struct pipe_depth_stencil_alpha_state pipe;
	struct nouveau_stateobj *so;
};

struct nv30_blend_state {
	struct pipe_blend_state pipe;
	struct nouveau_stateobj *so;
};


struct nv30_state {
	unsigned scissor_enabled;
	unsigned stipple_enabled;
	unsigned viewport_bypass;

	uint64_t dirty;
	struct nouveau_stateobj *hw[NV30_STATE_MAX];
};

struct nv30_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nv30_screen *screen;
	unsigned pctx_id;

	struct draw_context *draw;

	/* HW state derived from pipe states */
	struct nv30_state state;

	struct nv30_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv30_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	/* Context state */
	unsigned dirty;
	struct pipe_scissor_state scissor;
	unsigned stipple[32];
	struct nv30_rasterizer_state *rasterizer;
	struct nv30_zsa_state *zsa;
	struct nv30_blend_state *blend;
	struct pipe_blend_color blend_colour;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;

	uint32_t rt_enable;
	struct pipe_buffer *rt[2];
	struct pipe_buffer *zeta;

	/*struct {
		struct pipe_buffer *buffer;
		uint32_t format;
	} tex[16];*/

	unsigned vb_enable;
	struct {
		struct pipe_buffer *buffer;
		unsigned delta;
	} vb[16];

	struct {
		struct nv30_vertex_program *active;

		struct nv30_vertex_program *current;
		struct pipe_buffer *constant_buf;
	} vertprog;

	struct {
		struct nv30_fragment_program *active;

		struct nv30_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_MAX_ATTRIBS];
	struct pipe_vertex_element vtxelt[PIPE_MAX_ATTRIBS];
};

static INLINE struct nv30_context *
nv30_context(struct pipe_context *pipe)
{
	return (struct nv30_context *)pipe;
}

struct nv30_state_entry {
	boolean (*validate)(struct nv30_context *nv30);
	struct {
		unsigned pipe;
		unsigned hw;
	} dirty;
};

extern void nv30_init_state_functions(struct nv30_context *nv30);
extern void nv30_init_surface_functions(struct nv30_context *nv30);
extern void nv30_init_query_functions(struct nv30_context *nv30);

extern void nv30_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv30_draw.c */
extern struct draw_stage *nv30_draw_render_stage(struct nv30_context *nv30);

/* nv30_vertprog.c */
extern void nv30_vertprog_translate(struct nv30_context *,
				    struct nv30_vertex_program *);
extern void nv30_vertprog_bind(struct nv30_context *,
			       struct nv30_vertex_program *);
extern void nv30_vertprog_destroy(struct nv30_context *,
				  struct nv30_vertex_program *);

/* nv30_fragprog.c */
extern void nv30_fragprog_translate(struct nv30_context *,
				    struct nv30_fragment_program *);
extern void nv30_fragprog_bind(struct nv30_context *,
			       struct nv30_fragment_program *);
extern void nv30_fragprog_destroy(struct nv30_context *,
				  struct nv30_fragment_program *);

/* nv30_fragtex.c */
extern void nv30_fragtex_bind(struct nv30_context *);

/* nv30_state.c and friends */
extern boolean nv30_state_validate(struct nv30_context *nv30);
extern void nv30_emit_hw_state(struct nv30_context *nv30);
extern void nv30_state_tex_update(struct nv30_context *nv30);
extern struct nv30_state_entry nv30_state_rasterizer;
extern struct nv30_state_entry nv30_state_scissor;
extern struct nv30_state_entry nv30_state_stipple;
extern struct nv30_state_entry nv30_state_blend;
extern struct nv30_state_entry nv30_state_blend_colour;
extern struct nv30_state_entry nv30_state_zsa;
extern struct nv30_state_entry nv30_state_framebuffer;

/* nv30_vbo.c */
extern boolean nv30_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv30_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);

/* nv30_clear.c */
extern void nv30_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

#endif
