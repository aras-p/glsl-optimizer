#include <stdio.h>

#include "state_tracker/drm_driver.h"

#include "i915_drm_winsys.h"
#include "i915_drm_public.h"
#include "util/u_memory.h"


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

struct i915_winsys *
i915_drm_winsys_create(int drmFD)
{
   struct i915_drm_winsys *idws;
   unsigned int deviceID;

   idws = CALLOC_STRUCT(i915_drm_winsys);
   if (!idws)
      return NULL;

   i915_drm_get_device_id(&deviceID);

   i915_drm_winsys_init_batchbuffer_functions(idws);
   i915_drm_winsys_init_buffer_functions(idws);
   i915_drm_winsys_init_fence_functions(idws);

   idws->fd = drmFD;
   idws->base.pci_id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = i915_drm_winsys_destroy;

   idws->pools.gem = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->pools.gem);

   idws->dump_cmd = debug_get_bool_option("I915_DUMP_CMD", FALSE);
   idws->send_cmd = !debug_get_bool_option("I915_NO_HW", FALSE);

   return &idws->base;
}
