#include "util/u_debug.h"

#include "svga_resource.h"
#include "svga_resource_buffer.h"
#include "svga_resource_texture.h"
#include "svga_context.h"
#include "svga_screen.h"


static struct pipe_resource *
svga_resource_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   if (template->target == PIPE_BUFFER)
      return svga_buffer_create(screen, template);
   else
      return svga_texture_create(screen, template);

}

static struct pipe_resource *
svga_resource_from_handle(struct pipe_screen * screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
   if (template->target == PIPE_BUFFER)
      return NULL;
   else
      return svga_texture_from_handle(screen, template, whandle);
}


void
svga_init_resource_functions(struct svga_context *svga)
{
   svga->pipe.get_transfer = u_get_transfer_vtbl;
   svga->pipe.transfer_map = u_transfer_map_vtbl;
   svga->pipe.transfer_flush_region = u_transfer_flush_region_vtbl;
   svga->pipe.transfer_unmap = u_transfer_unmap_vtbl;
   svga->pipe.transfer_destroy = u_transfer_destroy_vtbl;
   svga->pipe.transfer_inline_write = u_transfer_inline_write_vtbl;
   svga->pipe.redefine_user_buffer = svga_redefine_user_buffer;
}

void
svga_init_screen_resource_functions(struct svga_screen *is)
{
   is->screen.resource_create = svga_resource_create;
   is->screen.resource_from_handle = svga_resource_from_handle;
   is->screen.resource_get_handle = u_resource_get_handle_vtbl;
   is->screen.resource_destroy = u_resource_destroy_vtbl;
   is->screen.user_buffer_create = svga_user_buffer_create;
}



