
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_winsys.h"
#include "nv50_resource.h"



static void nv50_buffer_destroy(struct pipe_screen *pscreen,
				struct pipe_resource *presource)
{
	struct nv50_resource *buffer = nv50_resource(presource);

	nouveau_screen_bo_release(pscreen, buffer->bo);
	FREE(buffer);
}




/* Utility functions for transfer create/destroy are hooked in and
 * just record the arguments to those functions.
 */
static void *
nv50_buffer_transfer_map( struct pipe_context *pipe,
			  struct pipe_transfer *transfer )
{
	struct nv50_resource *buffer = nv50_resource(transfer->resource);
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



static void nv50_buffer_transfer_flush_region( struct pipe_context *pipe,
					       struct pipe_transfer *transfer,
					       const struct pipe_box *box)
{
	struct nv50_resource *buffer = nv50_resource(transfer->resource);

	nouveau_screen_bo_map_flush_range(pipe->screen,
					  buffer->bo,
					  transfer->box.x + box->x,
					  box->width);
}

static void nv50_buffer_transfer_unmap( struct pipe_context *pipe,
					struct pipe_transfer *transfer )
{
	struct nv50_resource *buffer = nv50_resource(transfer->resource);

	nouveau_screen_bo_unmap(pipe->screen, buffer->bo);
}




const struct u_resource_vtbl nv50_buffer_vtbl =
{
	u_default_resource_get_handle,      /* get_handle */
	nv50_buffer_destroy,		    /* resource_destroy */
	NULL,			            /* is_resource_referenced */
	u_default_get_transfer,		    /* get_transfer */
	u_default_transfer_destroy,	    /* transfer_destroy */
	nv50_buffer_transfer_map,	    /* transfer_map */
	nv50_buffer_transfer_flush_region,  /* transfer_flush_region */
	nv50_buffer_transfer_unmap,	    /* transfer_unmap */
	u_default_transfer_inline_write	    /* transfer_inline_write */
};




struct pipe_resource *
nv50_buffer_create(struct pipe_screen *pscreen,
		   const struct pipe_resource *template)
{
	struct nv50_resource *buffer;

	buffer = CALLOC_STRUCT(nv50_resource);
	if (!buffer)
		return NULL;

	buffer->base = *template;
	buffer->vtbl = &nv50_buffer_vtbl;
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
nv50_user_buffer_create(struct pipe_screen *pscreen,
			void *ptr,
			unsigned bytes,
			unsigned bind)
{
	struct nv50_resource *buffer;

	buffer = CALLOC_STRUCT(nv50_resource);
	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->vtbl = &nv50_buffer_vtbl;
	buffer->base.screen = pscreen;
	buffer->base.format = PIPE_FORMAT_R8_UNORM;
	buffer->base.usage = PIPE_USAGE_IMMUTABLE;
	buffer->base.bind = bind;
	buffer->base.width0 = bytes;
	buffer->base.height0 = 1;
	buffer->base.depth0 = 1;
	buffer->base.array_size = 1;

	buffer->bo = nouveau_screen_bo_user(pscreen, ptr, bytes);
	if (!buffer->bo)
		goto fail;
	
	return &buffer->base;

fail:
	FREE(buffer);
	return NULL;
}

