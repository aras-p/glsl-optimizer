
#include "target-helpers/inline_wrapper_sw_helper.h"
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "i915/drm/i915_drm_public.h"
#include "i915/i915_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct brw_winsys_screen *bws;
   struct pipe_screen *screen;

   bws = i915_drm_winsys_screen_create(fd);
   if (!bws)
      return NULL;

   screen = i915_screen_create(bws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("i915", "i915", create_screen)
