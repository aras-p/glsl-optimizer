#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "nouveau_context.h"
#include <nouveau_drm.h>
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"

#if NOUVEAU_DRM_HEADER_PATCHLEVEL != 11
#error nouveau_drm.h version does not match expected version
#endif

/*
PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
DRI_CONF_END;
static const GLuint __driNConfigOptions = 0;
*/

int nouveau_check_dri_drm_ddx(dri_version_t *dri, dri_version_t *drm, dri_version_t *ddx)
{
	static const dri_version_t ddx_expected = {0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL};
	static const dri_version_t dri_expected = {4, 0, 0};
	static const dri_version_t drm_expected = {0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL};

	assert(dri);
	assert(drm);
	assert(ddx);

	if (dri->major != dri_expected.major || dri->minor < dri_expected.minor)
	{
		NOUVEAU_ERR("Unexpected DRI version.\n");
		return 1;
	}
	if (drm->major != drm_expected.major || drm->minor < drm_expected.minor)
	{
		NOUVEAU_ERR("Unexpected DRM version.\n");
		return 1;
	}
	if (ddx->major != ddx_expected.major || ddx->minor < ddx_expected.minor)
	{
		NOUVEAU_ERR("Unexpected DDX version.\n");
		return 1;
	}

	return 0;
}

int
nouveau_screen_create(dri_screen_t *dri_screen, dri_framebuffer_t *dri_framebuf)
{
	struct nouveau_dri	*nv_dri = dri_framebuf->private;
	struct nouveau_screen	*nv_screen;
	int			ret;

	if (nouveau_check_dri_drm_ddx(&dri_screen->dri, &dri_screen->drm, &dri_screen->ddx))
		return 1;

	nv_screen = CALLOC_STRUCT(nouveau_screen);
	if (!nv_screen)
		return 1;
	nv_screen->dri_screen = dri_screen;
	dri_screen->private = (void*)nv_screen;

	/*
	driParseOptionInfo(&nv_screen->option_cache,
			   __driConfigOptions, __driNConfigOptions);
	*/

	if ((ret = nouveau_device_open_existing(&nv_screen->device, 0,
						dri_screen->fd, 0))) {
		NOUVEAU_ERR("Failed opening nouveau device: %d.\n", ret);
		return 1;
	}

	nv_screen->front_offset = nv_dri->front_offset;
	nv_screen->front_pitch  = nv_dri->front_pitch * (nv_dri->bpp / 8);
	nv_screen->front_cpp = nv_dri->bpp / 8;
	nv_screen->front_height = nv_dri->height;

	return 0;
}

void
nouveau_screen_destroy(dri_screen_t *dri_screen)
{
	struct nouveau_screen *nv_screen = dri_screen->private;

	FREE(nv_screen);
}
