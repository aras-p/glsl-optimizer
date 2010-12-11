#ifndef TEXRENDER_H
#define TEXRENDER_H

struct gl_context;
struct gl_framebuffer;
struct gl_renderbuffer_attachment;

extern void
_mesa_render_texture(struct gl_context *ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att);

extern void
_mesa_finish_render_texture(struct gl_context *ctx,
                            struct gl_renderbuffer_attachment *att);


#endif /* TEXRENDER_H */
