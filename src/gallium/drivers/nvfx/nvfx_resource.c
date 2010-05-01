
#include "pipe/p_context.h"
#include "nvfx_resource.h"
#include "nouveau/nouveau_screen.h"


/* This doesn't look quite right - this query is supposed to ask
 * whether the particular context has references to the resource in
 * any unflushed rendering command buffer, and hence requires a
 * pipe->flush() for serializing some modification to that resource.
 *
 * This seems to be answering the question of whether the resource is
 * currently on hardware.
 */
static unsigned int
nvfx_resource_is_referenced(struct pipe_context *pipe,
			    struct pipe_resource *resource,
			    unsigned face, unsigned level)
{
	return nouveau_reference_flags(nvfx_resource(resource)->bo);
}

static struct pipe_resource *
nvfx_resource_create(struct pipe_screen *screen,
		     const struct pipe_resource *template)
{
	if (template->target == PIPE_BUFFER)
		return nvfx_buffer_create(screen, template);
	else
		return nvfx_miptree_create(screen, template);
}

static struct pipe_resource *
nvfx_resource_from_handle(struct pipe_screen * screen,
			  const struct pipe_resource *template,
			  struct winsys_handle *whandle)
{
	if (template->target == PIPE_BUFFER)
		return NULL;
	else
		return nvfx_miptree_from_handle(screen, template, whandle);
}

void
nvfx_init_resource_functions(struct pipe_context *pipe)
{
	pipe->get_transfer = u_get_transfer_vtbl;
	pipe->transfer_map = u_transfer_map_vtbl;
	pipe->transfer_flush_region = u_transfer_flush_region_vtbl;
	pipe->transfer_unmap = u_transfer_unmap_vtbl;
	pipe->transfer_destroy = u_transfer_destroy_vtbl;
	pipe->transfer_inline_write = u_transfer_inline_write_vtbl;
	pipe->is_resource_referenced = nvfx_resource_is_referenced;
}

void
nvfx_screen_init_resource_functions(struct pipe_screen *pscreen)
{
	pscreen->resource_create = nvfx_resource_create;
	pscreen->resource_from_handle = nvfx_resource_from_handle;
	pscreen->resource_get_handle = u_resource_get_handle_vtbl;
	pscreen->resource_destroy = u_resource_destroy_vtbl;
	pscreen->user_buffer_create = nvfx_user_buffer_create;
   
	pscreen->get_tex_surface = nvfx_miptree_surface_new;
	pscreen->tex_surface_destroy = nvfx_miptree_surface_del;
}
