#ifndef XLIB_SW_WINSYS_H
#define XLIB_SW_WINSYS_H

#include "state_tracker/sw_winsys.h"
#include <X11/Xlib.h>


struct pipe_screen;
struct pipe_surface;

/* This is what the xlib software winsys expects to find in the
 * "private" field of flush_frontbuffers().  Xlib-based state trackers
 * somehow need to know this.
 */
struct xlib_drawable {
   Visual *visual;
   int depth;
   Drawable drawable;
   GC gc;                       /* temporary? */
};


struct xm_driver {
   struct pipe_screen *(*create_pipe_screen)( Display *display );

   void (*display_surface)( struct xlib_drawable *, 
                            struct pipe_surface * );
};

/* Called by the libgl-xlib target code to build the rendering stack.
 */
struct xm_driver *xlib_sw_winsys_init( void );

#endif
