#ifndef __NOUVEAU_OBJECT_H__
#define __NOUVEAU_OBJECT_H__

#include "nouveau_context.h"

#define ALLOW_MULTI_SUBCHANNEL

void nouveauObjectInit(nouveauContextPtr nmesa);

enum DMAObjects {
	Nv3D                    = 0x80000019,
	NvCtxSurf2D		= 0x80000020,
	NvImageBlit		= 0x80000021,
	NvDmaFB			= 0xD0FB0001,
	NvDmaAGP		= 0xD0AA0001
};

enum DMASubchannel {
	NvSubCtxSurf2D	= 0,
	NvSubImageBlit	= 1,
	NvSub3D		= 7,
};

extern void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle);
#endif
