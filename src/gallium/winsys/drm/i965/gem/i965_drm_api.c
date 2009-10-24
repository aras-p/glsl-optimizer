
#include "state_tracker/drm_api.h"

#include "i965_drm_winsys.h"
#include "util/u_memory.h"

#include "brw/brw_context.h"	/* XXX: shouldn't be doing this */
#include "brw/brw_screen.h"	/* XXX: shouldn't be doing this */

#include "trace/tr_drm.h"

/*
 * Helper functions
 */


static void
i965_drm_get_device_id(unsigned int *device_id)
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

static struct i965_buffer *
i965_drm_buffer_from_handle(struct i965_drm_winsys *idws,
                             const char* name, unsigned handle)
{
   struct i965_drm_buffer *buf = CALLOC_STRUCT(i965_drm_buffer);
   uint32_t tile = 0, swizzle = 0;

   if (!buf)
      return NULL;

   buf->magic = 0xDEAD1337;
   buf->bo = drm_i965_bo_gem_create_from_name(idws->pools.gem, name, handle);
   buf->flinked = TRUE;
   buf->flink = handle;

   if (!buf->bo)
      goto err;

   drm_i965_bo_get_tiling(buf->bo, &tile, &swizzle);
   if (tile != I965_TILE_NONE)
      buf->map_gtt = TRUE;

   return (struct i965_buffer *)buf;

err:
   FREE(buf);
   return NULL;
}


/*
 * Exported functions
 */


static struct pipe_texture *
i965_drm_texture_from_shared_handle(struct drm_api *api,
                                     struct pipe_screen *screen,
                                     struct pipe_texture *templ,
                                     const char* name,
                                     unsigned pitch,
                                     unsigned handle)
{
   struct i965_drm_winsys *idws = i965_drm_winsys(i965_screen(screen)->iws);
   struct i965_buffer *buffer;

   buffer = i965_drm_buffer_from_handle(idws, name, handle);
   if (!buffer)
      return NULL;

   return i965_texture_blanket_i965(screen, templ, pitch, buffer);
}

static boolean
i965_drm_shared_handle_from_texture(struct drm_api *api,
                                     struct pipe_screen *screen,
                                     struct pipe_texture *texture,
                                     unsigned *pitch,
                                     unsigned *handle)
{
   struct i965_drm_buffer *buf = NULL;
   struct i965_buffer *buffer = NULL;
   if (!i965_get_texture_buffer_i965(texture, &buffer, pitch))
      return FALSE;

   buf = i965_drm_buffer(buffer);
   if (!buf->flinked) {
      if (drm_i965_bo_flink(buf->bo, &buf->flink))
         return FALSE;
      buf->flinked = TRUE;
   }

   *handle = buf->flink;

   return TRUE;
}

static boolean
i965_drm_local_handle_from_texture(struct drm_api *api,
                                    struct pipe_screen *screen,
                                    struct pipe_texture *texture,
                                    unsigned *pitch,
                                    unsigned *handle)
{
   struct i965_buffer *buffer = NULL;
   if (!i965_get_texture_buffer_i965(texture, &buffer, pitch))
      return FALSE;

   *handle = i965_drm_buffer(buffer)->bo->handle;

   return TRUE;
}

static void
i965_drm_winsys_destroy(struct i965_winsys *iws)
{
   struct i965_drm_winsys *idws = i965_drm_winsys(iws);

   drm_i965_bufmgr_destroy(idws->pools.gem);

   FREE(idws);
}

static struct pipe_screen *
i965_drm_create_screen(struct drm_api *api, int drmFD,
		      struct drm_create_screen_arg *arg)
{
   struct i965_drm_winsys *idws;
   unsigned int deviceID;

   if (arg != NULL) {
      switch(arg->mode) {
      case DRM_CREATE_NORMAL:
         break;
      default:
         return NULL;
      }
   }

   idws = CALLOC_STRUCT(i965_drm_winsys);
   if (!idws)
      return NULL;

   i965_drm_get_device_id(&deviceID);

   i965_drm_winsys_init_batchbuffer_functions(idws);
   i965_drm_winsys_init_buffer_functions(idws);
   i965_drm_winsys_init_fence_functions(idws);

   idws->fd = drmFD;
   idws->id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = i965_drm_winsys_destroy;

   idws->pools.gem = drm_i965_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_i965_bufmgr_gem_enable_reuse(idws->pools.gem);

   idws->softpipe = FALSE;
   idws->dump_cmd = debug_get_bool_option("I965_DUMP_CMD", FALSE);

   return i965_create_screen(&idws->base, deviceID);
}

static struct pipe_context *
i965_drm_create_context(struct drm_api *api, struct pipe_screen *screen)
{
   return i965_create_context(screen);
}

static void
destroy(struct drm_api *api)
{

}

struct drm_api i965_drm_api =
{
   .create_context = i965_drm_create_context,
   .create_screen = i965_drm_create_screen,
   .texture_from_shared_handle = i965_drm_texture_from_shared_handle,
   .shared_handle_from_texture = i965_drm_shared_handle_from_texture,
   .local_handle_from_texture = i965_drm_local_handle_from_texture,
   .destroy = destroy,
};

struct drm_api *
drm_api_create()
{
   return trace_drm_create(&i965_drm_api);
}
