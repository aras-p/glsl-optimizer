#ifndef __NV40_CONTEXT_H__
#define __NV40_CONTEXT_H__

#include "nvfx_context.h"

extern void nv40_init_state_functions(struct nvfx_context *nvfx);
extern void nv40_init_surface_functions(struct nvfx_context *nvfx);
extern void nv40_init_query_functions(struct nvfx_context *nvfx);
extern void nv40_init_transfer_functions(struct nvfx_context *nvfx);

extern void nv40_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv40_draw.c */
extern struct draw_stage *nv40_draw_render_stage(struct nvfx_context *nvfx);
extern void nv40_draw_elements_swtnl(struct pipe_context *pipe,
					struct pipe_buffer *idxbuf,
					unsigned ib_size, unsigned mode,
					unsigned start, unsigned count);

/* nv40_vertprog.c */
extern void nv40_vertprog_destroy(struct nvfx_context *,
				  struct nvfx_vertex_program *);

/* nv40_fragprog.c */
extern void nv40_fragprog_destroy(struct nvfx_context *,
				  struct nvfx_fragment_program *);

/* nv40_fragtex.c */
extern void nv40_fragtex_bind(struct nvfx_context *);

/* nv40_state.c and friends */
extern boolean nv40_state_validate(struct nvfx_context *nvfx);
extern boolean nv40_state_validate_swtnl(struct nvfx_context *nvfx);
extern void nv40_state_emit(struct nvfx_context *nvfx);
extern void nv40_state_flush_notify(struct nouveau_channel *chan);
extern struct nvfx_state_entry nv40_state_rasterizer;
extern struct nvfx_state_entry nv40_state_scissor;
extern struct nvfx_state_entry nv40_state_stipple;
extern struct nvfx_state_entry nv40_state_fragprog;
extern struct nvfx_state_entry nv40_state_vertprog;
extern struct nvfx_state_entry nv40_state_blend;
extern struct nvfx_state_entry nv40_state_blend_colour;
extern struct nvfx_state_entry nv40_state_zsa;
extern struct nvfx_state_entry nv40_state_viewport;
extern struct nvfx_state_entry nv40_state_framebuffer;
extern struct nvfx_state_entry nv40_state_fragtex;
extern struct nvfx_state_entry nv40_state_vbo;
extern struct nvfx_state_entry nv40_state_vtxfmt;
extern struct nvfx_state_entry nv40_state_sr;

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

/* nvfx_context.c */
struct pipe_context *
nv40_create(struct pipe_screen *pscreen, void *priv);

#endif
