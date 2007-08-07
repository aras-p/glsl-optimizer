
#ifndef INTEL_TEX_LAYOUT_H
#define INTEL_TEX_LAYOUT_H

struct pipe_context;
struct pipe_mipmap_tree;


extern GLboolean
i915_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);

extern GLboolean
i945_miptree_layout(struct pipe_context *, struct pipe_mipmap_tree *);


#endif /* INTEL_TEX_LAYOUT_H */
