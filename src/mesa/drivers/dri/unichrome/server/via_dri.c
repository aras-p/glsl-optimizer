/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_dri.c,v 1.4 2003/09/24 02:43:30 dawes Exp $ */
/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#if 0
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Priv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#define _XF86DRI_SERVER_
#include "GL/glxtokens.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
 
#include "driver.h"
#include "drm.h"
#endif

#include "dri_util.h"
#include "sarea.h"

#include "via_context.h"
#include "via_dri.h"
#include "via_driver.h"
#include "via_common.h"
#include "xf86drm.h"

static void VIAEnableMMIO(DRIDriverContext * ctx);
static void VIADisableMMIO(DRIDriverContext * ctx);
static void VIADisableExtendedFIFO(DRIDriverContext *ctx);
static void VIAEnableExtendedFIFO(DRIDriverContext *ctx);
static void VIAInitialize2DEngine(DRIDriverContext *ctx);
static void VIAInitialize3DEngine(DRIDriverContext *ctx);

static int VIADRIScreenInit(DRIDriverContext * ctx);
static void VIADRICloseScreen(DRIDriverContext * ctx);
static int VIADRIFinishScreenInit(DRIDriverContext * ctx);

/* TODO XXX _SOLO temp macros */
typedef unsigned char CARD8;
typedef unsigned short CARD16;
#define xf86DrvMsg(a, b, ...) fprintf(stderr, __VA_ARGS__)
#define MMIO_IN8(base, addr) ((*(((volatile CARD8*)base)+(addr)))+0)
#define MMIO_OUT8(base, addr, val) ((*(((volatile CARD8*)base)+(addr)))=((CARD8)val))
#define MMIO_OUT16(base, addr, val) ((*(volatile CARD16*)(((CARD8*)base)+(addr)))=((CARD16)val))
#define VGA_MISC_OUT_R  0x3cc
#define VGA_MISC_OUT_W  0x3c2

#define VIDEO	0 
#define AGP		1
#define AGP_PAGE_SIZE 4096
#define AGP_PAGES 8192
#define AGP_SIZE (AGP_PAGE_SIZE * AGP_PAGES)
#define AGP_CMDBUF_PAGES 256
#define AGP_CMDBUF_SIZE (AGP_PAGE_SIZE * AGP_CMDBUF_PAGES)

static char VIAKernelDriverName[] = "via";
static char VIAClientDriverName[] = "via";

static int VIADRIAgpInit(const DRIDriverContext *ctx, VIAPtr pVia);
static int VIADRIPciInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIFBInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIKernelInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIMapInit(DRIDriverContext * ctx, VIAPtr pVia);

static int VIADRIAgpInit(const DRIDriverContext *ctx, VIAPtr pVia)
{
    unsigned long  agp_phys;
    unsigned int agpaddr;
    VIADRIPtr pVIADRI;
    pVIADRI = pVia->devPrivate;
    pVia->agpSize = 0;

    if (drmAgpAcquire(pVia->drmFD) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR, "[drm] drmAgpAcquire failed %d\n", errno);
        return FALSE;
    }

    if (drmAgpEnable(pVia->drmFD, drmAgpGetMode(pVia->drmFD)&~0x0) < 0) {
         xf86DrvMsg(pScreen->myNum, X_ERROR, "[drm] drmAgpEnable failed\n");
        return FALSE;
    }
    
    xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] drmAgpEnabled succeeded\n");

    if (drmAgpAlloc(pVia->drmFD, AGP_SIZE, 0, &agp_phys, &pVia->agpHandle) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "[drm] drmAgpAlloc failed\n");
        drmAgpRelease(pVia->drmFD);
        return FALSE;
    }
   
    if (drmAgpBind(pVia->drmFD, pVia->agpHandle, 0) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "[drm] drmAgpBind failed\n");
        drmAgpFree(pVia->drmFD, pVia->agpHandle);
        drmAgpRelease(pVia->drmFD);

        return FALSE;
    }

    pVia->agpSize = AGP_SIZE;
    pVia->agpAddr = drmAgpBase(pVia->drmFD);
    xf86DrvMsg(pScreen->myNum, X_INFO,
                 "[drm] agpAddr = 0x%08lx\n",pVia->agpAddr);
		 
    pVIADRI->agp.size = pVia->agpSize;
    if (drmAddMap(pVia->drmFD, (drmHandle)0,
                 pVIADRI->agp.size, DRM_AGP, 0, 
                 &pVIADRI->agp.handle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
	    "[drm] Failed to map public agp area\n");
        pVIADRI->agp.size = 0;
        return FALSE;
    }  
    /* Map AGP from kernel to Xserver - Not really needed */
    drmMap(pVia->drmFD, pVIADRI->agp.handle,pVIADRI->agp.size,
	(drmAddressPtr)&agpaddr);

