
#ifndef _DRM_API_H_
#define _DRM_API_H_

#include "pipe/p_compiler.h"

struct pipe_screen;
struct pipe_winsys;
struct pipe_buffer;
struct pipe_context;
struct pipe_texture;

enum drm_create_screen_mode {
	DRM_CREATE_NORMAL = 0,
	DRM_CREATE_DRI1,
	DRM_CREATE_DRIVER = 1024,
	DRM_CREATE_MAX
};

#define DRM_API_HANDLE_TYPE_SHARED 0
#define DRM_API_HANDLE_TYPE_KMS    1

/**
 * For use with pipe_screen::{texture_from_handle|texture_get_handle}.
 */
struct winsys_handle
{
	/**
	 * Unused for texture_from_handle, always DRM_API_HANDLE_TYPE_SHARED.
	 * Input to texture_get_handle, use TEXTURE_USAGE to select handle for kms or ipc.
	 */
	unsigned type;
	/**
	 * Input to texture_from_handle.
	 * Output for texture_get_handle.
	 */
	unsigned handle;
	/**
	 * Input to texture_from_handle.
	 * Output for texture_get_handle.
	 */
	unsigned stride;
};

/**
 * Modes other than DRM_CREATE_NORMAL derive from this struct.
 */
/*@{*/
struct drm_create_screen_arg {
	enum drm_create_screen_mode mode;
};
/*@}*/

struct drm_api
{
	void (*destroy)(struct drm_api *api);

        const char *name;

	/**
	 * Kernel driver name, as accepted by drmOpenByName.
	 */
	const char *driver_name;

	/**
	 * Create a pipe srcreen.
	 */
	struct pipe_screen*  (*create_screen)(struct drm_api *api, int drm_fd,
	                                      struct drm_create_screen_arg *arg);
};

extern struct drm_api * drm_api_create(void);

#endif
