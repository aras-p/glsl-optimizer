
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#include "nvfx_resource.h"

void nvfx_buffer_destroy(struct pipe_screen *pscreen,
				struct pipe_resource *presource)
{
	struct nvfx_resource *buffer = nvfx_resource(presource);

	nouveau_screen_bo_release(pscreen, buffer->bo);
	FREE(buffer);
}

struct pipe_resource *
nvfx_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template)
{
	struct nvfx_resource *buffer;

	buffer = CALLOC_STRUCT(nvfx_resource);
	if (!buffer)
		return NULL;

	buffer->base = *template;
	buffer->base.flags |= NVFX_RESOURCE_FLAG_LINEAR;
	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.screen = pscreen;

	buffer->bo = nouveau_screen_bo_new(pscreen,
					   16,
					   buffer->base.usage,
					   buffer->base.bind,
					   buffer->base.width0);

	if (buffer->bo == NULL)
		goto fail;

	return &buffer->base;

fail:
	FREE(buffer);
	return NULL;
}


struct pipe_resource *
nvfx_user_buffer_create(struct pipe_screen *pscreen,
			void *ptr,
			unsigned bytes,
			unsigned usage)
{
	struct nvfx_resource *buffer;

	buffer = CALLOC_STRUCT(nvfx_resource);
	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.flags = NVFX_RESOURCE_FLAG_LINEAR;
	buffer->base.screen = pscreen;
	buffer->base.format = PIPE_FORMAT_R8_UNORM;
	buffer->base.usage = PIPE_USAGE_IMMUTABLE;
	buffer->base.bind = usage;
	buffer->base.width0 = bytes;
	buffer->base.height0 = 1;
	buffer->base.depth0 = 1;

	buffer->bo = nouveau_screen_bo_user(pscreen, ptr, bytes);
	if (!buffer->bo)
		goto fail;

	return &buffer->base;

fail:
	FREE(buffer);
	return NULL;
}
