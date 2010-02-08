#include <stdio.h>

#include "state_tracker/drm_api.h"

#include "intel_drm_winsys.h"
#include "util/u_memory.h"

#include "i915/i915_context.h"
#include "i915/i915_screen.h"

#include "trace/tr_drm.h"

/*
 * Helper functions
 */


static void
intel_drm_get_device_id(unsigned int *device_id)
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

static struct intel_buffer *
intel_drm_buffer_from_handle(struct intel_drm_winsys *idws,
                             const char* name, unsigned handle)
{
   struct intel_drm_buffer *buf = CALLOC_STRUCT(intel_drm_buffer);
   uint32_t tile = 0, swizzle = 0;

   if (!buf)
      return NULL;

   buf->magic = 0xDEAD1337;
   buf->bo = drm_intel_bo_gem_create_from_name(idws->pools.gem, name, handle);
   buf->flinked = TRUE;
   buf->flink = handle;

   if (!buf->bo)
      goto err;

   drm_intel_bo_get_tiling(buf->bo, &tile, &swizzle);
   if (tile != INTEL_TILE_NONE)
      buf->map_gtt = TRUE;

   return (struct intel_buffer *)buf;

err:
   FREE(buf);
   return NULL;
}


/*
 * Exported functions
 */


static struct pipe_texture *
intel_drm_texture_from_shared_handle(struct drm_api *api,
                                     struct pipe_screen *screen,
                                     struct pipe_texture *templ,
                                     const char* name,
                                     unsigned pitch,
                                     unsigned handle)
{
   struct intel_drm_winsys *idws = intel_drm_winsys(i915_screen(screen)->iws);
   struct intel_buffer *buffer;

   buffer = intel_drm_buffer_from_handle(idws, name, handle);
   if (!buffer)
      return NULL;

   return i915_texture_blanket_intel(screen, templ, pitch, buffer);
}

static boolean
intel_drm_shared_handle_from_texture(struct drm_api *api,
                                     struct pipe_screen *screen,
                                     struct pipe_texture *texture,
                                     unsigned *pitch,
                                     unsigned *handle)
{
   struct intel_drm_buffer *buf = NULL;
   struct intel_buffer *buffer = NULL;
   if (!i915_get_texture_buffer_intel(texture, &buffer, pitch))
      return FALSE;

   buf = intel_drm_buffer(buffer);
   if (!buf->flinked) {
      if (drm_intel_bo_flink(buf->bo, &buf->flink))
         return FALSE;
      buf->flinked = TRUE;
   }

   *handle = buf->flink;

   return TRUE;
}

static boolean
intel_drm_local_handle_from_texture(struct drm_api *api,
                                    struct pipe_screen *screen,
                                    struct pipe_texture *texture,
                                    unsigned *pitch,
                                    unsigned *handle)
{
   struct intel_buffer *buffer = NULL;
   if (!i915_get_texture_buffer_intel(texture, &buffer, pitch))
      return FALSE;

   *handle = intel_drm_buffer(buffer)->bo->handle;

   return TRUE;
}

static void
intel_drm_winsys_destroy(struct intel_winsys *iws)
{
   struct intel_drm_winsys *idws = intel_drm_winsys(iws);

   drm_intel_bufmgr_destroy(idws->pools.gem);

   FREE(idws);
}

static struct pipe_screen *
intel_drm_create_screen(struct drm_api *api, int drmFD,
                        struct drm_create_screen_arg *arg)
{
   struct intel_drm_winsys *idws;
   unsigned int deviceID;

   if (arg != NULL) {
      switch(arg->mode) {
      case DRM_CREATE_NORMAL:
         break;
      default:
         return NULL;
      }
   }

   idws = CALLOC_STRUCT(intel_drm_winsys);
   if (!idws)
      return NULL;

   intel_drm_get_device_id(&deviceID);

   intel_drm_winsys_init_batchbuffer_functions(idws);
   intel_drm_winsys_init_buffer_functions(idws);
   intel_drm_winsys_init_fence_functions(idws);

   idws->fd = drmFD;
   idws->id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = intel_drm_winsys_destroy;

   idws->pools.gem = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->pools.gem);

   idws->dump_cmd = debug_get_bool_option("INTEL_DUMP_CMD", FALSE);

   return i915_create_screen(&idws->base, deviceID);
}

static void
destroy(struct drm_api *api)
{

}

struct drm_api intel_drm_api =
{
   .name = "i915",
   .driver_name = "i915",
   .create_screen = intel_drm_create_screen,
   .texture_from_shared_handle = intel_drm_texture_from_shared_handle,
   .shared_handle_from_texture = intel_drm_shared_handle_from_texture,
   .local_handle_from_texture = intel_drm_local_handle_from_texture,
   .destroy = destroy,
};

struct drm_api *
drm_api_create()
{
   return trace_drm_create(&intel_drm_api);
}