#if 0
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agpBase = %p\n", pVia->agpBase);
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agpAddr = 0x%08lx\n", pVia->agpAddr);
#endif
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agpSize = 0x%08x\n", pVia->agpSize);
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agp physical addr = 0x%08lx\n", agp_phys);

    {
	drm_via_agp_t agp;
	agp.offset = 0;
	agp.size = AGP_SIZE;
	if (drmCommandWrite(pVia->drmFD, DRM_VIA_AGP_INIT, &agp,
			    sizeof(drm_via_agp_t)) < 0)
	    return FALSE;
    }

    return TRUE;
}

static int VIADRIFBInit(DRIDriverContext * ctx, VIAPtr pVia)
{   
    int FBSize = pVia->FBFreeEnd-pVia->FBFreeStart;
    int FBOffset = pVia->FBFreeStart; 
    VIADRIPtr pVIADRI = pVia->devPrivate;
    pVIADRI->fbOffset = FBOffset;
    pVIADRI->fbSize = pVia->videoRambytes;
    
    {
	drm_via_fb_t fb;
	fb.offset = FBOffset;
	fb.size = FBSize;
	
	if (drmCommandWrite(pVia->drmFD, DRM_VIA_FB_INIT, &fb,
			    sizeof(drm_via_fb_t)) < 0) {
	    xf86DrvMsg(pScreen->myNum, X_ERROR,
		       "[drm] failed to init frame buffer area\n");
	    return FALSE;
	} else {
	    xf86DrvMsg(pScreen->myNum, X_INFO,
		       "[drm] FBFreeStart= 0x%08x FBFreeEnd= 0x%08x "
		       "FBSize= 0x%08x\n",
		       pVia->FBFreeStart, pVia->FBFreeEnd, FBSize);
	    return TRUE;	
	}   
    }
}

static int VIADRIPciInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    return TRUE;	
}

static int VIADRIScreenInit(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI;
    int err;

#if 0
    ctx->shared.SAREASize = ((sizeof(XF86DRISAREARec) + 0xfff) & 0x1000);
#else
    if (sizeof(XF86DRISAREARec)+sizeof(VIASAREAPriv) > SAREA_MAX) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"Data does not fit in SAREA\n");
	return FALSE;
    }
    ctx->shared.SAREASize = SAREA_MAX;
