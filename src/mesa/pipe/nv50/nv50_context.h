#ifndef __NV50_CONTEXT_H__
#define __NV50_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "pipe/draw/draw_vertex.h"

#include "pipe/nouveau/nouveau_winsys.h"
#include "pipe/nouveau/nouveau_gldefs.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv50_context *ctx = nv50
#include "pipe/nouveau/nouveau_push.h"

#include "nv50_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

struct nv50_context {
	struct pipe_context pipe;
	struct nouveau_winsys *nvws;

	struct draw_context *draw;

	int chipset;
	struct nouveau_grobj *tesla;
	struct nouveau_notifier *sync;
};


extern void nv50_init_miptree_functions(struct nv50_context *nv50);
extern void nv50_init_surface_functions(struct nv50_context *nv50);
extern void nv50_init_state_functions(struct nv50_context *nv50);
extern void nv50_init_query_functions(struct nv50_context *nv50);

/* nv50_draw.c */
extern struct draw_stage *nv50_draw_render_stage(struct nv50_context *nv50);

/* nv50_vbo.c */
extern boolean nv50_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv50_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer_handle *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);

/* nv50_clear.c */
extern void nv50_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

#endif
