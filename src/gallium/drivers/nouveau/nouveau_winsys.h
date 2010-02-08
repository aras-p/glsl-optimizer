#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include "util/u_simple_screen.h"
#include "pipe/p_defines.h"

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_class.h"
#include "nouveau/nouveau_device.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_notifier.h"
#include "nouveau/nouveau_resource.h"
#include "nouveau/nouveau_pushbuf.h"

#define NOUVEAU_CAP_HW_VTXBUF (0xbeef0000)
#define NOUVEAU_CAP_HW_IDXBUF (0xbeef0001)

#define NOUVEAU_TEXTURE_USAGE_LINEAR (1 << 16)

#define NOUVEAU_BUFFER_USAGE_TEXTURE  (1 << 16)
#define NOUVEAU_BUFFER_USAGE_ZETA     (1 << 17)
#define NOUVEAU_BUFFER_USAGE_TRANSFER (1 << 18)

/* use along with GPU_WRITE for 2D-only writes */
#define NOUVEAU_BUFFER_USAGE_NO_RENDER (1 << 19)

extern struct pipe_screen *
nv30_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_screen *
nv40_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

#endif
