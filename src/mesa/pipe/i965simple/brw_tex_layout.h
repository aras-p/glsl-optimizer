#ifndef BRW_TEX_LAYOUT_H
#define BRW_TEX_LAYOUT_H

#include "pipe/p_compiler.h"

struct pipe_context;
struct pipe_texture;

extern void
brw_texture_create(struct pipe_context *pipe, struct pipe_texture **pt);

extern void
brw_texture_release(struct pipe_context *pipe, struct pipe_texture **pt);

#endif
