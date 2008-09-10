

/*
 * Authors: Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 *			 Jakob Bornecrantz <jakob-at-tungstengraphics-dot-com>
 */

#include "intel_be_device.h"
#include "ws_dri_bufmgr.h"
#include "ws_dri_bufpool.h"
#include "ws_dri_fencemgr.h"

#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"
#include "util/u_memory.h"

#include "i915simple/i915_screen.h"

/* Turn a pipe winsys into an intel/pipe winsys:
 */
static INLINE struct intel_be_device *
intel_be_device( struct pipe_winsys *winsys )
{
	return (struct intel_be_device *)winsys;
}


/*
 * Buffer functions.
 *
 * Most callbacks map direcly onto dri_bufmgr operations:
 */

static void *intel_be_buffer_map(struct pipe_winsys *winsys,
					struct pipe_buffer *buf,
					unsigned flags )
{
	unsigned drm_flags = 0;
	
	if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
		drm_flags |= DRM_BO_FLAG_WRITE;

	if (flags & PIPE_BUFFER_USAGE_CPU_READ)
		drm_flags |= DRM_BO_FLAG_READ;

	return driBOMap( dri_bo(buf), drm_flags, 0 );
}

static void intel_be_buffer_unmap(struct pipe_winsys *winsys,
					 struct pipe_buffer *buf)
{
	driBOUnmap( dri_bo(buf) );
}

static void
intel_be_buffer_destroy(struct pipe_winsys *winsys,
			  struct pipe_buffer *buf)
{
	driBOUnReference( dri_bo(buf) );
	FREE(buf);
}

static struct pipe_buffer *
intel_be_buffer_create(struct pipe_winsys *winsys,
						  unsigned alignment,
						  unsigned usage,
						  unsigned size )
{
	struct intel_be_buffer *buffer = CALLOC_STRUCT( intel_be_buffer );
	struct intel_be_device *iws = intel_be_device(winsys);
	unsigned flags = 0;
	struct _DriBufferPool *pool;

	buffer->base.refcount = 1;
	buffer->base.alignment = alignment;
	buffer->base.usage = usage;
	buffer->base.size = size;

	if (usage & (PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_CONSTANT)) {
		flags |= DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_CACHED;
		pool = iws->mallocPool;
	} else if (usage & PIPE_BUFFER_USAGE_CUSTOM) {
		/* For vertex buffers */
		flags |= DRM_BO_FLAG_MEM_VRAM | DRM_BO_FLAG_MEM_TT;
		pool = iws->vertexPool;
	} else {
		flags |= DRM_BO_FLAG_MEM_VRAM | DRM_BO_FLAG_MEM_TT;
		pool = iws->regionPool;
	}

	if (usage & PIPE_BUFFER_USAGE_GPU_READ)
		flags |= DRM_BO_FLAG_READ;

	if (usage & PIPE_BUFFER_USAGE_GPU_WRITE)
		flags |= DRM_BO_FLAG_WRITE;

	/* drm complains if we don't set any read/write flags.
	 */
	if ((flags & (DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE)) == 0)
		flags |= DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE;

	buffer->pool = pool;
	driGenBuffers( buffer->pool,
		"pipe buffer", 1, &buffer->driBO, alignment, flags, 0 );

	driBOData( buffer->driBO, size, NULL, buffer->pool, 0 );

	return &buffer->base;
}


static struct pipe_buffer *
intel_be_user_buffer_create(struct pipe_winsys *winsys, void *ptr, unsigned bytes)
{
	struct intel_be_buffer *buffer = CALLOC_STRUCT( intel_be_buffer );
	struct intel_be_device *iws = intel_be_device(winsys);

	driGenUserBuffer( iws->regionPool,
			  "pipe user buffer", &buffer->driBO, ptr, bytes );

	buffer->base.refcount = 1;

	return &buffer->base;
}

struct pipe_buffer *
intel_be_buffer_from_handle(struct intel_be_device *device,
                            const char* name, unsigned handle)
{
	struct intel_be_buffer *be_buf = malloc(sizeof(*be_buf));
	struct pipe_buffer *buffer;

	if (!be_buf)
		goto err;

	memset(be_buf, 0, sizeof(*be_buf));

	driGenBuffers(device->staticPool, name, 1, &be_buf->driBO, 0, 0, 0);
    driBOSetReferenced(be_buf->driBO, handle);

	if (0) /** XXX TODO check error */
		goto err_bo;
	
	buffer = &be_buf->base;
	buffer->refcount = 1;
	buffer->alignment = 0;
	buffer->usage = 0;
	buffer->size = driBOSize(be_buf->driBO);

	return buffer;
err_bo:
	free(be_buf);
err:
	return NULL;
}


/*
 * Surface functions.
 *
 * Deprecated!
 */

static struct pipe_surface *
intel_i915_surface_alloc(struct pipe_winsys *winsys)
{
	assert((size_t)"intel_i915_surface_alloc is deprecated" & 0);
	return NULL;
}

