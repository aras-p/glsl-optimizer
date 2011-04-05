#ifndef __R600_VIDEO_CONTEXT_H__
#define __R600_VIDEO_CONTEXT_H__

#include <pipe/p_video_context.h>

struct pipe_video_context *
r600_video_create(struct pipe_screen *screen, void *priv);

#endif
