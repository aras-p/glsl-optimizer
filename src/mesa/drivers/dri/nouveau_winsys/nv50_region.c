#include "pipe/p_context.h"

#include "nouveau_context.h"

static int
nv50_region_display(void)
{
	NOUVEAU_ERR("unimplemented\n");
	return 0;
}

static int
nv50_region_copy(struct nouveau_context *nv, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dx, unsigned dy,
		 struct pipe_region *src, unsigned src_offset,
		 unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	NOUVEAU_ERR("unimplemented!!\n");
	return 0;
}

static int
nv50_region_fill(struct nouveau_context *nv,
		 struct pipe_region *dst, unsigned dst_offset,
		 unsigned dx, unsigned dy, unsigned w, unsigned h,
		 unsigned value)
{
	NOUVEAU_ERR("unimplemented!!\n");
	return 0;
}

static int
nv50_region_data(struct nouveau_context *nv, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dx, unsigned dy,
		 const void *src, unsigned src_pitch,
		 unsigned sx, unsigned sy, unsigned w, unsigned h)
{
	NOUVEAU_ERR("unimplemented!!\n");
	return 0;
}

int
nouveau_region_init_nv50(struct nouveau_context *nv)
{
	nv->region_display = nv50_region_display;
	nv->region_copy = nv50_region_copy;
	nv->region_fill = nv50_region_fill;
	nv->region_data = nv50_region_data;
	return 0;
}

