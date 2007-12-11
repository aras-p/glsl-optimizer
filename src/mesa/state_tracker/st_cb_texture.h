#ifndef ST_CB_TEXTURE_H
#define ST_CB_TEXTURE_H


extern struct pipe_texture *
st_get_texobj_texture(struct gl_texture_object *texObj);


extern GLboolean
st_finalize_texture(GLcontext *ctx,
		    struct pipe_context *pipe, 
		    struct gl_texture_object *tObj,
		    GLboolean *needFlush);


extern void
st_init_texture_functions(struct dd_function_table *functions);


#endif /* ST_CB_TEXTURE_H */
