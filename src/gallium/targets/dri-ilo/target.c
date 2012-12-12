#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "intel/drm/intel_drm_public.h"
#include "intel/drm/intel_winsys.h"
#include "ilo/ilo_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct intel_winsys *iws;
   struct pipe_screen *screen;

   iws = intel_drm_winsys_create(fd);
   if (!iws)
      return NULL;

   screen = ilo_screen_create(iws);
   if (!screen) {
      iws->destroy(iws);
      return NULL;
   }

   screen = debug_screen_wrap(screen);

   return screen;
}

DRM_DRIVER_DESCRIPTOR("i965", "i915", create_screen, NULL)
