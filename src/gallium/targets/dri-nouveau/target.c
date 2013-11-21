
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "nouveau/drm/nouveau_drm_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = nouveau_drm_screen_create(fd);
   if (!screen)
      return NULL;

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

DRM_DRIVER_DESCRIPTOR("nouveau", "nouveau", create_screen, drm_configuration)
