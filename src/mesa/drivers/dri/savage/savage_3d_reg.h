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


#ifndef SAVAGE_3D_REG_H
#define SAVAGE_3D_REG_H

#define uint32 unsigned long
#define uint16 unsigned short

#define VIDEO_MEM_ADR                   0x02
#define SYSTEM_MEM_ADR                  0x01
#define AGP_MEM_ADR                     0x03

/***********************************************************

  ----------- 3D ENGINE UNIT Registers -------------

  *********************************************************/

typedef union
{
    struct
    {
        unsigned int reserved : 4;
        unsigned int ofs      : 28;
    }ni;
    unsigned int ui;
} savageRegZPixelOffset;

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int cmpFunc     :  3;
        unsigned int stencilEn   :  1;
        unsigned int readMask    :  8;
        unsigned int writeMask   :  8;
        unsigned int failOp      :  3;
        unsigned int passZfailOp :  3;
        unsigned int passZpassOp :  3;
        unsigned int reserved    :  3;
    }ni;
    unsigned int ui;
} savageRegStencilCtrl;

/**************************
 Texture Registers
**************************/
/* The layout of this reg differs between Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int tex0Width  : 4;
        unsigned int tex0Height : 4;
        unsigned int tex0Fmt    : 4;
        unsigned int tex1Width  : 4;
        unsigned int tex1Height : 4;
        unsigned int tex1Fmt    : 4;
        unsigned int texBLoopEn : 1;
        unsigned int tex0En     : 1;
        unsigned int tex1En     : 1;
        unsigned int orthProjEn : 1;
        unsigned int reserved   : 1;
        unsigned int palSize    : 2;
        unsigned int newPal     : 1;
    }ni;
    unsigned int ui;
} savageRegTexDescr_s4;
typedef union
{
    struct
    {
        unsigned int texWidth  : 4;
        unsigned int reserved1 : 4;
        unsigned int texHeight : 4;
        unsigned int reserved2 : 4;
	/* Savage3D supports only the first 8 texture formats defined in
	   enum TexFmt in savge_bci.h. */
        unsigned int texFmt    : 3;
        unsigned int palSize   : 2;
        unsigned int reserved3 : 10;
        unsigned int newPal    : 1;
    }ni;
    unsigned int ui;
} savageRegTexDescr_s3d;

/* The layout of this reg is the same on Savage4 and Savage3D,
   but the Savage4 has two of them, Savage3D has only one. */
typedef union
{
    struct
    {
        unsigned int inSysTex : 1;
        unsigned int inAGPTex : 1;
        unsigned int reserved : 1;
        unsigned int addr     : 29;
    }ni;
    unsigned int ui;
} savageRegTexAddr;

/* The layout of this reg is the same on Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int addr     : 29;
    }ni;
    unsigned int ui;
} savageRegTexPalAddr;

/* The layout of this reg on Savage4 and Savage3D are very similar. */
typedef union
{
    struct
    {
        unsigned int xprClr0 : 16;
        unsigned int xprClr1 : 16; /* this is reserved on Savage3D */
    }ni;
    unsigned int ui;
} savageRegTexXprClr;   /* transparency color in RGB565 format*/

/* The layout of this reg differs between Savage4 and Savage3D.
 * Savage4 has two of them, Savage3D has only one. */
typedef union
{
    struct
    {
        unsigned int filterMode         : 2;
        unsigned int mipmapEnable       : 1;
        unsigned int dBias              : 9;
        unsigned int dMax               : 4;
        unsigned int uMode              : 2;
        unsigned int vMode              : 2;
        unsigned int useDFraction       : 1;
        unsigned int texXprEn           : 1;
        unsigned int clrBlendAlphaSel   : 2;
        unsigned int clrArg1CopyAlpha   : 1;
        unsigned int clrArg2CopyAlpha   : 1;
        unsigned int clrArg1Invert      : 1;
        unsigned int clrArg2Invert      : 1;
        unsigned int alphaBlendAlphaSel : 2;
        unsigned int alphaArg1Invert    : 1;
        unsigned int alphaArg2Invert    : 1;
    }ni;
    unsigned int ui;
} savageRegTexCtrl_s4;
typedef union
{
    struct
    {
        unsigned int filterMode    : 2;
        unsigned int mipmapDisable : 1;
        unsigned int dBias         : 9;
        unsigned int uWrapEn       : 1;
        unsigned int vWrapEn       : 1;
        unsigned int wrapMode      : 2;
        unsigned int texEn         : 1;
        unsigned int useDFraction  : 1;
        unsigned int reserved1     : 1;
	/* Color Compare Alpha Blend Control
           0 -  reduce dest alpha to 0 or 1
           1 - blend with destination
	   The Utah-Driver doesn't know how to use it and sets it to 0. */
        unsigned int CCA           : 1;
        unsigned int texXprEn      : 1;
        unsigned int reserved2     : 11;
    }ni;
    unsigned int ui;
} savageRegTexCtrl_s3d;

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int colorArg1Sel    : 2;
        unsigned int colorArg2Sel    : 3;
        unsigned int colorInvAlphaEn : 1;
        unsigned int colorInvArg2En  : 1;
        unsigned int colorPremodSel  : 1;
        unsigned int colorMod1Sel    : 1;
        unsigned int colorMod2Sel    : 2;
        unsigned int colorAddSel     : 2;
        unsigned int colorDoBlend    : 1;
        unsigned int colorDo2sCompl  : 1;
        unsigned int colorAddBiasEn  : 1;
        unsigned int alphaArg1Sel    : 2;
        unsigned int alphaArg2Sel    : 3;
        unsigned int alphaMod1Sel    : 1;
        unsigned int alphaMod2Sel    : 2;
        unsigned int alphaAdd0Sel    : 1;
        unsigned int alphaDoBlend    : 1;
        unsigned int alphaDo2sCompl  : 1;
        unsigned int colorStageClamp : 1;
        unsigned int alphaStageClamp : 1;
        unsigned int colorDoDiffMul  : 1;
        unsigned int LeftShiftVal    : 2;
    }ni;
    unsigned int ui;
} savageRegTexBlendCtrl;

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int blue  : 8;
        unsigned int green : 8;
        unsigned int red   : 8;
        unsigned int alpha : 8;
    }ni;
    unsigned int ui;
} savageRegTexBlendColor;

