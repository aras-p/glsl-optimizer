#ifndef SP_TEX_LAYOUT_H
#define SP_TEX_LAYOUT_H


struct pipe_context;
struct pipe_mipmap_tree;


extern boolean
softpipe_mipmap_tree_layout(struct pipe_context *pipe,
                            struct pipe_mipmap_tree *mt);


#endif /* SP_TEX_LAYOUT_H */


