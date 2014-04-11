#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeon/drm/radeon_winsys.h"
#include "r300/r300_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct radeon_winsys *sws;

   sws = radeon_drm_winsys_create(fd, r300_screen_create);
   return sws ? debug_screen_wrap(sws->screen) : NULL;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("r300", "radeon", create_screen, NULL)
