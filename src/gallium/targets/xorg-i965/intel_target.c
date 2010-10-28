
#include "target-helpers/inline_wrapper_sw_helper.h"
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "i965/drm/i965_drm_public.h"
#include "i965/brw_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct brw_winsys_screen *bws;
   struct pipe_screen *screen;

   bws = i965_drm_winsys_screen_create(fd);
   if (!bws)
      return NULL;

   screen = brw_screen_create(bws);
   if (!screen)
      return NULL;

   screen = sw_screen_wrap(screen);

   screen = debug_screen_wrap(screen);

   return screen;
}

DRM_DRIVER_DESCRIPTOR("i915", "i965", create_screen)
