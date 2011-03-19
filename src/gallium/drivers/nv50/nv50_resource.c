
#include "pipe/p_context.h"
#include "nv50_resource.h"
#include "nouveau/nouveau_screen.h"


static struct pipe_resource *
nv50_resource_create(struct pipe_screen *screen,
                     const struct pipe_resource *templ)
{
   switch (templ->target) {
   case PIPE_BUFFER:
      return nouveau_buffer_create(screen, templ);
   default:
      return nv50_miptree_create(screen, templ);
   }
}

static struct pipe_resource *
nv50_resource_from_handle(struct pipe_screen * screen,
                          const struct pipe_resource *templ,
                          struct winsys_handle *whandle)
{
   if (templ->target == PIPE_BUFFER)
      return NULL;
   else
      return nv50_miptree_from_handle(screen, templ, whandle);
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
   pscreen->user_buffer_create = nouveau_user_buffer_create;
}
