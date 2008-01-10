#ifndef SP_TEXTURE_H
#define SP_TEXTURE_H


struct pipe_context;
struct pipe_texture;


struct softpipe_texture
{
   struct pipe_texture base;

   unsigned long level_offset[PIPE_MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct pipe_buffer_handle *buffer;
   unsigned long buffer_size;
};


/** cast wrapper */
static INLINE struct softpipe_texture *
softpipe_texture(struct pipe_texture *pt)
{
   return (struct softpipe_texture *) pt;
}



extern void
softpipe_texture_create(struct pipe_context *pipe, struct pipe_texture **pt);

extern void
softpipe_texture_release(struct pipe_context *pipe, struct pipe_texture **pt);


#endif /* SP_TEXTURE */


