#ifndef __NV20_CONTEXT_H__
#define __NV20_CONTEXT_H__

#include <stdio.h>

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

#include "nv20_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

#define NV20_NEW_VERTPROG	(1 << 0)
#define NV20_NEW_FRAGPROG	(1 << 1)
#define NV20_NEW_VTXARRAYS	(1 << 2)
#define NV20_NEW_BLEND		(1 << 3)
#define NV20_NEW_BLENDCOL	(1 << 4)
#define NV20_NEW_RAST 		(1 << 5)
#define NV20_NEW_DSA  		(1 << 6)
#define NV20_NEW_VIEWPORT	(1 << 7)
#define NV20_NEW_SCISSOR	(1 << 8)
#define NV20_NEW_FRAMEBUFFER	(1 << 9)

#include "nv20_screen.h"

struct nv20_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nv20_screen *screen;
	unsigned pctx_id;

	struct draw_context *draw;

	uint32_t dirty;

	struct nv20_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nv20_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned dirty_samplers;
	unsigned fp_samplers;
	unsigned vp_samplers;

	uint32_t rt_enable;
	struct pipe_buffer *rt[4];
	struct pipe_buffer *zeta;
	uint32_t lma_offset;

	struct nv20_blend_state *blend;
	struct pipe_blend_color *blend_color;
	struct nv20_rasterizer_state *rast;
	struct nv20_depth_stencil_alpha_state *dsa;
	struct pipe_viewport_state *viewport;
	struct pipe_scissor_state *scissor;
	struct pipe_framebuffer_state *framebuffer;

	//struct pipe_buffer *constbuf[PIPE_SHADER_TYPES];
	float *constbuf[PIPE_SHADER_TYPES][32][4];
	unsigned constbuf_nr[PIPE_SHADER_TYPES];

	struct vertex_info vertex_info;

	struct {
		struct pipe_buffer *buffer;
		uint32_t format;
	} tex[2];

	unsigned vb_enable;
	struct {
		struct pipe_buffer *buffer;
		unsigned delta;
	} vb[16];

/*	struct {
	
		struct nouveau_resource *exec_heap;
		struct nouveau_resource *data_heap;

		struct nv20_vertex_program *active;

		struct nv20_vertex_program *current;
	} vertprog;
*/
	struct {
		struct nv20_fragment_program *active;

		struct nv20_fragment_program *current;
		struct pipe_buffer *constant_buf;
	} fragprog;

	struct pipe_vertex_buffer  vtxbuf[PIPE_MAX_ATTRIBS];
	struct pipe_vertex_element vtxelt[PIPE_MAX_ATTRIBS];
};

static INLINE struct nv20_context *
nv20_context(struct pipe_context *pipe)
{
	return (struct nv20_context *)pipe;
}

extern void nv20_init_state_functions(struct nv20_context *nv20);
extern void nv20_init_surface_functions(struct nv20_context *nv20);

extern void nv20_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv20_clear.c */
extern void nv20_clear(struct pipe_context *pipe, unsigned buffers,
		       const float *rgba, double depth, unsigned stencil);

/* nv20_draw.c */
extern struct draw_stage *nv20_draw_render_stage(struct nv20_context *nv20);

/* nv20_fragprog.c */
extern void nv20_fragprog_bind(struct nv20_context *,
			       struct nv20_fragment_program *);
extern void nv20_fragprog_destroy(struct nv20_context *,
				  struct nv20_fragment_program *);

/* nv20_fragtex.c */
extern void nv20_fragtex_bind(struct nv20_context *);

/* nv20_prim_vbuf.c */
struct draw_stage *nv20_draw_vbuf_stage( struct nv20_context *nv20 );
extern void nv20_vtxbuf_bind(struct nv20_context* nv20);

/* nv20_state.c and friends */
extern void nv20_emit_hw_state(struct nv20_context *nv20);
extern void nv20_state_tex_update(struct nv20_context *nv20);

/* nv20_vbo.c */
extern void nv20_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern void nv20_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count);


#endif
