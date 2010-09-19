#include <malloc.h>
#include <pipe/p_screen.h>
#include <pipebuffer/pb_bufmgr.h>
#include "radeon_priv.h"

struct radeon_ws_bo *radeon_ws_bo(struct radeon *radeon,
				  unsigned size, unsigned alignment, unsigned usage)
{
	struct radeon_ws_bo *ws_bo = calloc(1, sizeof(struct radeon_ws_bo));
	struct pb_desc desc;
	struct pb_manager *man;

	desc.alignment = alignment;
	desc.usage = usage;

	if (!radeon->use_mem_constant && (usage & PIPE_BIND_CONSTANT_BUFFER)) {
		man = radeon->mman;
	} else if (usage & (PIPE_BIND_CONSTANT_BUFFER | PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
		man = radeon->cman;
	else
		man = radeon->kman;

	ws_bo->pb = man->create_buffer(man, size, &desc);
	if (ws_bo->pb == NULL) {
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

	ws_bo->pb = radeon_bo_pb_create_buffer_from_handle(radeon->kman, handle);
	if (!ws_bo->pb) {
		free(ws_bo);
		return NULL;
	}
	pipe_reference_init(&ws_bo->reference, 1);
	return ws_bo;
}

void *radeon_ws_bo_map(struct radeon *radeon, struct radeon_ws_bo *bo, unsigned usage, void *ctx)
{
	return pb_map(bo->pb, usage, ctx);
}

void radeon_ws_bo_unmap(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	pb_unmap(bo->pb);
}

static void radeon_ws_bo_destroy(struct radeon *radeon, struct radeon_ws_bo *bo)
{
	if (bo->pb)
		pb_reference(&bo->pb, NULL);
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

unsigned radeon_ws_bo_get_handle(struct radeon_ws_bo *pb_bo)
{
	struct radeon_bo *bo;

	bo = radeon_bo_pb_get_bo(pb_bo->pb);
	if (!bo)
		return 0;

	return bo->handle;
}

unsigned radeon_ws_bo_get_size(struct radeon_ws_bo *pb_bo)
{
	struct radeon_bo *bo;

	bo = radeon_bo_pb_get_bo(pb_bo->pb);
	if (!bo)
		return 0;

	return bo->size;
}
