#ifndef vl_compositor_h
#define vl_compositor_h

#include <pipe/p_compiler.h>
#include <pipe/p_state.h>
#include <pipe/p_video_state.h>

struct pipe_context;
struct pipe_texture;

struct vl_compositor
{
   struct pipe_context *pipe;

   struct pipe_framebuffer_state fb_state;
   void *sampler;
   void *vertex_shader;
   void *fragment_shader;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_bufs[2];
   struct pipe_vertex_element vertex_elems[2];
   struct pipe_constant_buffer vs_const_buf, fs_const_buf;
};

bool vl_compositor_init(struct vl_compositor *compositor, struct pipe_context *pipe);

void vl_compositor_cleanup(struct vl_compositor *compositor);

void vl_compositor_render(struct vl_compositor          *compositor,
                          /*struct pipe_texture         *backround,
                          struct pipe_video_rect        *backround_area,*/
                          struct pipe_texture           *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_texture           *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_texture           *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_texture           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          /*unsigned                      num_layers,
                          struct pipe_texture           *layers,
                          struct pipe_video_rect        *layer_src_areas,
                          struct pipe_video_rect        *layer_dst_areas,*/
                          struct pipe_fence_handle      **fence);

#endif /* vl_compositor_h */
