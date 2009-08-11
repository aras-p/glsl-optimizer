
#include "intel_be_device.h"

#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_math.h"

#include "intel_be_fence.h"

#include "i915simple/i915_winsys.h"
#include "softpipe/sp_winsys.h"

#include "intel_be_api.h"
#include <stdio.h>

#define I915_TILING_X 1

/*
 * Buffer
 */

static void *
intel_be_buffer_map(struct pipe_winsys *winsys,
		    struct pipe_buffer *buf,
		    unsigned flags)
{
	struct intel_be_buffer *buffer = intel_be_buffer(buf);
	drm_intel_bo *bo = intel_bo(buf);
	int write = 0;
	int ret = 0;

        if (flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
           /* Remove this when drm_intel_bo_map supports DONTBLOCK 
            */
           return NULL;
        }

	if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
		write = 1;

	if (buffer->map_count)
		goto out;

	if (buffer->map_gtt)
		ret = drm_intel_gem_bo_map_gtt(bo);
	else
		ret = drm_intel_bo_map(bo, write);

	buffer->ptr = bo->virtual;

out:
	if (ret)
		return NULL;

	buffer->map_count++;
	return buffer->ptr;
}

static void
intel_be_buffer_unmap(struct pipe_winsys *winsys,
		      struct pipe_buffer *buf)
{
	struct intel_be_buffer *buffer = intel_be_buffer(buf);

	if (--buffer->map_count)
		return;

	if (buffer->map_gtt)
		drm_intel_gem_bo_unmap_gtt(intel_bo(buf));
	else
		drm_intel_bo_unmap(intel_bo(buf));
}

static void
intel_be_buffer_destroy(struct pipe_buffer *buf)
{
	drm_intel_bo_unreference(intel_bo(buf));
	free(buf);
}

static struct pipe_buffer *
intel_be_buffer_create(struct pipe_winsys *winsys,
		       unsigned alignment,
		       unsigned usage,
		       unsigned size)
{
	struct intel_be_buffer *buffer = CALLOC_STRUCT(intel_be_buffer);
	struct intel_be_device *dev = intel_be_device(winsys);
	drm_intel_bufmgr *pool;
	char *name;

	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.alignment = alignment;
	buffer->base.usage = usage;
	buffer->base.size = size;
	buffer->flinked = FALSE;
	buffer->flink = 0;
	buffer->map_gtt = FALSE;

	if (usage & I915_BUFFER_USAGE_SCANOUT) {
		/* Scanout buffer */
		name = "gallium3d_scanout";
		pool = dev->pools.gem;
	} else if (usage & (PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_CONSTANT)) {
		/* Local buffer */
		name = "gallium3d_local";
		pool = dev->pools.gem;
	} else if (usage & PIPE_BUFFER_USAGE_CUSTOM) {
		/* For vertex buffers */
		name = "gallium3d_internal_vertex";
		pool = dev->pools.gem;
	} else {
		/* Regular buffers */
		name = "gallium3d_regular";
		pool = dev->pools.gem;
	}

	buffer->bo = drm_intel_bo_alloc(pool, name, size, alignment);
	if (usage & I915_BUFFER_USAGE_SCANOUT) {
		unsigned tiling = I915_TILING_X;
		unsigned stride = 2048 * 4; /* TODO do something smarter here */
		drm_intel_bo_set_tiling(buffer->bo, &tiling, stride);
		buffer->map_gtt = TRUE;
	}

	if (!buffer->bo)
		goto err;

	return &buffer->base;

err:
	free(buffer);
	return NULL;
}

static struct pipe_buffer *
intel_be_user_buffer_create(struct pipe_winsys *winsys, void *ptr, unsigned bytes)
{
	struct intel_be_buffer *buffer = CALLOC_STRUCT(intel_be_buffer);
	struct intel_be_device *dev = intel_be_device(winsys);
	int ret;

	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.alignment = 0;
	buffer->base.usage = 0;
	buffer->base.size = bytes;

	buffer->bo = drm_intel_bo_alloc(dev->pools.gem,
	                                "gallium3d_user_buffer",
	                                bytes, 0);

	if (!buffer->bo)
		goto err;

	ret = drm_intel_bo_subdata(buffer->bo,
	                           0, bytes, ptr);

	if (ret)
		goto err;

	return &buffer->base;

err:
	free(buffer);
	return NULL;
}

static struct pipe_buffer *
intel_be_surface_buffer_create(struct pipe_winsys *winsys,
                               unsigned width, unsigned height,
                               enum pipe_format format,
                               unsigned usage,
                               unsigned tex_usage,
                               unsigned *stride)
{
	struct pipe_format_block block;
	unsigned buf_usage = 0;
	unsigned buf_stride = 0;
	unsigned buf_size = 0;

	pf_get_block(format, &block);
	buf_stride = pf_get_stride(&block, width);
	buf_stride = align(buf_stride, 64);

	if (tex_usage & PIPE_TEXTURE_USAGE_PRIMARY) {
		/* TODO more checks */
		assert(buf_stride <= 2048*4);
		assert(height % 8 == 0);
		buf_stride = 2048 * 4;
		buf_usage |= I915_BUFFER_USAGE_SCANOUT;
	}

	buf_size = buf_stride * height;
	*stride = buf_stride;

	return intel_be_buffer_create(winsys,
	                              0,
	                              buf_usage,
	                              buf_size);
}

