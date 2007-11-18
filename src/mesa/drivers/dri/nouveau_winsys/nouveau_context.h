#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "glheader.h"
#include "context.h"

#include "dri_util.h"
#include "xmlconfig.h"

#include "pipe/nouveau/nouveau_winsys.h"
#include "nouveau_device.h"
#include "nouveau_drmif.h"
#include "nouveau_dma.h"

struct nouveau_framebuffer {
	struct st_framebuffer *stfb;
};

struct nouveau_context {
	struct st_context *st;

	/* Misc HW info */
	uint64_t chipset;

	/* DRI stuff */
	__DRIscreenPrivate    *dri_screen;
	__DRIdrawablePrivate  *dri_drawable;
	unsigned int           last_stamp;
	driOptionCache         dri_option_cache;
	drm_context_t          drm_context;
	drmLock                drm_lock;
	GLboolean              locked;
	struct nouveau_screen *nv_screen;

	/* Bufmgr */
	struct {
		struct nouveau_channel *channel;
		struct nouveau_notifier *notify;
		struct nouveau_grobj *m2mf;
		uint32_t m2mf_src_ctxdma;
		uint32_t m2mf_dst_ctxdma;
		uint32_t next_sequence;
	} bo;

	/* Relocations */
	struct nouveau_bo *reloc_head;

	/* Hardware context */
	struct nouveau_channel  *channel;
	struct nouveau_notifier *sync_notifier;
	struct nouveau_grobj    *NvNull;
	struct nouveau_grobj    *NvCtxSurf2D;
	struct nouveau_grobj    *NvImageBlit;
	struct nouveau_grobj    *NvGdiRect;
	struct nouveau_grobj    *NvM2MF;
	uint32_t                 next_handle;
	uint32_t                 next_sequence;

	/* pipe_region accel */
	int (*region_display)(void);
	int (*region_copy)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, struct pipe_region *,
			   unsigned, unsigned, unsigned, unsigned, unsigned);
	int (*region_fill)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, unsigned, unsigned,
			   unsigned);
	int (*region_data)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, const void *,
			   unsigned, unsigned, unsigned, unsigned, unsigned);
};

static INLINE struct nouveau_context *
nouveau_context(GLcontext *ctx)
{
	return (struct nouveau_context *)ctx;
}

extern GLboolean nouveau_context_create(const __GLcontextModes *,
					__DRIcontextPrivate *, void *);
extern void nouveau_context_destroy(__DRIcontextPrivate *);
extern GLboolean nouveau_context_bind(__DRIcontextPrivate *,
				      __DRIdrawablePrivate *draw,
				      __DRIdrawablePrivate *read);
extern GLboolean nouveau_context_unbind(__DRIcontextPrivate *);

#ifdef DEBUG
extern int __nouveau_debug;

#define DEBUG_BO (1 << 0)

#define DBG(flag, ...) do {                   \
	if (__nouveau_debug & (DEBUG_##flag)) \
		NOUVEAU_ERR(__VA_ARGS__);     \
} while(0)
#else
#define DBG(flag, ...)
#endif

extern void LOCK_HARDWARE(struct nouveau_context *);
extern void UNLOCK_HARDWARE(struct nouveau_context *);

extern int nouveau_region_init_nv04(struct nouveau_context *);
extern int nouveau_region_init_nv50(struct nouveau_context *);

#endif
