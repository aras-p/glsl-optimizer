#include "r600_video_context.h"
#include <softpipe/sp_video_context.h>

struct pipe_video_context *
r600_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, void *priv)
{
   struct pipe_context *pipe;

   assert(screen);

   pipe = screen->context_create(screen, priv);
   if (!pipe)
      return NULL;

   return sp_video_create_ex(pipe, profile, chroma_format, width, height,
                             VL_MPEG12_MC_RENDERER_BUFFER_PICTURE,
                             true,
                             PIPE_FORMAT_VUYX);
}
