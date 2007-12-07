#ifndef SP_TEXTURE_H
#define SP_TEXTURE_H


struct pipe_context;
struct pipe_texture;


struct softpipe_texture
{
   struct pipe_texture base;

   /* Derived from the above:
    */
   unsigned pitch;
   unsigned depth_pitch;          /* per-image on i945? */
   unsigned total_height;

   unsigned nr_images[PIPE_MAX_TEXTURE_LEVELS];

   /* Explicitly store the offset of each image for each cube face or
    * depth value.  Pretty much have to accept that hardware formats
    * are going to be so diverse that there is no unified way to
    * compute the offsets of depth/cube images within a mipmap level,
    * so have to store them as a lookup table:
    */
   unsigned *image_offset[PIPE_MAX_TEXTURE_LEVELS];   /**< array [depth] of offsets */

   /* Includes image offset tables:
    */
   unsigned long level_offset[PIPE_MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct pipe_buffer_handle *buffer;
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


