/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_driver.h,v 1.7 2003/11/06 18:38:11 tsi Exp $ */
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

#ifndef _VIA_DRIVER_H_
#define _VIA_DRIVER_H_ 1

#if 0 /* DEBUG is use in VIA DRI code as a flag */
/* #define DEBUG_PRINT */
#ifdef DEBUG_PRINT
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif
#endif

#if 0
#include "vgaHW.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Cursor.h"
#include "mipointer.h"
#include "micmap.h"

#define USE_FB
#ifdef USE_FB
#include "fb.h"
#else
#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
#endif

#include "xf86cmap.h"
#include "vbe.h"
#include "xaa.h"

#include "via_regs.h"
#include "via_bios.h"
#include "via_gpioi2c.h"
#include "via_priv.h"
#include "ginfo.h"

#ifdef XF86DRI
#define _XF86DRI_SERVER_
#include "sarea.h"
#include "dri.h"
#include "GL/glxint.h"
#include "via_dri.h"
#endif
#else
#include "via_priv.h"
#include "via_regs.h"

#include "sarea.h"
#include "dri.h"
#include "via_dri.h"
#endif

#define DRIVER_NAME     "via"
#define DRIVER_VERSION  "4.1.0"
#define VERSION_MAJOR   4
#define VERSION_MINOR   1
#define PATCHLEVEL      30
#define VIA_VERSION     ((VERSION_MAJOR<<24) | (VERSION_MINOR<<16) | PATCHLEVEL)

#define VGAIN8(addr)        MMIO_IN8(pVia->MapBase+0x8000, addr)
#define VGAIN16(addr)       MMIO_IN16(pVia->MapBase+0x8000, addr)
#define VGAIN(addr)         MMIO_IN32(pVia->MapBase+0x8000, addr)

#define VGAOUT8(addr, val)  MMIO_OUT8(pVia->MapBase+0x8000, addr, val)
#define VGAOUT16(addr, val) MMIO_OUT16(pVia->MapBase+0x8000, addr, val)
#define VGAOUT(addr, val)   MMIO_OUT32(pVia->MapBase+0x8000, addr, val)

#define INREG(addr)         MMIO_IN32(pVia->MapBase, addr)
#define OUTREG(addr, val)   MMIO_OUT32(pVia->MapBase, addr, val)
#define INREG16(addr)       MMIO_IN16(pVia->MapBase, addr)
#define OUTREG16(addr, val) MMIO_OUT16(pVia->MapBase, addr, val)

#define VIA_PIXMAP_CACHE_SIZE   (256 * 1024)
#define VIA_CURSOR_SIZE         (4 * 1024)
#define VIA_VQ_SIZE             (256 * 1024)

typedef struct {
    unsigned int    mode, refresh, resMode;
    int             countWidthByQWord;
    int             offsetWidthByQWord;
    unsigned char   SR08, SR0A, SR0F;

    /*   extended Sequencer registers */
    unsigned char   SR10, SR11, SR12, SR13,SR14,SR15,SR16;
    unsigned char   SR17, SR18, SR19, SR1A,SR1B,SR1C,SR1D,SR1E;
    unsigned char   SR1F, SR20, SR21, SR22,SR23,SR24,SR25,SR26;
    unsigned char   SR27, SR28, SR29, SR2A,SR2B,SR2C,SR2D,SR2E;
    unsigned char   SR2F, SR30, SR31, SR32,SR33,SR34,SR40,SR41;
    unsigned char   SR42, SR43, SR44, SR45,SR46,SR47;

    unsigned char   Clock;

    /*   extended CRTC registers */
    unsigned char   CR13, CR30, CR31, CR32, CR33, CR34, CR35, CR36;
    unsigned char   CR37, CR38, CR39, CR3A, CR40, CR41, CR42, CR43;
    unsigned char   CR44, CR45, CR46, CR47, CR48, CR49, CR4A;
    unsigned char   CRTCRegs[68];
    unsigned char   TVRegs[0xFF];
/*    unsigned char   LCDRegs[0x40];*/
} VIARegRec, *VIARegPtr;

/*Definition for  CapturePortID*/
#define PORT0     0      /* Capture Port 0*/
#define PORT1     1      /* Capture Port 1*/