/********************************
 Tiled Surface Registers
**********************************/

typedef union
{
    struct
    {
        unsigned int frmBufOffset : 13;
        unsigned int reserved     : 12;
        unsigned int widthInTile  : 6;
        unsigned int bitPerPixel  : 1;
    }ni;
    unsigned int ui;
} savageRegTiledSurface;

/********************************
 Draw/Shading Control Registers
**********************************/

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int scissorXStart : 11;
        unsigned int dPerfAccelEn  : 1;
        unsigned int scissorYStart : 12;
        unsigned int alphaRefVal   : 8;
    }ni;
    unsigned int ui;
} savageRegDrawCtrl0;

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int scissorXEnd      : 11;
        unsigned int xyOffsetEn       :  1;
        unsigned int scissorYEnd      : 12;
        unsigned int ditherEn         :  1;
        unsigned int nonNormTexCoord  :  1;
        unsigned int cullMode         :  2;
        unsigned int alphaTestCmpFunc :  3;
        unsigned int alphaTestEn      :  1;
    }ni;
    unsigned int ui;
} savageRegDrawCtrl1;

/* This reg exists only on Savage4. */
typedef union
{
    struct
    {
        unsigned int dstAlphaMode        :  3;
        unsigned int dstMinusSrc         :  1;
        unsigned int srcAlphaMode        :  3;
        unsigned int binaryFinalAlpha    :  1;
        unsigned int dstAlphaModeHighBit :  1;
        unsigned int srcAlphaModeHighBit :  1;
        unsigned int reserved1           : 15;
        unsigned int wrZafterAlphaTst    :  1;
        unsigned int drawUpdateEn        :  1;
        unsigned int zUpdateEn           :  1;
        unsigned int flatShadeEn         :  1;
        unsigned int specShadeEn         :  1;
        unsigned int flushPdDestWrites   :  1;
        unsigned int flushPdZbufWrites   :  1;
    }ni;
    unsigned int ui;
} savageRegDrawLocalCtrl;

/* This reg exists only on Savage3D. */
typedef union
{
    struct
    {
        unsigned int ditherEn          : 1;
        unsigned int xyOffsetEn        : 1;
        unsigned int cullMode          : 2;
        unsigned int vertexCountReset  : 1;
        unsigned int flatShadeEn       : 1;
        unsigned int specShadeEn       : 1;
        unsigned int dstAlphaMode      : 3;
        unsigned int srcAlphaMode      : 3;
        unsigned int reserved1         : 1;
        unsigned int alphaTestCmpFunc  : 3;
        unsigned int alphaTestEn       : 1;
        unsigned int alphaRefVal       : 8;
        unsigned int texBlendCtrl      : 3;
        unsigned int flushPdDestWrites : 1;
        unsigned int flushPdZbufWrites : 1;
	/* havn't found an equivalent for Savage4. Utah-driver sets it to 0. */
        unsigned int interpMode        : 1;
    }ni;
    unsigned int ui;
} savageRegDrawCtrl;

#define SAVAGETBC_DECAL_S3D                     0
#define SAVAGETBC_MODULATE_S3D                  1
#define SAVAGETBC_DECALALPHA_S3D                2
#define SAVAGETBC_MODULATEALPHA_S3D             3
#define SAVAGETBC_4_S3D                         4
#define SAVAGETBC_5_S3D                         5
#define SAVAGETBC_COPY_S3D                      6
#define SAVAGETBC_7_S3D                         7

/* This reg exists only on Savage3D. */
typedef union
{
    struct
    {
        unsigned int scissorXStart : 11;
	unsigned int reserved1     : 5;
        unsigned int scissorYStart : 11;
	unsigned int reserved2     : 5;
    } ni;
    unsigned int ui;
} savageRegScissorsStart;

/* This reg exists only on Savage3D. */
typedef union
{
    struct
    {
        unsigned int scissorXEnd : 11;
	unsigned int reserved1   : 5;
        unsigned int scissorYEnd : 11;
	unsigned int reserved2   : 5;
    } ni;
    unsigned int ui;
} savageRegScissorsEnd;

/********************************
 Address Registers
**********************************/

/* I havn't found a Savage3D equivalent of this reg in the Utah-driver. 
 * But Tim Roberts claims that the Savage3D supports DMA vertex and
 * command buffers. */
typedef union
{
    struct
    {
        unsigned int isSys    : 1;
        unsigned int isAGP    : 1;
        unsigned int reserved : 1;
        unsigned int addr     : 29; /*quad word aligned*/
    }ni;
    unsigned int ui;
} savageRegVertBufAddr;

