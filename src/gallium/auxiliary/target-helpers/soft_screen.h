#ifndef SOFT_SCREEN_HELPER_H
#define SOFT_SCREEN_HELPER_H

#include "pipe/p_compiler.h"

struct pipe_screen;
struct sw_winsys;

struct pipe_screen *
gallium_soft_create_screen( struct sw_winsys *winsys );

#endif
