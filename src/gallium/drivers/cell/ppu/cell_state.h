

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
#define CELL_NEW_VERTEX_INFO   0x8000




void *cell_create_fs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void cell_bind_fs_state(struct pipe_context *, void *);
void cell_delete_fs_state(struct pipe_context *, void *);
void *cell_create_vs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void cell_bind_vs_state(struct pipe_context *, void *);
void cell_delete_vs_state(struct pipe_context *, void *);

void
cell_set_constant_buffer(struct pipe_context *pipe,
                         uint shader, uint index,
                         const struct pipe_constant_buffer *buf);


void cell_set_vertex_element(struct pipe_context *,
                             unsigned index,
                             const struct pipe_vertex_element *);

void cell_set_vertex_buffer(struct pipe_context *,
                            unsigned index,
                            const struct pipe_vertex_buffer *);

void cell_update_derived( struct cell_context *softpipe );

#endif
