#ifndef GALLIUM_RAW_H
#define GALLIUM_RAW_H

/* This is an API for exercising gallium functionality in a
 * platform-neutral fashion.  Whatever platform integration is
 * necessary to implement this interface is orchestrated by the
 * individual target building this entity.
 *
 * For instance, the graw-xlib target includes code to implent these
 * interfaces on top of the X window system.
 *
 * Programs using this interface may additionally benefit from some of
 * the utilities currently in the libgallium.a library, especially
 * those for parsing text representations of TGSI shaders.
 */

#include "pipe/p_format.h"

struct pipe_screen;

struct pipe_screen *graw_init( void );

/* Returns a handle to be used with flush_frontbuffer()/present().
 *
 * Query format support with screen::is_format_supported and usage
 * XXX.
 */
void *graw_create_window( int x,
                          int y,
                          unsigned width,
                          unsigned height,
                          enum pipe_format format );

void graw_destroy_window( void *handle );

#endif