/* I havn't found a Savage3D equivalent of this reg in the Utah-driver. 
 * But Tim Roberts claims that the Savage3D supports DMA vertex and
 * command buffers. */
typedef union
{
    struct
    {
        unsigned int isSys    : 1;
        unsigned int isAGP    : 1;
        unsigned int reserved : 1;
        unsigned int addr     : 29; /*4-quad word aligned*/
    }ni;
    unsigned int ui;
} savageRegDMABufAddr;

/********************************
 H/W Debug Registers
**********************************/
/* The layout of this reg is the same on Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int y01        : 1;
        unsigned int y12        : 1;
        unsigned int y20        : 1;
        unsigned int u01        : 1;
        unsigned int u12        : 1;
        unsigned int u20        : 1;
        unsigned int v01        : 1;
        unsigned int v12        : 1;
        unsigned int v20        : 1;
        unsigned int cullEn     : 1;
        unsigned int cullOrient : 1;
        unsigned int loadNewTex : 1;
        unsigned int loadNewPal : 1;
        unsigned int doDSetup   : 1;
        unsigned int reserved   : 17;
        unsigned int kickOff    : 1;
    }ni;
    unsigned int ui;
} savageRegFlag;

/********************************
 Z Buffer Registers -- Global
**********************************/

/* The layout of this reg differs between Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int zCmpFunc      : 3;
        unsigned int reserved1     : 2;
        unsigned int zBufEn        : 1;
        unsigned int reserved2     : 1;
        unsigned int zExpOffset    : 8;
        unsigned int reserved3     : 1;
        unsigned int stencilRefVal : 8;
        unsigned int autoZEnable   : 1;
        unsigned int frameID       : 1;
        unsigned int reserved4     : 4;
        unsigned int floatZEn      : 1;
        unsigned int wToZEn        : 1;
    }ni;
    unsigned int ui;
} savageRegZBufCtrl_s4;
typedef union
{
    struct {
        unsigned int zCmpFunc         : 3;
        unsigned int drawUpdateEn     : 1;
        unsigned int zUpdateEn        : 1;
        unsigned int zBufEn           : 1;
        unsigned int reserved1        : 2;
        unsigned int zExpOffset       : 8;
        unsigned int wrZafterAlphaTst : 1;
        unsigned int reserved2        : 15;
    }ni;
    unsigned int ui;
} savageRegZBufCtrl_s3d;

/* The layout of this reg on Savage4 and Savage3D is very similar. */
typedef union
{
    struct
    {
	/* In the Utah-Driver the offset is defined as 13-bit, 2k-aligned. */
        unsigned int offset           : 14;
        unsigned int reserved         : 11; /* 12-bits in Utah-driver */
        unsigned int zBufWidthInTiles : 6;
        unsigned int zDepthSelect     : 1;
    }ni;
    unsigned int ui;
} savageRegZBufOffset;

/* The layout of this reg is the same on Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int rLow      : 6;
        unsigned int reserved1 : 2;
        unsigned int rHigh     : 6;
        unsigned int reserved2 : 2;
        unsigned int wLow      : 6;
        unsigned int reserved3 : 2;
        unsigned int wHigh     : 6;
        unsigned int reserved4 : 2;
    }ni;
    unsigned int ui;
} savageRegZWatermarks;

/********************************
 Fog Registers -- Global
**********************************/
/* The layout of this reg is the same on Savage4 and Savage3D. */
typedef union
{
    struct
    {
        unsigned int fogClr      : 24;
        unsigned int expShift    : 3;
        unsigned int reserved    : 1;
        unsigned int fogEn       : 1;
        unsigned int fogMode     : 1;
        unsigned int fogEndShift : 2;
    }ni;
    unsigned int ui;
} savageRegFogCtrl;

/*not in spec, but tempo for pp and driver*/
typedef union
{
    struct
    {
        unsigned int fogDensity : 16;
        unsigned int fogStart   : 16;
    }ni;
    unsigned int ui;
} savageRegFogParam;

/**************************************
 Destination Buffer Registers -- Global
***************************************/

/* The layout of this reg on Savage4 and Savage3D are very similar. */
typedef union
{
    struct
    {
        unsigned int dstWidthInTile :  7;
        unsigned int reserved       :  1;
	/* In the Utah-Driver the offset is defined as 13-bit, 2k-aligned. */
        unsigned int offset         : 14;
        unsigned int reserved1      :  7;
	/* antiAliasMode does not exist in the Utah-driver. But it includes the
	 * high bit of this in the destPixFmt. However, only values 0 and 2
	 * are used as dstPixFmt, so antiAliasMode is effectively always 0
	 * in the Utah-driver. In other words, treat as reserved on Savage3D.*/
        unsigned int antiAliasMode  :  2;
        unsigned int dstPixFmt      :  1;
    }ni;
    unsigned int ui;
} savageRegDestCtrl;

/* The layout of this reg on Savage4 and Savage3D are very similar. */
typedef union
{
    struct
    {
        unsigned int destReadLow   : 6;
        unsigned int destReadHigh  : 6;
        unsigned int destWriteLow  : 6;
        unsigned int destWriteHigh : 6;
        unsigned int texRead       : 4;
        unsigned int reserved4     : 2;
	/* The Utah-driver calls this pixel FIFO length:
	 * 00 - 240, 01 - 180, 10 - 120, 11 - 60
	 * However, it is not used in either driver. */
        unsigned int destFlush     : 2;
    }ni;
    unsigned int ui;
} savageRegDestTexWatermarks;

