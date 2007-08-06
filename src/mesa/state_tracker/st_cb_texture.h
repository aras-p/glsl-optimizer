#ifndef ST_CB_TEXTURE_H
#define ST_CB_TEXTURE_H


extern GLuint
st_finalize_mipmap_tree(GLcontext *ctx,
                        struct pipe_context *pipe, GLuint unit,
                        GLboolean *needFlush);


extern void
st_init_cb_texture( struct st_context *st );


extern void
st_destroy_cb_texture( struct st_context *st );


#endif /* ST_CB_TEXTURE_H */
