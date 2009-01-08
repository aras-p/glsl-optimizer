
#ifndef XLIB_SOFTPIPE_H
#define XLIB_SOFTPIPE_H

struct pipe_winsys;
struct pipe_screen;
struct pipe_context;
struct pipe_surface;
struct xmesa_buffer;


struct pipe_winsys *
xlib_create_softpipe_winsys( void );

struct pipe_screen *
xlib_create_softpipe_screen( struct pipe_winsys *pws );

struct pipe_context *
xlib_create_softpipe_context( struct pipe_screen *screen,
                              void *context_priv );

void 
xlib_softpipe_display_surface( struct xmesa_buffer *, 
                               struct pipe_surface * );

/***********************************************************************
 * Cell piggybacks on softpipe code still.
 *
 * Should be untangled sufficiently to live in a separate file, at
 * least.  That would mean removing #ifdef GALLIUM_CELL's from above
 * and creating cell-specific versions of either those functions or
 * the entire file.
 */
struct pipe_winsys *
xlib_create_cell_winsys( void );

struct pipe_screen *
xlib_create_cell_screen( struct pipe_winsys *pws );


struct pipe_context *
xlib_create_cell_context( struct pipe_screen *screen,
                          void *priv );

void 
xlib_cell_display_surface( struct xmesa_buffer *, 
                           struct pipe_surface * );


#endif
