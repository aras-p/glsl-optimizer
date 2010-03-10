#ifndef WRAP_SCREEN_HELPER_H
#define WRAP_SCREEN_HELPER_H

#include "pipe/p_compiler.h"

struct pipe_screen;

/* Centralized code to inject common wrapping layers.  Other layers
 * can be introduced by specific targets, but these are the generally
 * helpful ones we probably want everywhere.
 */
struct pipe_screen *
gallium_wrap_screen( struct pipe_screen *screen );


#endif
