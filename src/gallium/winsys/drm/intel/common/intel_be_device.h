#ifndef INTEL_DRM_DEVICE_H
#define INTEL_DRM_DEVICE_H

#include "pipe/p_winsys.h"
#include "pipe/p_context.h"

/*
 * Device
 */

struct intel_be_device
{
	struct pipe_winsys base;

	/**
	 * Hw level screen
	 */
	struct pipe_screen *screen;

	int fd; /**< Drm file discriptor */

	size_t max_batch_size;
	size_t max_vertex_size;

	struct _DriFenceMgr *fenceMgr;

	struct _DriBufferPool *batchPool;
	struct _DriBufferPool *regionPool;
	struct _DriBufferPool *mallocPool;
	struct _DriBufferPool *vertexPool;
	struct _DriBufferPool *staticPool;
	struct _DriFreeSlabManager *fMan;
};

boolean
intel_be_init_device(struct intel_be_device *device, int fd, unsigned id);

void
intel_be_destroy_device(struct intel_be_device *dev);

/*
 * Buffer
 */

struct intel_be_buffer {
	struct pipe_buffer base;
	struct _DriBufferPool *pool;
	struct _DriBufferObject *driBO;
};

/**
 * Create a be buffer from a drm bo handle
 *
 * Takes a reference
 */
struct pipe_buffer *
intel_be_buffer_from_handle(struct intel_be_device *device,
                            const char* name, unsigned handle);

static INLINE struct intel_be_buffer *
intel_be_buffer(struct pipe_buffer *buf)
{
	return (struct intel_be_buffer *)buf;
}

static INLINE struct _DriBufferObject *
dri_bo(struct pipe_buffer *buf)
{
	return intel_be_buffer(buf)->driBO;
}

#endif