typedef struct __viaVideoControl {
  CARD32 PORTID;
  CARD32 dwCompose;
  CARD32 dwHighQVDO;
  CARD32 VideoStatus;
  CARD32 dwAction;
#define ACTION_SET_PORTID      0
#define ACTION_SET_COMPOSE     1
#define ACTION_SET_HQV         2
#define ACTION_SET_BOB	       4
#define ACTION_SET_VIDEOSTATUS 8
  Bool  Cap0OnScreen1;   /* True: Capture0 On Screen1 ; False: Capture0 On Screen0 */
  Bool  Cap1OnScreen1;   /* True: Capture1 On Screen1 ; False: Capture1 On Screen0 */
  Bool  MPEGOnScreen1;   /* True: MPEG On Screen1 ; False: MPEG On Screen0 */
} VIAVideoControlRec, VIAVideoControlPtr;

/*For Video HW Difference */
#define VIA_REVISION_CLEC0        0x10
#define VIA_REVISION_CLEC1        0x11
#define VIA_REVISION_CLECX        0x10

#define VID_HWDIFF_TRUE           0x00000001
#define VID_HWDIFF_FALSE          0x00000000

/*
 *	Video HW Difference Structure
 */

typedef struct __VIAHWRec
{
    unsigned long dwThreeHQVBuffer;		/* Use Three HQV Buffers*/
    unsigned long dwV3SrcHeightSetting;		/* Set Video Source Width and Height*/
    unsigned long dwSupportExtendFIFO;		/* Support Extand FIFO*/
    unsigned long dwHQVFetchByteUnit;		/* HQV Fetch Count unit is byte*/
    unsigned long dwHQVInitPatch;		/* Initialize HQV Engine 2 times*/
    unsigned long dwSupportV3Gamma;		/* Support V3 Gamma */
    unsigned long dwUpdFlip;			/* Set HQV3D0[15] to flip video*/
    unsigned long dwHQVDisablePatch;		/* Change Video Engine Clock setting for HQV disable bug*/
    unsigned long dwSUBFlip;			/* Set HQV3D0[15] to flip video for sub-picture blending*/
    unsigned long dwNeedV3Prefetch;		/* V3 pre-fetch function for K8*/
    unsigned long dwNeedV4Prefetch;		/* V4 pre-fetch function for K8*/
    unsigned long dwUseSystemMemory;		/* Use system memory for DXVA compressed data buffers*/
    unsigned long dwExpandVerPatch;		/* Patch video HW bug in expand SIM mode or same display path*/
    unsigned long dwExpandVerHorPatch;		/* Patch video HW bug in expand SAMM mode or same display path*/
    unsigned long dwV3ExpireNumTune;		/* Change V3 expire number setting for V3 bandwidth issue*/
    unsigned long dwV3FIFOThresholdTune;	/* Change V3 FIFO, Threshold and Pre-threshold setting for V3 bandwidth issue*/
    unsigned long dwCheckHQVFIFOEmpty;          /* HW Flip path, need to check HQV FIFO status */
    unsigned long dwUseMPEGAGP;                 /* Use MPEG AGP function*/
    unsigned long dwV3FIFOPatch;                /* For CLE V3 FIFO Bug (srcWidth <= 8)*/
    unsigned long dwSupportTwoColorKey;         /* Support two color key*/
    unsigned long dwCxColorSpace;               /* CLE_Cx ColorSpace*/
} VIAHWRec;

/*Wait Function Structure and Flag*/
typedef struct _WaitHWINFO
{
    unsigned char *	pjVideo;		/* MMIO Address Info*/
    unsigned long	dwVideoFlag;		/* Video Flag*/
}WaitHWINFO, * LPWaitHWINFO;

#if 0
/* VIA Tuners */
typedef struct
{
    int			decoderType;		/* Decoder I2C Type */
#define SAA7108H		0
#define SAA7113H		1
#define SAA7114H		2
    I2CDevPtr 		I2C;			/* Decoder I2C */
    I2CDevPtr 		FMI2C;			/* FM Tuner I2C */
    
    /* Not yet used */
    int			autoDetect;		/* Autodetect mode */
    int			tunerMode;		/* Fixed mode */
} ViaTunerRec, *ViaTunerPtr;
#endif

/*
 * New style overlay structure for the SubPicture overlay
 */

#if 0
typedef struct
{
    VIAMem Memory;
    int visible:1;	/* Idea is for the top bits to become a generic class */
    CARD32 width;
    CARD32 height;
    CARD32 pitch;
    CARD32 base[2];	/* Some channels have 3 so 3 for the generic unit */
    struct
    {
    	CARD8 Y;
    	CARD8 Cb;
    	CARD8 Cr;
    } palette[16];
} ViaSubPictureRecord;

