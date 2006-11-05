
#include "nouveau_fifo.h"
#include "nouveau_object.h"


static GLboolean NVDmaCreateContextObject(nouveauContextPtr nmesa, int handle, int class, uint32_t flags,
			      uint32_t dma_in, uint32_t dma_out, uint32_t dma_notifier)
{
	drm_nouveau_object_init_t cto;
	int ret;

	cto.handle = handle;
	cto.class  = class;
	cto.flags  = flags;
	cto.dma0= dma_in;
	cto.dma1= dma_out;
	cto.dma_notifier = dma_notifier;
	ret = drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_OBJECT_INIT, &cto, sizeof(cto));

	return ret == 0;
}

static void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int handle, int subchannel)
{
	BEGIN_RING_SIZE(subchannel, 0, 1);
	OUT_RING(handle);
}

void nouveauObjectInit(nouveauContextPtr nmesa)
{
	NVDmaCreateContextObject(nmesa, Nv3D, nmesa->screen->card->class_3d, 0, 0, 0, 0);
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);
}



