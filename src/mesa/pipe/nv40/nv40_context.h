#ifndef __NV40_CONTEXT_H__
#define __NV40_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "pipe/draw/draw_vertex.h"

#include "pipe/nouveau/nouveau_winsys.h"
#include "pipe/nouveau/nouveau_gldefs.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv40_context *ctx = nv40
#include "pipe/nouveau/nouveau_push.h"

#include "nv40_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#define NV40_NEW_VERTPROG	(1 << 1)
#define NV40_NEW_FRAGPROG	(1 << 2)
#define NV40_NEW_ARRAYS		(1 << 3)

struct nv40_context {
	struct pipe_context pipe;
	struct nouveau_winsys *nvws;

	struct draw_context *draw;

	int chipset;
	struct nouveau_grobj *curie;
	struct nouveau_notifier *sync;

	/* query objects */
	struct nouveau_notifier *query;
	struct nouveau_resource *query_heap;

	uint32_t dirty;

	struct nv40_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv40_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	uint32_t rt_enable;
	struct pipe_buffer *rt[4];
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

		struct nv40_vertex_program *active;

		struct nv40_vertex_program *current;
		struct pipe_buffer *constant_buf;
	} vertprog;

	struct {
		struct nv40_fragment_program *active;

		struct nv40_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_ATTRIB_MAX];
	struct pipe_vertex_element vtxelt[PIPE_ATTRIB_MAX];
};

static inline struct nv40_context *
nv40_context(struct pipe_context *pipe)
{
	return (struct nv40_context *)pipe;
}

extern void nv40_init_state_functions(struct nv40_context *nv40);
extern void nv40_init_surface_functions(struct nv40_context *nv40);
extern void nv40_init_miptree_functions(struct nv40_context *nv40);
extern void nv40_init_query_functions(struct nv40_context *nv40);

/* nv40_draw.c */
extern struct draw_stage *nv40_draw_render_stage(struct nv40_context *nv40);

/* nv40_vertprog.c */
extern void nv40_vertprog_translate(struct nv40_context *,
				    struct nv40_vertex_program *);
extern void nv40_vertprog_bind(struct nv40_context *,
			       struct nv40_vertex_program *);
extern void nv40_vertprog_destroy(struct nv40_context *,
				  struct nv40_vertex_program *);

/* nv40_fragprog.c */
extern void nv40_fragprog_translate(struct nv40_context *,
				    struct nv40_fragment_program *);
extern void nv40_fragprog_bind(struct nv40_context *,
			       struct nv40_fragment_program *);
extern void nv40_fragprog_destroy(struct nv40_context *,
				  struct nv40_fragment_program *);

/* nv40_fragtex.c */
extern void nv40_fragtex_bind(struct nv40_context *);

/* nv40_state.c and friends */
extern void nv40_emit_hw_state(struct nv40_context *nv40);
extern void nv40_state_tex_update(struct nv40_context *nv40);

/* nv40_vbo.c */
extern boolean nv40_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv40_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);

/* nv40_clear.c */
extern void nv40_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

#endif
