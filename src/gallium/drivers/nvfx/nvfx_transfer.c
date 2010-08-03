#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_staging.h"
#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"
#include "nvfx_transfer.h"

struct nvfx_staging_transfer
{
	struct util_staging_transfer base;

	unsigned offset;
	unsigned map_count;
};

struct pipe_transfer *
nvfx_transfer_new(struct pipe_context *pipe,
			  struct pipe_resource *pt,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
	struct nvfx_staging_transfer* tx;
	bool direct = !nvfx_resource_on_gpu(pt) && pt->flags & NVFX_RESOURCE_FLAG_LINEAR;

	tx = CALLOC_STRUCT(nvfx_staging_transfer);
	if(!tx)
		return NULL;

	util_staging_transfer_init(pipe, pt, sr, usage, box, direct, tx);

	if(pt->target == PIPE_BUFFER)
	{
		tx->base.base.slice_stride = tx->base.base.stride = ((struct nvfx_resource*)tx->base.staging_resource)->bo->size;
		if(direct)
			tx->offset = util_format_get_stride(pt->format, box->x);
		else
			tx->offset = 0;
	}
	else
	{
		if(direct)
		{
			tx->base.base.stride = nvfx_subresource_pitch(pt, sr.level);
			tx->base.base.slice_stride = tx->base.base.stride * u_minify(pt->height0, sr.level);
			tx->offset = nvfx_subresource_offset(pt, sr.face, sr.level, box->z)
				+ util_format_get_2d_size(pt->format, tx->base.base.stride, box->y)
				+ util_format_get_stride(pt->format, box->x);
		}
		else
		{
			tx->base.base.stride = nvfx_subresource_pitch(tx->base.staging_resource, 0);
			tx->base.base.slice_stride = tx->base.base.stride * tx->base.staging_resource->height0;
			tx->offset = 0;
		}

		assert(tx->base.base.stride);

		return &tx->base.base;
	}
}

void *
nvfx_transfer_map(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	struct nvfx_staging_transfer *tx = (struct nvfx_staging_transfer *)ptx;
	if(!ptx->data)
	{
		struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->base.staging_resource;
		uint8_t *map = nouveau_screen_bo_map(pipe->screen, mt->base.bo, nouveau_screen_transfer_flags(ptx->usage));
		ptx->data = map + tx->offset;
	}
	++tx->map_count;
	return ptx->data;
}

void
nvfx_transfer_unmap(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	struct nvfx_staging_transfer *tx = (struct nvfx_staging_transfer *)ptx;
	struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->base.staging_resource;

	if(!--tx->map_count)
		nouveau_screen_bo_unmap(pipe->screen, mt->base.bo);
}
