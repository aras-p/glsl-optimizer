
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#include "nvfx_resource.h"
#include "nvfx_screen.h"

void nvfx_buffer_destroy(struct pipe_screen *pscreen,
				struct pipe_resource *presource)
{
	struct nvfx_buffer *buffer = nvfx_buffer(presource);

	if(!(buffer->base.base.flags & NVFX_RESOURCE_FLAG_USER))
		align_free(buffer->data);
	nouveau_screen_bo_release(pscreen, buffer->base.bo);
	FREE(buffer);
}

struct pipe_resource *
nvfx_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template)
{
	struct nvfx_screen* screen = nvfx_screen(pscreen);
	struct nvfx_buffer* buffer;

	buffer = CALLOC_STRUCT(nvfx_buffer);
	if (!buffer)
		return NULL;

	buffer->base.base = *template;
	buffer->base.base.flags |= NOUVEAU_RESOURCE_FLAG_LINEAR;
	pipe_reference_init(&buffer->base.base.reference, 1);
	buffer->base.base.screen = pscreen;
	buffer->size = util_format_get_stride(template->format, template->width0);
	buffer->bytes_to_draw_until_static = buffer->size * screen->static_reuse_threshold;
	buffer->data = align_malloc(buffer->size, 16);

	return &buffer->base.base;
}


struct pipe_resource *
nvfx_user_buffer_create(struct pipe_screen *pscreen,
			void *ptr,
			unsigned bytes,
			unsigned usage)
{
	struct nvfx_screen* screen = nvfx_screen(pscreen);
	struct nvfx_buffer* buffer;

	buffer = CALLOC_STRUCT(nvfx_buffer);
	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.base.reference, 1);
	buffer->base.base.flags =
		NOUVEAU_RESOURCE_FLAG_LINEAR | NVFX_RESOURCE_FLAG_USER;
	buffer->base.base.screen = pscreen;
	buffer->base.base.format = PIPE_FORMAT_R8_UNORM;
	buffer->base.base.usage = PIPE_USAGE_IMMUTABLE;
	buffer->base.base.bind = usage;
	buffer->base.base.width0 = bytes;
	buffer->base.base.height0 = 1;
	buffer->base.base.depth0 = 1;
	buffer->base.base.array_size = 1;
	buffer->data = ptr;
	buffer->size = bytes;
	buffer->bytes_to_draw_until_static = bytes * screen->static_reuse_threshold;
	buffer->dirty_end = bytes;

	return &buffer->base.base;
}

void nvfx_buffer_upload(struct nvfx_buffer* buffer)
{
	unsigned dirty = buffer->dirty_end - buffer->dirty_begin;
	if(!buffer->base.bo)
	{
		buffer->base.bo = nouveau_screen_bo_new(buffer->base.base.screen,
					   16,
					   buffer->base.base.usage,
					   buffer->base.base.bind,
					   buffer->base.base.width0);
	}

	if(dirty)
	{
		// TODO: may want to use a temporary in some cases
		nouveau_bo_map(buffer->base.bo, NOUVEAU_BO_WR
				| (buffer->dirty_unsynchronized ? NOUVEAU_BO_NOSYNC : 0));
		memcpy((uint8_t*)buffer->base.bo->map + buffer->dirty_begin, buffer->data + buffer->dirty_begin, dirty);
		nouveau_bo_unmap(buffer->base.bo);
		buffer->dirty_begin = buffer->dirty_end = 0;
	}
}
