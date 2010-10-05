#include "util/u_debug.h"
#include "util/u_surface.h"

#include "brw_resource.h"
#include "brw_context.h"
#include "brw_screen.h"


static struct pipe_resource *
brw_resource_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   if (template->target == PIPE_BUFFER)
      return brw_buffer_create(screen, template);
   else
      return brw_texture_create(screen, template);

}

static struct pipe_resource *
brw_resource_from_handle(struct pipe_screen * screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
   if (template->target == PIPE_BUFFER)
      return NULL;
   else
      return brw_texture_from_handle(screen, template, whandle);
}


void
brw_init_resource_functions(struct brw_context *brw )
{
   brw->base.get_transfer = u_get_transfer_vtbl;
   brw->base.transfer_map = u_transfer_map_vtbl;
   brw->base.transfer_flush_region = u_transfer_flush_region_vtbl;
   brw->base.transfer_unmap = u_transfer_unmap_vtbl;
   brw->base.transfer_destroy = u_transfer_destroy_vtbl;
   brw->base.transfer_inline_write = u_transfer_inline_write_vtbl;
   brw->base.resource_copy_region = util_resource_copy_region;
}

void
brw_init_screen_resource_functions(struct brw_screen *is)
{
   is->base.resource_create = brw_resource_create;
   is->base.resource_from_handle = brw_resource_from_handle;
   is->base.resource_get_handle = u_resource_get_handle_vtbl;
   is->base.resource_destroy = u_resource_destroy_vtbl;
   is->base.user_buffer_create = brw_user_buffer_create;
}
