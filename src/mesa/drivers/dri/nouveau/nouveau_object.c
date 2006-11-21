
#include "nouveau_fifo.h"
#include "nouveau_object.h"


static GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa, int handle, int class, uint32_t flags, uint32_t dma_in, uint32_t dma_out, uint32_t dma_notifier)
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

static GLboolean nouveauCreateDmaObject(nouveauContextPtr nmesa,
      					uint32_t handle,
      					uint32_t offset,
					uint32_t size,
					int	 target,
					int	 access)
{
	drm_nouveau_dma_object_init_t dma;
	int ret;

	dma.handle = handle;
	dma.target = target;
	dma.access = access;
	dma.offset = offset;
	dma.handle = handle;
	ret = drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_DMA_OBJECT_INIT,
				  &dma, sizeof(dma));
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

	nouveauCreateContextObject(nmesa, Nv3D, nmesa->screen->card->class_3d, 0, 0, 0, 0);
	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);
/* We need to know vram size.. */
#if 0	
	nouveauCreateDmaObject( nmesa, NvDmaFB,
				0, (256*1024*1024),
				0 /*NV_DMA_TARGET_FB*/, 0 /*NV_DMA_ACCESS_RW*/);
#endif
}



