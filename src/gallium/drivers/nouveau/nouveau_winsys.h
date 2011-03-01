#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include "pipe/p_defines.h"

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_device.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_notifier.h"
#ifndef NOUVEAU_NVC0
#include "nouveau/nv04_pushbuf.h"
#endif

#ifndef NV04_PFIFO_MAX_PACKET_LEN
#define NV04_PFIFO_MAX_PACKET_LEN 2047
#endif

static INLINE uint32_t
nouveau_screen_transfer_flags(unsigned pipe)
{
	uint32_t flags = 0;

	if (pipe & PIPE_TRANSFER_READ)
		flags |= NOUVEAU_BO_RD;
	if (pipe & PIPE_TRANSFER_WRITE)
		flags |= NOUVEAU_BO_WR;
	if (pipe & PIPE_TRANSFER_DISCARD)
		flags |= NOUVEAU_BO_INVAL;
	if (pipe & PIPE_TRANSFER_UNSYNCHRONIZED)
		flags |= NOUVEAU_BO_NOSYNC;
	else if (pipe & PIPE_TRANSFER_DONTBLOCK)
		flags |= NOUVEAU_BO_NOWAIT;

	return flags;
}

extern struct pipe_screen *
nvfx_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_screen *
nvc0_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

#endif
