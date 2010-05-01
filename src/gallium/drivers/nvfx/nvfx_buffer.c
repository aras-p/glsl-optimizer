
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#include "nvfx_resource.h"


/* Currently using separate implementations for buffers and textures,
 * even though gallium has a unified abstraction of these objects.
 * Eventually these should be combined, and mechanisms like transfers
 * be adapted to work for both buffer and texture uploads.
 */
static void nvfx_buffer_destroy(struct pipe_screen *pscreen,
				struct pipe_resource *presource)
{
	struct nvfx_resource *buffer = nvfx_resource(presource);

	nouveau_screen_bo_release(pscreen, buffer->bo);
	FREE(buffer);
}




/* Utility functions for transfer create/destroy are hooked in and
 * just record the arguments to those functions.
 */
static void *
nvfx_buffer_transfer_map( struct pipe_context *pipe,
			  struct pipe_transfer *transfer )
{
	struct nvfx_resource *buffer = nvfx_resource(transfer->resource);
	uint8_t *map;

	map = nouveau_screen_bo_map_range( pipe->screen,
					   buffer->bo,
					   transfer->box.x,
					   transfer->box.width,
					   nouveau_screen_transfer_flags(transfer->usage) );
	if (map == NULL)
		return NULL;
	
	return map + transfer->box.x;
}



static void nvfx_buffer_transfer_flush_region( struct pipe_context *pipe,
					       struct pipe_transfer *transfer,
					       const struct pipe_box *box)
{
	struct nvfx_resource *buffer = nvfx_resource(transfer->resource);

	nouveau_screen_bo_map_flush_range(pipe->screen,
					  buffer->bo,
					  transfer->box.x + box->x,
					  box->width);
}

static void nvfx_buffer_transfer_unmap( struct pipe_context *pipe,
					struct pipe_transfer *transfer )
{
	struct nvfx_resource *buffer = nvfx_resource(transfer->resource);

	nouveau_screen_bo_unmap(pipe->screen, buffer->bo);
}




struct u_resource_vtbl nvfx_buffer_vtbl = 
{
	u_default_resource_get_handle,      /* get_handle */
	nvfx_buffer_destroy,		     /* resource_destroy */
	NULL,			    /* is_resource_referenced */
	u_default_get_transfer,	     /* get_transfer */
	u_default_transfer_destroy,	     /* transfer_destroy */
	nvfx_buffer_transfer_map,	     /* transfer_map */
	nvfx_buffer_transfer_flush_region,  /* transfer_flush_region */
	nvfx_buffer_transfer_unmap,	     /* transfer_unmap */
	u_default_transfer_inline_write   /* transfer_inline_write */
};



struct pipe_resource *
nvfx_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template)
{
	struct nvfx_resource *buffer;

	buffer = CALLOC_STRUCT(nvfx_resource);
	if (!buffer)
		return NULL;

	buffer->base = *template;
	buffer->vtbl = &nvfx_buffer_vtbl;
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
	buffer->vtbl = &nvfx_buffer_vtbl;
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