/* Savage4/Twister/ProSavage register BCI addresses */
#define SAVAGE_DRAWLOCALCTRL_S4       0x1e
#define SAVAGE_TEXPALADDR_S4          0x1f
#define SAVAGE_TEXCTRL0_S4            0x20
#define SAVAGE_TEXCTRL1_S4            0x21
#define SAVAGE_TEXADDR0_S4            0x22
#define SAVAGE_TEXADDR1_S4            0x23
#define SAVAGE_TEXBLEND0_S4           0x24
#define SAVAGE_TEXBLEND1_S4           0x25
#define SAVAGE_TEXXPRCLR_S4           0x26 /* never used */
#define SAVAGE_TEXDESCR_S4            0x27
#define SAVAGE_FOGTABLE_S4            0x28
#define SAVAGE_FOGCTRL_S4             0x30
#define SAVAGE_STENCILCTRL_S4         0x31
#define SAVAGE_ZBUFCTRL_S4            0x32
#define SAVAGE_ZBUFOFF_S4             0x33
#define SAVAGE_DESTCTRL_S4            0x34
#define SAVAGE_DRAWCTRLGLOBAL0_S4     0x35
#define SAVAGE_DRAWCTRLGLOBAL1_S4     0x36
#define SAVAGE_ZWATERMARK_S4          0x37
#define SAVAGE_DESTTEXRWWATERMARK_S4  0x38
#define SAVAGE_TEXBLENDCOLOR_S4       0x39
/* Savage3D/MX/IC register BCI addresses */
#define SAVAGE_TEXPALADDR_S3D         0x18
#define SAVAGE_TEXXPRCLR_S3D          0x19 /* never used */
#define SAVAGE_TEXADDR_S3D            0x1A
#define SAVAGE_TEXDESCR_S3D           0x1B
#define SAVAGE_TEXCTRL_S3D            0x1C
#define SAVAGE_FOGTABLE_S3D           0x20
#define SAVAGE_FOGCTRL_S3D            0x30
#define SAVAGE_DRAWCTRL_S3D           0x31
#define SAVAGE_ZBUFCTRL_S3D           0x32
#define SAVAGE_ZBUFOFF_S3D            0x33
#define SAVAGE_DESTCTRL_S3D           0x34
#define SAVAGE_SCSTART_S3D            0x35
#define SAVAGE_SCEND_S3D              0x36
#define SAVAGE_ZWATERMARK_S3D         0x37 
#define SAVAGE_DESTTEXRWWATERMARK_S3D 0x38

#define SAVAGE_FIRST_REG 0x18
#define SAVAGE_NR_REGS   34
typedef struct savage_registers_s4_t {
    unsigned int               unused1[6];        /* 0x18-0x1d */
    savageRegDrawLocalCtrl     drawLocalCtrl;     /* 0x1e */
    savageRegTexPalAddr        texPalAddr;        /* 0x1f */
    savageRegTexCtrl_s4        texCtrl[2];        /* 0x20, 0x21 */
    savageRegTexAddr           texAddr[2];        /* 0x22, 0x23 */
    savageRegTexBlendCtrl      texBlendCtrl[2];   /* 0x24, 0x25 */
    savageRegTexXprClr         texXprClr;         /* 0x26 */
    savageRegTexDescr_s4       texDescr;          /* 0x27 */
    unsigned char              fogTable[32];      /* 0x28-0x2f (8dwords) */
    savageRegFogCtrl           fogCtrl;           /* 0x30 */
    savageRegStencilCtrl       stencilCtrl;       /* 0x31 */
    savageRegZBufCtrl_s4       zBufCtrl;          /* 0x32 */
    savageRegZBufOffset        zBufOffset;        /* 0x33 */
    savageRegDestCtrl          destCtrl;          /* 0x34 */
    savageRegDrawCtrl0         drawCtrl0;         /* 0x35 */
    savageRegDrawCtrl1         drawCtrl1;         /* 0x36 */
    savageRegZWatermarks       zWatermarks;       /* 0x37 */
    savageRegDestTexWatermarks destTexWatermarks; /* 0x38 */
    savageRegTexBlendColor     texBlendColor;     /* 0x39 */
} savageRegistersS4;
typedef struct savage_registers_s3d_t {
    savageRegTexPalAddr        texPalAddr;        /* 0x18 */
    savageRegTexXprClr         texXprClr;         /* 0x19 */
    savageRegTexAddr           texAddr;           /* 0x1a */
    savageRegTexDescr_s3d      texDescr;          /* 0x1b */
    savageRegTexCtrl_s3d       texCtrl;           /* 0x1c */
    unsigned int               unused1[3];        /* 0x1d-0x1f */
    unsigned char              fogTable[64];      /* 0x20-0x2f (16dwords) */
    savageRegFogCtrl           fogCtrl;           /* 0x30 */
    savageRegDrawCtrl          drawCtrl;          /* 0x31 */
    savageRegZBufCtrl_s3d      zBufCtrl;          /* 0x32 */
    savageRegZBufOffset        zBufOffset;        /* 0x33 */
    savageRegDestCtrl          destCtrl;          /* 0x34 */
    savageRegScissorsStart     scissorsStart;     /* 0x35 */
    savageRegScissorsEnd       scissorsEnd;       /* 0x36 */
    savageRegZWatermarks       zWatermarks;       /* 0x37 */
    savageRegDestTexWatermarks destTexWatermarks; /* 0x38 */
    unsigned int               unused2;           /* 0x39 */
} savageRegistersS3D;
typedef union savage_registers_t {
    savageRegistersS4  s4;
    savageRegistersS3D s3d;
    unsigned int       ui[SAVAGE_NR_REGS];
} savageRegisters;


