
#ifndef __NV50_TRANSFER_H__
#define __NV50_TRANSFER_H__

#include "pipe/p_state.h"

struct pipe_transfer *
nv50_miptree_transfer_new(struct pipe_context *pcontext,
                          struct pipe_resource *pt,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box);
void
nv50_miptree_transfer_del(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void *
nv50_miptree_transfer_map(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void
nv50_miptree_transfer_unmap(struct pipe_context *pcontext,
                            struct pipe_transfer *ptx);

struct nv50_m2mf_rect {
   struct nouveau_bo *bo;
   uint32_t base;
   unsigned domain;
   uint32_t pitch;
   uint32_t width;
   uint32_t x;
   uint32_t height;
   uint32_t y;
   uint16_t depth;
   uint16_t z;
   uint16_t tile_mode;
   uint16_t cpp;
};

#endif
