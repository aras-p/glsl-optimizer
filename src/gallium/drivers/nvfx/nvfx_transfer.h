
#ifndef NVFX_TRANSFER_H
#define NVFX_TRANSFER_H

#include "util/u_transfer.h"
#include "pipe/p_state.h"


struct pipe_transfer *
nvfx_transfer_new(struct pipe_context *pcontext,
			  struct pipe_resource *pt,
			  unsigned level,
			  unsigned usage,
			  const struct pipe_box *box);

void *
nvfx_transfer_map(struct pipe_context *pcontext,
			  struct pipe_transfer *ptx);
void
nvfx_transfer_unmap(struct pipe_context *pcontext,
			    struct pipe_transfer *ptx);


#endif
