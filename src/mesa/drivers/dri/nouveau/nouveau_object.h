#ifndef __NOUVEAU_OBJECT_H__
#define __NOUVEAU_OBJECT_H__

#include "nouveau_context.h"

//#define ALLOW_MULTI_SUBCHANNEL

void nouveauObjectInit(nouveauContextPtr nmesa);

enum DMAObjects {
	Nv3D                    = 0x80000019,
	NvCtxSurf2D		= 0x80000020,
	NvImageBlit		= 0x80000021,
	NvDmaFB			= 0xD0FB0001,
	NvDmaAGP		= 0xD0AA0001,
	NvSyncNotify		= 0xD0000001
};

enum DMASubchannel {
	NvSubCtxSurf2D	= 0,
	NvSubImageBlit	= 1,
	NvSub3D		= 7,
};

extern void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle);

extern GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa,
      					    int handle, int class,
					    uint32_t flags,
					    uint32_t dma_in,
					    uint32_t dma_out,
					    uint32_t dma_notifier);
extern GLboolean nouveauCreateDmaObject(nouveauContextPtr nmesa,
      					uint32_t handle,
					uint32_t offset,
					uint32_t size,
					int      target,
					int      access);
#endif