#endif

    ctx->drmFD = drmOpen(VIAKernelDriverName, NULL);
    if (ctx->drmFD < 0) {
        fprintf(stderr, "[drm] drmOpen failed\n");
        return 0;
    }
    pVia->drmFD = ctx->drmFD;

    err = drmSetBusid(ctx->drmFD, ctx->pciBusID);
    if (err < 0) {
        fprintf(stderr, "[drm] drmSetBusid failed (%d, %s), %s\n",
                ctx->drmFD, ctx->pciBusID, strerror(-err));
        return 0;
    }

    err = drmAddMap(ctx->drmFD, 0, ctx->shared.SAREASize, DRM_SHM,
                  DRM_CONTAINS_LOCK, &ctx->shared.hSAREA);
    if (err < 0) {
        fprintf(stderr, "[drm] drmAddMap failed\n");
        return 0;
    }
    fprintf(stderr, "[drm] added %d byte SAREA at 0x%08lx\n",
            ctx->shared.SAREASize, ctx->shared.hSAREA);

    if (drmMap(ctx->drmFD,
               ctx->shared.hSAREA,
               ctx->shared.SAREASize,
               (drmAddressPtr)(&ctx->pSAREA)) < 0)
    {
        fprintf(stderr, "[drm] drmMap failed\n");
        return 0;
    }
    memset(ctx->pSAREA, 0, ctx->shared.SAREASize);
    fprintf(stderr, "[drm] mapped SAREA 0x%08lx to %p, size %d\n",
            ctx->shared.hSAREA, ctx->pSAREA, ctx->shared.SAREASize);

    /* Need to AddMap the framebuffer and mmio regions here:
     */
    if (drmAddMap(ctx->drmFD,
                  (drmHandle)ctx->FBStart,
                  ctx->FBSize,
                  DRM_FRAME_BUFFER,
#ifndef _EMBEDDED
                   0,
#else
                   DRM_READ_ONLY,
#endif
                   &ctx->shared.hFrameBuffer) < 0)
    {
        fprintf(stderr, "[drm] drmAddMap framebuffer failed\n");
        return 0;
    }

    fprintf(stderr, "[drm] framebuffer handle = 0x%08lx\n",
            ctx->shared.hFrameBuffer);

    pVIADRI = (VIADRIPtr) calloc(1, sizeof(VIADRIRec));
    if (!pVIADRI) {
        drmClose(ctx->drmFD);
        return FALSE;
    }
    pVia->devPrivate = pVIADRI;
    ctx->driverClientMsg = pVIADRI;
    ctx->driverClientMsgSize = sizeof(*pVIADRI);

    pVia->IsPCI = !VIADRIAgpInit(ctx, pVia);

    if (pVia->IsPCI) {
        VIADRIPciInit(ctx, pVia);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] use pci.\n" );
    }
    else
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] use agp.\n" );

    if (!(VIADRIFBInit(ctx, pVia))) {
	VIADRICloseScreen(ctx);
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "[dri] frame buffer initialize fial .\n" );
        return FALSE;
    }
    
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] frame buffer initialized.\n" );
 
    /* DRIScreenInit doesn't add all the common mappings.  Add additional mappings here. */
    if (!VIADRIMapInit(ctx, pVia)) {
	VIADRICloseScreen(ctx);
	return FALSE;
    }
    pVIADRI->regs.size = VIA_MMIO_REGSIZE;
    pVIADRI->regs.map = 0;
    pVIADRI->regs.handle = pVia->registerHandle;
    xf86DrvMsg(ctx->myNum, X_INFO, "[drm] mmio Registers = 0x%08lx\n",
	pVIADRI->regs.handle);

    if (drmMap(pVia->drmFD,
               pVIADRI->regs.handle,
               pVIADRI->regs.size,
               (drmAddress *)&pVia->MapBase) != 0)
    {
        VIADRICloseScreen(ctx);
        return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] mmio mapped.\n" );

    return VIADRIFinishScreenInit(ctx);
}

static void
VIADRICloseScreen(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI=(VIADRIPtr)pVia->devPrivate;

    if (pVia->MapBase) {
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Unmapping MMIO registers\n");
        drmUnmap(pVia->MapBase, pVIADRI->regs.size);
    }

    if (pVia->agpSize) {
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Freeing agp memory\n");
        drmAgpFree(pVia->drmFD, pVia->agpHandle);
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Releasing agp module\n");
    	drmAgpRelease(pVia->drmFD);
    }
}

