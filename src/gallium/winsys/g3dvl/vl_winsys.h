#ifndef vl_winsys_h
#define vl_winsys_h

#include <X11/Xlib.h>
#include <pipe/p_defines.h>
#include <pipe/p_format.h>

struct pipe_screen;
struct pipe_video_context;

struct pipe_screen*
vl_screen_create(Display *display, int screen);

struct pipe_video_context*
vl_video_create(struct pipe_screen *screen,
                enum pipe_video_profile profile,
                enum pipe_video_chroma_format chroma_format,
                unsigned width, unsigned height);

Drawable
vl_video_bind_drawable(struct pipe_video_context *vpipe, Drawable drawable);

#endif
