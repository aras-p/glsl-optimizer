
#ifndef __NVC0_TRANSFER_H__
#define __NVC0_TRANSFER_H__

#include "pipe/p_state.h"

struct pipe_transfer *
nvc0_miptree_transfer_new(struct pipe_context *pcontext,
                          struct pipe_resource *pt,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box);
void
nvc0_miptree_transfer_del(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void *
nvc0_miptree_transfer_map(struct pipe_context *pcontext,
                          struct pipe_transfer *ptx);
void
nvc0_miptree_transfer_unmap(struct pipe_context *pcontext,
                            struct pipe_transfer *ptx);

struct nvc0_m2mf_rect {
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

void
nvc0_m2mf_transfer_rect(struct pipe_screen *pscreen,
                        const struct nvc0_m2mf_rect *dst,
                        const struct nvc0_m2mf_rect *src,
                        uint32_t nblocksx, uint32_t nblocksy);

#endif
