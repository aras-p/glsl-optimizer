#ifndef driclient_h
#define driclient_h

#include <stdint.h>
#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <drm_sarea.h>
//#include <X11/extensions/dri2proto.h>
#include "xf86dri.h"
#include "dri2.h"

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

int dri2CreateScreen(Display *display, int screen, dri_screen_t **dri_screen);
int dri2DestroyScreen(dri_screen_t *dri_screen);
int dri2CreateDrawable(dri_screen_t *dri_screen, XID drawable);
int dri2DestroyDrawable(dri_screen_t *dri_screen, XID drawable);
int dri2CopyDrawable(dri_screen_t *dri_screen, XID drawable, int dest, int src);

#define DRI_BUFFER_FRONT_LEFT		0
#define DRI_BUFFER_BACK_LEFT		1
#define DRI_BUFFER_FRONT_RIGHT		2
#define DRI_BUFFER_BACK_RIGHT		3
#define DRI_BUFFER_DEPTH		4
#define DRI_BUFFER_STENCIL		5
#define DRI_BUFFER_ACCUM		6
#define DRI_BUFFER_FAKE_FRONT_LEFT	7
#define DRI_BUFFER_FAKE_FRONT_RIGHT	8
#define DRI_BUFFER_DEPTH_STENCIL	9  /**< Only available with DRI2 1.1 */

#endif

