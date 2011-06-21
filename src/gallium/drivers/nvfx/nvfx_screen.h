#ifndef __NVFX_SCREEN_H__
#define __NVFX_SCREEN_H__

#include "pipe/p_compiler.h"
#include "util/u_double_list.h"
#include "nouveau/nouveau_screen.h"

struct pipe_screen;

struct nvfx_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;
	struct nouveau_bo *fence;

	struct nvfx_context *cur_ctx;

	unsigned is_nv4x; /* either 0 or ~0 */
	unsigned use_nv4x; /* either 0 or ~0 */
	boolean force_swtnl;
	boolean trace_draw;
	unsigned vertex_buffer_reloc_flags;
	unsigned index_buffer_reloc_flags;
	unsigned advertise_fp16;
	unsigned advertise_fp32;
	unsigned advertise_npot;
	unsigned advertise_blend_equation_separate;

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

	/* Once the amount of bytes drawn from the buffer reaches the updated size times this value,
	 * we will assume that the buffer will be drawn an huge number of times before the
	 * next modification
	 */
	float static_reuse_threshold;

	/* Cost of allocating a buffer in terms of the cost of copying a byte to an hardware buffer */
	unsigned buffer_allocation_cost;

	/* inline_cost/hardware_cost conversion ration */
	float inline_cost_per_hardware_cost;
};

static INLINE struct nvfx_screen *
nvfx_screen(struct pipe_screen *screen)
{
	return (struct nvfx_screen *)screen;
}

int nvfx_screen_surface_init(struct pipe_screen *pscreen);
void nvfx_screen_surface_takedown(struct pipe_screen *pscreen);

#endif
