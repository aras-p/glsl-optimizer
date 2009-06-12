
#ifndef INTEL_DRM_DEVICE_H
#define INTEL_DRM_DEVICE_H

#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_context.h"

#include "drm.h"
#include "intel_bufmgr.h"

/*
 * Device
 */

struct intel_be_device
{
	struct pipe_winsys base;

	boolean softpipe;

	int fd; /**< Drm file discriptor */

	unsigned id;

	size_t max_batch_size;
	size_t max_vertex_size;

	struct {
		drm_intel_bufmgr *gem;
	} pools;
};

static INLINE struct intel_be_device *
intel_be_device(struct pipe_winsys *winsys)
{
	return (struct intel_be_device *)winsys;
}

boolean
intel_be_init_device(struct intel_be_device *device, int fd, unsigned id);

/*
 * Buffer
 */

struct intel_be_buffer {
	struct pipe_buffer base;
	drm_intel_bo *bo;
	boolean flinked;
	unsigned flink;
};

/**
 * Create a be buffer from a drm bo handle.
 *
 * Takes a reference.
 */
struct pipe_buffer *
intel_be_buffer_from_handle(struct pipe_screen *screen,
                            const char* name, unsigned handle);

/**
 * Gets a handle from a buffer.
 *
 * If buffer is destroyed handle may become invalid.
 */
boolean
intel_be_handle_from_buffer(struct pipe_screen *screen,
                            struct pipe_buffer *buffer,
                            unsigned *handle);

/**
 * Gets the global handle from a buffer.
 *
 * If buffer is destroyed handle may become invalid.
 */
boolean
intel_be_global_handle_from_buffer(struct pipe_screen *screen,
                                   struct pipe_buffer *buffer,
                                   unsigned *handle);

static INLINE struct intel_be_buffer *
intel_be_buffer(struct pipe_buffer *buf)
{
	return (struct intel_be_buffer *)buf;
}

static INLINE drm_intel_bo *
intel_bo(struct pipe_buffer *buf)
{
	return intel_be_buffer(buf)->bo;
}

#endif
