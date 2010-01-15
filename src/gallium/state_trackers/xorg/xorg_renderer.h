#ifndef XORG_RENDERER_H
#define XORG_RENDERER_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct xorg_shaders;
struct exa_pixmap_priv;

/* max number of vertices *
 * max number of attributes per vertex *
 * max number of components per attribute
 *
 * currently the max is 100 quads
 */
#define BUF_SIZE (100 * 4 * 3 * 4)

struct xorg_renderer {
   struct pipe_context *pipe;

   struct cso_context *cso;
   struct xorg_shaders *shaders;

   int fb_width;
   int fb_height;
   struct pipe_buffer *vs_const_buffer;
   struct pipe_buffer *fs_const_buffer;

   float buffer[BUF_SIZE];
   int buffer_size;

   /* number of attributes per vertex for the current
    * draw operation */
   int attrs_per_vertex;
};

struct xorg_renderer *renderer_create(struct pipe_context *pipe);
void renderer_destroy(struct xorg_renderer *renderer);

void renderer_bind_destination(struct xorg_renderer *r,
                               struct pipe_surface *surface,
                               int width,
                               int height );

void renderer_bind_framebuffer(struct xorg_renderer *r,
                               struct exa_pixmap_priv *priv);
void renderer_bind_viewport(struct xorg_renderer *r,
                            struct exa_pixmap_priv *dst);
void renderer_set_constants(struct xorg_renderer *r,
                            int shader_type,
                            const float *buffer,
                            int size);


void renderer_draw_yuv(struct xorg_renderer *r,
                       int src_x, int src_y, int src_w, int src_h,
                       int dst_x, int dst_y, int dst_w, int dst_h,
                       struct pipe_texture **textures);

void renderer_begin_solid(struct xorg_renderer *r);
void renderer_solid(struct xorg_renderer *r,
                    int x0, int y0,
                    int x1, int y1,
                    float *color);

void renderer_begin_textures(struct xorg_renderer *r,
                             struct pipe_texture **textures,
                             int num_textures);
void renderer_texture(struct xorg_renderer *r,
                      int *pos,
                      int width, int height,
                      struct pipe_texture **textures,
                      int num_textures,
                      float *src_matrix,
                      float *mask_matrix);

void renderer_draw_flush(struct xorg_renderer *r);

struct pipe_texture *
renderer_clone_texture(struct xorg_renderer *r,
                       struct pipe_texture *src);

void renderer_copy_prepare(struct xorg_renderer *r,
                           struct pipe_surface *dst_surface,
                           struct pipe_texture *src_texture);

void renderer_copy_pixmap(struct xorg_renderer *r,
                          int dx, int dy,
                          int sx, int sy,
                          int width, int height,
                          float src_width,
                          float src_height);


#endif
