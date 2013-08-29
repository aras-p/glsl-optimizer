
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "freedreno/drm/freedreno_drm_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = fd_drm_screen_create(fd);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

DRM_DRIVER_DESCRIPTOR("freedreno", "msm", create_screen, NULL)
