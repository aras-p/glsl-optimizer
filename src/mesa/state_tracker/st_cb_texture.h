#ifndef ST_CB_TEXTURE_H
#define ST_CB_TEXTURE_H


extern struct pipe_mipmap_tree *
st_get_texobj_mipmap_tree(struct gl_texture_object *texObj);


extern GLuint
st_finalize_mipmap_tree(GLcontext *ctx,
                        struct pipe_context *pipe, GLuint unit,
                        GLboolean *needFlush);


extern void
st_init_texture_functions(struct dd_function_table *functions);


#endif /* ST_CB_TEXTURE_H */