boolean
intel_be_get_texture_buffer(struct drm_api *api,
                            struct pipe_texture *texture,
                            struct pipe_buffer **buffer,
                            unsigned *stride)
{
	struct intel_be_device *dev;

	if (!texture)
		return FALSE;

	dev = intel_be_device(texture->screen->winsys);
	if (dev->softpipe)
		return softpipe_get_texture_buffer(texture, buffer, stride);
	else
		return i915_get_texture_buffer(texture, buffer, stride);
}

struct pipe_buffer *
intel_be_buffer_from_handle(struct drm_api *api,
                            struct pipe_screen *screen,
                            const char* name, unsigned handle)
{
	struct intel_be_device *dev = intel_be_device(screen->winsys);
	struct intel_be_buffer *buffer = CALLOC_STRUCT(intel_be_buffer);

	if (!buffer)
		return NULL;

	buffer->bo = drm_intel_bo_gem_create_from_name(dev->pools.gem, name, handle);

	if (!buffer->bo)
		goto err;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.screen = screen;
	buffer->base.alignment = buffer->bo->align;
	buffer->base.usage = PIPE_BUFFER_USAGE_GPU_READ |
	                     PIPE_BUFFER_USAGE_GPU_WRITE |
	                     PIPE_BUFFER_USAGE_CPU_READ |
	                     PIPE_BUFFER_USAGE_CPU_WRITE;
	buffer->base.size = buffer->bo->size;

	return &buffer->base;

err:
	free(buffer);
	return NULL;
}

boolean
intel_be_handle_from_buffer(struct drm_api *api,
                            struct pipe_screen *screen,
                            struct pipe_buffer *buffer,
                            unsigned *handle)
{
	if (!buffer)
		return FALSE;

	*handle = intel_bo(buffer)->handle;
	return TRUE;
}

boolean
intel_be_global_handle_from_buffer(struct drm_api *api,
                                   struct pipe_screen *screen,
				   struct pipe_buffer *buffer,
				   unsigned *handle)
{
	struct intel_be_buffer *buf = intel_be_buffer(buffer);

	if (!buffer)
		return FALSE;

	if (!buf->flinked) {
		if (drm_intel_bo_flink(intel_bo(buffer), &buf->flink))
			return FALSE;
		buf->flinked = TRUE;
	}

	*handle = buf->flink;
	return TRUE;
}

/*
 * Fence
 */

static void
intel_be_fence_refunref(struct pipe_winsys *sws,
			 struct pipe_fence_handle **ptr,
			 struct pipe_fence_handle *fence)
{
	struct intel_be_fence **p = (struct intel_be_fence **)ptr;
	struct intel_be_fence *f = (struct intel_be_fence *)fence;

        intel_be_fence_reference(p, f);
}

static int
intel_be_fence_signalled(struct pipe_winsys *sws,
			 struct pipe_fence_handle *fence,
			 unsigned flag)
{
	assert(0);

	return 0;
}

static int
intel_be_fence_finish(struct pipe_winsys *sws,
		      struct pipe_fence_handle *fence,
		      unsigned flag)
{
	struct intel_be_fence *f = (struct intel_be_fence *)fence;

	/* fence already expired */
	if (!f->bo)
		return 0;

	drm_intel_bo_wait_rendering(f->bo);
	drm_intel_bo_unreference(f->bo);
	f->bo = NULL;

	return 0;
}

/*
 * Misc functions
 */

static void
intel_be_destroy_winsys(struct pipe_winsys *winsys)
{
	struct intel_be_device *dev = intel_be_device(winsys);

	drm_intel_bufmgr_destroy(dev->pools.gem);

	free(dev);
}

boolean
intel_be_init_device(struct intel_be_device *dev, int fd, unsigned id)
{
	dev->fd = fd;
	dev->id = id;
	dev->max_batch_size = 16 * 4096;
	dev->max_vertex_size = 128 * 4096;

	dev->base.buffer_create = intel_be_buffer_create;
	dev->base.user_buffer_create = intel_be_user_buffer_create;
	dev->base.buffer_map = intel_be_buffer_map;
	dev->base.buffer_unmap = intel_be_buffer_unmap;
	dev->base.buffer_destroy = intel_be_buffer_destroy;

	/* Used by softpipe */
	dev->base.surface_buffer_create = intel_be_surface_buffer_create;

	dev->base.fence_reference = intel_be_fence_refunref;
	dev->base.fence_signalled = intel_be_fence_signalled;
	dev->base.fence_finish = intel_be_fence_finish;

	dev->base.destroy = intel_be_destroy_winsys;

	dev->pools.gem = drm_intel_bufmgr_gem_init(dev->fd, dev->max_batch_size);

	dev->softpipe = debug_get_bool_option("INTEL_SOFTPIPE", FALSE);
	dev->dump_cmd = debug_get_bool_option("INTEL_DUMP_CMD", FALSE);

	return true;
}

static void
intel_be_get_device_id(unsigned int *device_id)
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

struct pipe_screen *
intel_be_create_screen(struct drm_api *api, int drmFD,
		       struct drm_create_screen_arg *arg)
{
	struct intel_be_device *dev;
	struct pipe_screen *screen;
	unsigned int deviceID;

	if (arg != NULL) {
		switch(arg->mode) {
		case DRM_CREATE_NORMAL:
			break;
		default:
			return NULL;
		}
	}

	/* Allocate the private area */
	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;
	memset(dev, 0, sizeof(*dev));

	intel_be_get_device_id(&deviceID);
	intel_be_init_device(dev, drmFD, deviceID);

	if (dev->softpipe) {
		screen = softpipe_create_screen(&dev->base);
	} else
		screen = i915_create_screen(&dev->base, deviceID);

	return screen;
}
