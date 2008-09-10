#include "main/glheader.h"
#include "glapi/glthread.h"
#include <GL/internal/glcore.h>
#include "utils.h"

#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"

#include "nouveau_context.h"
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
	{ "bo", DEBUG_BO },
	{ NULL, 0 }
};
int __nouveau_debug = 0;
#endif

static void
nouveau_channel_context_destroy(struct nouveau_channel_context *nvc)
{
	nouveau_grobj_free(&nvc->NvCtxSurf2D);
	nouveau_grobj_free(&nvc->NvImageBlit);
	nouveau_grobj_free(&nvc->NvGdiRect);
	nouveau_grobj_free(&nvc->NvM2MF);
	nouveau_grobj_free(&nvc->Nv2D);
	nouveau_grobj_free(&nvc->NvSwzSurf);
	nouveau_grobj_free(&nvc->NvSIFM);

	nouveau_notifier_free(&nvc->sync_notifier);

	nouveau_channel_free(&nvc->channel);

	FREE(nvc);
}

static struct nouveau_channel_context *
nouveau_channel_context_create(struct nouveau_device *dev)
{
	struct nouveau_channel_context *nvc;
	int ret;

	nvc = CALLOC_STRUCT(nouveau_channel_context);
	if (!nvc)
		return NULL;

	if ((ret = nouveau_channel_alloc(dev, 0x8003d001, 0x8003d002,
					 &nvc->channel))) {
		NOUVEAU_ERR("Error creating GPU channel: %d\n", ret);
		nouveau_channel_context_destroy(nvc);
		return NULL;
	}

	nvc->next_handle = 0x80000000;

	if ((ret = nouveau_notifier_alloc(nvc->channel, nvc->next_handle++, 1,
					  &nvc->sync_notifier))) {
		NOUVEAU_ERR("Error creating channel sync notifier: %d\n", ret);
		nouveau_channel_context_destroy(nvc);
		return NULL;
	}

	switch (dev->chipset & 0xf0) {
	case 0x50:
	case 0x80:
	case 0x90:
		ret = nouveau_surface_channel_create_nv50(nvc);
		break;
	default:
		ret = nouveau_surface_channel_create_nv04(nvc);
		break;
	}

	if (ret) {
		NOUVEAU_ERR("Error initialising surface objects: %d\n", ret);
		nouveau_channel_context_destroy(nvc);
		return NULL;
	}

	return nvc;
}

