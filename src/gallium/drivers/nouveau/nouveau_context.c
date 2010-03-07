#include "pipe/p_defines.h"
#include "pipe/p_context.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_context.h"

#include "nouveau/nouveau_bo.h"

static unsigned int
nouveau_reference_flags(struct nouveau_bo *bo)
{
	uint32_t bo_flags;
	int flags = 0;

	bo_flags = nouveau_bo_pending(bo);
	if (bo_flags & NOUVEAU_BO_RD)
		flags |= PIPE_REFERENCED_FOR_READ;
	if (bo_flags & NOUVEAU_BO_WR)
		flags |= PIPE_REFERENCED_FOR_WRITE;

	return flags;
}

unsigned int
nouveau_is_texture_referenced(struct pipe_context *pipe,
			      struct pipe_texture *pt,
			      unsigned face, unsigned level)
{
	struct nouveau_miptree *mt = nouveau_miptree(pt);

	return nouveau_reference_flags(mt->bo);
}

unsigned int
nouveau_is_buffer_referenced(struct pipe_context *pipe, struct pipe_buffer *pb)
{
	struct nouveau_bo *bo = nouveau_bo(pb);

	return nouveau_reference_flags(bo);
}

