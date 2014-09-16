
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

static const struct drm_conf_ret throttle_ret = {
   .type = DRM_CONF_INT,
   .val.val_int = 2,
};

static const struct drm_conf_ret share_fd_ret = {
   .type = DRM_CONF_BOOL,
   .val.val_int = true,
};

static const struct drm_conf_ret *drm_configuration(enum drm_conf conf)
{
   switch (conf) {
   case DRM_CONF_THROTTLE:
      return &throttle_ret;
   case DRM_CONF_SHARE_FD:
      return &share_fd_ret;
   default:
      break;
   }
   return NULL;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("msm", "freedreno", create_screen, drm_configuration)
