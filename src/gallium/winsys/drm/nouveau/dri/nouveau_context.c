#include <main/glheader.h>
#include <glapi/glthread.h>
#include <GL/internal/glcore.h>
#include <utils.h>

#include <state_tracker/st_public.h>
#include <state_tracker/st_context.h>
#include <state_tracker/drm_api.h>
#include <pipe/p_defines.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>

#include "nouveau_context.h"
#include "nouveau_screen.h"

#include "nouveau_drmif.h"

GLboolean
nouveau_context_create(const __GLcontextModes *glVis,
		       __DRIcontextPrivate *driContextPriv,
		       void *sharedContextPrivate)
{
	__DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
	struct nouveau_screen  *nv_screen = driScrnPriv->private;
	struct nouveau_context *nv;
	struct pipe_context *pipe;
	struct st_context *st_share = NULL;

	if (sharedContextPrivate)
		st_share = ((struct nouveau_context *)sharedContextPrivate)->st;

	nv = CALLOC_STRUCT(nouveau_context);
	if (!nv)
		return GL_FALSE;

	{
		struct nouveau_device_priv *nvdev =
			nouveau_device(nv_screen->device);

		nvdev->ctx  = driContextPriv->hHWContext;
		nvdev->lock = (drmLock *)&driScrnPriv->pSAREA->lock;
	}

	pipe = drm_api_hooks.create_context(nv_screen->pscreen);
	if (!pipe) {
		FREE(nv);
		return GL_FALSE;
	}
	pipe->priv = nv;

	driContextPriv->driverPrivate = nv;
	nv->dri_screen = driScrnPriv;

	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");

	nv->st = st_create_context(pipe, glVis, st_share);
	return GL_TRUE;
}

void
nouveau_context_destroy(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context *nv = driContextPriv->driverPrivate;

	assert(nv);

	st_finish(nv->st);
	st_destroy_context(nv->st);

	FREE(nv);
}

GLboolean
nouveau_context_bind(__DRIcontextPrivate *driContextPriv,
		     __DRIdrawablePrivate *driDrawPriv,
		     __DRIdrawablePrivate *driReadPriv)
{
	struct nouveau_context *nv;
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
	struct nouveau_context *nv = driContextPriv->driverPrivate;
	(void)nv;

	st_flush(nv->st, 0, NULL);
	return GL_TRUE;
}

