#ifndef vl_winsys_h
#define vl_winsys_h

#include <X11/Xlib.h>

struct pipe_context;

struct pipe_context* create_pipe_context(Display *display, int screen);
int destroy_pipe_context(struct pipe_context *pipe);
int bind_pipe_drawable(struct pipe_context *pipe, Drawable drawable);
int unbind_pipe_drawable(struct pipe_context *pipe);

#endif