#define DV_PF_555           (0x1<<8)
#define DV_PF_565           (0x2<<8)
#define DV_PF_8888          (0x4<<8)

#define SAVAGEPACKCOLORA4L4(l,a) \
  ((l >> 4) | (a & 0xf0))

#define SAVAGEPACKCOLOR4444(r,g,b,a) \
  ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

#define SAVAGEPACKCOLOR1555(r,g,b,a) \
  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
    ((a) ? 0x8000 : 0))

#define SAVAGEPACKCOLOR8888(r,g,b,a) \
  (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define SAVAGEPACKCOLOR565(r,g,b) \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))



/*AlphaFunc*/
#define LCS_A_NEVER              0x0200
#define LCS_A_LESS               0x0201
#define LCS_A_EQUAL              0x0202
#define LCS_A_LEQUAL             0x0203
#define LCS_A_GREATER            0x0204
#define LCS_A_NOTEQUAL           0x0205
#define LCS_A_GEQUAL             0x0206
#define LCS_A_ALWAYS             0x0207
/*stencilFunc*/
#define LCS_S_NEVER              0x0200
#define LCS_S_LESS               0x0201
#define LCS_S_EQUAL              0x0202
#define LCS_S_LEQUAL             0x0203
#define LCS_S_GREATER            0x0204
#define LCS_S_NOTEQUAL           0x0205
#define LCS_S_GEQUAL             0x0206
#define LCS_S_ALWAYS             0x0207
/*depthFunc*/

#define LCS_Z_NEVER              0x0200
#define LCS_Z_LESS               0x0201
#define LCS_Z_EQUAL              0x0202
#define LCS_Z_LEQUAL             0x0203
#define LCS_Z_GREATER            0x0204
#define LCS_Z_NOTEQUAL           0x0205
#define LCS_Z_GEQUAL             0x0206
#define LCS_Z_ALWAYS             0x0207
#if 0
#define LCS_UPDATE_LINEWIDTH    (0x1<<15)
#define LCS_LINEWIDTH_MASK      (0x7<<12)
#define LCS_LINEWIDTH_SHIFT           12
#define LCS_LINEWIDTH_0_5       (0x1<<12)
#define LCS_LINEWIDTH_1_0       (0x2<<12)
#define LCS_LINEWIDTH_2_0       (0x4<<12)
#define LCS_LINEWIDTH_3_0       (0x6<<12)
#define LCS_UPDATE_ALPHA_INTERP (0x1<<11)
#define LCS_ALPHA_FLAT          (0x1<<10)
#define LCS_ALPHA_INTERP        (0x0<<10)
#define LCS_UPDATE_FOG_INTERP   (0x1<<9)
#define LCS_FOG_INTERP          (0x0<<8)
#define LCS_FOG_FLAT            (0x1<<8)
#define LCS_UPDATE_SPEC_INTERP  (0x1<<7)
#define LCS_SPEC_INTERP         (0x0<<6)
#define LCS_SPEC_FLAT           (0x1<<6)
#define LCS_UPDATE_RGB_INTERP   (0x1<<5)
#define LCS_RGB_INTERP          (0x0<<4)
#define LCS_RGB_FLAT            (0x1<<4)
#define LCS_UPDATE_CULL_MODE    (0x1<<3)
#define LCS_CULL_MASK           (0x7<<0)
#define LCS_CULL_DISABLE        (0x1<<0)
#define LCS_CULL_CW             (0x2<<0)
#define LCS_CULL_CCW            (0x3<<0)
#define LCS_CULL_BOTH           (0x4<<0)

#define LCS_INTERP_FLAT (LCS_ALPHA_FLAT|LCS_RGB_FLAT|LCS_SPEC_FLAT)
#define LCS_UPDATE_INTERP (LCS_UPDATE_ALPHA_INTERP| 	\
			   LCS_UPDATE_RGB_INTERP|	\
			   LCS_UPDATE_SPEC_INTERP)

#endif 

/*#define GFX_OP_PRIMITIVE     ((0x3<<29)|(0x1f<<24))*/
#define PR_TRIANGLES         (0x0<<18)
/*#define PR_TRISTRIP_0        (0x1<<18)*/
/*#define PR_TRISTRIP_1        (0x2<<18)*/
/*#define PR_TRIFAN            (0x3<<18)*/
#define PR_POLYGON           (0x4<<18)
#define PR_LINES             (0x5<<18)
/*#define PR_LINESTRIP         (0x6<<18)*/
/*#define PR_RECTS             (0x7<<18)*/

/* GFXRENDERSTATE_MAP_COORD_SETS, p116
 */
