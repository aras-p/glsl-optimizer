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
 * currently the max is 5 quads
 */
#define BUF_SIZE (20 * 3 * 4)

struct xorg_renderer {
   struct pipe_context *pipe;

   struct cso_context *cso;
   struct xorg_shaders *shaders;

   struct pipe_constant_buffer vs_const_buffer;
   struct pipe_constant_buffer fs_const_buffer;

   float vertices[BUF_SIZE];
   int num_vertices;
};

struct xorg_renderer *renderer_create(struct pipe_context *pipe);
void renderer_destroy(struct xorg_renderer *renderer);

void renderer_bind_framebuffer(struct xorg_renderer *r,
                               struct exa_pixmap_priv *priv);
void renderer_bind_viewport(struct xorg_renderer *r,
                            struct exa_pixmap_priv *dst);
void renderer_bind_rasterizer(struct xorg_renderer *r);
void renderer_set_constants(struct xorg_renderer *r,
                            int shader_type,
                            const float *buffer,
                            int size);
void renderer_copy_pixmap(struct xorg_renderer *r,
                          struct exa_pixmap_priv *dst_priv, int dx, int dy,
                          struct exa_pixmap_priv *src_priv, int sx, int sy,
                          int width, int height);

void renderer_draw_solid_rect(struct xorg_renderer *r,
                              int x0, int y0,
                              int x1, int y1,
                              float *color);

void renderer_draw_textures(struct xorg_renderer *r,
                            int *pos,
                            int width, int height,
                            struct pipe_texture **textures,
                            int num_textures,
                            float *src_matrix,
                            float *mask_matrix);

void renderer_draw_yuv(struct xorg_renderer *r,
                       int src_x, int src_y, int src_w, int src_h,
                       int dst_x, int dst_y, int dst_w, int dst_h,
                       struct pipe_texture **textures);

void renderer_begin_solid(struct xorg_renderer *r,
                          float *color);
void renderer_solid(struct xorg_renderer *r,
                    int x0, int y0,
                    int x1, int y1);
void renderer_end_solid(struct xorg_renderer *r);


#endif
