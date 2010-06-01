
#include "state_tracker/drm_driver.h"
#include "i915/drm/i915_drm_public.h"
#include "i915/i915_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct i915_winsys *iws;
   iws = i915_drm_winsys_create(fd);
   if (!iws)
      return NULL;

   return i915_screen_create(iws);
}

DRM_DRIVER_DESCRIPTOR("i915", "i915", create_screen)

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
