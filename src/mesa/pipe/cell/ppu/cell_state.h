

#ifndef CELL_STATE_H
#define CELL_STATE_H


#define CELL_NEW_VIEWPORT      0x1
#define CELL_NEW_RASTERIZER    0x2
#define CELL_NEW_FS            0x4
#define CELL_NEW_BLEND         0x8
#define CELL_NEW_CLIP          0x10
#define CELL_NEW_SCISSOR       0x20
#define CELL_NEW_STIPPLE       0x40
#define CELL_NEW_FRAMEBUFFER   0x80
#define CELL_NEW_ALPHA_TEST    0x100
#define CELL_NEW_DEPTH_STENCIL 0x200
#define CELL_NEW_SAMPLER       0x400
#define CELL_NEW_TEXTURE       0x800
#define CELL_NEW_VERTEX        0x1000
#define CELL_NEW_VS            0x2000
#define CELL_NEW_CONSTANTS     0x4000



extern void
cell_set_framebuffer_state( struct pipe_context *,
                            const struct pipe_framebuffer_state * );



extern void *
cell_create_blend_state(struct pipe_context *, const struct pipe_blend_state *);
extern void cell_bind_blend_state(struct pipe_context *, void *);
extern void cell_delete_blend_state(struct pipe_context *, void *);

extern void cell_set_blend_color( struct pipe_context *pipe,
                                  const struct pipe_blend_color *blend_color );


void *
cell_create_sampler_state(struct pipe_context *,
                          const struct pipe_sampler_state *);

extern void
cell_bind_sampler_state(struct pipe_context *, unsigned, void *);

extern void
cell_delete_sampler_state(struct pipe_context *, void *);


extern void *
cell_create_depth_stencil_state(struct pipe_context *,
                                const struct pipe_depth_stencil_alpha_state *);

extern void
cell_bind_depth_stencil_state(struct pipe_context *, void *);

extern void
cell_delete_depth_stencil_state(struct pipe_context *, void *);


void *cell_create_fs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void cell_bind_fs_state(struct pipe_context *, void *);
void cell_delete_fs_state(struct pipe_context *, void *);
void *cell_create_vs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void cell_bind_vs_state(struct pipe_context *, void *);
void cell_delete_vs_state(struct pipe_context *, void *);


void *
cell_create_rasterizer_state(struct pipe_context *,
                             const struct pipe_rasterizer_state *);
void cell_bind_rasterizer_state(struct pipe_context *, void *);
void cell_delete_rasterizer_state(struct pipe_context *, void *);


void cell_set_clip_state( struct pipe_context *,
                          const struct pipe_clip_state * );

void cell_set_constant_buffer(struct pipe_context *pipe,
                              uint shader, uint index,
                              const struct pipe_constant_buffer *buf);

void cell_set_polygon_stipple( struct pipe_context *,
                               const struct pipe_poly_stipple * );

void cell_set_scissor_state( struct pipe_context *,
                             const struct pipe_scissor_state * );

void cell_set_texture_state( struct pipe_context *,
                             unsigned unit, struct pipe_texture * );

void cell_set_vertex_element(struct pipe_context *,
                             unsigned index,
                             const struct pipe_vertex_element *);

void cell_set_vertex_buffer(struct pipe_context *,
                            unsigned index,
                            const struct pipe_vertex_buffer *);

void cell_set_viewport_state( struct pipe_context *,
                              const struct pipe_viewport_state * );


#endif