typedef struct
{
    VIAMem Memory;
    int visible:1;	/* Visible */
    CARD32 width;
    CARD32 height;
    CARD32 pitch;
    CARD32 base[3];
    int Channel;
    int HQV;		/* Own HQV */
} ViaTVRecord;
    
typedef struct
{
    VIAMem Memory;
    CARD32 base[3];
    int Busy;
} ViaHQVRecord;
#endif

/*
 * Variables that need to be shared among different screens.
 */
typedef struct {
    Bool b3DRegsInitialized;
} ViaSharedRec, *ViaSharedPtr;


typedef struct _VIA {
    VIARegRec           SavedReg;
    VIARegRec           ModeReg;
    //xf86CursorInfoPtr   CursorInfoRec;
    Bool                ModeStructInit;
    int                 Bpp, Bpl, ScissB;
    unsigned            PlaneMask;

    unsigned long       videoRambytes;
    int                 videoRamKbytes;
    int                 FBFreeStart;
    int                 FBFreeEnd;
    int                 CursorStart;
    int                 VQStart;
    int                 VQEnd;

    /* These are physical addresses. */
    unsigned long       FrameBufferBase;
    unsigned long       MmioBase;

    /* These are linear addresses. */
    unsigned char*      MapBase;
    unsigned char*      VidMapBase;
    unsigned char*      BltBase;
    unsigned char*      MapBaseDense;
    unsigned char*      FBBase;
    unsigned char*      FBStart;
    
    /* Private memory pool management */
    int			SWOVUsed[MEM_BLOCKS]; /* Free map for SWOV pool */
    unsigned long	SWOVPool;	/* Base of SWOV pool */
    unsigned long	SWOVSize;	/* Size of SWOV blocks */

    Bool                PrimaryVidMapped;
    int                 dacSpeedBpp;
    int                 minClock, maxClock;
    int                 MCLK, REFCLK, LCDclk;
    double              refclk_fact;

    /* Here are all the Options */
    Bool                VQEnable;
    Bool                pci_burst;
    Bool                NoPCIRetry;
    Bool                hwcursor;
    Bool                NoAccel;
    Bool                shadowFB;
    Bool                NoDDCValue;
    int                 rotate;

    //CloseScreenProcPtr  CloseScreen;
    //pciVideoPtr         PciInfo;
    //PCITAG              PciTag;
    int                 Chipset;
    int                 ChipId;
    int                 ChipRev;
    //vbeInfoPtr          pVbe;
    int                 EntityIndex;

    /* Support for shadowFB and rotation */
    unsigned char*      ShadowPtr;
    int                 ShadowPitch;
    void                (*PointerMoved)(int index, int x, int y);

    /* Support for XAA acceleration */
    //XAAInfoRecPtr       AccelInfoRec;
    //xRectangle          Rect;
    CARD32              SavedCmd;
    CARD32              SavedFgColor;
    CARD32              SavedBgColor;
    CARD32              SavedPattern0;
    CARD32              SavedPattern1;
    CARD32              SavedPatternAddr;

    /* Support for Int10 processing */
    //xf86Int10InfoPtr    pInt10;

    /* BIOS Info Ptr */
    //VIABIOSInfoPtr      pBIOSInfo;

    /* Support for DGA */
    int                 numDGAModes;
    //DGAModePtr          DGAModes;
    Bool                DGAactive;
    int                 DGAViewportStatus;
    int			DGAOldDisplayWidth;
    int			DGAOldBitsPerPixel;
    int			DGAOldDepth;
    /* The various wait handlers. */
    int                 (*myWaitIdle)(struct _VIA*);

    /* I2C & DDC */
    //I2CBusPtr           I2C_Port1;
    //I2CBusPtr           I2C_Port2;
    //xf86MonPtr          DDC1;
    //xf86MonPtr          DDC2;

    /* MHS */
    Bool                IsSecondary;
    Bool                HasSecondary;

    /* Capture de-interlace Mode */
    CARD32              Cap0_Deinterlace;
    CARD32              Cap1_Deinterlace;

    Bool                Cap0_FieldSwap;

#ifdef XF86DRI
    Bool		directRenderingEnabled;
    DRIInfoPtr		pDRIInfo;
    int 		drmFD;
    int 		numVisualConfigs;
    __GLXvisualConfig* 	pVisualConfigs;
    VIAConfigPrivPtr 	pVisualConfigsPriv;
    unsigned long 	agpHandle;
    unsigned long 	registerHandle;
    unsigned long 	agpAddr;
    unsigned char 	*agpBase;
    unsigned int 	agpSize;
    Bool 		IsPCI;
    Bool 		drixinerama;
#else
    int 		drmFD;
    unsigned long 	agpHandle;
    unsigned long 	registerHandle;
    unsigned long 	agpAddr;
    unsigned char 	*agpBase;
    unsigned int 	agpSize;
    Bool 		IsPCI;
#endif
    Bool		OldDRI;		/* True if DRM < 2.0 found */
    
    unsigned char	ActiveDevice;	/* if SAMM, non-equal pBIOSInfo->ActiveDevice */
    unsigned char       *CursorImage;
    CARD32		CursorFG;
    CARD32		CursorBG;
    CARD32		CursorMC;

#if 0
    /* Video */
    swovRec		swov;
    VIAVideoControlRec  Video;
    VIAHWRec		ViaHW;
    unsigned long	dwV1, dwV3;
    unsigned long	OverlaySupported;
    unsigned long	dwFrameNum;

    pointer		VidReg;
    unsigned long	gdwVidRegCounter;
    unsigned long	old_dwUseExtendedFIFO;
    
    /* Overlay TV Tuners */
    ViaTunerPtr		Tuner[2];
    I2CDevPtr		CXA2104S;
    int			AudioMode;
    int			AudioMute;
#endif

    /* SubPicture */
    //ViaSubPictureRecord	SubPicture;
    //ViaHQVRecord	HQV;
    //ViaTVRecord		TV0, TV1;
    
    /* TODO: MPEG TV0 TV1 */
    
    /* Global 2D state block - needs to slowly die */
    //ViaGraphicRec	graphicInfo;    
    ViaSharedPtr	sharedData;

    VIADRIPtr devPrivate;
} VIARec, *VIAPtr;