static int
intel_i915_surface_alloc_storage(struct pipe_winsys *winsys,
				 struct pipe_surface *surf,
				 unsigned width, unsigned height,
				 enum pipe_format format,
				 unsigned flags,
				 unsigned tex_usage)
{
	assert((size_t)"intel_i915_surface_alloc_storage is deprecated" & 0);
	return -1;
}

static void
intel_i915_surface_release(struct pipe_winsys *winsys, struct pipe_surface **s)
{
	assert((size_t)"intel_i915_surface_release is deprecated" & 0);
}


/*
 * Fence functions
 */

static void
intel_be_fence_reference( struct pipe_winsys *sws,
		       struct pipe_fence_handle **ptr,
		       struct pipe_fence_handle *fence )
{
	if (*ptr)
		driFenceUnReference((struct _DriFenceObject **)ptr);

	if (fence)
		*ptr = (struct pipe_fence_handle *)driFenceReference((struct _DriFenceObject *)fence);
}

static int
intel_be_fence_signalled( struct pipe_winsys *sws,
		       struct pipe_fence_handle *fence,
		       unsigned flag )
{
	return driFenceSignaled((struct _DriFenceObject *)fence, flag);
}

static int
intel_be_fence_finish( struct pipe_winsys *sws,
		    struct pipe_fence_handle *fence,
		    unsigned flag )
{
	return driFenceFinish((struct _DriFenceObject *)fence, flag, 0);
}


/*
 * Misc functions
 */

boolean
intel_be_init_device(struct intel_be_device *dev, int fd, unsigned id)
{
	dev->fd = fd;
	dev->max_batch_size = 16 * 4096;
	dev->max_vertex_size = 128 * 4096;

	dev->base.buffer_create = intel_be_buffer_create;
	dev->base.user_buffer_create = intel_be_user_buffer_create;
	dev->base.buffer_map = intel_be_buffer_map;
	dev->base.buffer_unmap = intel_be_buffer_unmap;
	dev->base.buffer_destroy = intel_be_buffer_destroy;
	dev->base.surface_alloc = intel_i915_surface_alloc;
	dev->base.surface_alloc_storage = intel_i915_surface_alloc_storage;
	dev->base.surface_release = intel_i915_surface_release;
	dev->base.fence_reference = intel_be_fence_reference;
	dev->base.fence_signalled = intel_be_fence_signalled;
	dev->base.fence_finish = intel_be_fence_finish;

#if 0 /* Set by the winsys */
	dev->base.flush_frontbuffer = intel_flush_frontbuffer;
	dev->base.get_name = intel_get_name;
#endif

	dev->fMan = driInitFreeSlabManager(10, 10);
	dev->fenceMgr = driFenceMgrTTMInit(dev->fd);

	dev->mallocPool = driMallocPoolInit();
	dev->staticPool = driDRMPoolInit(dev->fd);
	/* Sizes: 64 128 256 512 1024 2048 4096 8192 16384 32768 */
	dev->regionPool = driSlabPoolInit(dev->fd,
					  DRM_BO_FLAG_READ |
					  DRM_BO_FLAG_WRITE |
					  DRM_BO_FLAG_MEM_TT,
					  DRM_BO_FLAG_READ |
					  DRM_BO_FLAG_WRITE |
					  DRM_BO_FLAG_MEM_TT,
					  64,
					  10, 120, 4096 * 64, 0,
					  dev->fMan);

	dev->vertexPool = driSlabPoolInit(dev->fd,
					  DRM_BO_FLAG_READ |
					  DRM_BO_FLAG_WRITE |
					  DRM_BO_FLAG_MEM_TT,
					  DRM_BO_FLAG_READ |
					  DRM_BO_FLAG_WRITE |
					  DRM_BO_FLAG_MEM_TT,
					  dev->max_vertex_size,
					  1, 120, dev->max_vertex_size * 4, 0,
					  dev->fMan);

	dev->batchPool = driSlabPoolInit(dev->fd,
					 DRM_BO_FLAG_EXE |
					 DRM_BO_FLAG_MEM_TT,
					 DRM_BO_FLAG_EXE |
					 DRM_BO_FLAG_MEM_TT,
					 dev->max_batch_size,
					 1, 40, dev->max_batch_size * 16, 0,
					 dev->fMan);

	/* Fill in this struct with callbacks that i915simple will need to
	 * communicate with the window system, buffer manager, etc.
	 */
	dev->screen = i915_create_screen(&dev->base, id);

	return true;
}

void
intel_be_destroy_device(struct intel_be_device *dev)
{
	driPoolTakeDown(dev->mallocPool);
	driPoolTakeDown(dev->staticPool);
	driPoolTakeDown(dev->regionPool);
	driPoolTakeDown(dev->vertexPool);
	driPoolTakeDown(dev->batchPool);

	/** TODO takedown fenceMgr and fMan */
}
