
#include "pipe/p_context.h"
#include "nv50_resource.h"
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
nv50_resource_is_referenced(struct pipe_context *pipe,
			    struct pipe_resource *resource,
			    unsigned level, int layer)
{
	return nouveau_reference_flags(nv50_resource(resource)->bo);
}

static struct pipe_resource *
nv50_resource_create(struct pipe_screen *screen,
		     const struct pipe_resource *template)
{
	if (template->target == PIPE_BUFFER)
		return nv50_buffer_create(screen, template);
	else
		return nv50_miptree_create(screen, template);
}

static struct pipe_resource *
nv50_resource_from_handle(struct pipe_screen * screen,
			  const struct pipe_resource *template,
			  struct winsys_handle *whandle)
{
	if (template->target == PIPE_BUFFER)
		return NULL;
	else
		return nv50_miptree_from_handle(screen, template, whandle);
}

void
nv50_init_resource_functions(struct pipe_context *pcontext)
{
	pcontext->get_transfer = u_get_transfer_vtbl;
	pcontext->transfer_map = u_transfer_map_vtbl;
	pcontext->transfer_flush_region = u_transfer_flush_region_vtbl;
	pcontext->transfer_unmap = u_transfer_unmap_vtbl;
	pcontext->transfer_destroy = u_transfer_destroy_vtbl;
	pcontext->transfer_inline_write = u_transfer_inline_write_vtbl;
	pcontext->is_resource_referenced = nv50_resource_is_referenced;

	pcontext->create_surface = nv50_miptree_surface_new;
	pcontext->surface_destroy = nv50_miptree_surface_del;
}

void
nv50_screen_init_resource_functions(struct pipe_screen *pscreen)
{
	pscreen->resource_create = nv50_resource_create;
	pscreen->resource_from_handle = nv50_resource_from_handle;
	pscreen->resource_get_handle = u_resource_get_handle_vtbl;
	pscreen->resource_destroy = u_resource_destroy_vtbl;
	pscreen->user_buffer_create = nv50_user_buffer_create;
}
