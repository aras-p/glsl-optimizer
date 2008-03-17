#ifndef __NV10_CONTEXT_H__
#define __NV10_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv10_context *ctx = nv10
#include "nouveau/nouveau_push.h"

#include "nv10_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#define NV10_NEW_VERTPROG	(1 << 1)
#define NV10_NEW_FRAGPROG	(1 << 2)
#define NV10_NEW_ARRAYS		(1 << 3)
#define NV10_NEW_VBO		(1 << 4)

struct nv10_context {
	struct pipe_context pipe;
	struct nouveau_winsys *nvws;

	struct draw_context *draw;

	int chipset;
	struct nouveau_grobj *celsius;
	struct nouveau_notifier *sync;

	uint32_t dirty;

	struct nv10_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv10_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	uint32_t rt_enable;
	struct pipe_buffer *rt[4];
	struct pipe_buffer *zeta;
	uint32_t lma_offset;

	struct {
		struct pipe_buffer *buffer;
		uint32_t format;
	} tex[2];

	unsigned vb_enable;
	struct {
		struct pipe_buffer *buffer;
		unsigned delta;
	} vb[16];

	struct vertex_info vertex_info;
	struct {
	
		struct nouveau_resource *exec_heap;
		struct nouveau_resource *data_heap;

		struct nv10_vertex_program *active;

		struct nv10_vertex_program *current;
		struct pipe_buffer *constant_buf;
	} vertprog;

	struct {
		struct nv10_fragment_program *active;

		struct nv10_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_ATTRIB_MAX];
	struct pipe_vertex_element vtxelt[PIPE_ATTRIB_MAX];
};

static INLINE struct nv10_context *
nv10_context(struct pipe_context *pipe)
{
	return (struct nv10_context *)pipe;
}

extern void nv10_init_state_functions(struct nv10_context *nv10);
extern void nv10_init_surface_functions(struct nv10_context *nv10);
extern void nv10_init_miptree_functions(struct pipe_screen *screen);

/* nv10_clear.c */
extern void nv10_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

/* nv10_draw.c */
extern struct draw_stage *nv10_draw_render_stage(struct nv10_context *nv10);

/* nv10_fragprog.c */
extern void nv10_fragprog_bind(struct nv10_context *,
			       struct nv10_fragment_program *);
extern void nv10_fragprog_destroy(struct nv10_context *,
				  struct nv10_fragment_program *);

/* nv10_fragtex.c */
extern void nv10_fragtex_bind(struct nv10_context *);

/* nv10_prim_vbuf.c */
struct draw_stage *nv10_draw_vbuf_stage( struct nv10_context *nv10 );

/* nv10_state.c and friends */
extern void nv10_emit_hw_state(struct nv10_context *nv10);
extern void nv10_state_tex_update(struct nv10_context *nv10);

/* nv10_vbo.c */
extern boolean nv10_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv10_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count);


#endif
