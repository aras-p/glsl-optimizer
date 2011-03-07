
#include "pipe/p_context.h"
#include "util/u_staging.h"
#include "nvfx_resource.h"
#include "nouveau/nouveau_screen.h"


static struct pipe_resource *
nvfx_resource_create(struct pipe_screen *screen,
		     const struct pipe_resource *template)
{
	if (template->target == PIPE_BUFFER)
		return nvfx_buffer_create(screen, template);
	else
		return nvfx_miptree_create(screen, template);
}

static void
nvfx_resource_destroy(struct pipe_screen *screen, struct pipe_resource *pr)
{
	if (pr->target == PIPE_BUFFER)
		return nvfx_buffer_destroy(screen, pr);
	else
		return nvfx_miptree_destroy(screen, pr);
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

static boolean
nvfx_resource_get_handle(struct pipe_screen *pscreen,
                        struct pipe_resource *pr,
                        struct winsys_handle *whandle)
{
	struct nvfx_resource* res = (struct nvfx_resource*)pr;

	if (!res || !res->bo)
		return FALSE;

	return nouveau_screen_bo_get_handle(pscreen, res->bo, nvfx_subresource_pitch(pr, 0), whandle);
}

void
nvfx_init_resource_functions(struct pipe_context *pipe)
{
	pipe->create_surface = nvfx_miptree_surface_new;
	pipe->surface_destroy = nvfx_miptree_surface_del;
}

void
nvfx_screen_init_resource_functions(struct pipe_screen *pscreen)
{
	pscreen->resource_create = nvfx_resource_create;
	pscreen->resource_from_handle = nvfx_resource_from_handle;
	pscreen->resource_get_handle = nvfx_resource_get_handle;
	pscreen->resource_destroy = nvfx_resource_destroy;
	pscreen->user_buffer_create = nvfx_user_buffer_create;
}
