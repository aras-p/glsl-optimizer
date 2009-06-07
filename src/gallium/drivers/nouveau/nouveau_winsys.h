#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include "pipe/internal/p_winsys_screen.h"
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

extern struct pipe_screen *
nv04_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv04_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv10_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv10_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv20_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv20_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv30_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv30_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv40_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv40_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *);

extern struct pipe_context *
nv50_create(struct pipe_screen *, unsigned pctx_id);

#endif
