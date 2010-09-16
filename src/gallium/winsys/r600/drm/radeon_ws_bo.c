#include <malloc.h>
#include "radeon_priv.h"

struct radeon_ws_bo *radeon_ws_bo(struct radeon *radeon,
				  unsigned size, unsigned alignment)
{
	struct radeon_ws_bo *ws_bo = calloc(1, sizeof(struct radeon_ws_bo));

	ws_bo->bo = radeon_bo(radeon, 0, size, alignment, NULL);
	if (!ws_bo->bo) {
		free(ws_bo);
		return NULL;
	}

	pipe_reference_init(&ws_bo->reference, 1);
	return ws_bo;
}

struct radeon_ws_bo *radeon_ws_bo_handle(struct radeon *radeon,
					 unsigned handle)
{
	struct radeon_ws_bo *ws_bo = calloc(1, sizeof(struct radeon_ws_bo));

	ws_bo->bo = radeon_bo(radeon, handle, 0, 0, NULL);
	if (!ws_bo->bo) {
		free(ws_bo);
		return NULL;
	}
	pipe_reference_init(&ws_bo->reference, 1);
	return ws_bo;
}

void *radeon_ws_bo_map(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	radeon_bo_map(radeon, bo->bo);
	return bo->bo->data;
}

void radeon_ws_bo_unmap(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	radeon_bo_unmap(radeon, bo->bo);
}

static void radeon_ws_bo_destroy(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	radeon_bo_reference(radeon, &bo->bo, NULL);
	free(bo);
}

void radeon_ws_bo_reference(struct radeon *radeon, struct radeon_ws_bo **dst,
			    struct radeon_ws_bo *src)
{
	struct radeon_ws_bo *old = *dst;
	if (pipe_reference(&(*dst)->reference, &src->reference)) {
		radeon_ws_bo_destroy(radeon, old);
	}
	*dst = src;
}

int radeon_ws_bo_wait(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	return radeon_bo_wait(radeon, bo->bo);
}