#if 0
typedef struct
{
    Bool IsDRIEnabled;

    Bool HasSecondary;
    Bool BypassSecondary;
    /*These two registers are used to make sure the CRTC2 is
      retored before CRTC_EXT, otherwise it could lead to blank screen.*/
    Bool IsSecondaryRestored;
    Bool RestorePrimary;

    ScrnInfoPtr pSecondaryScrn;
    ScrnInfoPtr pPrimaryScrn;
}VIAEntRec, *VIAEntPtr;
#endif


/* Shortcuts.  These depend on a local symbol "pVia". */

#define WaitIdle()      pVia->myWaitIdle(pVia)
#define VIAPTR(p)       ((VIAPtr)((p)->driverPrivate))


#if 0
/* Prototypes. */
void VIAAdjustFrame(int scrnIndex, int y, int x, int flags);
Bool VIASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

/* In HwDiff.c */
void VIAvfInitHWDiff(VIAPtr pVia );

/* In via_cursor.c. */
Bool VIAHWCursorInit(ScreenPtr pScreen);
void VIAShowCursor(ScrnInfoPtr);
void VIAHideCursor(ScrnInfoPtr);


/* In via_accel.c. */
Bool VIAInitAccel(ScreenPtr);
void VIAInitialize2DEngine(ScrnInfoPtr);
void VIAAccelSync(ScrnInfoPtr);
void VIAInitLinear(ScreenPtr pScreen);


/* In via_shadow.c */
void VIAPointerMoved(int index, int x, int y);
void VIARefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void VIARefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void VIARefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void VIARefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void VIARefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);


/* In via_bios.c */
void VIAEnableLCD(VIABIOSInfoPtr pBIOSInfo);
void VIADisableLCD(VIABIOSInfoPtr pBIOSInfo);

/* In via_dga.c */
Bool VIADGAInit(ScreenPtr);

/* In via_i2c.c */
Bool VIAI2CInit(ScrnInfoPtr pScrn);

/* In via_gpioi2c.c */
Bool VIAGPIOI2C_Write(VIABIOSInfoPtr pBIOSInfo, int SubAddress, CARD8 Data);
Bool VIAGPIOI2C_Read(VIABIOSInfoPtr pBIOSInfo, int SubAddress, CARD8 *Buffer, int BufferLen);
Bool VIAGPIOI2C_ReadByte(VIABIOSInfoPtr pBIOSInfo, int SubAddress, CARD8 *Buffer);
Bool VIAGPIOI2C_Initial(VIABIOSInfoPtr pBIOSInfo, CARD8 SlaveDevice);

