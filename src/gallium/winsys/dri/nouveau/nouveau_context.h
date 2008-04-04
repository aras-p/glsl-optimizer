#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "dri_util.h"
#include "xmlconfig.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau_device.h"
#include "nouveau_drmif.h"
#include "nouveau_dma.h"

struct nouveau_framebuffer {
	struct st_framebuffer *stfb;
};

struct nouveau_channel_context {
	struct pipe_screen *pscreen;
	int refcount;

	unsigned cur_pctx;
	unsigned nr_pctx;
	struct pipe_context **pctx;

	unsigned chipset;

	struct nouveau_channel  *channel;

	struct nouveau_notifier *sync_notifier;

	struct nouveau_grobj    *NvNull;
	struct nouveau_grobj    *NvCtxSurf2D;
	struct nouveau_grobj    *NvImageBlit;
	struct nouveau_grobj    *NvGdiRect;
	struct nouveau_grobj    *NvM2MF;
	struct nouveau_grobj    *Nv2D;

	uint32_t                 next_handle;
	uint32_t                 next_subchannel;
	uint32_t                 next_sequence;
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
	struct pipe_surface *frontbuffer;

	struct {
		int hw_vertex_buffer;
		int hw_index_buffer;
	} cap;

	/* Hardware context */
	struct nouveau_channel_context *nvc;
	int pctx_id;

	/* pipe_surface accel */
	struct pipe_surface *surf_src, *surf_dst;
	unsigned surf_src_offset, surf_dst_offset;
	int  (*surface_copy_prep)(struct nouveau_context *,
				  struct pipe_surface *dst,
				  struct pipe_surface *src);
	void (*surface_copy)(struct nouveau_context *, unsigned dx, unsigned dy,
			     unsigned sx, unsigned sy, unsigned w, unsigned h);
	void (*surface_copy_done)(struct nouveau_context *);
	int (*surface_fill)(struct nouveau_context *, struct pipe_surface *,
			    unsigned, unsigned, unsigned, unsigned, unsigned);
};

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

extern int
nouveau_surface_channel_create_nv04(struct nouveau_channel_context *);
extern int
nouveau_surface_channel_create_nv50(struct nouveau_channel_context *);
extern int nouveau_surface_init_nv04(struct nouveau_context *);
extern int nouveau_surface_init_nv50(struct nouveau_context *);

extern uint32_t *nouveau_pipe_dma_beginp(struct nouveau_grobj *, int, int);
extern void nouveau_pipe_dma_kickoff(struct nouveau_channel *);

#endif
