
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
        const char *name;

	/**
	 * Kernel driver name, as accepted by drmOpenByName.
	 */
	const char *driver_name;

	/**
	 * Special buffer functions
	 */
	/*@{*/
	struct pipe_screen*  (*create_screen)(struct drm_api *api, int drm_fd,
	                                      struct drm_create_screen_arg *arg);
	/*@}*/

	/**
	 * Special buffer functions
	 */
	/*@{*/
	struct pipe_texture*
	    (*texture_from_shared_handle)(struct drm_api *api,
	                                  struct pipe_screen *screen,
	                                  struct pipe_texture *templ,
	                                  const char *name,
	                                  unsigned stride,
	                                  unsigned handle);
	boolean (*shared_handle_from_texture)(struct drm_api *api,
	                                      struct pipe_screen *screen,
	                                      struct pipe_texture *texture,
	                                      unsigned *stride,
	                                      unsigned *handle);
	boolean (*local_handle_from_texture)(struct drm_api *api,
	                                     struct pipe_screen *screen,
	                                     struct pipe_texture *texture,
	                                     unsigned *stride,
	                                     unsigned *handle);
	/*@}*/

	void (*destroy)(struct drm_api *api);
};

extern struct drm_api * drm_api_create(void);

#endif