#define GFX_OP_MAP_COORD_SETS ((0x3<<29)|(0x1c<<24)|(0x1<<19))
#define MCS_COORD_ID_SHIFT         16
#define MCS_COORD_0                (0<<16)
#define MCS_COORD_1                (1<<16)
#define MCS_UPDATE_NORMALIZED      (1<<15)
#define MCS_NORMALIZED_COORDS_MASK (1<<14)
#define MCS_NORMALIZED_COORDS      (1<<14)
#define MCS_UPDATE_V_STATE         (1<<7)
#define MCS_V_STATE_MASK           (0x3<<4)
#define MCS_V_WRAP                 (0x0<<4)
#define MCS_V_MIRROR               (0x1<<4)
#define MCS_V_CLAMP                (0x2<<4)
#define MCS_V_WRAP_SHORTEST        (0x3<<4)
#define MCS_UPDATE_U_STATE         (1<<3)
#define MCS_U_STATE_MASK           (0x3<<0)
#define MCS_U_WRAP                 (0x0<<0)
#define MCS_U_MIRROR               (0x1<<0)
#define MCS_U_CLAMP                (0x2<<0)
#define MCS_U_WRAP_SHORTEST        (0x3<<0)

#define GFX_OP_MAP_TEXELS   ((0x3<<29)|(0x1c<<24)|(0x0<<19))
#define MT_UPDATE_TEXEL1_STATE     (1<<15)
#define MT_TEXEL1_DISABLE          (0<<14)
#define MT_TEXEL1_ENABLE           (1<<14)
#define MT_TEXEL1_COORD0           (0<<11)
#define MT_TEXEL1_COORD1           (1<<11)
#define MT_TEXEL1_MAP0             (0<<8)
#define MT_TEXEL1_MAP1             (1<<8)
#define MT_UPDATE_TEXEL0_STATE     (1<<7)
#define MT_TEXEL0_DISABLE          (0<<6)
#define MT_TEXEL0_ENABLE           (1<<6)
#define MT_TEXEL0_COORD0           (0<<3)
#define MT_TEXEL0_COORD1           (1<<3)
#define MT_TEXEL0_MAP0             (0<<0)
#define MT_TEXEL0_MAP1             (1<<0)

#define GFX_OP_COLOR_FACTOR      ((0x3<<29)|(0x1d<<24)|(0x1<<16)|0x0)

/* GFXRENDERSTATE_MAP_ALPHA_BLEND_STAGES, p132
 */
#define GFX_OP_MAP_ALPHA_STAGES  ((0x3<<29)|(0x1<<24))
#define MA_STAGE_SHIFT           20
#define MA_STAGE_0               (0<<20)
#define MA_STAGE_1               (1<<20)
#define MA_STAGE_2               (2<<20)
#define MA_UPDATE_ARG1           (1<<18)
#define MA_ARG1_MASK             ((0x7<<15)|(0x1<<13))
#define MA_ARG1_ALPHA_FACTOR     (0x1<<15)
#define MA_ARG1_ITERATED_ALPHA   (0x3<<15)
#define MA_ARG1_CURRENT_ALPHA    (0x5<<15)
#define MA_ARG1_TEX0_ALPHA       (0x6<<15)
#define MA_ARG1_TEX1_ALPHA       (0x7<<15)
#define MA_ARG1_INVERT           (0x1<<13)
#define MA_ARG1_DONT_INVERT      (0x0<<13)
#define MA_UPDATE_ARG2           (1<<12)
#define MA_ARG2_MASK             ((0x7<<8)|(0x1<<6))
#define MA_ARG2_ALPHA_FACTOR     (0x1<<8)
#define MA_ARG2_ITERATED_ALPHA   (0x3<<8)
#define MA_ARG2_CURRENT_ALPHA    (0x5<<8)
#define MA_ARG2_TEX0_ALPHA       (0x6<<8)
#define MA_ARG2_TEX1_ALPHA       (0x7<<8)
#define MA_ARG2_INVERT           (0x1<<6)
#define MA_ARG2_DONT_INVERT      (0x0<<6)
#define MA_UPDATE_OP             (1<<5)
#define MA_OP_MASK                   (0xf)
#define MA_OP_ARG1                   (0x1)
#define MA_OP_ARG2                   (0x2)
#define MA_OP_MODULATE               (0x3)
#define MA_OP_MODULATE_X2            (0x4)
#define MA_OP_MODULATE_X4            (0x5)
#define MA_OP_ADD                    (0x6)
#define MA_OP_ADD_SIGNED             (0x7)
#define MA_OP_LIN_BLEND_ITER_ALPHA   (0x8)
#define MA_OP_LIN_BLEND_ALPHA_FACTOR (0xa)
#define MA_OP_LIN_BLEND_TEX0_ALPHA   (0x10)
#define MA_OP_LIN_BLEND_TEX1_ALPHA   (0x11)


/* GFXRENDERSTATE_MAP_COLOR_BLEND_STAGES, p129
 */
