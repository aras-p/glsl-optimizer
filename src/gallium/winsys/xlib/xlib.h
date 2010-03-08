
#ifndef XLIB_H
#define XLIB_H

#include "pipe/p_compiler.h"
#include "state_tracker/xlib_sw_winsys.h"

extern struct xm_driver xlib_softpipe_driver;
extern struct xm_driver xlib_llvmpipe_driver;
extern struct xm_driver xlib_cell_driver;

struct sw_winsys *xlib_create_sw_winsys( Display *display );

void xlib_sw_display(struct xlib_drawable *xm_buffer,
                     struct sw_displaytarget *dt);


#endif