static int
VIADRIFinishScreenInit(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI;
    int err;

    err = drmCreateContext(ctx->drmFD, &ctx->serverContext);
    if (err != 0) {
        fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
        return FALSE;
    }

    DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0);


    if (!VIADRIKernelInit(ctx, pVia)) {
	VIADRICloseScreen(ctx);
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO, "[dri] kernel data initialized.\n");

    /* set SAREA value */
    {
	VIASAREAPriv *saPriv;

	saPriv=(VIASAREAPriv*)(((char*)ctx->pSAREA) +
                               sizeof(XF86DRISAREARec));
	assert(saPriv);
	memset(saPriv, 0, sizeof(*saPriv));
	saPriv->CtxOwner = -1;
    }
    pVIADRI=(VIADRIPtr)pVia->devPrivate;
    pVIADRI->deviceID=pVia->Chipset;  
    pVIADRI->width=ctx->shared.virtualWidth;
    pVIADRI->height=ctx->shared.virtualHeight;
    pVIADRI->mem=ctx->shared.fbSize;
    pVIADRI->bytesPerPixel= (ctx->bpp+7) / 8; 
    pVIADRI->sarea_priv_offset = sizeof(XF86DRISAREARec);
    /* TODO */
    pVIADRI->scrnX=pVIADRI->width;
    pVIADRI->scrnY=pVIADRI->height;

    return TRUE;
}

/* Initialize the kernel data structures. */
static int VIADRIKernelInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    drm_via_init_t drmInfo;
    memset(&drmInfo, 0, sizeof(drm_via_init_t));
    drmInfo.func = VIA_INIT_MAP;
    drmInfo.sarea_priv_offset   = sizeof(XF86DRISAREARec);
    drmInfo.fb_offset           = pVia->FrameBufferBase;
    drmInfo.mmio_offset         = pVia->registerHandle;
    if (pVia->IsPCI)
	drmInfo.agpAddr = (CARD32)NULL;
    else
	drmInfo.agpAddr = (CARD32)pVia->agpAddr;

	if ((drmCommandWrite(pVia->drmFD, DRM_VIA_MAP_INIT,&drmInfo,
			     sizeof(drm_via_init_t))) < 0)
	    return FALSE;

    return TRUE;
}
/* Add a map for the MMIO registers */
static int VIADRIMapInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    int flags = 0;

    if (drmAddMap(pVia->drmFD, pVia->MmioBase, VIA_MMIO_REGSIZE,
		  DRM_REGISTERS, flags, &pVia->registerHandle) < 0) {
	return FALSE;
    }

    xf86DrvMsg(pScreen->myNum, X_INFO,
	"[drm] register handle = 0x%08lx\n", pVia->registerHandle);

    return TRUE;
}

const __GLcontextModes __glModes[] =
{
    /* 32 bit, RGBA Depth=24 Stencil=8 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_TRUE,
     .redBits = 8, .greenBits = 8, .blueBits = 8, .alphaBits = 8,
     .redMask = 0xff0000, .greenMask = 0xff00, .blueMask = 0xff, .alphaMask = 0xff000000,
     .rgbBits = 32, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 16, .stencilBits = 8,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },

    /* 16 bit, RGB Depth=16 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_FALSE,
     .redBits = 5, .greenBits = 6, .blueBits = 5, .alphaBits = 0,
     .redMask = 0xf800, .greenMask = 0x07e0, .blueMask = 0x001f, .alphaMask = 0x0,
     .rgbBits = 16, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 16, .stencilBits = 0,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },
};

static int viaInitContextModes(const DRIDriverContext *ctx,
                                  int *numModes, const __GLcontextModes **modes)
{
    *numModes = sizeof(__glModes)/sizeof(__GLcontextModes *);
    *modes = &__glModes[0];
    return 1;
}

static int viaValidateMode(const DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);

    return 1;
}

static int viaPostValidateMode(const DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);

    return 1;
}

static void VIAEnableMMIO(DRIDriverContext * ctx)
{
    /*vgaHWPtr hwp = VGAHWPTR(ctx);*/
    VIAPtr pVia = VIAPTR(ctx);
    unsigned char val;

#if 0
    if (xf86IsPrimaryPci(pVia->PciInfo)) {
        /* If we are primary card, we still use std vga port. If we use
         * MMIO, system will hang in vgaHWSave when our card used in
         * PLE and KLE (integrated Trident MVP4)
         */
        vgaHWSetStdFuncs(hwp);
    }
    else {
        vgaHWSetMmioFuncs(hwp, pVia->MapBase, 0x8000);
    }
#endif

    val = VGAIN8(0x3c3);
    VGAOUT8(0x3c3, val | 0x01);
    val = VGAIN8(VGA_MISC_OUT_R);
    VGAOUT8(VGA_MISC_OUT_W, val | 0x01);

    /* Unlock Extended IO Space */
    VGAOUT8(0x3c4, 0x10);
    VGAOUT8(0x3c5, 0x01);

    /* Enable MMIO */
    if(!pVia->IsSecondary) {
	VGAOUT8(0x3c4, 0x1a);
	val = VGAIN8(0x3c5);
#ifdef DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "primary val = %x\n", val);
#endif
	VGAOUT8(0x3c5, val | 0x68);
    }
    else {
	VGAOUT8(0x3c4, 0x1a);
	val = VGAIN8(0x3c5);
#ifdef DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "secondary val = %x\n", val);
#endif
	VGAOUT8(0x3c5, val | 0x38);
    }

    return;
}