#define GFX_OP_MAP_COLOR_STAGES  ((0x3<<29)|(0x0<<24))
#define MC_STAGE_SHIFT           20
#define MC_STAGE_0               (0<<20)
#define MC_STAGE_1               (1<<20)
#define MC_STAGE_2               (2<<20)
#define MC_UPDATE_DEST           (1<<19)
#define MC_DEST_MASK             (1<<18)
#define MC_DEST_CURRENT          (0<<18)
#define MC_DEST_ACCUMULATOR      (1<<18)
#define MC_UPDATE_ARG1           (1<<17)
#define MC_ARG1_MASK             ((0x7<<14)|(0x1<<13)|(0x1<<12))
#define MC_ARG1_ONE              (0x0<<14)
#define MC_ARG1_COLOR_FACTOR     (0x1<<14)
#define MC_ARG1_ACCUMULATOR      (0x2<<14)
#define MC_ARG1_ITERATED_COLOR   (0x3<<14)
#define MC_ARG1_SPECULAR_COLOR   (0x4<<14)
#define MC_ARG1_CURRENT_COLOR    (0x5<<14)
#define MC_ARG1_TEX0_COLOR       (0x6<<14)
#define MC_ARG1_TEX1_COLOR       (0x7<<14)
#define MC_ARG1_DONT_REPLICATE_ALPHA   (0x0<<13)
#define MC_ARG1_REPLICATE_ALPHA        (0x1<<13)
#define MC_ARG1_DONT_INVERT      (0x0<<12)
#define MC_ARG1_INVERT           (0x1<<12)
#define MC_UPDATE_ARG2           (1<<11)
#define MC_ARG2_MASK             ((0x7<<8)|(0x1<<7)|(0x1<<6))
#define MC_ARG2_ONE              (0x0<<8)
#define MC_ARG2_COLOR_FACTOR     (0x1<<8)
#define MC_ARG2_ACCUMULATOR      (0x2<<8)
#define MC_ARG2_ITERATED_COLOR   (0x3<<8)
#define MC_ARG2_SPECULAR_COLOR   (0x4<<8)
#define MC_ARG2_CURRENT_COLOR    (0x5<<8)
#define MC_ARG2_TEX0_COLOR       (0x6<<8)
#define MC_ARG2_TEX1_COLOR       (0x7<<8)
#define MC_ARG2_DONT_REPLICATE_ALPHA   (0x0<<7)
#define MC_ARG2_REPLICATE_ALPHA        (0x1<<7)
#define MC_ARG2_DONT_INVERT      (0x0<<6)
#define MC_ARG2_INVERT           (0x1<<6)
#define MC_UPDATE_OP             (1<<5)
#define MC_OP_MASK                   (0xf)
#define MC_OP_DISABLE                (0x0)
#define MC_OP_ARG1                   (0x1)
#define MC_OP_ARG2                   (0x2)
#define MC_OP_MODULATE               (0x3)
#define MC_OP_MODULATE_X2            (0x4)
#define MC_OP_MODULATE_X4            (0x5)
#define MC_OP_ADD                    (0x6)
#define MC_OP_ADD_SIGNED             (0x7)
#define MC_OP_LIN_BLEND_ITER_ALPHA   (0x8)
#define MC_OP_LIN_BLEND_ALPHA_FACTOR (0xa)
#define MC_OP_LIN_BLEND_TEX0_ALPHA   (0x10)
#define MC_OP_LIN_BLEND_TEX1_ALPHA   (0x11)
#define MC_OP_LIN_BLEND_TEX0_COLOR   (0x12)
#define MC_OP_LIN_BLEND_TEX1_COLOR   (0x13)
#define MC_OP_SUBTRACT               (0x14)

/* GFXRENDERSTATE_MAP_PALETTE_LOAD, p128
 * 
 * Format:
 *     0:  GFX_OP_MAP_PALETTE_LOAD
 *     1:  16bpp color[0]
 *     ...
 *     256: 16bpp color[255]
 */
#define GFX_OP_MAP_PALETTE_LOAD ((0x3<<29)|(0x1d<<24)|(0x82<<16)|0xff)

/* GFXRENDERSTATE_MAP_LOD_CONTROL, p127
 */
#define GFX_OP_MAP_LOD_CTL       ((0x3<<29)|(0x1c<<24)|(0x4<<19))
#define MLC_MAP_ID_SHIFT         16
#define MLC_MAP_0                (0<<16)
#define MLC_MAP_1                (1<<16)
#define MLC_UPDATE_DITHER_WEIGHT (1<<10)
#define MLC_DITHER_WEIGHT_MASK   (0x3<<8)
#define MLC_DITHER_WEIGHT_FULL   (0x0<<8)
#define MLC_DITHER_WEIGHT_50     (0x1<<8)
#define MLC_DITHER_WEIGHT_25     (0x2<<8)
#define MLC_DITHER_WEIGHT_12     (0x3<<8)
#define MLC_UPDATE_LOD_BIAS      (1<<7)
#define MLC_LOD_BIAS_MASK        ((1<<7)-1)

/* GFXRENDERSTATE_MAP_LOD_LIMITS, p126
 */
#define GFX_OP_MAP_LOD_LIMITS   ((0x3<<29)|(0x1c<<24)|(0x3<<19))
#define MLL_MAP_ID_SHIFT         16
#define MLL_MAP_0                (0<<16)
#define MLL_MAP_1                (1<<16)
#define MLL_UPDATE_MAX_MIP       (1<<13)
#define MLL_MAX_MIP_SHIFT        5
#define MLL_MAX_MIP_MASK         (0xff<<5)
#define MLL_MAX_MIP_ONE          (0x10<<5)
#define MLL_UPDATE_MIN_MIP       (1<<4)
#define MLL_MIN_MIP_SHIFT        0
#define MLL_MIN_MIP_MASK         (0xf<<0)

/* GFXRENDERSTATE_MAP_FILTER, p124
 */
