#ifndef __NOUVEAU_OBJECT_H__
#define __NOUVEAU_OBJECT_H__

#include "nouveau_context.h"

void nouveauObjectInit(nouveauContextPtr nmesa);

enum DMAObjects {
	Nv3D                    = 0x80000019,
	NvCtxSurf2D		= 0x80000020,
	NvImageBlit		= 0x80000021,
	NvMemFormat		= 0x80000022,
	NvCtxSurf3D		= 0x80000023,
	NvGdiRectText		= 0x80000024,
	NvPattern		= 0x80000025,
	NvRasterOp		= 0x80000026,
	NvDmaFB			= 0xD0FB0001,
	NvDmaTT			= 0xD0AA0001,
	NvSyncNotify		= 0xD0000001,
	NvQueryNotify		= 0xD0000002
};

enum DMASubchannel {
	NvSubCtxSurf2D	= 0,
	NvSubImageBlit	= 1,
	NvSubMemFormat	= 2,
	NvSubCtxSurf3D	= 3,
	NvSubGdiRectText= 4,
	NvSubPattern	= 5,
	NvSubRasterOp	= 6,
	NvSub3D		= 7,
};

extern void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle);

extern GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa,
      					    uint32_t handle, int class);

#endif
