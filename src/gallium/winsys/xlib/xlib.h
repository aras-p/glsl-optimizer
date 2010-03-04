
#ifndef XLIB_H
#define XLIB_H

#include "pipe/p_compiler.h"
#include "xm_winsys.h"

extern struct xm_driver xlib_softpipe_driver;
extern struct xm_driver xlib_llvmpipe_driver;
extern struct xm_driver xlib_cell_driver;

/* Internal:
 */
struct sw_winsys;
struct sw_displaytarget;
struct sw_winsys *xlib_create_sw_winsys( void );
void xlib_sw_display(struct xmesa_buffer *xm_buffer,
                     struct sw_displaytarget *dt);

#endif
