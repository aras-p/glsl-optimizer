#include <pipe/p_defines.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_memory.h>
#include "nouveau_context.h"
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

static void
nouveau_channel_context_destroy(struct nouveau_channel_context *nvc)
{
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

	nvc->next_handle = 0x77000000;
	return nvc;
}

int
nouveau_context_init(struct nouveau_screen *nv_screen,
                     drm_context_t hHWContext, drmLock *sarea_lock,
                     struct nouveau_context *nv_share,
                     struct nouveau_context *nv)
{
	struct pipe_context *pipe = NULL;
	struct nouveau_channel_context *nvc = NULL;
	struct nouveau_device *dev = nv_screen->device;
	int i;

	switch (dev->chipset & 0xf0) {
	case 0x00:
		/* NV04 */
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

	nv->nv_screen  = nv_screen;

	{
		struct nouveau_device_priv *nvdev = nouveau_device(dev);

		nvdev->ctx  = hHWContext;
		nvdev->lock = sarea_lock;
	}

	/* Attempt to share a single channel between multiple contexts from
	 * a single process.
	 */
	nvc = nv_screen->nvc;
	if (!nvc && nv_share)
		nvc = nv_share->nvc;

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
			return 1;
		}
		nv_screen->nvc = nvc;
		pipe_reference_init(&nvc->reference, 1);
	}

	pipe_reference((struct pipe_reference**)&nv->nvc, &nvc->reference);

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
	if (!getenv("NOUVEAU_FORCE_SOFTPIPE")) {
		struct pipe_screen *pscreen;

		pipe = nouveau_pipe_create(nv);
		if (!pipe) {
			NOUVEAU_ERR("Couldn't create hw pipe\n");
			return 1;
		}
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
			return 1;
		}
	}

	{
		struct pipe_texture *fb_tex;
		struct pipe_surface *fb_surf;
		struct nouveau_pipe_buffer *fb_buf;
		enum pipe_format format;

		fb_buf = calloc(1, sizeof(struct nouveau_pipe_buffer));
		pipe_reference_init(&fb_buf->base.reference, 1);
		fb_buf->base.usage = PIPE_BUFFER_USAGE_PIXEL;

		nouveau_bo_fake(dev, nv_screen->front_offset, NOUVEAU_BO_VRAM,
				nv_screen->front_pitch*nv_screen->front_height,
				NULL, &fb_buf->bo);

		if (nv_screen->front_cpp == 4)
			format = PIPE_FORMAT_A8R8G8B8_UNORM;
		else
			format = PIPE_FORMAT_R5G6B5_UNORM;

		fb_surf = nouveau_surface_buffer_ref(nv, &fb_buf->base, format,
						     nv_screen->front_pitch /
						     nv_screen->front_cpp,
						     nv_screen->front_height,
						     nv_screen->front_pitch,
						     &fb_tex);

		nv->frontbuffer = fb_surf;
		nv->frontbuffer_texture = fb_tex;
	}

	pipe->priv = nv;
	return 0;
}

void
nouveau_context_cleanup(struct nouveau_context *nv)
{
	struct nouveau_channel_context *nvc = nv->nvc;

	assert(nv);

	if (nv->pctx_id >= 0) {
		nvc->pctx[nv->pctx_id] = NULL;
		if (pipe_reference((struct pipe_reference**)&nv->nvc, NULL)) {
			nouveau_channel_context_destroy(nvc);
			nv->nv_screen->nvc = NULL;
		}
	}

	/* XXX: Who cleans up the pipe? */
}

