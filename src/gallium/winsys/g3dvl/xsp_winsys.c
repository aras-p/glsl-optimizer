#include "xsp_winsys.h"
#include <X11/Xutil.h>
#include <pipe/p_winsys.h>
#include <pipe/p_state.h>
#include <pipe/p_util.h>
#include <pipe/p_inlines.h>
#include <softpipe/sp_winsys.h>

/* pipe_winsys implementation */

struct xsp_pipe_winsys
{
	struct pipe_winsys	base;
	Display			*display;
	XImage			fbimage;
};

struct xsp_buffer
{
	struct pipe_buffer	base;
	boolean			is_user_buffer;
	void			*data;
	void			*mapped_data;
};

static struct pipe_buffer* xsp_buffer_create(struct pipe_winsys *pws, unsigned alignment, unsigned usage, unsigned size)
{
	struct xsp_buffer *buffer;
	
	assert(pws);
	
	buffer = calloc(1, sizeof(struct xsp_buffer));
	buffer->base.refcount = 1;
	buffer->base.alignment = alignment;
	buffer->base.usage = usage;
	buffer->base.size = size;
	buffer->data = align_malloc(size, alignment);
	
	return (struct pipe_buffer*)buffer;
}

static struct pipe_buffer* xsp_user_buffer_create(struct pipe_winsys *pws, void *data, unsigned size)
{
	struct xsp_buffer *buffer;
	
	assert(pws);
	
	buffer = calloc(1, sizeof(struct xsp_buffer));
	buffer->base.refcount = 1;
	buffer->base.size = size;
	buffer->is_user_buffer = TRUE;
	buffer->data = data;
	
	return (struct pipe_buffer*)buffer;
}

static void* xsp_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buffer, unsigned flags)
{
	struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;
	
	assert(pws);
	assert(buffer);
	
	xsp_buf->mapped_data = xsp_buf->data;
	return xsp_buf->mapped_data;
}

static void xsp_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buffer)
{
	struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;
	
	assert(pws);
	assert(buffer);
	
	xsp_buf->mapped_data = NULL;
}

static void xsp_buffer_destroy(struct pipe_winsys *pws, struct pipe_buffer *buffer)
{
	struct xsp_buffer *xsp_buf = (struct xsp_buffer*)buffer;
	
	assert(pws);
	assert(buffer);
	
	if (!xsp_buf->is_user_buffer)
		align_free(xsp_buf->data);
	
	free(xsp_buf);
}

static struct pipe_surface* xsp_surface_alloc(struct pipe_winsys *pws)
{
	struct pipe_surface *surface;
	
	assert(pws);
	
	surface = calloc(1, sizeof(struct pipe_surface));
	surface->refcount = 1;
	surface->winsys = pws;
	
	return surface;
}

static int xsp_surface_alloc_storage
(
	struct pipe_winsys *pws,
	struct pipe_surface *surface,
	unsigned width,
	unsigned height,
	enum pipe_format format,
	unsigned flags,
	unsigned tex_usage
)
{
	const unsigned int ALIGNMENT = 1;
	
	assert(pws);
	assert(surface);
	
	surface->width = width;
	surface->height = height;
	surface->format = format;
	surface->cpp = pf_get_size(format);
	surface->pitch = width;
	surface->usage = flags;
	surface->buffer = pws->buffer_create(pws, ALIGNMENT, PIPE_BUFFER_USAGE_PIXEL, surface->pitch * surface->cpp * height);
	
	return 0;
}

static void xsp_surface_release(struct pipe_winsys *pws, struct pipe_surface **surface)
{
	struct pipe_surface *s;
	
	assert(pws);
	assert(surface);
	assert(*surface);
	
	s = *surface;
	
	s->refcount--;
	
	if (s->refcount == 0)
	{
		pipe_buffer_reference(pws, &s->buffer, NULL);
		free(s);
	}
	
	*surface = NULL;
}

static void xsp_flush_frontbuffer(struct pipe_winsys *pws, struct pipe_surface *surface, void *context_private)
{	
	struct xsp_pipe_winsys *xsp_winsys;
	
	assert(pws);
	assert(surface);
	assert(context_private);
	
	xsp_winsys = (struct xsp_pipe_winsys*)pws;
	
	xsp_winsys->fbimage.width = surface->width;
	xsp_winsys->fbimage.height = surface->height;
	xsp_winsys->fbimage.bytes_per_line = surface->width * (xsp_winsys->fbimage.bits_per_pixel >> 3);
	xsp_winsys->fbimage.data = pipe_surface_map(surface, 0);
	
	XPutImage
	(
		xsp_winsys->display,
		*(Drawable*)context_private,
		XDefaultGC(xsp_winsys->display, XDefaultScreen(xsp_winsys->display)),
		&xsp_winsys->fbimage,
		0,
		0,
		0,
		0,
		surface->width,
		surface->height
	);
	XFlush(xsp_winsys->display);
	pipe_surface_unmap(surface);
}

static const char* xsp_get_name(struct pipe_winsys *pws)
{
	assert(pws);
	return "X11 SoftPipe";
}

/* softpipe_winsys implementation */

static boolean xsp_is_format_supported(struct softpipe_winsys *spws, enum pipe_format format)
{
	assert(spws);
	
	/* TODO: Test that 'format' is equal to our output window's format */
	return TRUE;
}

/* Show starts here */

struct pipe_context* create_pipe_context(Display *display)
{
	struct xsp_pipe_winsys	*xsp_winsys;
	struct softpipe_winsys	*sp_winsys;
	struct pipe_screen	*p_screen;
	struct pipe_context	*p_context;
	
	assert(display);
	
	xsp_winsys = calloc(1, sizeof(struct xsp_pipe_winsys));
	xsp_winsys->base.buffer_create = xsp_buffer_create;
	xsp_winsys->base.user_buffer_create = xsp_user_buffer_create;
	xsp_winsys->base.buffer_map = xsp_buffer_map;
	xsp_winsys->base.buffer_unmap = xsp_buffer_unmap;
	xsp_winsys->base.buffer_destroy = xsp_buffer_destroy;
	xsp_winsys->base.surface_alloc = xsp_surface_alloc;
	xsp_winsys->base.surface_alloc_storage = xsp_surface_alloc_storage;
	xsp_winsys->base.surface_release = xsp_surface_release;
	xsp_winsys->base.flush_frontbuffer = xsp_flush_frontbuffer;
	xsp_winsys->base.get_name = xsp_get_name;
	xsp_winsys->display = display;
	
	{
		/* XXX: Can't use the returned XImage* directly,
		since we don't have control over winsys destruction
		and we wouldn't be able to free it */
		XImage *template = XCreateImage
		(
			display,
			XDefaultVisual(display, XDefaultScreen(display)),
			XDefaultDepth(display, XDefaultScreen(display)),
			ZPixmap,
			0,
			NULL,
			0,	/* Don't know the width and height until flush_frontbuffer */
			0,
			32,
			0
		);
		
		memcpy(&xsp_winsys->fbimage, template, sizeof(XImage));
		XInitImage(&xsp_winsys->fbimage);
		
		XDestroyImage(template);
	}

	sp_winsys = calloc(1, sizeof(struct softpipe_winsys));
	sp_winsys->is_format_supported = xsp_is_format_supported;
	
	p_screen = softpipe_create_screen((struct pipe_winsys*)xsp_winsys);
	p_context = softpipe_create(p_screen, (struct pipe_winsys*)xsp_winsys, sp_winsys);
	
	return p_context;
}

