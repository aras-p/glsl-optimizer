#ifndef SP_TEXTURE_H
#define SP_TEXTURE_H


struct pipe_context;
struct pipe_texture;


extern void
softpipe_texture_create(struct pipe_context *pipe, struct pipe_texture **pt);

extern void
softpipe_texture_release(struct pipe_context *pipe, struct pipe_texture **pt);


#endif /* SP_TEXTURE */


