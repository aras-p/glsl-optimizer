
#ifndef _DRM_API_H_
#define _DRM_API_H_

struct pipe_screen;
struct pipe_winsys;
struct pipe_context;

struct drm_api
{
	/**
	 * Special buffer function
	 */
	/*@{*/
	struct pipe_screen*  (*create_screen)(int drmFB, int pciID);
	struct pipe_context* (*create_context)(struct pipe_screen *screen);
	/*@}*/

	/**
	 * Special buffer function
	 */
	/*@{*/
	struct pipe_buffer* (*buffer_from_handle)(struct pipe_winsys *winsys, const char *name, unsigned handle);
	unsigned (*handle_from_buffer)(struct pipe_winsys *winsys, struct pipe_buffer *buffer);
	/*@}*/
};

/**
 * A driver needs to export this symbol
 */
extern struct drm_api drm_api_hocks;

#endif
