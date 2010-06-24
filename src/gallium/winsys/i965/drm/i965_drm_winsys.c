
#include <stdio.h>
#include "state_tracker/drm_driver.h"

#include "i965_drm_winsys.h"
#include "i965_drm_public.h"
#include "util/u_memory.h"

/*
 * Helper functions
 */


static void
i965_libdrm_get_device_id(unsigned int *device_id)
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
   sscanf(path, "%x", device_id);
   fclose(file);
}

static void
i965_libdrm_winsys_destroy(struct brw_winsys_screen *iws)
{
   struct i965_libdrm_winsys *idws = i965_libdrm_winsys(iws);

   if (BRW_DUMP)
      debug_printf("%s\n", __FUNCTION__);

   drm_intel_bufmgr_destroy(idws->gem);

   FREE(idws);
}

struct brw_winsys_screen *
i965_drm_winsys_screen_create(int drmFD)
{
   struct i965_libdrm_winsys *idws;

   debug_printf("%s\n", __FUNCTION__);

   idws = CALLOC_STRUCT(i965_libdrm_winsys);
   if (!idws)
      return NULL;

   i965_libdrm_get_device_id(&idws->base.pci_id);

   i965_libdrm_winsys_init_buffer_functions(idws);

   idws->fd = drmFD;

   idws->base.destroy = i965_libdrm_winsys_destroy;

   idws->gem = drm_intel_bufmgr_gem_init(idws->fd, BRW_BATCH_SIZE);
   drm_intel_bufmgr_gem_enable_reuse(idws->gem);

   idws->send_cmd = !debug_get_bool_option("BRW_NO_HW", FALSE);

   return &idws->base;
}
