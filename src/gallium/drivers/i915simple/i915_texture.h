
#ifndef I915_TEXTURE_H
#define I915_TEXTURE_H

struct pipe_context;
struct pipe_texture;


struct pipe_texture *
i915_texture_create(struct pipe_context *pipe,
                    const struct pipe_texture *templat);

extern void
i915_texture_release(struct pipe_context *pipe, struct pipe_texture **pt);


#endif /* I915_TEXTURE_H */
