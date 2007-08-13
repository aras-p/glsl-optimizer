
#ifndef I915_TEX_LAYOUT_H
#define I915_TEX_LAYOUT_H

struct pipe_context;
struct pipe_mipmap_tree;


extern boolean
i915_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);

extern boolean
i945_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);


#endif /* I915_TEX_LAYOUT_H */
