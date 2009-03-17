#ifndef __NOUVEAU_CONTEXT_DRI_H__
#define __NOUVEAU_CONTEXT_DRI_H__

#include <dri_util.h>
#include <xmlconfig.h>

#include "nouveau/nouveau_winsys.h"

#define NOUVEAU_ERR(fmt, args...) debug_printf("%s: "fmt, __func__, ##args)

struct nouveau_framebuffer {
	struct st_framebuffer *stfb;
};

struct nouveau_context {
	struct st_context *st;

	/* DRI stuff */
	__DRIscreenPrivate    *dri_screen;
	__DRIdrawablePrivate  *dri_drawable;
	unsigned int           last_stamp;
	driOptionCache         dri_option_cache;
	drm_context_t          drm_context;
	drmLock                drm_lock;
	int                    locked;
};

extern GLboolean nouveau_context_create(const __GLcontextModes *,
					__DRIcontextPrivate *, void *);
extern void nouveau_context_destroy(__DRIcontextPrivate *);
extern GLboolean nouveau_context_bind(__DRIcontextPrivate *,
				      __DRIdrawablePrivate *draw,
				      __DRIdrawablePrivate *read);
extern GLboolean nouveau_context_unbind(__DRIcontextPrivate *);

extern void nouveau_contended_lock(struct nouveau_context *nv);
extern void LOCK_HARDWARE(struct nouveau_context *nv);
extern void UNLOCK_HARDWARE(struct nouveau_context *nv);

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

#endif
