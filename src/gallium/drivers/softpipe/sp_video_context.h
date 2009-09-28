#ifndef SP_VIDEO_CONTEXT_H
#define SP_VIDEO_CONTEXT_H

#include <pipe/p_video_context.h>
#include <vl/vl_mpeg12_mc_renderer.h>
#include <vl/vl_compositor.h>

struct pipe_screen;
struct pipe_context;
struct pipe_video_surface;

struct sp_mpeg12_context
{
   struct pipe_video_context base;
   struct pipe_context *pipe;
   struct pipe_video_surface *decode_target;
   struct vl_mpeg12_mc_renderer mc_renderer;
   struct vl_compositor compositor;

   void *rast;
   void *dsa;
   void *blend;
};

struct pipe_video_context *
sp_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                enum pipe_video_chroma_format chroma_format,
                unsigned width, unsigned height);

#endif /* SP_VIDEO_CONTEXT_H */
