#ifndef __NV30_CONTEXT_H__
#define __NV30_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv30_context *ctx = nv30
#include "nouveau/nouveau_push.h"

#include "nv30_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#define NV30_NEW_VERTPROG	(1 << 1)
#define NV30_NEW_FRAGPROG	(1 << 2)
#define NV30_NEW_ARRAYS		(1 << 3)

struct nv30_context {
	struct pipe_context pipe;
	struct nouveau_winsys *nvws;

	struct draw_context *draw;

	int chipset;
	struct nouveau_grobj *rankine;
	struct nouveau_notifier *sync;

	/* query objects */
	struct nouveau_notifier *query;
	struct nouveau_resource *query_heap;

	uint32_t dirty;

	struct nv30_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv30_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	uint32_t rt_enable;
	struct pipe_buffer *rt[2];
	struct pipe_buffer *zeta;

	struct {
		struct pipe_buffer *buffer;
		uint32_t format;
	} tex[16];

	unsigned vb_enable;
	struct {
		struct pipe_buffer *buffer;
		unsigned delta;
	} vb[16];

	struct {
		struct nouveau_resource *exec_heap;
		struct nouveau_resource *data_heap;

		struct nv30_vertex_program *active;

		struct nv30_vertex_program *current;
		struct pipe_buffer *constant_buf;
	} vertprog;

	struct {
		struct nv30_fragment_program *active;

		struct nv30_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_ATTRIB_MAX];
	struct pipe_vertex_element vtxelt[PIPE_ATTRIB_MAX];
};

static INLINE struct nv30_context *
nv30_context(struct pipe_context *pipe)
{
	return (struct nv30_context *)pipe;
}

extern void nv30_init_state_functions(struct nv30_context *nv30);
extern void nv30_init_surface_functions(struct nv30_context *nv30);
extern void nv30_init_miptree_functions(struct nv30_context *nv30);
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
extern void nv30_emit_hw_state(struct nv30_context *nv30);
extern void nv30_state_tex_update(struct nv30_context *nv30);

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
