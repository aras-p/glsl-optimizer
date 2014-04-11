#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeon/drm/radeon_winsys.h"
#include "radeonsi/si_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct radeon_winsys *rw;

   rw = radeon_drm_winsys_create(fd, radeonsi_screen_create);
   return rw ? debug_screen_wrap(rw->screen) : NULL;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("radeonsi", "radeon", create_screen, NULL)
