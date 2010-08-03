#ifndef __NVFX_SCREEN_H__
#define __NVFX_SCREEN_H__

#include "util/u_double_list.h"
#include "nouveau/nouveau_screen.h"
#include "nvfx_context.h"

struct nv04_2d_context;

struct nvfx_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	struct nvfx_context *cur_ctx;

	unsigned is_nv4x; /* either 0 or ~0 */
	boolean force_swtnl;
	unsigned vertex_buffer_reloc_flags;
	unsigned index_buffer_reloc_flags;

	/* HW graphics objects */
	struct nouveau_grobj *eng3d;
	struct nouveau_notifier *sync;

	/* Query object resources */
	struct nouveau_notifier *query;
	struct nouveau_resource *query_heap;
	struct list_head query_list;

	/* Vtxprog resources */
	struct nouveau_resource *vp_exec_heap;
	struct nouveau_resource *vp_data_heap;

	struct nv04_2d_context* eng2d;
};

static INLINE struct nvfx_screen *
nvfx_screen(struct pipe_screen *screen)
{
	return (struct nvfx_screen *)screen;
}

int nvfx_screen_surface_init(struct pipe_screen *pscreen);
void nvfx_screen_surface_takedown(struct pipe_screen *pscreen);

#endif