GLboolean
nouveau_context_create(const __GLcontextModes *glVis,
		       __DRIcontextPrivate *driContextPriv,
		       void *sharedContextPrivate)
{
	__DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
	struct nouveau_screen  *nv_screen = driScrnPriv->private;
	struct nouveau_context *nv = CALLOC_STRUCT(nouveau_context);
	struct pipe_context *pipe = NULL;
	struct st_context *st_share = NULL;
	struct nouveau_channel_context *nvc = NULL;
	struct nouveau_device *dev = nv_screen->device;
	int i;

	if (sharedContextPrivate) {
		st_share = ((struct nouveau_context *)sharedContextPrivate)->st;
	}

	switch (dev->chipset & 0xf0) {
	case 0x10:
	case 0x20:
		/* NV10 */
	case 0x30:
		/* NV30 */
	case 0x40:
	case 0x60:
		/* NV40 */
	case 0x50:
	case 0x80:
	case 0x90:
		/* G80 */
		break;
	default:
		NOUVEAU_ERR("Unsupported chipset: NV%02x\n", dev->chipset);
		return GL_FALSE;
	}

	driContextPriv->driverPrivate = (void *)nv;
	nv->nv_screen  = nv_screen;
	nv->dri_screen = driScrnPriv;

	{
		struct nouveau_device_priv *nvdev = nouveau_device(dev);

		nvdev->ctx  = driContextPriv->hHWContext;
		nvdev->lock = (drmLock *)&driScrnPriv->pSAREA->lock;
	}

	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");
#ifdef DEBUG
	__nouveau_debug = driParseDebugString(getenv("NOUVEAU_DEBUG"),
					      debug_control);
#endif

	/*XXX: Hack up a fake region and buffer object for front buffer.
	 *     This will go away with TTM, replaced with a simple reference
	 *     of the front buffer handle passed to us by the DDX.
	 */
	{
		struct pipe_surface *fb_surf;
		struct nouveau_pipe_buffer *fb_buf;
		struct nouveau_bo_priv *fb_bo;

		fb_bo = calloc(1, sizeof(struct nouveau_bo_priv));
		fb_bo->drm.offset = nv_screen->front_offset;
		fb_bo->drm.flags = NOUVEAU_MEM_FB;
		fb_bo->drm.size = nv_screen->front_pitch * 
				  nv_screen->front_height;
		fb_bo->refcount = 1;
		fb_bo->base.flags = NOUVEAU_BO_PIN | NOUVEAU_BO_VRAM;
		fb_bo->base.offset = fb_bo->drm.offset;
		fb_bo->base.handle = (unsigned long)fb_bo;
		fb_bo->base.size = fb_bo->drm.size;
		fb_bo->base.device = nv_screen->device;

		fb_buf = calloc(1, sizeof(struct nouveau_pipe_buffer));
		fb_buf->bo = &fb_bo->base;

		fb_surf = calloc(1, sizeof(struct pipe_surface));
		if (nv_screen->front_cpp == 2)
			fb_surf->format = PIPE_FORMAT_R5G6B5_UNORM;
		else
			fb_surf->format = PIPE_FORMAT_A8R8G8B8_UNORM;
		pf_get_block(fb_surf->format, &fb_surf->block);
		fb_surf->width = nv_screen->front_pitch / nv_screen->front_cpp;
		fb_surf->height = nv_screen->front_height;
		fb_surf->stride = fb_surf->width * fb_surf->block.size;
		fb_surf->refcount = 1;
		fb_surf->buffer = &fb_buf->base;

		nv->frontbuffer = fb_surf;
	}

	/* Attempt to share a single channel between multiple contexts from
	 * a single process.
	 */
	nvc = nv_screen->nvc;
	if (!nvc && st_share) {
		struct nouveau_context *snv = st_share->pipe->priv;
		if (snv) {
			nvc = snv->nvc;
		}
	}

	/*XXX: temporary - disable multi-context/single-channel on pre-NV4x */
	switch (dev->chipset & 0xf0) {
	case 0x40:
	case 0x60:
		/* NV40 class */
	case 0x50:
	case 0x80:
	case 0x90:
		/* G80 class */
		break;
	default:
		nvc = NULL;
		break;
	}

	if (!nvc) {
		nvc = nouveau_channel_context_create(dev);
		if (!nvc) {
			NOUVEAU_ERR("Failed initialising GPU context\n");
			return GL_FALSE;
		}
		nv_screen->nvc = nvc;
	}

	nvc->refcount++;
	nv->nvc = nvc;

	/* Find a free slot for a pipe context, allocate a new one if needed */
	nv->pctx_id = -1;
	for (i = 0; i < nvc->nr_pctx; i++) {
		if (nvc->pctx[i] == NULL) {
			nv->pctx_id = i;
			break;
		}
	}

	if (nv->pctx_id < 0) {
		nv->pctx_id = nvc->nr_pctx++;
		nvc->pctx =
			realloc(nvc->pctx,
				sizeof(struct pipe_context *) * nvc->nr_pctx);
	}

	/* Create pipe */
	switch (dev->chipset & 0xf0) {
	case 0x50:
	case 0x80:
	case 0x90:
		if (nouveau_surface_init_nv50(nv))
			return GL_FALSE;
		break;
	default:
		if (nouveau_surface_init_nv04(nv))
			return GL_FALSE;
		break;
	}

	if (!getenv("NOUVEAU_FORCE_SOFTPIPE")) {
		struct pipe_screen *pscreen;

		pipe = nouveau_pipe_create(nv);
		if (!pipe)
			NOUVEAU_ERR("Couldn't create hw pipe\n");
		pscreen = nvc->pscreen;

		nv->cap.hw_vertex_buffer =
			pscreen->get_param(pscreen, NOUVEAU_CAP_HW_VTXBUF);
		nv->cap.hw_index_buffer =
			pscreen->get_param(pscreen, NOUVEAU_CAP_HW_IDXBUF);
	}

	if (!pipe) {
		NOUVEAU_MSG("Using softpipe\n");
		pipe = nouveau_create_softpipe(nv);
		if (!pipe) {
			NOUVEAU_ERR("Error creating pipe, bailing\n");
			return GL_FALSE;
		}
	}

	pipe->priv = nv;
	nv->st = st_create_context(pipe, glVis, st_share);
	return GL_TRUE;
}

void
nouveau_context_destroy(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context *nv = driContextPriv->driverPrivate;
	struct nouveau_channel_context *nvc = nv->nvc;

	assert(nv);

	st_finish(nv->st);
	st_destroy_context(nv->st);

	if (nv->pctx_id >= 0) {
		nvc->pctx[nv->pctx_id] = NULL;
		if (--nvc->refcount <= 0) {
			nouveau_channel_context_destroy(nvc);
			nv->nv_screen->nvc = NULL;
		}
	}

	free(nv);
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