#define GFX_OP_MAP_FILTER       ((0x3<<29)|(0x1c<<24)|(0x2<<19))
#define MF_MAP_ID_SHIFT         16
#define MF_MAP_0                (0<<16)
#define MF_MAP_1                (1<<16)
#define MF_UPDATE_ANISOTROPIC   (1<<12)
#define MF_ANISOTROPIC_MASK     (1<<10)
#define MF_ANISOTROPIC_ENABLE   (1<<10)
#define MF_UPDATE_MIP_FILTER    (1<<9)
#define MF_MIP_MASK             (0x3<<6)
#define MF_MIP_NONE             (0x0<<6)
#define MF_MIP_NEAREST          (0x1<<6)
#define MF_MIP_DITHER           (0x2<<6)
#define MF_MIP_LINEAR           (0x3<<6)
#define MF_UPDATE_MAG_FILTER    (1<<5)
#define MF_MAG_MASK             (1<<3)
#define MF_MAG_LINEAR           (1<<3)
#define MF_MAG_NEAREST          (0<<3)
#define MF_UPDATE_MIN_FILTER    (1<<2)
#define MF_MIN_MASK             (1<<0)
#define MF_MIN_LINEAR           (1<<0)
#define MF_MIN_NEAREST          (0<<0)

/* GFXRENDERSTATE_MAP_INFO, p118
 */
#define GFX_OP_MAP_INFO      ((0x3<<29)|(0x1d<<24)|0x2)
#define MI1_MAP_ID_SHIFT         28
#define MI1_MAP_0                (0<<28)
#define MI1_MAP_1                (1<<28)
#define MI1_FMT_MASK             (0x7<<24)
#define MI1_FMT_8CI              (0x0<<24)
#define MI1_FMT_8BPP             (0x1<<24)
#define MI1_FMT_16BPP            (0x2<<24)
#define MI1_FMT_422              (0x5<<24)
#define MI1_PF_MASK              (0x3<<21)
#define MI1_PF_8CI_RGB565         (0x0<<21)
#define MI1_PF_8CI_ARGB1555       (0x1<<21)
#define MI1_PF_8CI_ARGB4444       (0x2<<21)
#define MI1_PF_8CI_AY88           (0x3<<21)
#define MI1_PF_16BPP_RGB565       (0x0<<21)
#define MI1_PF_16BPP_ARGB1555     (0x1<<21)
#define MI1_PF_16BPP_ARGB4444     (0x2<<21)
#define MI1_PF_16BPP_AY88         (0x3<<21)
#define MI1_PF_422_YCRCB_SWAP_Y   (0x0<<21)
#define MI1_PF_422_YCRCB          (0x1<<21)
#define MI1_PF_422_YCRCB_SWAP_UV  (0x2<<21)
#define MI1_PF_422_YCRCB_SWAP_YUV (0x3<<21)
#define MI1_OUTPUT_CHANNEL_MASK   (0x3<<19)
#define MI1_COLOR_CONV_ENABLE     (1<<18)
#define MI1_VERT_STRIDE_MASK      (1<<17)
#define MI1_VERT_STRIDE_1         (1<<17)
#define MI1_VERT_OFFSET_MASK      (1<<16)
#define MI1_VERT_OFFSET_1         (1<<16)
#define MI1_ENABLE_FENCE_REGS     (1<<10)
#define MI1_TILED_SURFACE         (1<<9)
#define MI1_TILE_WALK_X           (0<<8)
#define MI1_TILE_WALK_Y           (1<<8)
#define MI1_PITCH_MASK            (0xf<<0)
#define MI2_DIMENSIONS_ARE_LOG2   (1<<31)
#define MI2_DIMENSIONS_ARE_EXACT  (0<<31)
#define MI2_HEIGHT_SHIFT          16
#define MI2_HEIGHT_MASK           (0x1ff<<16)
#define MI2_WIDTH_SHIFT           0
#define MI2_WIDTH_MASK            (0x1ff<<0)
#define MI3_BASE_ADDR_MASK        (~0xf)

#define SAVAGE_VFMT_T0 (GFX_OP_VERTEX_FMT |	\
		      VF_TEXCOORD_COUNT_1 |	\
		      VF_SPEC_FOG_ENABLE |	\
		      VF_RGBA_ENABLE |		\
		      VF_XYZW)

#define SAVAGE_VFMT_T0T1 (GFX_OP_VERTEX_FMT |	\
		        VF_TEXCOORD_COUNT_2 |	\
		        VF_SPEC_FOG_ENABLE |	\
		        VF_RGBA_ENABLE |	\
		        VF_XYZW)

#define GFX_OP_VERTEX_FMT  ((0x3<<29)|(0x5<<24))
#define VF_TEXCOORD_COUNT_SHIFT    8
#define VF_TEXCOORD_COUNT_0        (0<<8)
#define VF_TEXCOORD_COUNT_1        (1<<8)
#define VF_TEXCOORD_COUNT_2        (2<<8)
#define VF_SPEC_FOG_ENABLE         (1<<7)
#define VF_RGBA_ENABLE             (1<<6)
#define VF_Z_OFFSET_ENABLE         (1<<5)
#define VF_XYZ                     (0x1<<1)
#define VF_XYZW                    (0x2<<1)
#define VF_XY                      (0x3<<1)
#define VF_XYW                     (0x4<<1)

/* Master data transfer engine */
#define MDT_SRCADD_ALIGMENT (~0x1fUL)
#define MDT_SRC_PCI 0x1
#define MDT_SRC_AGP 0x3
#endif


