#ifndef SWRAST_XLIB_HELPER_H
#define SWRAST_XLIB_HELPER_H

#include <X11/Xlib.h>
#include "pipe/p_compiler.h"

/* Helper to build the xlib winsys, choose between the software
 * rasterizers and construct the lower part of a driver stack.
 *
 * Just add a state tracker.
 */
struct pipe_screen *swrast_xlib_create_screen( Display *display );


#endif
