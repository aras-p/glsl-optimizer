
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
	/**
	 * Special buffer functions
	 */
	/*@{*/
	struct pipe_screen*  (*create_screen)(int drm_fd,
					      struct drm_create_screen_arg *arg);
	struct pipe_context* (*create_context)(struct pipe_screen *screen);
	/*@}*/

	/**
	 * Special buffer functions
	 */
	/*@{*/
	boolean (*buffer_from_texture)(struct pipe_texture *texture,
                                       struct pipe_buffer **buffer,
                                       unsigned *stride);
	struct pipe_buffer* (*buffer_from_handle)(struct pipe_screen *screen,
                                                  const char *name,
                                                  unsigned handle);
	boolean (*handle_from_buffer)(struct pipe_screen *screen,
                                      struct pipe_buffer *buffer,
                                      unsigned *handle);
	boolean (*global_handle_from_buffer)(struct pipe_screen *screen,
                                             struct pipe_buffer *buffer,
                                             unsigned *handle);
	/*@}*/
};

/**
 * A driver needs to export this symbol
 */
extern struct drm_api drm_api_hooks;

#endif
