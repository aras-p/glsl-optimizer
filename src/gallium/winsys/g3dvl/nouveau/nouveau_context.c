#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"

#include "nouveau_context.h"
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

/*
#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
	{ "bo", DEBUG_BO },
	{ NULL, 0 }
};
int __nouveau_debug = 0;
#endif
*/

/*
 * TODO: Re-examine dri_screen, dri_context, nouveau_screen, nouveau_context
 * relationships, seems like there is a lot of room for simplification there.
 */

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

int
nouveau_context_create(dri_context_t *dri_context)
{
	dri_screen_t			*dri_screen = dri_context->dri_screen;
	struct nouveau_screen 		*nv_screen = dri_screen->private;
	struct nouveau_context		*nv = CALLOC_STRUCT(nouveau_context);
	struct pipe_context		*pipe = NULL;
	struct nouveau_channel_context	*nvc = NULL;
	struct nouveau_device		*dev = nv_screen->device;
	int				i;

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
		return 1;
	}

	dri_context->private = (void*)nv;
	nv->dri_context = dri_context;
	nv->nv_screen  = nv_screen;

	{
		struct nouveau_device_priv *nvdev = nouveau_device(dev);

		nvdev->ctx  = dri_context->drm_context;
		nvdev->lock = (drmLock*)&dri_screen->sarea->lock;
	}

	/*
	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");
#ifdef DEBUG
	__nouveau_debug = driParseDebugString(getenv("NOUVEAU_DEBUG"),
					      debug_control);
#endif
	*/

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

	nvc = nv_screen->nvc;

	if (!nvc) {
		nvc = nouveau_channel_context_create(dev);
		if (!nvc) {
			NOUVEAU_ERR("Failed initialising GPU context\n");
			return 1;
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
			return 1;
		break;
	default:
		if (nouveau_surface_init_nv04(nv))
			return 1;
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

	/* XXX: nouveau_winsys_softpipe needs a mesa header removed before we can compile it. */
	/*
	if (!pipe) {
		NOUVEAU_MSG("Using softpipe\n");
		pipe = nouveau_create_softpipe(nv);
		if (!pipe) {
			NOUVEAU_ERR("Error creating pipe, bailing\n");
			return 1;
		}
	}
	*/
	if (!pipe) {
		NOUVEAU_ERR("Error creating pipe, bailing\n");
		return 1;
	}

	pipe->priv = nv;

	return 0;
}

void
nouveau_context_destroy(dri_context_t *dri_context)
{
	struct nouveau_context *nv = dri_context->private;
	struct nouveau_channel_context *nvc = nv->nvc;

	assert(nv);

	if (nv->pctx_id >= 0) {
		nvc->pctx[nv->pctx_id] = NULL;
		if (--nvc->refcount <= 0) {
			nouveau_channel_context_destroy(nvc);
			nv->nv_screen->nvc = NULL;
		}
	}

	free(nv);
}

int
nouveau_context_bind(struct nouveau_context *nv, dri_drawable_t *dri_drawable)
{
	assert(nv);
	assert(dri_drawable);

	if (nv->dri_drawable != dri_drawable)
	{
		nv->dri_drawable = dri_drawable;
		dri_drawable->private = nv;
	}

	return 0;
}

int
nouveau_context_unbind(struct nouveau_context *nv)
{
	assert(nv);

	nv->dri_drawable = NULL;

	return 0;
}

/* Show starts here */

int bind_pipe_drawable(struct pipe_context *pipe, Drawable drawable)
{
	struct nouveau_context	*nv;
	dri_drawable_t		*dri_drawable;

	nv = pipe->priv;

	driCreateDrawable(nv->nv_screen->dri_screen, drawable, &dri_drawable);

	nouveau_context_bind(nv, dri_drawable);

	return 0;
}

int unbind_pipe_drawable(struct pipe_context *pipe)
{
	nouveau_context_unbind(pipe->priv);

	return 0;
}

struct pipe_context* create_pipe_context(Display *display, int screen)
{
	dri_screen_t		*dri_screen;
	dri_framebuffer_t	dri_framebuf;
	dri_context_t		*dri_context;
	struct nouveau_context	*nv;

	driCreateScreen(display, screen, &dri_screen, &dri_framebuf);
	driCreateContext(dri_screen, XDefaultVisual(display, screen), &dri_context);

	nouveau_screen_create(dri_screen, &dri_framebuf);
	nouveau_context_create(dri_context);

	nv = dri_context->private;

	return nv->nvc->pctx[nv->pctx_id];
}

int destroy_pipe_context(struct pipe_context *pipe)
{
	struct pipe_screen	*screen;
	struct pipe_winsys	*winsys;
	struct nouveau_context	*nv;
	dri_screen_t		*dri_screen;
	dri_context_t		*dri_context;

	assert(pipe);

	screen = pipe->screen;
	winsys = pipe->winsys;
	nv = pipe->priv;
	dri_context = nv->dri_context;
	dri_screen = dri_context->dri_screen;

	pipe->destroy(pipe);
	screen->destroy(screen);
	free(winsys);

	nouveau_context_destroy(dri_context);
	nouveau_screen_destroy(dri_screen);
	driDestroyContext(dri_context);
	driDestroyScreen(dri_screen);

	return 0;
}