/*In via_video.c*/
void viaInitVideo(ScreenPtr pScreen);
void viaExitVideo(ScrnInfoPtr pScrn);
void viaResetVideo(ScrnInfoPtr pScrn);
void viaSaveVideo(ScrnInfoPtr pScrn);
void viaRestoreVideo(ScrnInfoPtr pScrn);

/*In via_utility.c */
void VIAXVUtilityProc(ScrnInfoPtr pScrn, unsigned char* buf);
Bool VIAUTGetInfo(VIABIOSInfoPtr pBIOSInfo);
Bool VIALoadUserSetting(VIABIOSInfoPtr pBIOSInfo);
Bool VIALoadGammaSetting(VIABIOSInfoPtr pBIOSInfo);
Bool VIARestoreUserSetting(VIABIOSInfoPtr pBIOSInfo);
void VIAUTRemoveRestartFlag(VIABIOSInfoPtr pBIOSInfo);

/* in via_overlay.c */
unsigned long viaOverlayHQVCalcZoomHeight (VIAPtr pVia, unsigned long srcHeight,unsigned long dstHeight,
                             unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag);
unsigned long viaOverlayGetSrcStartAddress (VIAPtr pVia, unsigned long dwVideoFlag,RECTL rSrc,RECTL rDest, unsigned long dwSrcPitch,LPDDPIXELFORMAT lpDPF,unsigned long * lpHQVoffset );
void viaOverlayGetDisplayCount(VIAPtr pVIa, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF,unsigned long dwSrcWidth,unsigned long * lpDisplayCountW);
unsigned long viaOverlayHQVCalcZoomWidth(VIAPtr pVia, unsigned long dwVideoFlag, unsigned long srcWidth , unsigned long dstWidth,
                           unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag);
void viaOverlayGetV1Format(VIAPtr pVia, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl );
void viaOverlayGetV3Format(VIAPtr pVia, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl );

/* In via_memory.c */
void VIAFreeLinear(VIAMemPtr);
unsigned long VIAAllocLinear(VIAMemPtr, ScrnInfoPtr, unsigned long);
void VIAInitPool(VIAPtr, unsigned long, unsigned long);

/* In via_tuner.c */
void ViaTunerStandard(ViaTunerPtr, int);
void ViaTunerBrightness(ViaTunerPtr, int);
void ViaTunerContrast(ViaTunerPtr, int);
void ViaTunerHue(ViaTunerPtr, int);
void ViaTunerLuminance(ViaTunerPtr, int);
void ViaTunerSaturation(ViaTunerPtr, int);
void ViaTunerInput(ViaTunerPtr, int);
#define MODE_TV		0
#define MODE_SVIDEO	1
#define MODE_COMPOSITE	2

void ViaTunerChannel(ViaTunerPtr, int, int);
void ViaAudioSelect(VIAPtr pVia, int tuner);
void ViaAudioInit(VIAPtr pVia);
void ViaAudioMode(VIAPtr pVia, int mode);
void ViaAudioMute(VIAPtr pVia, int mute);
void ViaTunerProbe(ScrnInfoPtr pScrn);
void ViaTunerDestroy(ScrnInfoPtr pScrn);

/* In via_lib.c */
int VIACLECXChipset(VIAPtr pVia);
void VIASetColorspace(VIAPtr pVia, int check_secondary);
void VIAYUVFillBlack(VIAPtr pVia, int offset, int pixels);

/* In via_subp.c */
unsigned long VIACreateSubPictureSurface(ScrnInfoPtr pScrn, CARD32 width, CARD32 height);
void VIADestroySubPictureSurface(ScrnInfoPtr pScrn);
void VIASubPicturePalette(ScrnInfoPtr pScrn);
void VIASubPictureStart(ScrnInfoPtr pScrn, int frame);
void VIASubPictureStop(ScrnInfoPtr pScrn);

/* In via_tv0.c */
unsigned long VIACreateTV0Surface(ScrnInfoPtr pScrn, CARD32 width, CARD32 height);
void VIADestroyTV0Surface(ScrnInfoPtr pScrn);
#endif

#endif /* _VIA_DRIVER_H_ */
