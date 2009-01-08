
#ifndef XLIB_TRACE_H
#define XLIB_TRACE_H

struct pipe_winsys;
struct pipe_screen;
struct pipe_context;
struct pipe_surface;
struct xmesa_buffer;

struct pipe_winsys *
xlib_create_trace_winsys( void );

struct pipe_screen *
xlib_create_trace_screen( struct pipe_winsys *winsys );

struct pipe_context *
xlib_create_trace_context( struct pipe_screen *screen,
                           void *priv );

void
xlib_trace_display_surface( struct xmesa_buffer *buffer,
                            struct pipe_surface *surf );


#endif
