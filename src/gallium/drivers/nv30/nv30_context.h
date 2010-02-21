#ifndef __NV30_CONTEXT_H__
#define __NV30_CONTEXT_H__

#include "nvfx_context.h"

extern void nv30_init_state_functions(struct nvfx_context *nvfx);

/* nv30_vertprog.c */
extern void nv30_vertprog_destroy(struct nvfx_context *,
				  struct nvfx_vertex_program *);

/* nv30_fragtex.c */
extern void nv30_fragtex_bind(struct nvfx_context *);

/* nv30_state.c and friends */
extern struct nvfx_state_entry nv30_state_vertprog;
extern struct nvfx_state_entry nv30_state_fragtex;

/* nvfx_context.c */
struct pipe_context *
nv30_create(struct pipe_screen *pscreen, void *priv);

#endif
