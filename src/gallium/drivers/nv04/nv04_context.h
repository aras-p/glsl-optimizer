#ifndef __NV04_CONTEXT_H__
#define __NV04_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_compiler.h"

#include "util/u_memory.h"
#include "util/u_math.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"
#include "nouveau/nouveau_context.h"

#define NOUVEAU_PUSH_CONTEXT(ctx)                                              \
	struct nv04_screen *ctx = nv04->screen
#include "nouveau/nouveau_push.h"

#include "nv04_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#include "nv04_screen.h"

#define NV04_NEW_VERTPROG	(1 << 1)
#define NV04_NEW_FRAGPROG	(1 << 2)
#define NV04_NEW_BLEND		(1 << 3)
#define NV04_NEW_RAST		(1 << 4)
#define NV04_NEW_CONTROL	(1 << 5)
#define NV04_NEW_VIEWPORT	(1 << 6)
#define NV04_NEW_SAMPLER	(1 << 7)
#define NV04_NEW_FRAMEBUFFER	(1 << 8)
#define NV04_NEW_VTXARRAYS	(1 << 9)

struct nv04_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nv04_screen *screen;
	unsigned pctx_id;

	struct draw_context *draw;

	int chipset;
	struct nouveau_notifier *sync;

	uint32_t dirty;

	struct nv04_blend_state *blend;
	struct nv04_sampler_state *sampler[PIPE_MAX_SAMPLERS];
	struct nv04_fragtex_state fragtex;
	struct nv04_rasterizer_state *rast;
	struct nv04_depth_stencil_alpha_state *dsa;

	struct nv04_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	uint32_t rt_enable;
	struct pipe_framebuffer_state *framebuffer;
	struct pipe_surface *rt;
	struct pipe_surface *zeta;

	struct {
		struct pipe_buffer *buffer;
		uint32_t format;
	} tex[16];

	unsigned vb_enable;
	struct {
		struct pipe_buffer *buffer;
		unsigned delta;
	} vb[16];

	float *constbuf[PIPE_SHADER_TYPES][32][4];
	unsigned constbuf_nr[PIPE_SHADER_TYPES];

	struct vertex_info vertex_info;
	struct {
	
		struct nouveau_resource *exec_heap;
		struct nouveau_resource *data_heap;

		struct nv04_vertex_program *active;

		struct nv04_vertex_program *current;
		struct pipe_buffer *constant_buf;
	} vertprog;

	struct {
		struct nv04_fragment_program *active;

		struct nv04_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_MAX_ATTRIBS];
	struct pipe_vertex_element vtxelt[PIPE_MAX_ATTRIBS];

	struct pipe_viewport_state viewport;
};

static INLINE struct nv04_context *
nv04_context(struct pipe_context *pipe)
{
	return (struct nv04_context *)pipe;
}

extern void nv04_init_state_functions(struct nv04_context *nv04);
extern void nv04_init_surface_functions(struct nv04_context *nv04);
extern void nv04_screen_init_miptree_functions(struct pipe_screen *screen);

/* nv04_clear.c */
extern void nv04_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		       unsigned clearValue);

/* nv04_draw.c */
extern struct draw_stage *nv04_draw_render_stage(struct nv04_context *nv04);

/* nv04_fragprog.c */
extern void nv04_fragprog_bind(struct nv04_context *,
			       struct nv04_fragment_program *);
extern void nv04_fragprog_destroy(struct nv04_context *,
				  struct nv04_fragment_program *);

/* nv04_fragtex.c */
extern void nv04_fragtex_bind(struct nv04_context *);

/* nv04_prim_vbuf.c */
struct draw_stage *nv04_draw_vbuf_stage( struct nv04_context *nv04 );

/* nv04_state.c and friends */
extern void nv04_emit_hw_state(struct nv04_context *nv04);
extern void nv04_state_tex_update(struct nv04_context *nv04);

/* nv04_vbo.c */
extern boolean nv04_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern boolean nv04_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count);


#endif
