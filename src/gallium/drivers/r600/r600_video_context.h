#ifndef __R600_VIDEO_CONTEXT_H__
#define __R600_VIDEO_CONTEXT_H__

#include <pipe/p_video_context.h>

struct pipe_video_context *
r600_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, void *priv);

#endif
