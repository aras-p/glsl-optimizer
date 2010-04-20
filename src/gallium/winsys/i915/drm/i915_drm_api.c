#include <stdio.h>

#include "state_tracker/drm_api.h"

#include "i915_drm_winsys.h"
#include "util/u_memory.h"

#include "i915/i915_context.h"
#include "i915/i915_screen.h"

#include "trace/tr_drm.h"

/*
 * Helper functions
 */


static void
i915_drm_get_device_id(unsigned int *device_id)
{
   char path[512];
   FILE *file;
   void *shutup_gcc;

   /*
    * FIXME: Fix this up to use a drm ioctl or whatever.
    */

   snprintf(path, sizeof(path), "/sys/class/drm/card0/device/device");
   file = fopen(path, "r");
   if (!file) {
      return;
   }

   shutup_gcc = fgets(path, sizeof(path), file);
   (void) shutup_gcc;
   sscanf(path, "%x", device_id);
   fclose(file);
}

static void
i915_drm_winsys_destroy(struct i915_winsys *iws)
{
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);

   drm_intel_bufmgr_destroy(idws->pools.gem);

   FREE(idws);
}

static struct pipe_screen *
i915_drm_create_screen(struct drm_api *api, int drmFD,
                       struct drm_create_screen_arg *arg)
{
   struct i915_drm_winsys *idws;
   unsigned int deviceID;

   if (arg != NULL) {
      switch(arg->mode) {
      case DRM_CREATE_NORMAL:
         break;
      default:
         return NULL;
      }
   }

   idws = CALLOC_STRUCT(i915_drm_winsys);
   if (!idws)
      return NULL;

   i915_drm_get_device_id(&deviceID);

   i915_drm_winsys_init_batchbuffer_functions(idws);
   i915_drm_winsys_init_buffer_functions(idws);
   i915_drm_winsys_init_fence_functions(idws);

   idws->fd = drmFD;
   idws->id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = i915_drm_winsys_destroy;

   idws->pools.gem = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->pools.gem);

   idws->dump_cmd = debug_get_bool_option("INTEL_DUMP_CMD", FALSE);

   return i915_create_screen(&idws->base, deviceID);
}

static struct drm_api i915_drm_api =
{
   .name = "i915",
   .driver_name = "i915",
   .create_screen = i915_drm_create_screen,
   .destroy = NULL,
};

struct drm_api *
drm_api_create()
{
   return trace_drm_create(&i915_drm_api);
}
