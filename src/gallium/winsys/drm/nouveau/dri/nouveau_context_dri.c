#include <main/glheader.h>
#include <glapi/glthread.h>
#include <GL/internal/glcore.h>
#include <utils.h>

#include <state_tracker/st_public.h>
#include <state_tracker/st_context.h>
#include <pipe/p_defines.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>

#include "../common/nouveau_winsys_pipe.h"
#include "../common/nouveau_dri.h"
#include "../common/nouveau_local.h"
#include "nouveau_context_dri.h"
#include "nouveau_screen_dri.h"

#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
	{ "bo", DEBUG_BO },
	{ NULL, 0 }
};
int __nouveau_debug = 0;
#endif

GLboolean
nouveau_context_create(const __GLcontextModes *glVis,
		       __DRIcontextPrivate *driContextPriv,
		       void *sharedContextPrivate)
{
	__DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
	struct nouveau_screen_dri  *nv_screen = driScrnPriv->private;
	struct nouveau_context_dri *nv = CALLOC_STRUCT(nouveau_context_dri);
	struct st_context *st_share = NULL;
	struct nouveau_context_dri *nv_share = NULL;
	struct pipe_context *pipe;

	if (sharedContextPrivate) {
		st_share = ((struct nouveau_context_dri *)sharedContextPrivate)->st;
		nv_share = st_share->pipe->priv;
	}

	if (nouveau_context_init(&nv_screen->base, driContextPriv->hHWContext,
	                         (drmLock *)&driScrnPriv->pSAREA->lock,
	                         nv_share, &nv->base)) {
		return GL_FALSE;
	}

	pipe = nv->base.nvc->pctx[nv->base.pctx_id];
	driContextPriv->driverPrivate = (void *)nv;
	//nv->nv_screen  = nv_screen;
	nv->dri_screen = driScrnPriv;

	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");
#ifdef DEBUG
	__nouveau_debug = driParseDebugString(getenv("NOUVEAU_DEBUG"),
					      debug_control);
#endif

	nv->st = st_create_context(pipe, glVis, st_share);
	return GL_TRUE;
}

void
nouveau_context_destroy(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context_dri *nv = driContextPriv->driverPrivate;

	assert(nv);

	st_finish(nv->st);
	st_destroy_context(nv->st);

	nouveau_context_cleanup(&nv->base);

	FREE(nv);
}

GLboolean
nouveau_context_bind(__DRIcontextPrivate *driContextPriv,
		     __DRIdrawablePrivate *driDrawPriv,
		     __DRIdrawablePrivate *driReadPriv)
{
	struct nouveau_context_dri *nv;
	struct nouveau_framebuffer *draw, *read;

	if (!driContextPriv) {
		st_make_current(NULL, NULL, NULL);
		return GL_TRUE;
	}

	nv = driContextPriv->driverPrivate;
	draw = driDrawPriv->driverPrivate;
	read = driReadPriv->driverPrivate;

	st_make_current(nv->st, draw->stfb, read->stfb);

	if ((nv->dri_drawable != driDrawPriv) ||
	    (nv->last_stamp != driDrawPriv->lastStamp)) {
		nv->dri_drawable = driDrawPriv;
		st_resize_framebuffer(draw->stfb, driDrawPriv->w,
				      driDrawPriv->h);
		nv->last_stamp = driDrawPriv->lastStamp;
	}

	if (driDrawPriv != driReadPriv) {
		st_resize_framebuffer(read->stfb, driReadPriv->w,
				      driReadPriv->h);
	}

	return GL_TRUE;
}

GLboolean
nouveau_context_unbind(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context_dri *nv = driContextPriv->driverPrivate;
	(void)nv;

	st_flush(nv->st, 0, NULL);
	return GL_TRUE;
}

