#ifndef __NV40_CONTEXT_H__
#define __NV40_CONTEXT_H__

#include "nvfx_context.h"

/* nv40_fragtex.c */
extern void nv40_fragtex_bind(struct nvfx_context *);
extern struct nvfx_state_entry nv40_state_fragtex;

/* nvfx_context.c */
struct pipe_context *
nv40_create(struct pipe_screen *pscreen, void *priv);

#endif
