
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_context.h"

static int
nouveau_screen_get_video_param(struct pipe_screen *pscreen,
                               enum pipe_video_profile profile,
                               enum pipe_video_cap param)
{
   switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      return vl_profile_supported(pscreen, profile);
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
      return vl_video_buffer_max_size(pscreen);
   default:
      debug_printf("unknown video param: %d\n", param);
      return 0;
   }
}

void
nouveau_screen_init_vdec(struct nouveau_screen *screen)
{
   screen->base.get_video_param = nouveau_screen_get_video_param;
   screen->base.is_video_format_supported = vl_video_buffer_is_format_supported;
}

void
nouveau_context_init_vdec(struct nouveau_context *nv)
{
   nv->pipe.create_video_decoder = vl_create_decoder;
   nv->pipe.create_video_buffer = vl_video_buffer_create;
}
