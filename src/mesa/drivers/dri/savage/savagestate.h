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


#ifndef _SAVAGE_STATE_H
#define _SAVAGE_STATE_H

#include "savagecontext.h"

extern void savageDDUpdateHwState( GLcontext *ctx );
extern void savageDDInitState( savageContextPtr imesa );
extern void savageDDInitStateFuncs( GLcontext *ctx );
extern void savageDDRenderStart(GLcontext *ctx);
extern void savageDDRenderEnd(GLcontext *ctx);

/*frank 2001/11/13 add macro for sarea state copy*/
#if 0
#define SAVAGE_STATE_COPY(ctx) { \
ctx->sarea->setup[0]=ctx->Registers.DrawLocalCtrl.ui; \
ctx->sarea->setup[1]=ctx->Registers.TexPalAddr.ui; \
ctx->sarea->setup[2]=ctx->Registers.TexCtrl[0].ui; \
ctx->sarea->setup[3]=ctx->Registers.TexCtrl[1].ui; \
ctx->sarea->setup[4]=ctx->Registers.TexAddr[0].ui; \
ctx->sarea->setup[5]=ctx->Registers.TexAddr[1].ui; \
ctx->sarea->setup[6]=ctx->Registers.TexBlendCtrl[0].ui; \
ctx->sarea->setup[7]=ctx->Registers.TexBlendCtrl[1].ui; \
ctx->sarea->setup[8]=ctx->Registers.TexXprClr.ui; \
ctx->sarea->setup[9]=ctx->Registers.TexDescr.ui; \
ctx->sarea->setup[10]=ctx->Registers.FogTable.ni.ulEntry[0]; \
ctx->sarea->setup[11]=ctx->Registers.FogTable.ni.ulEntry[1]; \
ctx->sarea->setup[12]=ctx->Registers.FogTable.ni.ulEntry[2]; \
ctx->sarea->setup[13]=ctx->Registers.FogTable.ni.ulEntry[3]; \
ctx->sarea->setup[14]=ctx->Registers.FogTable.ni.ulEntry[4]; \
ctx->sarea->setup[15]=ctx->Registers.FogTable.ni.ulEntry[5]; \
ctx->sarea->setup[16]=ctx->Registers.FogTable.ni.ulEntry[6]; \
ctx->sarea->setup[17]=ctx->Registers.FogTable.ni.ulEntry[7]; \
ctx->sarea->setup[18]=ctx->Registers.FogCtrl.ui; \
ctx->sarea->setup[19]=ctx->Registers.StencilCtrl.ui; \
ctx->sarea->setup[20]=ctx->Registers.ZBufCtrl.ui; \
ctx->sarea->setup[21]=ctx->Registers.ZBufOffset.ui; \
ctx->sarea->setup[22]=ctx->Registers.DestCtrl.ui; \
ctx->sarea->setup[23]=ctx->Registers.DrawCtrl0.ui; \
ctx->sarea->setup[24]=ctx->Registers.DrawCtrl1.ui; \
ctx->sarea->setup[25]=ctx->Registers.ZWatermarks.ui; \
ctx->sarea->setup[26]=ctx->Registers.DestTexWatermarks.ui; \
ctx->sarea->setup[27]=ctx->Registers.TexBlendColor.ui; \
}
#endif
#endif
