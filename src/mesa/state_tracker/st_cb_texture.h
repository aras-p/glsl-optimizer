#ifndef ST_CB_TEXTURE_H
#define ST_CB_TEXTURE_H


extern GLuint
st_finalize_mipmap_tree(GLcontext *ctx,
                        struct pipe_context *pipe, GLuint unit,
                        GLboolean *needFlush);


extern void
st_init_texture_functions(struct dd_function_table *functions);


#endif /* ST_CB_TEXTURE_H */
