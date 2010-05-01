
#ifndef NV50_TRANSFER_H
#define NV50_TRANSFER_H

#include "pipe/p_state.h"


struct pipe_transfer *
nv50_miptree_transfer_new(struct pipe_context *pcontext,
			  struct pipe_resource *pt,
			  struct pipe_subresource sr,
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

extern void
nv50_upload_sifc(struct nv50_context *nv50,
		 struct nouveau_bo *bo, unsigned dst_offset, unsigned reloc,
		 unsigned dst_format, int dst_w, int dst_h, int dst_pitch,
		 void *src, unsigned src_format, int src_pitch,
		 int x, int y, int w, int h, int cpp);

#endif
