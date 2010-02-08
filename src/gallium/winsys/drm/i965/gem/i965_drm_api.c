
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

static struct i965_libdrm_buffer *
i965_libdrm_buffer_from_handle(struct i965_libdrm_winsys *idws,
                               const char* name, unsigned handle)
{
   struct i965_libdrm_buffer *buf = CALLOC_STRUCT(i965_libdrm_buffer);
   uint32_t swizzle = 0;

   if (BRW_DUMP)
      debug_printf("%s\n", __FUNCTION__);

   if (!buf)
      return NULL;
   pipe_reference_init(&buf->base.reference, 1);
   buf->bo = drm_intel_bo_gem_create_from_name(idws->gem, name, handle);
   buf->base.size = buf->bo->size;
   buf->base.sws = &idws->base;
   buf->flinked = TRUE;
   buf->flink = handle;


   if (!buf->bo)
      goto err;

   drm_intel_bo_get_tiling(buf->bo, &buf->tiling, &swizzle);
   if (buf->tiling != 0)
      buf->map_gtt = TRUE;

   return buf;

err:
   FREE(buf);
   return NULL;
}


/*
 * Exported functions
 */


static struct pipe_texture *
i965_libdrm_texture_from_shared_handle(struct drm_api *api,
                                       struct pipe_screen *screen,
                                       struct pipe_texture *template,
                                       const char* name,
                                       unsigned pitch,
                                       unsigned handle)
{
   /* XXX: this is silly -- there should be a way to get directly from
    * the "drm_api" struct to ourselves, without peering into
    * unrelated code:
    */
   struct i965_libdrm_winsys *idws = i965_libdrm_winsys(brw_screen(screen)->sws);
   struct i965_libdrm_buffer *buffer;

   if (BRW_DUMP)
      debug_printf("%s %s pitch %d handle 0x%x\n", __FUNCTION__,
		   name, pitch, handle);

   buffer = i965_libdrm_buffer_from_handle(idws, name, handle);
   if (!buffer)
      return NULL;

   return brw_texture_blanket_winsys_buffer(screen, template, pitch,
					    buffer->tiling,
					    &buffer->base);
}


static boolean
i965_libdrm_shared_handle_from_texture(struct drm_api *api,
                                       struct pipe_screen *screen,
                                       struct pipe_texture *texture,
                                       unsigned *pitch,
                                       unsigned *handle)
{
   struct i965_libdrm_buffer *buf = NULL;
   struct brw_winsys_buffer *buffer = NULL;

   if (BRW_DUMP)
      debug_printf("%s\n", __FUNCTION__);

   if (!brw_texture_get_winsys_buffer(texture, &buffer, pitch))
      return FALSE;

   buf = i965_libdrm_buffer(buffer);
   if (!buf->flinked) {
      if (drm_intel_bo_flink(buf->bo, &buf->flink))
         return FALSE;
      buf->flinked = TRUE;
   }

   *handle = buf->flink;

   if (BRW_DUMP)
      debug_printf("   -> pitch %d handle 0x%x\n", *pitch, *handle);

   return TRUE;
}

static boolean
i965_libdrm_local_handle_from_texture(struct drm_api *api,
                                      struct pipe_screen *screen,
                                      struct pipe_texture *texture,
                                      unsigned *pitch,
                                      unsigned *handle)
{
   struct brw_winsys_buffer *buffer = NULL;

   if (BRW_DUMP)
      debug_printf("%s\n", __FUNCTION__);

   if (!brw_texture_get_winsys_buffer(texture, &buffer, pitch))
      return FALSE;

   *handle = i965_libdrm_buffer(buffer)->bo->handle;

   if (BRW_DUMP)
      debug_printf("   -> pitch %d handle 0x%x\n", *pitch, *handle);

   return TRUE;
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
   .texture_from_shared_handle = i965_libdrm_texture_from_shared_handle,
   .shared_handle_from_texture = i965_libdrm_shared_handle_from_texture,
   .local_handle_from_texture = i965_libdrm_local_handle_from_texture,
   .destroy = destroy,
};

struct drm_api *
drm_api_create()
{
   return trace_drm_create(&i965_libdrm_api);
}
