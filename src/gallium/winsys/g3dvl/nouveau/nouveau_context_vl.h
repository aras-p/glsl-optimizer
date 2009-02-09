#ifndef __NOUVEAU_CONTEXT_VL_H__
#define __NOUVEAU_CONTEXT_VL_H__

#include <driclient.h>
#include <nouveau/nouveau_winsys.h>
#include <common/nouveau_context.h>

/*#include "xmlconfig.h"*/

struct nouveau_context_vl {
	struct nouveau_context		base;
	struct nouveau_screen_vl	*nv_screen;
	dri_context_t			*dri_context;
	dri_drawable_t			*dri_drawable;
	unsigned int			last_stamp;
	/*driOptionCache		dri_option_cache;*/
	drm_context_t			drm_context;
	drmLock				drm_lock;
};

extern int nouveau_context_create(dri_context_t *);
extern void nouveau_context_destroy(dri_context_t *);
extern int nouveau_context_bind(struct nouveau_context_vl *, dri_drawable_t *);
extern int nouveau_context_unbind(struct nouveau_context_vl *);

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
