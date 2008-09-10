
#ifndef _INTEL_EGL_H_
#define _INTEL_EGL_H_

#include <xf86drm.h>

struct egl_drm_device
{
	void *priv;
	int drmFD;

	drmVersionPtr version;
	int deviceID;
};

struct egl_drm_context
{
	void *priv;
	struct egl_drm_device *device;
};

struct egl_drm_drawable
{
	void *priv;
	struct egl_drm_device *device;
	size_t h;
	size_t w;
};

struct egl_drm_frontbuffer
{
	uint32_t handle;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
};

#include "GL/internal/glcore.h"

int intel_create_device(struct egl_drm_device *device);
int intel_destroy_device(struct egl_drm_device *device);

int intel_create_context(struct egl_drm_context *context, const __GLcontextModes *visual, void *sharedContextPrivate);
int intel_destroy_context(struct egl_drm_context *context);

int intel_create_drawable(struct egl_drm_drawable *drawable, const __GLcontextModes * visual);
int intel_destroy_drawable(struct egl_drm_drawable *drawable);

void intel_make_current(struct egl_drm_context *context, struct egl_drm_drawable *draw, struct egl_drm_drawable *read);
void intel_swap_buffers(struct egl_drm_drawable *draw);
void intel_bind_frontbuffer(struct egl_drm_drawable *draw, struct egl_drm_frontbuffer *front);

#endif
