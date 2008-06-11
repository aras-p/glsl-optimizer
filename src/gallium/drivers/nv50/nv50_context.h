#ifndef __NV50_CONTEXT_H__
#define __NV50_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"
#include "nouveau/nouveau_stateobj.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv50_screen *ctx = nv50->screen
#include "nouveau/nouveau_push.h"

#include "nv50_screen.h"
#include "nv50_program.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

/* Constant buffer assignment */
#define NV50_CB_PMISC		1
#define NV50_CB_PVP		0
#define NV50_CB_PFP		2
#define NV50_CB_PGP		3
#define NV50_CB_TIC		4
#define NV50_CB_TSC		5

#define NV50_NEW_BLEND		(1 << 0)
#define NV50_NEW_ZSA		(1 << 1)
#define NV50_NEW_BLEND_COLOUR	(1 << 2)
#define NV50_NEW_STIPPLE	(1 << 3)
#define NV50_NEW_SCISSOR	(1 << 4)
#define NV50_NEW_VIEWPORT	(1 << 5)
#define NV50_NEW_RASTERIZER	(1 << 6)
#define NV50_NEW_FRAMEBUFFER	(1 << 7)
#define NV50_NEW_VERTPROG	(1 << 8)
#define NV50_NEW_VERTPROG_CB	(1 << 9)
#define NV50_NEW_FRAGPROG	(1 << 10)
#define NV50_NEW_FRAGPROG_CB	(1 << 11)
#define NV50_NEW_ARRAYS		(1 << 12)

struct nv50_blend_stateobj {
	struct pipe_blend_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_zsa_stateobj {
	struct pipe_depth_stencil_alpha_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_rasterizer_stateobj {
	struct pipe_rasterizer_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_context {
	struct pipe_context pipe;

	struct nv50_screen *screen;
	unsigned pctx_id;

	struct draw_context *draw;

	unsigned dirty;
	struct nv50_blend_stateobj *blend;
	struct nv50_zsa_stateobj *zsa;
	struct nv50_rasterizer_stateobj *rasterizer;
	struct pipe_blend_color blend_colour;
	struct pipe_poly_stipple stipple;
	struct pipe_scissor_state scissor;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct nv50_program *vertprog;
	struct nv50_program *fragprog;
	struct pipe_buffer *constbuf[PIPE_SHADER_TYPES];
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct pipe_vertex_element vtxelt[PIPE_MAX_ATTRIBS];
	unsigned vtxelt_nr;
};

static INLINE struct nv50_context *
nv50_context(struct pipe_context *pipe)
{
	return (struct nv50_context *)pipe;
}

extern void nv50_init_surface_functions(struct nv50_context *nv50);
extern void nv50_init_state_functions(struct nv50_context *nv50);
extern void nv50_init_query_functions(struct nv50_context *nv50);

extern void nv50_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv50_draw.c */
extern struct draw_stage *nv50_draw_render_stage(struct nv50_context *nv50);

/* nv50_vbo.c */
extern boolean nv50_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv50_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);
extern void nv50_vbo_validate(struct nv50_context *nv50);

/* nv50_clear.c */
extern void nv50_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

/* nv50_program.c */
extern void nv50_vertprog_validate(struct nv50_context *nv50);
extern void nv50_fragprog_validate(struct nv50_context *nv50);
extern void nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p);

/* nv50_state_validate.c */
extern boolean nv50_state_validate(struct nv50_context *nv50);

#endif
