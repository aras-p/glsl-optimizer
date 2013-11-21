#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "intel/intel_winsys.h"
#include "ilo/ilo_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct intel_winsys *iws;
   struct pipe_screen *screen;

   iws = intel_winsys_create_for_fd(fd);
   if (!iws)
      return NULL;

   screen = ilo_screen_create(iws);
   if (!screen) {
      intel_winsys_destroy(iws);
      return NULL;
   }

   screen = debug_screen_wrap(screen);

   return screen;
}


static const struct drm_conf_ret share_fd_ret = {
   .type = DRM_CONF_BOOL,
   .val.val_int = true,
};

static const struct drm_conf_ret *drm_configuration(enum drm_conf conf)
{
   switch (conf) {
   case DRM_CONF_SHARE_FD:
      return &share_fd_ret;
   default:
      break;
   }
   return NULL;
}

DRM_DRIVER_DESCRIPTOR("i965", "i915", create_screen, drm_configuration)
