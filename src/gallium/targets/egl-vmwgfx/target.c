
#include "state_tracker/drm_driver.h"
#include "svga/drm/svga_drm_public.h"
#include "svga/svga_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct svga_winsys_screen *sws;
   sws = svga_drm_winsys_screen_create(fd);
   if (!sws)
      return NULL;

   return svga_screen_create(sws);
}

DRM_DRIVER_DESCRIPTOR("vmwgfx", "vmwgfx", create_screen)

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
