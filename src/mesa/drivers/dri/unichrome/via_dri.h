#ifndef _VIA_DRI_
#define _VIA_DRI_

#include "xf86drm.h"

#define VIA_MAX_DRAWABLES 256

#define VIA_VERSION_MAJOR		4
#define VIA_VERSION_MINOR		1

typedef struct {
    int CtxOwner;
} VIASAREAPriv;

typedef struct {
    drm_handle_t handle;
    drmSize size;
    drmAddress map;
} viaRegion, *viaRegionPtr;

typedef struct {
    viaRegion regs, agp;
    int deviceID;
    int width;
    int height;
    int mem;
    int bytesPerPixel;
    int priv1;
    int priv2;
    int fbOffset;
    int fbSize;
    int backOffset;
    int depthOffset;
    int textureOffset;
    int textureSize;
    int irqEnabled;
    unsigned int scrnX, scrnY;
    int sarea_priv_offset;
    /*=* John Sheng [2003.12.9] Tuxracer & VQ *=*/
    int VQEnable;
    int DRIIrqEnable;
    int ringBufActive;
} VIADRIRec, *VIADRIPtr;

typedef struct {
    int dummy;
} VIAConfigPrivRec, *VIAConfigPrivPtr;

typedef struct {
    int dummy;
} VIADRIContextRec, *VIADRIContextPtr;

#ifdef XFree86Server

#include "screenint.h"

Bool VIADRIScreenInit(ScreenPtr pScreen);
void VIADRICloseScreen(ScreenPtr pScreen);
Bool VIADRIFinishScreenInit(ScreenPtr pScreen);

#endif
#endif
