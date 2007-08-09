
#ifndef INTEL_SURFACE_H
#define INTEL_SURFACE_H


extern struct pipe_surface *
intel_new_surface(struct pipe_context *pipe, GLuint pipeFormat);


extern const GLuint *
intel_supported_formats(struct pipe_context *pipe, GLuint *numFormats);


#endif /* INTEL_SURFACE_H */
