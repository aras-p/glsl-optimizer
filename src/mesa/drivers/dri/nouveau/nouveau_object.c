
#include "nouveau_fifo.h"
#include "nouveau_object.h"
#include "nouveau_reg.h"


GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa,
				     uint32_t handle, int class)
{
	struct drm_nouveau_grobj_alloc cto;
	int ret;

	cto.channel = nmesa->fifo.channel;
	cto.handle  = handle;
	cto.class   = class;
	ret = drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_GROBJ_ALLOC,
			      &cto, sizeof(cto));

	return ret == 0;
}

void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle)
{
	BEGIN_RING_SIZE(subchannel, 0, 1);
	OUT_RING(handle);
}

void nouveauObjectInit(nouveauContextPtr nmesa)
{
#ifdef NOUVEAU_RING_DEBUG
	return;
#endif

	nouveauCreateContextObject(nmesa, Nv3D, nmesa->screen->card->class_3d);
	if (nmesa->screen->card->type>=NV_10) {
		nouveauCreateContextObject(nmesa, NvCtxSurf2D, NV10_CONTEXT_SURFACES_2D);
	} else {
		nouveauCreateContextObject(nmesa, NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D);
		nouveauCreateContextObject(nmesa, NvCtxSurf3D, NV04_CONTEXT_SURFACES_3D);
	}
	if (nmesa->screen->card->type>=NV_11) {
		nouveauCreateContextObject(nmesa, NvImageBlit, NV10_IMAGE_BLIT);
	} else {
		nouveauCreateContextObject(nmesa, NvImageBlit, NV_IMAGE_BLIT);
	}
	nouveauCreateContextObject(nmesa, NvMemFormat, NV_MEMORY_TO_MEMORY_FORMAT);

#ifdef ALLOW_MULTI_SUBCHANNEL
	nouveauObjectOnSubchannel(nmesa, NvSubCtxSurf2D, NvCtxSurf2D);
	BEGIN_RING_SIZE(NvSubCtxSurf2D, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, 2);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);

	nouveauObjectOnSubchannel(nmesa, NvSubImageBlit, NvImageBlit);
	BEGIN_RING_SIZE(NvSubImageBlit, NV10_IMAGE_BLIT_SET_CONTEXT_SURFACES_2D, 1);
	OUT_RING(NvCtxSurf2D);
	BEGIN_RING_SIZE(NvSubImageBlit, NV10_IMAGE_BLIT_SET_OPERATION, 1);
	OUT_RING(3); /* SRCCOPY */

	nouveauObjectOnSubchannel(nmesa, NvSubMemFormat, NvMemFormat);
#endif

	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);
}



