#ifndef __NV30_CONTEXT_H__
#define __NV30_CONTEXT_H__

#include "nvfx_context.h"

extern void nv30_init_state_functions(struct nvfx_context *nvfx);
extern void nv30_init_surface_functions(struct nvfx_context *nvfx);
extern void nv30_init_query_functions(struct nvfx_context *nvfx);

extern void nv30_screen_init_miptree_functions(struct pipe_screen *pscreen);

/* nv30_draw.c */
extern struct draw_stage *nv30_draw_render_stage(struct nvfx_context *nvfx);

/* nv30_vertprog.c */
extern void nv30_vertprog_destroy(struct nvfx_context *,
				  struct nvfx_vertex_program *);

/* nv30_fragprog.c */
extern void nv30_fragprog_destroy(struct nvfx_context *,
				  struct nvfx_fragment_program *);

/* nv30_fragtex.c */
extern void nv30_fragtex_bind(struct nvfx_context *);

/* nv30_state.c and friends */
extern struct nvfx_state_entry nv30_state_scissor;
extern struct nvfx_state_entry nv30_state_stipple;
extern struct nvfx_state_entry nv30_state_fragprog;
extern struct nvfx_state_entry nv30_state_vertprog;
extern struct nvfx_state_entry nv30_state_zsa;
extern struct nvfx_state_entry nv30_state_viewport;
extern struct nvfx_state_entry nv30_state_framebuffer;
extern struct nvfx_state_entry nv30_state_fragtex;
extern struct nvfx_state_entry nv30_state_vbo;
extern struct nvfx_state_entry nv30_state_sr;

/* nv30_vbo.c */
extern void nv30_draw_arrays(struct pipe_context *, unsigned mode,
				unsigned start, unsigned count);
extern void nv30_draw_elements(struct pipe_context *pipe,
				  struct pipe_buffer *indexBuffer,
				  unsigned indexSize,
				  unsigned mode, unsigned start,
				  unsigned count);

/* nvfx_context.c */
struct pipe_context *
nv30_create(struct pipe_screen *pscreen, void *priv);

#endif
