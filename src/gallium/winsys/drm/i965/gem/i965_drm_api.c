
#include <stdio.h>
#include "state_tracker/drm_api.h"

#include "i965_drm_winsys.h"
#include "util/u_memory.h"

#include "i965/brw_context.h"        /* XXX: shouldn't be doing this */
#include "i965/brw_screen.h"         /* XXX: shouldn't be doing this */

#include "trace/tr_drm.h"

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

static struct pipe_screen *
i965_libdrm_create_screen(struct drm_api *api, int drmFD,
                          struct drm_create_screen_arg *arg)
{
   struct i965_libdrm_winsys *idws;
   unsigned int deviceID;

   debug_printf("%s\n", __FUNCTION__);

   if (arg != NULL) {
      switch(arg->mode) {
      case DRM_CREATE_NORMAL:
         break;
      default:
         return NULL;
      }
   }

   idws = CALLOC_STRUCT(i965_libdrm_winsys);
   if (!idws)
      return NULL;

   i965_libdrm_get_device_id(&deviceID);

   i965_libdrm_winsys_init_buffer_functions(idws);

   idws->fd = drmFD;
   idws->id = deviceID;

   idws->base.destroy = i965_libdrm_winsys_destroy;

   idws->gem = drm_intel_bufmgr_gem_init(idws->fd, BRW_BATCH_SIZE);
   drm_intel_bufmgr_gem_enable_reuse(idws->gem);

   idws->send_cmd = !debug_get_bool_option("BRW_NO_HW", FALSE);

   return brw_create_screen(&idws->base, deviceID);
}


static void
destroy(struct drm_api *api)
{
   if (BRW_DUMP)
      debug_printf("%s\n", __FUNCTION__);

}

struct drm_api i965_libdrm_api =
{
   .name = "i965",
   .create_screen = i965_libdrm_create_screen,
   .destroy = destroy,
};

struct drm_api *
drm_api_create()
{
   return trace_drm_create(&i965_libdrm_api);
}
