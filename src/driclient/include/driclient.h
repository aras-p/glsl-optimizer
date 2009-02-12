#ifndef driclient_h
#define driclient_h

#include <stdint.h>
#include <X11/Xlib.h>
#include <drm_sarea.h>
#include "xf86dri.h"

/* TODO: Bring in DRI XML options */

typedef struct dri_version
{
	int major;
	int minor;
	int patch;
} dri_version_t;

typedef struct dri_screen
{
	Display			*display;
	unsigned int		num;
	dri_version_t		ddx, dri, drm;
	int			draw_lock_id;
	int			fd;
	drm_sarea_t		*sarea;
	void			*drawable_hash;
	void			*private;
} dri_screen_t;

struct dri_context;

typedef struct dri_drawable
{
	drm_drawable_t		drm_drawable;
	Drawable		x_drawable;
	unsigned int		sarea_index;
	unsigned int		*sarea_stamp;
	unsigned int		last_sarea_stamp;
	int			x, y, w, h;
	int			back_x, back_y;
	int			num_cliprects, num_back_cliprects;
	drm_clip_rect_t		*cliprects, *back_cliprects;
	dri_screen_t		*dri_screen;
	unsigned int		refcount;
	void			*private;
} dri_drawable_t;

typedef struct dri_context
{
	XID			id;
	drm_context_t		drm_context;
	dri_screen_t		*dri_screen;
	void			*private;
} dri_context_t;

typedef struct dri_framebuffer
{
	drm_handle_t		drm_handle;
	int			base, size, stride;
	int			private_size;
	void			*private;
} dri_framebuffer_t;

int driCreateScreen(Display *display, int screen, dri_screen_t **dri_screen, dri_framebuffer_t *dri_framebuf);
int driDestroyScreen(dri_screen_t *dri_screen);
int driCreateDrawable(dri_screen_t *dri_screen, Drawable drawable, dri_drawable_t **dri_drawable);
int driUpdateDrawableInfo(dri_drawable_t *dri_drawable);
int driDestroyDrawable(dri_drawable_t *dri_drawable);
int driCreateContext(dri_screen_t *dri_screen, Visual *visual, dri_context_t **dri_context);
int driDestroyContext(dri_context_t *dri_context);

#define DRI_VALIDATE_DRAWABLE_INFO_ONCE(dri_drawable)					\
do											\
{											\
	if (*(dri_drawable->sarea_stamp) != dri_drawable->last_sarea_stamp)		\
		driUpdateDrawableInfo(dri_drawable);					\
} while (0)

#define DRI_VALIDATE_DRAWABLE_INFO(dri_screen, dri_drawable)					\
do												\
{												\
	while (*(dri_drawable->sarea_stamp) != dri_drawable->last_sarea_stamp)			\
	{											\
		register unsigned int hwContext = dri_screen->sarea->lock.lock &		\
		~(DRM_LOCK_HELD | DRM_LOCK_CONT);						\
		DRM_UNLOCK(dri_screen->fd, &dri_screen->sarea->lock, hwContext);		\
												\
		DRM_SPINLOCK(&dri_screen->sarea->drawable_lock, dri_screen->draw_lock_id);	\
		DRI_VALIDATE_DRAWABLE_INFO_ONCE(dri_drawable);					\
		DRM_SPINUNLOCK(&dri_screen->sarea->drawable_lock, dri_screen->draw_lock_id);	\
												\
		DRM_LIGHT_LOCK(dri_screen->fd, &dri_screen->sarea->lock, hwContext);		\
	}											\
} while (0)

#endif

