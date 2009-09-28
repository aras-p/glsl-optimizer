#ifndef xvmc_private_h
#define xvmc_private_h

#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>

#define BLOCK_SIZE_SAMPLES 64
#define BLOCK_SIZE_BYTES (BLOCK_SIZE_SAMPLES * 2)

struct pipe_video_context;
struct pipe_surface;
struct pipe_fence_handle;

typedef struct
{
	struct pipe_video_context *vpipe;
	struct pipe_surface *backbuffer;
} XvMCContextPrivate;

typedef struct
{
	struct pipe_video_surface *pipe_vsfc;
	struct pipe_fence_handle *render_fence;
	struct pipe_fence_handle *disp_fence;
	
	/* Some XvMC functions take a surface but not a context,
	   so we keep track of which context each surface belongs to. */
	XvMCContext *context;
} XvMCSurfacePrivate;

#endif /* xvmc_private_h */