static void VIADisableMMIO(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    unsigned char val;

    VGAOUT8(0x3c4, 0x1a);
    val = VGAIN8(0x3c5);
    VGAOUT8(0x3c5, val & 0x97);

    return;
}

static void VIADisableExtendedFIFO(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    CARD32          dwTemp;

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp |= 0x20000000;
    VIASETREG(0x298, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x230);
    dwTemp &= ~0x00200000;
    VIASETREG(0x230, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp &= ~0x20000000;
    VIASETREG(0x298, dwTemp);
}

static void VIAEnableExtendedFIFO(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    CARD32          dwTemp;
    CARD8           bTemp;

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp |= 0x20000000;
    VIASETREG(0x298, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x230);
    dwTemp |= 0x00200000;
    VIASETREG(0x230, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp &= ~0x20000000;
    VIASETREG(0x298, dwTemp);

    VGAOUT8(0x3C4, 0x17);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x7F;
    bTemp |= 0x2F;
    VGAOUT8(0x3C5, bTemp);

    VGAOUT8(0x3C4, 0x16);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x3F;
    bTemp |= 0x17;
    VGAOUT8(0x3C5, bTemp);

    VGAOUT8(0x3C4, 0x18);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x3F;
    bTemp |= 0x17;
    bTemp |= 0x40; /* force the preq always higher than treq */
    VGAOUT8(0x3C5, bTemp);
}

static void VIAInitialize2DEngine(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    CARD32  dwVQStartAddr, dwVQEndAddr;
    CARD32  dwVQLen, dwVQStartL, dwVQEndL, dwVQStartEndH;
    CARD32  dwGEMode;

    /* init 2D engine regs to reset 2D engine */
    VIASETREG(0x04, 0x0);
    VIASETREG(0x08, 0x0);
    VIASETREG(0x0c, 0x0);
    VIASETREG(0x10, 0x0);
    VIASETREG(0x14, 0x0);
    VIASETREG(0x18, 0x0);
    VIASETREG(0x1c, 0x0);
    VIASETREG(0x20, 0x0);
    VIASETREG(0x24, 0x0);
    VIASETREG(0x28, 0x0);
    VIASETREG(0x2c, 0x0);
    VIASETREG(0x30, 0x0);
    VIASETREG(0x34, 0x0);
    VIASETREG(0x38, 0x0);
    VIASETREG(0x3c, 0x0);
    VIASETREG(0x40, 0x0);

    /* Init AGP and VQ regs */
    VIASETREG(0x43c, 0x00100000);
    VIASETREG(0x440, 0x00000000);
    VIASETREG(0x440, 0x00333004);
    VIASETREG(0x440, 0x60000000);
    VIASETREG(0x440, 0x61000000);
    VIASETREG(0x440, 0x62000000);
    VIASETREG(0x440, 0x63000000);
    VIASETREG(0x440, 0x64000000);
    VIASETREG(0x440, 0x7D000000);

    VIASETREG(0x43c, 0xfe020000);
    VIASETREG(0x440, 0x00000000);

    if (pVia->VQStart != 0) {
        /* Enable VQ */
        dwVQStartAddr = pVia->VQStart;
        dwVQEndAddr = pVia->VQEnd;
        dwVQStartL = 0x50000000 | (dwVQStartAddr & 0xFFFFFF);
        dwVQEndL = 0x51000000 | (dwVQEndAddr & 0xFFFFFF);
        dwVQStartEndH = 0x52000000 | ((dwVQStartAddr & 0xFF000000) >> 24) |
                        ((dwVQEndAddr & 0xFF000000) >> 16);
        dwVQLen = 0x53000000 | (VIA_VQ_SIZE >> 3);

        VIASETREG(0x43c, 0x00fe0000);
        VIASETREG(0x440, 0x080003fe);
        VIASETREG(0x440, 0x0a00027c);
        VIASETREG(0x440, 0x0b000260);
        VIASETREG(0x440, 0x0c000274);
        VIASETREG(0x440, 0x0d000264);
        VIASETREG(0x440, 0x0e000000);
        VIASETREG(0x440, 0x0f000020);
        VIASETREG(0x440, 0x1000027e);
        VIASETREG(0x440, 0x110002fe);
        VIASETREG(0x440, 0x200f0060);

        VIASETREG(0x440, 0x00000006);
        VIASETREG(0x440, 0x40008c0f);
        VIASETREG(0x440, 0x44000000);
        VIASETREG(0x440, 0x45080c04);
        VIASETREG(0x440, 0x46800408);

        VIASETREG(0x440, dwVQStartEndH);
        VIASETREG(0x440, dwVQStartL);
        VIASETREG(0x440, dwVQEndL);
        VIASETREG(0x440, dwVQLen);
    }
    else {
        /* Diable VQ */
        VIASETREG(0x43c, 0x00fe0000);
        VIASETREG(0x440, 0x00000004);
        VIASETREG(0x440, 0x40008c0f);
        VIASETREG(0x440, 0x44000000);
        VIASETREG(0x440, 0x45080c04);
        VIASETREG(0x440, 0x46800408);
    }

    dwGEMode = 0;

    switch (ctx->bpp) {
    case 16:
        dwGEMode |= VIA_GEM_16bpp;
        break;
    case 32:
        dwGEMode |= VIA_GEM_32bpp;
    default:
        dwGEMode |= VIA_GEM_8bpp;
        break;
    }

#if 0
    switch (ctx->shared.virtualWidth) {
    case 800:
        dwGEMode |= VIA_GEM_800;
        break;
    case 1024:
        dwGEMode |= VIA_GEM_1024;
        break;
    case 1280:
        dwGEMode |= VIA_GEM_1280;
        break;
    case 1600:
        dwGEMode |= VIA_GEM_1600;
        break;
    case 2048:
        dwGEMode |= VIA_GEM_2048;
        break;
    default:
        dwGEMode |= VIA_GEM_640;
        break;
    }
#endif

    /* Set BPP and Pitch */
    VIASETREG(VIA_REG_GEMODE, dwGEMode);

    /* Set Src and Dst base address and pitch, pitch is qword */
    VIASETREG(VIA_REG_SRCBASE, 0x0);
    VIASETREG(VIA_REG_DSTBASE, 0x0);
    VIASETREG(VIA_REG_PITCH, VIA_PITCH_ENABLE |
              ((ctx->shared.virtualWidth * ctx->bpp >> 3) >> 3) |
              (((ctx->shared.virtualWidth * ctx->bpp >> 3) >> 3) << 16));
}

static void VIAInitialize3DEngine(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    int i;

    if (!pVia->sharedData->b3DRegsInitialized)
    {

        VIASETREG(0x43C, 0x00010000);

        for (i = 0; i <= 0x7D; i++)
        {
            VIASETREG(0x440, (CARD32) i << 24);
        }

        VIASETREG(0x43C, 0x00020000);

        for (i = 0; i <= 0x94; i++)
        {
            VIASETREG(0x440, (CARD32) i << 24);
        }

        VIASETREG(0x440, 0x82400000);

        VIASETREG(0x43C, 0x01020000);


        for (i = 0; i <= 0x94; i++)
        {
            VIASETREG(0x440, (CARD32) i << 24);
        }

        VIASETREG(0x440, 0x82400000);
        VIASETREG(0x43C, 0xfe020000);

        for (i = 0; i <= 0x03; i++)
        {
            VIASETREG(0x440, (CARD32) i << 24);
        }

        VIASETREG(0x43C, 0x00030000);

        for (i = 0; i <= 0xff; i++)
        {
            VIASETREG(0x440, 0);
        }
        VIASETREG(0x43C, 0x00100000);
        VIASETREG(0x440, 0x00333004);
        VIASETREG(0x440, 0x10000002);
        VIASETREG(0x440, 0x60000000);
        VIASETREG(0x440, 0x61000000);
        VIASETREG(0x440, 0x62000000);
        VIASETREG(0x440, 0x63000000);
        VIASETREG(0x440, 0x64000000);

        VIASETREG(0x43C, 0x00fe0000);

        if (pVia->ChipRev >= 3 )
            VIASETREG(0x440,0x40008c0f);
        else
            VIASETREG(0x440,0x4000800f);

        VIASETREG(0x440,0x44000000);
        VIASETREG(0x440,0x45080C04);
        VIASETREG(0x440,0x46800408);
        VIASETREG(0x440,0x50000000);
        VIASETREG(0x440,0x51000000);
        VIASETREG(0x440,0x52000000);
        VIASETREG(0x440,0x53000000);

        pVia->sharedData->b3DRegsInitialized = 1;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "3D Engine has been initialized.\n");
    }

    VIASETREG(0x43C,0x00fe0000);
    VIASETREG(0x440,0x08000001);
    VIASETREG(0x440,0x0A000183);
    VIASETREG(0x440,0x0B00019F);
    VIASETREG(0x440,0x0C00018B);
    VIASETREG(0x440,0x0D00019B);
    VIASETREG(0x440,0x0E000000);
    VIASETREG(0x440,0x0F000000);
    VIASETREG(0x440,0x10000000);
    VIASETREG(0x440,0x11000000);
    VIASETREG(0x440,0x20000000);
}

static int viaInitFBDev(DRIDriverContext *ctx)
{
    VIAPtr pVia = calloc(1, sizeof(*pVia));

    ctx->driverPrivate = (void *)pVia;

    pVia->Chipset = ctx->chipset;

    pVia->videoRambytes = ctx->shared.fbSize;
    pVia->MmioBase = ctx->MMIOStart;
    pVia->FrameBufferBase = ctx->FBStart & 0xfc000000;

    pVia->FBFreeStart = ctx->shared.virtualWidth * ctx->cpp *
        ctx->shared.virtualHeight;
    pVia->FBFreeEnd = pVia->videoRambytes;

    pVia->sharedData = (ViaSharedPtr) calloc(1, sizeof(ViaSharedRec));

    if (!VIADRIScreenInit(ctx))
        return 0;

    VIAEnableMMIO(ctx);

    /* 3D rendering has noise if not enabled. */
    VIAEnableExtendedFIFO(ctx);

    VIAInitialize2DEngine(ctx);
    VIAInitialize3DEngine(ctx);

    /* Must disable MMIO or 3D won't work. */
    VIADisableMMIO(ctx);

    return 1;
}

static void viaHaltFBDev(DRIDriverContext *ctx)
{
    drmUnmap( ctx->pSAREA, ctx->shared.SAREASize );
    drmClose(ctx->drmFD);

    if (ctx->driverPrivate) {
        free(ctx->driverPrivate);
        ctx->driverPrivate = 0;
    }
}

static int viaEngineShutdown(const DRIDriverContext *ctx)
{
    return 1;
}

static int viaEngineRestore(const DRIDriverContext *ctx)
{
    return 1;
}

const struct DRIDriverRec __driDriver =
{
    viaInitContextModes,
    viaValidateMode,
    viaPostValidateMode,
    viaInitFBDev,
    viaHaltFBDev,
    viaEngineShutdown,
    viaEngineRestore,  
    0,
};

