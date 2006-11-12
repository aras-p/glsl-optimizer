#ifndef __NOUVEAU_OBJECT_H__
#define __NOUVEAU_OBJECT_H__

#include "nouveau_context.h"

void nouveauObjectInit(nouveauContextPtr nmesa);

enum DMAObjects {
	Nv3D                    = 0x80000019,
};

enum DMASubchannel {
	NvSub3D		= 1,
};

#endif
