#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "nouveau/nouveau_winsys.h"
#include "nouveau_drmif.h"
#include "nouveau_dma.h"

struct nouveau_channel_context {
	struct pipe_screen *pscreen;
	int refcount;

	unsigned cur_pctx;
	unsigned nr_pctx;
	struct pipe_context **pctx;

	struct nouveau_channel  *channel;

	struct nouveau_notifier *sync_notifier;

	/* Common */
	struct nouveau_grobj    *NvM2MF;
	/* NV04-NV40 */
	struct nouveau_grobj    *NvCtxSurf2D;
	struct nouveau_grobj	*NvSwzSurf;
	struct nouveau_grobj    *NvImageBlit;
	struct nouveau_grobj    *NvGdiRect;
	struct nouveau_grobj	*NvSIFM;
	/* G80 */
	struct nouveau_grobj    *Nv2D;

	uint32_t                 next_handle;
	uint32_t                 next_subchannel;
	uint32_t                 next_sequence;
};

struct nouveau_context {
	int locked;
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

extern int nouveau_context_init(struct nouveau_screen *nv_screen,
                                drm_context_t hHWContext, drmLock *sarea_lock,
                                struct nouveau_context *nv_share,
                                struct nouveau_context *nv);
extern void nouveau_context_cleanup(struct nouveau_context *nv);

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

/* Must be provided by clients of common code */
extern void
nouveau_contended_lock(struct nouveau_context *nv);

#endif
