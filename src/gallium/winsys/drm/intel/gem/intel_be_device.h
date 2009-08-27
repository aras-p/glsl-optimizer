
#ifndef INTEL_DRM_DEVICE_H
#define INTEL_DRM_DEVICE_H

#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_context.h"

#include "drm.h"
#include "intel_bufmgr.h"

struct drm_api;

/*
 * Device
 */

struct intel_be_device
{
	struct pipe_winsys base;

	boolean softpipe;
	boolean dump_cmd;

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
	void *ptr;
	unsigned map_count;
	boolean map_gtt;

	drm_intel_bo *bo;
	boolean flinked;
	unsigned flink;
};

/**
 * Create a texture from a shared drm handle.
 */
struct pipe_texture *
intel_be_texture_from_shared_handle(struct drm_api *api,
                                    struct pipe_screen *screen,
                                    struct pipe_texture *templ,
                                    const char* name,
                                    unsigned pitch,
                                    unsigned handle);

/**
 * Gets a shared handle from a texture.
 *
 * If texture is destroyed handle may become invalid.
 */
boolean
intel_be_shared_handle_from_texture(struct drm_api *api,
                                    struct pipe_screen *screen,
                                    struct pipe_texture *texture,
                                    unsigned *pitch,
                                    unsigned *handle);

/**
 * Gets the local handle from a texture. As used by KMS.
 *
 * If texture is destroyed handle may become invalid.
 */
boolean
intel_be_local_handle_from_texture(struct drm_api *api,
                                   struct pipe_screen *screen,
                                   struct pipe_texture *texture,
                                   unsigned *pitch,
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
