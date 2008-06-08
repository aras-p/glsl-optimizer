#ifndef xsp_winsys_h
#define xsp_winsys_h

#include <X11/Xlib.h>

struct pipe_context;

struct pipe_context* create_pipe_context(Display *display);

#endif

