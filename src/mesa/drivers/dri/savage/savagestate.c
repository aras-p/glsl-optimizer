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


#include <stdio.h>

#include "mtypes.h"
#include "enums.h"
#include "macros.h"
#include "dd.h"

#include "mm.h"
#include "savagedd.h"
#include "savagecontext.h"

#include "savagestate.h"
#include "savagetex.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "savage_bci.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

static void savageBlendFunc_s4(GLcontext *);
static void savageBlendFunc_s3d(GLcontext *);

static __inline__ GLuint savagePackColor(GLuint format, 
                                         GLubyte r, GLubyte g, 
                                         GLubyte b, GLubyte a)
{
    switch (format) {
        case DV_PF_8888:
            return SAVAGEPACKCOLOR8888(r,g,b,a);
        case DV_PF_565:
            return SAVAGEPACKCOLOR565(r,g,b);
        default:
            
            return 0;
    }
}


static void savageDDAlphaFunc_s4(GLcontext *ctx, GLenum func, GLfloat ref)
{
    /* This can be done in BlendFunc*/
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    imesa->dirty |= SAVAGE_UPLOAD_CTX;
    savageBlendFunc_s4(ctx);
}
static void savageDDAlphaFunc_s3d(GLcontext *ctx, GLenum func, GLfloat ref)
{
    /* This can be done in BlendFunc*/
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    imesa->dirty |= SAVAGE_UPLOAD_CTX;
    savageBlendFunc_s3d(ctx);
}

static void savageDDBlendEquationSeparate(GLcontext *ctx,
					  GLenum modeRGB, GLenum modeA)
{
    assert( modeRGB == modeA );

    /* BlendEquation sets ColorLogicOpEnabled in an unexpected 
     * manner.  
     */
    FALLBACK( ctx, SAVAGE_FALLBACK_LOGICOP,
	      (ctx->Color.ColorLogicOpEnabled && 
	       ctx->Color.LogicOp != GL_COPY));

   /* Can only do blend addition, not min, max, subtract, etc. */
   FALLBACK( ctx, SAVAGE_FALLBACK_BLEND_EQ,
	     modeRGB != GL_FUNC_ADD);
}


static void savageBlendFunc_s4(GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    Reg_DrawLocalCtrl DrawLocalCtrl;

    /* set up draw control register (including blending, alpha
     * test, dithering, and shading model)
     */

    /*
     * And mask removes flushPdDestWrites
     */

    DrawLocalCtrl.ui = imesa->Registers.DrawLocalCtrl.ui & ~0x40000000;

    /*
     * blend modes
     */
    if(ctx->Color.BlendEnabled){
        switch (ctx->Color.BlendDstRGB)
        {
            case GL_ZERO:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
                break;

            case GL_ONE:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_One;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_COLOR:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_SrcClr;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_COLOR:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_1SrcClr;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_SrcAlpha;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                DrawLocalCtrl.ni.dstAlphaMode = DAM_1SrcAlpha;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawLocalCtrl.ni.dstAlphaMode = DAM_One;
                }
                else
                {
                    DrawLocalCtrl.ni.dstAlphaMode = DAM_DstAlpha;
                }
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
                }
                else
                {
                    DrawLocalCtrl.ni.dstAlphaMode = DAM_1DstAlpha;
                    DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }

        switch (ctx->Color.BlendSrcRGB)
        {
            case GL_ZERO:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_Zero;
                break;

            case GL_ONE:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_One;
                break;

            case GL_DST_COLOR:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_DstClr;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_COLOR:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_1DstClr;
                DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_SrcAlpha;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                DrawLocalCtrl.ni.srcAlphaMode = SAM_1SrcAlpha;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawLocalCtrl.ni.srcAlphaMode = SAM_One;
                }
                else
                {
                    DrawLocalCtrl.ni.srcAlphaMode = SAM_DstAlpha;
                    DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)          
                {
                    DrawLocalCtrl.ni.srcAlphaMode = SAM_Zero;
                }
                else
                {
                    DrawLocalCtrl.ni.srcAlphaMode = SAM_1DstAlpha;
                    DrawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }
    }
    else
    {
        DrawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
        DrawLocalCtrl.ni.srcAlphaMode = SAM_One;
    }

    /* alpha test*/

    if(ctx->Color.AlphaEnabled) 
    {
        int a;
	GLubyte alphaRef;

	CLAMPED_FLOAT_TO_UBYTE(alphaRef,ctx->Color.AlphaRef);
         
        switch(ctx->Color.AlphaFunc)  { 
            case GL_NEVER: a = LCS_A_NEVER; break;
            case GL_ALWAYS: a = LCS_A_ALWAYS; break;
            case GL_LESS: a = LCS_A_LESS; break; 
            case GL_LEQUAL: a = LCS_A_LEQUAL; break;
            case GL_EQUAL: a = LCS_A_EQUAL; break;
            case GL_GREATER: a = LCS_A_GREATER; break;
            case GL_GEQUAL: a = LCS_A_GEQUAL; break;
            case GL_NOTEQUAL: a = LCS_A_NOTEQUAL; break;
            default:return;
        }   
      
        if (imesa->Registers.DrawCtrl1.ni.alphaTestEn != GL_TRUE)
        {
            imesa->Registers.DrawCtrl1.ni.alphaTestEn = GL_TRUE;
            imesa->Registers.changed.ni.fDrawCtrl1Changed = GL_TRUE;
        }

        if (imesa->Registers.DrawCtrl1.ni.alphaTestCmpFunc !=
            (a & 0x0F))
        {
            imesa->Registers.DrawCtrl1.ni.alphaTestCmpFunc =
                a & 0x0F;
            imesa->Registers.changed.ni.fDrawCtrl1Changed = GL_TRUE;
        }

        /* looks like rounding control is different on katmai than p2*/

        if (imesa->Registers.DrawCtrl0.ni.alphaRefVal != alphaRef)
        {
            imesa->Registers.DrawCtrl0.ni.alphaRefVal = alphaRef;
            imesa->Registers.changed.ni.fDrawCtrl0Changed = GL_TRUE;
        }
    }
    else
    {
        if (imesa->Registers.DrawCtrl1.ni.alphaTestEn != GL_FALSE)
        {
            imesa->Registers.DrawCtrl1.ni.alphaTestEn      = GL_FALSE;
            imesa->Registers.changed.ni.fDrawCtrl1Changed   = GL_TRUE;
        }
    }

    /* Set/Reset Z-after-alpha*/

    DrawLocalCtrl.ni.wrZafterAlphaTst = imesa->Registers.DrawCtrl1.ni.alphaTestEn;
    /*DrawLocalCtrl.ni.zUpdateEn = ~DrawLocalCtrl.ni.wrZafterAlphaTst;*/

    if (imesa->Registers.DrawLocalCtrl.ui != DrawLocalCtrl.ui)
    {
        imesa->Registers.DrawLocalCtrl.ui = DrawLocalCtrl.ui;
        imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
    }

    /* dithering*/

    if ( ctx->Color.DitherFlag )
    {
        if (imesa->Registers.DrawCtrl1.ni.ditherEn != GL_TRUE)
        {
            imesa->Registers.DrawCtrl1.ni.ditherEn = GL_TRUE;
            imesa->Registers.changed.ni.fDrawCtrl1Changed = GL_TRUE;
        }
    }
    else
    {
        if (imesa->Registers.DrawCtrl1.ni.ditherEn != GL_FALSE)
        {
            imesa->Registers.DrawCtrl1.ni.ditherEn = GL_FALSE;
            imesa->Registers.changed.ni.fDrawCtrl1Changed = GL_TRUE;
        }
    }
}
static void savageBlendFunc_s3d(GLcontext *ctx)
{
  
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    Reg_DrawCtrl DrawCtrl;

    /* set up draw control register (including blending, alpha
     * test, dithering, and shading model)
     */

    /*
     * And mask removes flushPdDestWrites
     */

    DrawCtrl.ui = imesa->Registers.DrawCtrl.ui & ~0x20000000;

    /*
     * blend modes
     */
    if(ctx->Color.BlendEnabled){
        switch (ctx->Color.BlendDstRGB)
        {
            case GL_ZERO:
                DrawCtrl.ni.dstAlphaMode = DAM_Zero;
                break;

            case GL_ONE:
                DrawCtrl.ni.dstAlphaMode = DAM_One;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_COLOR:
                DrawCtrl.ni.dstAlphaMode = DAM_SrcClr;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_COLOR:
                DrawCtrl.ni.dstAlphaMode = DAM_1SrcClr;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                DrawCtrl.ni.dstAlphaMode = DAM_SrcAlpha;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                DrawCtrl.ni.dstAlphaMode = DAM_1SrcAlpha;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawCtrl.ni.dstAlphaMode = DAM_One;
                }
                else
                {
                    DrawCtrl.ni.dstAlphaMode = DAM_DstAlpha;
                }
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawCtrl.ni.dstAlphaMode = DAM_Zero;
                }
                else
                {
                    DrawCtrl.ni.dstAlphaMode = DAM_1DstAlpha;
                    DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }

        switch (ctx->Color.BlendSrcRGB)
        {
            case GL_ZERO:
                DrawCtrl.ni.srcAlphaMode = SAM_Zero;
                break;

            case GL_ONE:
                DrawCtrl.ni.srcAlphaMode = SAM_One;
                break;

            case GL_DST_COLOR:
                DrawCtrl.ni.srcAlphaMode = SAM_DstClr;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_COLOR:
                DrawCtrl.ni.srcAlphaMode = SAM_1DstClr;
                DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                DrawCtrl.ni.srcAlphaMode = SAM_SrcAlpha;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                DrawCtrl.ni.srcAlphaMode = SAM_1SrcAlpha;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    DrawCtrl.ni.srcAlphaMode = SAM_One;
                }
                else
                {
                    DrawCtrl.ni.srcAlphaMode = SAM_DstAlpha;
                    DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)          
                {
                    DrawCtrl.ni.srcAlphaMode = SAM_Zero;
                }
                else
                {
                    DrawCtrl.ni.srcAlphaMode = SAM_1DstAlpha;
                    DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }
    }
    else
    {
        DrawCtrl.ni.dstAlphaMode = DAM_Zero;
        DrawCtrl.ni.srcAlphaMode = SAM_One;
    }

    /* alpha test*/

    if(ctx->Color.AlphaEnabled) 
    {
        GLint a;
	GLubyte alphaRef;

	CLAMPED_FLOAT_TO_UBYTE(alphaRef,ctx->Color.AlphaRef);
         
        switch(ctx->Color.AlphaFunc)  { 
            case GL_NEVER: a = LCS_A_NEVER; break;
            case GL_ALWAYS: a = LCS_A_ALWAYS; break;
            case GL_LESS: a = LCS_A_LESS; break; 
            case GL_LEQUAL: a = LCS_A_LEQUAL; break;
            case GL_EQUAL: a = LCS_A_EQUAL; break;
            case GL_GREATER: a = LCS_A_GREATER; break;
            case GL_GEQUAL: a = LCS_A_GEQUAL; break;
            case GL_NOTEQUAL: a = LCS_A_NOTEQUAL; break;
            default:return;
        }   

	DrawCtrl.ni.alphaTestEn = GL_TRUE;
	DrawCtrl.ni.alphaTestCmpFunc = a & 0x07;
	DrawCtrl.ni.alphaRefVal = alphaRef;
    }
    else
    {
	DrawCtrl.ni.alphaTestEn = GL_FALSE;
    }

    /* Set/Reset Z-after-alpha*/

    if (imesa->Registers.ZBufCtrl.s3d.wrZafterAlphaTst !=
	DrawCtrl.ni.alphaTestEn)
    {
	imesa->Registers.ZBufCtrl.s3d.wrZafterAlphaTst =
	    DrawCtrl.ni.alphaTestEn;
	imesa->Registers.changed.ni.fZBufCtrlChanged = GL_TRUE;
    }
    /*DrawLocalCtrl.ni.zUpdateEn = ~DrawLocalCtrl.ni.wrZafterAlphaTst;*/

    /* dithering*/

    if ( ctx->Color.DitherFlag )
    {
	DrawCtrl.ni.ditherEn = GL_TRUE;
    }
    else
    {
	DrawCtrl.ni.ditherEn = GL_FALSE;
    }

    if (imesa->Registers.DrawCtrl.ui != DrawCtrl.ui)
    {
        imesa->Registers.DrawCtrl.ui = DrawCtrl.ui;
        imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
    }
}

static void savageDDBlendFuncSeparate_s4( GLcontext *ctx, GLenum sfactorRGB, 
					  GLenum dfactorRGB, GLenum sfactorA,
					  GLenum dfactorA )
{
    assert (dfactorRGB == dfactorA && sfactorRGB == sfactorA);
    savageBlendFunc_s4( ctx );
}
static void savageDDBlendFuncSeparate_s3d( GLcontext *ctx, GLenum sfactorRGB, 
					   GLenum dfactorRGB, GLenum sfactorA,
					   GLenum dfactorA )
{
    assert (dfactorRGB == dfactorA && sfactorRGB == sfactorA);
    savageBlendFunc_s3d( ctx );
}



static void savageDDDepthFunc_s4(GLcontext *ctx, GLenum func)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    int zmode;
#define depthIndex 0

    /* set up z-buffer control register (global)
     * set up z-buffer offset register (global)
     * set up z read/write watermarks register (global)
     */

    switch(func)  { 
        case GL_NEVER: zmode = LCS_Z_NEVER; break;
        case GL_ALWAYS: zmode = LCS_Z_ALWAYS; break;
        case GL_LESS: zmode = LCS_Z_LESS; break; 
        case GL_LEQUAL: zmode = LCS_Z_LEQUAL; break;
        case GL_EQUAL: zmode = LCS_Z_EQUAL; break;
        case GL_GREATER: zmode = LCS_Z_GREATER; break;
        case GL_GEQUAL: zmode = LCS_Z_GEQUAL; break;
        case GL_NOTEQUAL: zmode = LCS_Z_NOTEQUAL; break;
        default:return;
    } 
    if (ctx->Depth.Test)
    {

        if (imesa->Registers.ZBufCtrl.s4.zCmpFunc != (zmode & 0x0F))
        {
            imesa->Registers.ZBufCtrl.s4.zCmpFunc = zmode & 0x0F;
            imesa->Registers.changed.ni.fZBufCtrlChanged  = GL_TRUE;
        }

        if (imesa->Registers.DrawLocalCtrl.ni.zUpdateEn != ctx->Depth.Mask)
        {
            imesa->Registers.DrawLocalCtrl.ni.zUpdateEn = ctx->Depth.Mask;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
        }

        if (imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites == GL_FALSE)
        {
            imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_TRUE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
#if 1
            imesa->Registers.ZWatermarks.ni.wLow = 0;
            imesa->Registers.changed.ni.fZWatermarksChanged = GL_TRUE;
#endif
        }

        if (imesa->Registers.ZBufCtrl.s4.zBufEn != GL_TRUE)
        {
            imesa->Registers.ZBufCtrl.s4.zBufEn = GL_TRUE;
            imesa->Registers.changed.ni.fZBufCtrlChanged = GL_TRUE;
        }
    }
    else if (imesa->glCtx->Stencil.Enabled &&
             !imesa->glCtx->DrawBuffer->UseSoftwareStencilBuffer)
    {
#define STENCIL (0x27)

        /* by Jiayo, tempory disable HW stencil in 24 bpp */
#if HW_STENCIL
        if(imesa->hw_stencil)
        {
            if ((imesa->Registers.ZBufCtrl.ui & STENCIL) != STENCIL)
            {
                imesa->Registers.ZBufCtrl.s4.zCmpFunc = GL_ALWAYS & 0x0F;
                imesa->Registers.ZBufCtrl.s4.zBufEn   = GL_TRUE;
                imesa->Registers.changed.ni.fZBufCtrlChanged  = GL_TRUE;
            }

            if (imesa->Registers.DrawLocalCtrl.ni.zUpdateEn != GL_FALSE)
            {
                imesa->Registers.DrawLocalCtrl.ni.zUpdateEn = GL_FALSE;
                imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;
            }

            if (imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites == GL_TRUE)
            {
                imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
                imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;

                imesa->Registers.ZWatermarks.ni.wLow        = 8;
                imesa->Registers.changed.ni.fZWatermarksChanged     = GL_TRUE;
            }
        }
#endif /* end #if HW_STENCIL */
    }
    else
    {

        if (imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn == GL_FALSE)
        {
            imesa->Registers.ZBufCtrl.s4.zCmpFunc = LCS_Z_ALWAYS & 0x0F;
            imesa->Registers.ZBufCtrl.s4.zBufEn   = GL_TRUE;
            imesa->Registers.changed.ni.fZBufCtrlChanged  = GL_TRUE;

            if (imesa->Registers.DrawLocalCtrl.ni.zUpdateEn != GL_FALSE)
            {
                imesa->Registers.DrawLocalCtrl.ni.zUpdateEn = GL_FALSE;
                imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;
            }

            if (imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites == GL_TRUE)
            {
                imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
                imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;

                imesa->Registers.ZWatermarks.ni.wLow        = 8;
                imesa->Registers.changed.ni.fZWatermarksChanged     = GL_TRUE;

            }
        }
        else

            /* DRAWUPDATE_REQUIRES_Z_ENABLED*/
        {
            if (imesa->Registers.ZBufCtrl.s4.zBufEn != GL_FALSE)
            {
                imesa->Registers.ZBufCtrl.s4.zBufEn         = GL_FALSE;
                imesa->Registers.DrawLocalCtrl.ni.zUpdateEn = GL_FALSE;
                imesa->Registers.changed.ni.fZBufCtrlChanged        = GL_TRUE;
                imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;
                imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
                imesa->Registers.ZWatermarks.ni.wLow        = 8;
                imesa->Registers.changed.ni.fZWatermarksChanged     = GL_TRUE;
            }
        }
    }
  
    imesa->dirty |= SAVAGE_UPLOAD_CTX;
}
static void savageDDDepthFunc_s3d(GLcontext *ctx, GLenum func)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    Reg_ZBufCtrl ZBufCtrl;
    int zmode;
#define depthIndex 0

    /* set up z-buffer control register (global)
     * set up z-buffer offset register (global)
     * set up z read/write watermarks register (global)
     */
    ZBufCtrl.ui = imesa->Registers.ZBufCtrl.ui;

    switch(func)  { 
        case GL_NEVER: zmode = LCS_Z_NEVER; break;
        case GL_ALWAYS: zmode = LCS_Z_ALWAYS; break;
        case GL_LESS: zmode = LCS_Z_LESS; break; 
        case GL_LEQUAL: zmode = LCS_Z_LEQUAL; break;
        case GL_EQUAL: zmode = LCS_Z_EQUAL; break;
        case GL_GREATER: zmode = LCS_Z_GREATER; break;
        case GL_GEQUAL: zmode = LCS_Z_GEQUAL; break;
        case GL_NOTEQUAL: zmode = LCS_Z_NOTEQUAL; break;
        default:return;
    } 
    if (ctx->Depth.Test)
    {
	ZBufCtrl.s3d.zBufEn = GL_TRUE;
	ZBufCtrl.s3d.zCmpFunc = zmode & 0x0F;
	ZBufCtrl.s3d.zUpdateEn = ctx->Depth.Mask;
	
        if (imesa->Registers.DrawCtrl.ni.flushPdZbufWrites == GL_FALSE)
        {
            imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_TRUE;
            imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
#if 1
            imesa->Registers.ZWatermarks.ni.wLow = 0;
            imesa->Registers.changed.ni.fZWatermarksChanged = GL_TRUE;
#endif
        }
    }
    else
    {
	if (ZBufCtrl.s3d.drawUpdateEn == GL_FALSE) {
	    ZBufCtrl.s3d.zCmpFunc = LCS_Z_ALWAYS & 0x0F;
            ZBufCtrl.s3d.zBufEn = GL_TRUE;
	    ZBufCtrl.s3d.zUpdateEn = GL_FALSE;
	}
        else

            /* DRAWUPDATE_REQUIRES_Z_ENABLED*/
        {
	    ZBufCtrl.s3d.zBufEn = GL_FALSE;
	    ZBufCtrl.s3d.zUpdateEn = GL_FALSE;
        }

	if (imesa->Registers.DrawCtrl.ni.flushPdZbufWrites == GL_TRUE)
        {
	    imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_FALSE;
	    imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;

	    imesa->Registers.ZWatermarks.ni.wLow = 8;
	    imesa->Registers.changed.ni.fZWatermarksChanged = GL_TRUE;
	}
    }
  
    if (imesa->Registers.ZBufCtrl.ui != ZBufCtrl.ui)
    {
        imesa->Registers.ZBufCtrl.ui = ZBufCtrl.ui;
        imesa->Registers.changed.ni.fZBufCtrlChanged = GL_TRUE;
    }

    imesa->dirty |= SAVAGE_UPLOAD_CTX;
}

static void savageDDDepthMask_s4(GLcontext *ctx, GLboolean flag)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

    imesa->dirty |= SAVAGE_UPLOAD_CTX;
    if (flag)
    {
        if (imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites == GL_FALSE)
        {
            imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_TRUE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
        }
    }
    else
    {
        if (imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites == GL_TRUE)
	{
            imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;
        }
    }
    savageDDDepthFunc_s4(ctx,ctx->Depth.Func);
}
static void savageDDDepthMask_s3d(GLcontext *ctx, GLboolean flag)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

    imesa->dirty |= SAVAGE_UPLOAD_CTX;
    if (flag)
    {
        if (imesa->Registers.DrawCtrl.ni.flushPdZbufWrites == GL_FALSE)
        {
            imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_TRUE;
            imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
        }
    }
    else
    {
        if (imesa->Registers.DrawCtrl.ni.flushPdZbufWrites == GL_TRUE)
        {
            imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_FALSE;
            imesa->Registers.changed.ni.fDrawCtrlChanged   = GL_TRUE;
        }
    }
    savageDDDepthFunc_s3d(ctx,ctx->Depth.Func);
}




/* =============================================================
 * Hardware clipping
 */


 void savageDDScissor( GLcontext *ctx, GLint x, GLint y, 
                             GLsizei w, GLsizei h )
{ 
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    imesa->scissor_rect.x1 = MAX2(imesa->drawX+x,imesa->draw_rect.x1);
    imesa->scissor_rect.y1 = MAX2(imesa->drawY+imesa->driDrawable->h -(y+h),
                                  imesa->draw_rect.y1);
    imesa->scissor_rect.x2 = MIN2(imesa->drawX+x+w,imesa->draw_rect.x2);
    imesa->scissor_rect.y2 = MIN2(imesa->drawY+imesa->driDrawable->h - y,
                                  imesa->draw_rect.y2);
    

    imesa->Registers.changed.ni.fScissorsChanged=GL_TRUE;

    imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
}



static void savageDDDrawBuffer(GLcontext *ctx, GLenum mode )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

    /*
     * _DrawDestMask is easier to cope with than <mode>.
     */
    switch ( ctx->Color._DrawDestMask ) {
    case FRONT_LEFT_BIT:
        imesa->IsDouble = GL_FALSE;
      
        if(imesa->IsFullScreen)
        {
            imesa->drawMap = (char *)imesa->apertureBase[TARGET_FRONT];
            imesa->readMap = (char *)imesa->apertureBase[TARGET_FRONT];
        }
        else
        {
            imesa->drawMap = (char *)imesa->apertureBase[TARGET_BACK];
            imesa->readMap = (char *)imesa->apertureBase[TARGET_BACK];
        }
        imesa->NotFirstFrame = GL_FALSE;
        imesa->dirty |= SAVAGE_UPLOAD_BUFFERS;
        savageXMesaSetFrontClipRects( imesa );
	FALLBACK( ctx, SAVAGE_FALLBACK_DRAW_BUFFER, GL_FALSE );
	break;
    case BACK_LEFT_BIT:
        imesa->IsDouble = GL_TRUE;
        imesa->drawMap = (char *)imesa->apertureBase[TARGET_BACK];
        imesa->readMap = (char *)imesa->apertureBase[TARGET_BACK];
        imesa->NotFirstFrame = GL_FALSE;
        imesa->dirty |= SAVAGE_UPLOAD_BUFFERS;
        savageXMesaSetBackClipRects( imesa );
	FALLBACK( ctx, SAVAGE_FALLBACK_DRAW_BUFFER, GL_FALSE );
	break;
    default:
	FALLBACK( ctx, SAVAGE_FALLBACK_DRAW_BUFFER, GL_TRUE );
	return;
    }
    
    /* We want to update the s/w rast state too so that r200SetBuffer() (?)
     * gets called.
     */
    _swrast_DrawBuffer(ctx, mode);
}

static void savageDDReadBuffer(GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}

#if 0
static void savageDDSetColor(GLcontext *ctx, 
                             GLubyte r, GLubyte g,
                             GLubyte b, GLubyte a )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    imesa->MonoColor = savagePackColor( imesa->savageScreen->frontFormat, r, g, b, a );
}
#endif

/* =============================================================
 * Window position and viewport transformation
 */

static void savageCalcViewport( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = imesa->hw_viewport;

   /* See also mga_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + imesa->drawX + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + imesa->driDrawable->h + imesa->drawY + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * imesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * imesa->depth_scale;

   imesa->SetupNewInputs = ~0;
}

static void savageViewport( GLcontext *ctx, 
			    GLint x, GLint y, 
			    GLsizei width, GLsizei height )
{
   savageCalcViewport( ctx );
}

static void savageDepthRange( GLcontext *ctx, 
			      GLclampd nearval, GLclampd farval )
{
   savageCalcViewport( ctx );
}


/* =============================================================
 * Miscellaneous
 */

static void savageDDClearColor(GLcontext *ctx, 
			       const GLfloat color[4] )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLubyte c[4];
    CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
    CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
    CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
    CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

    imesa->ClearColor = savagePackColor( imesa->savageScreen->frontFormat,
					 c[0], c[1], c[2], c[3] );
}

/* Fallback to swrast for select and feedback.
 */
static void savageRenderMode( GLcontext *ctx, GLenum mode )
{
   FALLBACK( ctx, SAVAGE_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}


#if HW_CULL

/* =============================================================
 * Culling - the savage isn't quite as clean here as the rest of
 *           its interfaces, but it's not bad.
 */
static void savageDDCullFaceFrontFace(GLcontext *ctx, GLenum unused)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLuint cullMode=imesa->LcsCullMode;        
    switch (ctx->Polygon.CullFaceMode)
    {
        case GL_FRONT:
            switch (ctx->Polygon.FrontFace)
            {
                case GL_CW:
                    cullMode = BCM_CW;
                    break;
                case GL_CCW:
                    cullMode = BCM_CCW;
                    break;
            }
            break;

        case GL_BACK:
            switch (ctx->Polygon.FrontFace)
            {
                case GL_CW:
                    cullMode = BCM_CCW;
                    break;
                case GL_CCW:
                    cullMode = BCM_CW;
                    break;
            }
            break;
    }
    imesa->LcsCullMode = cullMode;    
}
#endif /* end #if HW_CULL */

static void savageUpdateCull( GLcontext *ctx )
{
#if HW_CULL
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLuint cullMode;
    if (ctx->Polygon.CullFlag &&
	imesa->raster_primitive == GL_TRIANGLES &&
	ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK)
	cullMode = imesa->LcsCullMode;
    else
	cullMode = BCM_None;
    if (imesa->savageScreen->chipset >= S3_SAVAGE4) {
	if (imesa->Registers.DrawCtrl1.ni.cullMode != cullMode) {
	    imesa->Registers.DrawCtrl1.ni.cullMode = cullMode;
	    imesa->Registers.changed.ni.fDrawCtrl1Changed = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
	}
    } else {
	if (imesa->Registers.DrawCtrl.ni.cullMode != cullMode) {
	    imesa->Registers.DrawCtrl.ni.cullMode = cullMode;
	    imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
	}
    }
#endif /* end  #if HW_CULL */
}



/* =============================================================
 * Color masks
 */

/* Mesa calls this from the wrong place - it is called a very large
 * number of redundant times.
 *
 * Colormask can be simulated by multipass or multitexture techniques.
 */
static void savageDDColorMask_s4(GLcontext *ctx, 
				 GLboolean r, GLboolean g, 
				 GLboolean b, GLboolean a )
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    GLuint enable;

    if (ctx->Visual.alphaBits)
    {
        enable = b | g | r | a;
    }
    else
    {
        enable = b | g | r;
    }

    if (enable)
    {
        if (imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn == GL_FALSE)
        {
            imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn = GL_TRUE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    else
    {
        if (imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn)
        {
            imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn = GL_FALSE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    /* TODO: need a software fallback */
}
static void savageDDColorMask_s3d(GLcontext *ctx, 
				  GLboolean r, GLboolean g, 
				  GLboolean b, GLboolean a )
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    GLuint enable;

    if (ctx->Visual.alphaBits)
    {
        enable = b | g | r | a;
    }
    else
    {
        enable = b | g | r;
    }

    if (enable)
    {
        if (imesa->Registers.ZBufCtrl.s3d.drawUpdateEn == GL_FALSE)
        {
            imesa->Registers.ZBufCtrl.s3d.drawUpdateEn = GL_TRUE;
            imesa->Registers.changed.ni.fZBufCtrlChanged = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    else
    {
        if (imesa->Registers.ZBufCtrl.s3d.drawUpdateEn)
        {
            imesa->Registers.ZBufCtrl.s3d.drawUpdateEn = GL_FALSE;
            imesa->Registers.changed.ni.fZBufCtrlChanged = GL_TRUE;
	    imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    /* TODO: need a software fallback */
}

/* Seperate specular not fully implemented in hardware...  Needs
 * some interaction with material state?  Just punt to software
 * in all cases?
 * FK: Don't fall back for now. Let's see the failure cases and
 *     fix them the right way. I don't see how this could be a
 *     hardware limitation.
 */
static void savageUpdateSpecular_s4(GLcontext *ctx) {
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

    if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR &&
	ctx->Light.Enabled) {
	if (imesa->Registers.DrawLocalCtrl.ni.specShadeEn == GL_FALSE) {
	    imesa->Registers.DrawLocalCtrl.ni.specShadeEn = GL_TRUE;
	    imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
	}	 
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_TRUE);*/
    } else {
	if (imesa->Registers.DrawLocalCtrl.ni.specShadeEn == GL_TRUE) {
	    imesa->Registers.DrawLocalCtrl.ni.specShadeEn = GL_FALSE;
	    imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
	}
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_FALSE);*/
    }
}
static void savageUpdateSpecular_s3d(GLcontext *ctx) {
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

    if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR &&
	ctx->Light.Enabled) {
	if (imesa->Registers.DrawCtrl.ni.specShadeEn == GL_FALSE) {
	    imesa->Registers.DrawCtrl.ni.specShadeEn = GL_TRUE;
	    imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
	}	 
	FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_TRUE);
    } else {
	if (imesa->Registers.DrawCtrl.ni.specShadeEn == GL_TRUE) {
	    imesa->Registers.DrawCtrl.ni.specShadeEn = GL_FALSE;
	    imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
	}
	FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_FALSE);
    }
}

static void savageDDLightModelfv_s4(GLcontext *ctx, GLenum pname, 
				    const GLfloat *param)
{
    savageUpdateSpecular_s4 (ctx);
}
static void savageDDLightModelfv_s3d(GLcontext *ctx, GLenum pname, 
				     const GLfloat *param)
{
    savageUpdateSpecular_s3d (ctx);
}

static void savageDDShadeModel_s4(GLcontext *ctx, GLuint mod)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

    if (mod == GL_SMOOTH)  
    {    
        if(imesa->Registers.DrawLocalCtrl.ni.flatShadeEn == GL_TRUE)
        {
            imesa->Registers.DrawLocalCtrl.ni.flatShadeEn = GL_FALSE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    else
    {
        if(imesa->Registers.DrawLocalCtrl.ni.flatShadeEn == GL_FALSE)
        {
            imesa->Registers.DrawLocalCtrl.ni.flatShadeEn = GL_TRUE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged = GL_TRUE;
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
}
static void savageDDShadeModel_s3d(GLcontext *ctx, GLuint mod)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

    if (mod == GL_SMOOTH)  
    {    
        if(imesa->Registers.DrawCtrl.ni.flatShadeEn == GL_TRUE)
        {
            imesa->Registers.DrawCtrl.ni.flatShadeEn = GL_FALSE;
            imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
    else
    {
        if(imesa->Registers.DrawCtrl.ni.flatShadeEn == GL_FALSE)
        {
            imesa->Registers.DrawCtrl.ni.flatShadeEn = GL_TRUE;
            imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
        }
    }
}


/* =============================================================
 * Fog
 */

static void savageDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLuint  fogClr;

    /*if ((ctx->Fog.Enabled) &&(pname == GL_FOG_COLOR))*/
    if (ctx->Fog.Enabled)
    {
        fogClr = (((GLubyte)(ctx->Fog.Color[0]*255.0F) << 16) |
                  ((GLubyte)(ctx->Fog.Color[1]*255.0F) << 8) |
                  ((GLubyte)(ctx->Fog.Color[2]*255.0F) << 0));
        if (imesa->Registers.FogCtrl.ni.fogEn != GL_TRUE)
        {
            imesa->Registers.FogCtrl.ni.fogEn  = GL_TRUE;
            imesa->Registers.changed.ni.fFogCtrlChanged = GL_TRUE;
        }
        /*cheap fog*/
        if (imesa->Registers.FogCtrl.ni.fogMode != GL_TRUE)
        {
            imesa->Registers.FogCtrl.ni.fogMode  = GL_TRUE;
            imesa->Registers.changed.ni.fFogCtrlChanged = GL_TRUE;
        }
        if (imesa->Registers.FogCtrl.ni.fogClr != fogClr)
        {
            imesa->Registers.FogCtrl.ni.fogClr = fogClr;    
            imesa->Registers.changed.ni.fFogCtrlChanged = GL_TRUE;
        }
        imesa->dirty |= SAVAGE_UPLOAD_CTX;      
    }    
    else
    {
        /*No fog*/
        
        if (imesa->Registers.FogCtrl.ni.fogEn != 0)
        {
            imesa->Registers.FogCtrl.ni.fogEn     = 0;
            imesa->Registers.FogCtrl.ni.fogMode   = 0;
            imesa->Registers.changed.ni.fFogCtrlChanged   = GL_TRUE;
        }
        return;
    }
}


#if HW_STENCIL
static void savageStencilFunc(GLcontext *);

static void savageDDStencilFunc(GLcontext *ctx, GLenum func, GLint ref,
                                GLuint mask)
{
    savageStencilFunc(ctx);
}

static void savageDDStencilMask(GLcontext *ctx, GLuint mask)
{
    savageStencilFunc(ctx);
}

static void savageDDStencilOp(GLcontext *ctx, GLenum fail, GLenum zfail,
                              GLenum zpass)
{
    savageStencilFunc(ctx);  
}

static void savageStencilFunc(GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    Reg_StencilCtrl StencilCtrl;
    int a=0;
    
    StencilCtrl.ui = 0x0; 
    
    if (ctx->Stencil.Enabled)
    {
         
        switch (ctx->Stencil.Function[0])
        {
            case GL_NEVER: a = LCS_S_NEVER; break;
            case GL_ALWAYS: a = LCS_S_ALWAYS; break;
            case GL_LESS: a = LCS_S_LESS; break; 
            case GL_LEQUAL: a = LCS_S_LEQUAL; break;
            case GL_EQUAL: a = LCS_S_EQUAL; break;
            case GL_GREATER: a = LCS_S_GREATER; break;
            case GL_GEQUAL: a = LCS_S_GEQUAL; break;
            case GL_NOTEQUAL: a = LCS_S_NOTEQUAL; break;      
            default:
                break;
        }

        StencilCtrl.ni.cmpFunc     = (GLuint)a & 0x0F;
        StencilCtrl.ni.stencilEn   = GL_TRUE;
        StencilCtrl.ni.readMask    = ctx->Stencil.ValueMask[0];
        StencilCtrl.ni.writeMask   = ctx->Stencil.WriteMask[0];

        switch (ctx->Stencil.FailFunc[0])
        {
            case GL_KEEP:
                StencilCtrl.ni.failOp = STC_FAIL_Keep;
                break;
            case GL_ZERO:
                StencilCtrl.ni.failOp = STC_FAIL_Zero;
                break;
            case GL_REPLACE:
                StencilCtrl.ni.failOp = STC_FAIL_Equal;
                break;
            case GL_INCR:
                StencilCtrl.ni.failOp = STC_FAIL_IncClamp;
                break;
            case GL_DECR:
                StencilCtrl.ni.failOp = STC_FAIL_DecClamp;
                break;
            case GL_INVERT:
                StencilCtrl.ni.failOp = STC_FAIL_Invert;
                break;
#if GL_EXT_stencil_wrap
            case GL_INCR_WRAP_EXT:
                StencilCtrl.ni.failOp = STC_FAIL_Inc;
                break;
            case GL_DECR_WRAP_EXT:
                StencilCtrl.ni.failOp = STC_FAIL_Dec;
                break;
#endif
        }

        switch (ctx->Stencil.ZFailFunc[0])
        {
            case GL_KEEP:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Keep;
                break;
            case GL_ZERO:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Zero;
                break;
            case GL_REPLACE:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Equal;
                break;
            case GL_INCR:
                StencilCtrl.ni.passZfailOp = STC_FAIL_IncClamp;
                break;
            case GL_DECR:
                StencilCtrl.ni.passZfailOp = STC_FAIL_DecClamp;
                break;
            case GL_INVERT:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Invert;
                break;
#if GL_EXT_stencil_wrap
            case GL_INCR_WRAP_EXT:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Inc;
                break;
            case GL_DECR_WRAP_EXT:
                StencilCtrl.ni.passZfailOp = STC_FAIL_Dec;
                break;
#endif
        }

        switch (ctx->Stencil.ZPassFunc[0])
        {
            case GL_KEEP:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Keep;
                break;
            case GL_ZERO:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Zero;
                break;
            case GL_REPLACE:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Equal;
                break;
            case GL_INCR:
                StencilCtrl.ni.passZpassOp = STC_FAIL_IncClamp;
                break;
            case GL_DECR:
                StencilCtrl.ni.passZpassOp = STC_FAIL_DecClamp;
                break;
            case GL_INVERT:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Invert;
                break;
#if GL_EXT_stencil_wrap
            case GL_INCR_WRAP_EXT:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Inc;
                break;
            case GL_DECR_WRAP_EXT:
                StencilCtrl.ni.passZpassOp = STC_FAIL_Dec;
                break;
#endif
        }


        if (imesa->Registers.StencilCtrl.ui != StencilCtrl.ui)
        {
            imesa->Registers.StencilCtrl.ui      = StencilCtrl.ui;
            imesa->Registers.changed.ni.fStencilCtrlChanged = GL_TRUE;
        }

        if (imesa->Registers.ZBufCtrl.s4.stencilRefVal != (GLuint) ctx->Stencil.Ref)    {
            imesa->Registers.ZBufCtrl.s4.stencilRefVal = ctx->Stencil.Ref[0];
            imesa->Registers.changed.ni.fZBufCtrlChanged       = GL_TRUE;
        }

        /*
         * force Z on, HW limitation
         */

        if (imesa->Registers.ZBufCtrl.s4.zBufEn != GL_TRUE)
        {
            imesa->Registers.ZBufCtrl.s4.zCmpFunc       = LCS_Z_ALWAYS & 0x0F;
            imesa->Registers.ZBufCtrl.s4.zBufEn         = GL_TRUE;
            imesa->Registers.changed.ni.fZBufCtrlChanged        = GL_TRUE;
            imesa->Registers.DrawLocalCtrl.ni.zUpdateEn = GL_FALSE;
            imesa->Registers.changed.ni.fDrawLocalCtrlChanged   = GL_TRUE;
        }
        imesa->dirty |= SAVAGE_UPLOAD_CTX;
    }
    else
    {
        if (imesa->Registers.StencilCtrl.ni.stencilEn != GL_FALSE)
        {
            imesa->Registers.StencilCtrl.ni.stencilEn = GL_FALSE;
            imesa->Registers.changed.ni.fStencilCtrlChanged   = GL_TRUE;
        }
    }
}
#endif /* end #if HW_STENCIL */
/* =============================================================
 */

static void savageDDEnable_s4(GLcontext *ctx, GLenum cap, GLboolean state)
{
   
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    unsigned int ui;
    switch(cap) {
        case GL_ALPHA_TEST:
            /* we should consider the disable case*/
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageBlendFunc_s4(ctx);
            break;
        case GL_BLEND:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            /*Can't find Enable bit in the 3D registers.*/ 
            /* For some reason enable(GL_BLEND) affects ColorLogicOpEnabled.
             */
	    FALLBACK (ctx, SAVAGE_FALLBACK_LOGICOP,
		      (ctx->Color.ColorLogicOpEnabled &&
		       ctx->Color.LogicOp != GL_COPY));
            /*add the savageBlendFunc 2001/11/25
             * if call no such function, then glDisable(GL_BLEND) will do noting,
             *our chip has no disable bit
             */ 
            savageBlendFunc_s4(ctx);
            break;
        case GL_DEPTH_TEST:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageDDDepthFunc_s4(ctx,ctx->Depth.Func);
            break;
        case GL_SCISSOR_TEST:
            imesa->scissor = state;
            imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
            break;
#if 0
        case GL_LINE_SMOOTH:
            if (ctx->PB->primitive == GL_LINE) {
                imesa->dirty |= SAVAGE_UPLOAD_CTX;
                if (state) {
                    ui=imesa->Registers.DrawLocalCtrl.ui;
                    imesa->Registers.DrawLocalCtrl.ni.flatShadeEn=GL_TRUE; 
                    if(imesa->Registers.DrawLocalCtrl.ui!=ui)
                        imesa->Registers.changed.ni.fDrawLocalCtrlChanged=GL_TRUE;
                }
            }
            break;
        case GL_POINT_SMOOTH:
            if (ctx->PB->primitive == GL_POINT) {
                imesa->dirty |= SAVAGE_UPLOAD_CTX;
                if (state) 
                {
                    ui=imesa->Registers.DrawLocalCtrl.ui;
                    imesa->Registers.DrawLocalCtrl.ni.flatShadeEn=GL_FALSE;
                    if(imesa->Registers.DrawLocalCtrl.ui!=ui)
                        imesa->Registers.changed.ni.fDrawLocalCtrlChanged=GL_TRUE;
                }
            }
            break;
        case GL_POLYGON_SMOOTH:
            if (ctx->PB->primitive == GL_POLYGON) {
                imesa->dirty |= SAVAGE_UPLOAD_CTX;
                if (state) {
                    ui=imesa->Registers.DrawLocalCtrl.ui;	  
                    imesa->Registers.DrawLocalCtrl.ni.flatShadeEn=GL_TRUE;
                    if(imesa->Registers.DrawLocalCtrl.ui!=ui)
                        imesa->Registers.changed.ni.fDrawLocalCtrlChanged=GL_TRUE;
                }  
            }
            break;
#endif
        case GL_STENCIL_TEST:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            if (state)
            { 
#if HW_STENCIL
                if(imesa->hw_stencil)
                {
                    ui=imesa->Registers.StencilCtrl.ui;
#endif /* end if HW_STENCIL */
                    if(!imesa->hw_stencil)
			FALLBACK (ctx, SAVAGE_FALLBACK_STENCIL, GL_TRUE);

#if HW_STENCIL
                    imesa->Registers.StencilCtrl.ni.stencilEn=GL_TRUE;
                    if(imesa->Registers.StencilCtrl.ui!=ui)
                        imesa->Registers.changed.ni.fStencilCtrlChanged=GL_TRUE;
                }
#endif /* end if HW_STENCIL */	
            }
	    
            else
            {
#if HW_STENCIL
                if(imesa->hw_stencil)
                {
                    if(imesa->Registers.StencilCtrl.ni.stencilEn == GL_TRUE)
                    {
                        imesa->Registers.StencilCtrl.ni.stencilEn=GL_FALSE;
                        imesa->Registers.changed.ni.fStencilCtrlChanged=GL_TRUE;
                    }
                }
		FALLBACK (ctx, SAVAGE_FALLBACK_STENCIL, GL_FALSE);
#endif      
            }
            break;
        case GL_FOG:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageDDFogfv(ctx,0,0);	
            break;
        case GL_CULL_FACE:
#if HW_CULL
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            ui=imesa->Registers.DrawCtrl1.ui;
            if (state)
            {
                savageDDCullFaceFrontFace(ctx,0);
            }
            else
            {
                imesa->Registers.DrawCtrl1.ni.cullMode=BCM_None;
            }
            if(imesa->Registers.DrawCtrl1.ui!=ui)
                imesa->Registers.changed.ni.fDrawCtrl1Changed=GL_TRUE;
#endif
            break;
        case GL_DITHER:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            if (state)
            {
                if ( ctx->Color.DitherFlag )
                {
                    ui=imesa->Registers.DrawCtrl1.ui;
                    imesa->Registers.DrawCtrl1.ni.ditherEn=GL_TRUE;
                    if(imesa->Registers.DrawCtrl1.ui!=ui)
                        imesa->Registers.changed.ni.fDrawCtrl1Changed=GL_TRUE;
                }
            }   
            if (!ctx->Color.DitherFlag )
            {
                ui=imesa->Registers.DrawCtrl1.ui;
                imesa->Registers.DrawCtrl1.ni.ditherEn=GL_FALSE;
                if(imesa->Registers.DrawCtrl1.ui!=ui)
                    imesa->Registers.changed.ni.fDrawCtrl1Changed=GL_TRUE;
            }
            break;
 
        case GL_LIGHTING:
	    savageUpdateSpecular_s4 (ctx);
            break;
        case GL_TEXTURE_1D:      
        case GL_TEXTURE_3D:      
            imesa->new_state |= SAVAGE_NEW_TEXTURE;
            break;
        case GL_TEXTURE_2D:      
            imesa->new_state |= SAVAGE_NEW_TEXTURE;
            break;
        default:
            ; 
    }    
}
static void savageDDEnable_s3d(GLcontext *ctx, GLenum cap, GLboolean state)
{
   
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    unsigned int ui;
    switch(cap) {
        case GL_ALPHA_TEST:
            /* we should consider the disable case*/
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageBlendFunc_s3d(ctx);
            break;
        case GL_BLEND:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            /*Can't find Enable bit in the 3D registers.*/ 
            /* For some reason enable(GL_BLEND) affects ColorLogicOpEnabled.
             */
	    FALLBACK (ctx, SAVAGE_FALLBACK_LOGICOP,
		      (ctx->Color.ColorLogicOpEnabled &&
		       ctx->Color.LogicOp != GL_COPY));
            /*add the savageBlendFunc 2001/11/25
             * if call no such function, then glDisable(GL_BLEND) will do noting,
             *our chip has no disable bit
             */ 
            savageBlendFunc_s3d(ctx);
            break;
        case GL_DEPTH_TEST:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageDDDepthFunc_s3d(ctx,ctx->Depth.Func);
            break;
        case GL_SCISSOR_TEST:
            imesa->scissor = state;
            imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
            break;
        case GL_FOG:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            savageDDFogfv(ctx,0,0);	
            break;
        case GL_CULL_FACE:
#if HW_CULL
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            ui=imesa->Registers.DrawCtrl.ui;
            if (state)
            {
                savageDDCullFaceFrontFace(ctx,0);
            }
            else
            {
                imesa->Registers.DrawCtrl.ni.cullMode=BCM_None;
            }
            if(imesa->Registers.DrawCtrl.ui!=ui)
                imesa->Registers.changed.ni.fDrawCtrlChanged=GL_TRUE;
#endif
            break;
        case GL_DITHER:
            imesa->dirty |= SAVAGE_UPLOAD_CTX;
            if (state)
            {
                if ( ctx->Color.DitherFlag )
                {
                    ui=imesa->Registers.DrawCtrl.ui;
                    imesa->Registers.DrawCtrl.ni.ditherEn=GL_TRUE;
                    if(imesa->Registers.DrawCtrl.ui!=ui)
                        imesa->Registers.changed.ni.fDrawCtrlChanged=GL_TRUE;
                }
            }
            if (!ctx->Color.DitherFlag )
            {
                ui=imesa->Registers.DrawCtrl.ui;
                imesa->Registers.DrawCtrl.ni.ditherEn=GL_FALSE;
                if(imesa->Registers.DrawCtrl.ui!=ui)
                    imesa->Registers.changed.ni.fDrawCtrlChanged=GL_TRUE;
            }
            break;
 
        case GL_LIGHTING:
	    savageUpdateSpecular_s3d (ctx);
            break;
        case GL_TEXTURE_1D:      
        case GL_TEXTURE_3D:      
            imesa->new_state |= SAVAGE_NEW_TEXTURE;
            break;
        case GL_TEXTURE_2D:      
            imesa->new_state |= SAVAGE_NEW_TEXTURE;
            break;
        default:
            ; 
    }    
}

void savageDDUpdateHwState( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   
    if(imesa->driDrawable)
    {
        LOCK_HARDWARE(imesa);
    }
    
    if (imesa->new_state & SAVAGE_NEW_TEXTURE) {
	savageUpdateTextureState( ctx );
    }
    if((imesa->dirty!=0)|| (imesa->new_state!=0))  
    {
        savageEmitHwStateLocked(imesa);
        imesa->new_state = 0;
    }
    if(imesa->driDrawable)
    {
        UNLOCK_HARDWARE(imesa);
    }
}


void savageEmitDrawingRectangle( savageContextPtr imesa )
{
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;
    savageScreenPrivate *savageScreen = imesa->savageScreen;
    XF86DRIClipRectPtr pbox;
    int nbox;
   

    int x0 = imesa->drawX;
    int y0 = imesa->drawY;
    int x1 = x0 + dPriv->w;
    int y1 = y0 + dPriv->h;

    pbox = dPriv->pClipRects;  
    nbox = dPriv->numClipRects;
       

   
    /* Coordinate origin of the window - may be offscreen.
     */
    /* imesa->BufferSetup[SAVAGE_DESTREG_DR4] = ((y0<<16) | 
       (((unsigned)x0)&0xFFFF));*/
  
    /* Clip to screen.
     */
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > savageScreen->width) x1 = savageScreen->width;
    if (y1 > savageScreen->height) y1 = savageScreen->height;


    if(nbox ==  1)
    {
        imesa->draw_rect.x1 = MAX2(x0,pbox->x1);
        imesa->draw_rect.y1 = MAX2(y0,pbox->y1);
        imesa->draw_rect.x2 = MIN2(x1,pbox->x2);
        imesa->draw_rect.y2 = MIN2(y1,pbox->y2);
    }
    else
    {
        imesa->draw_rect.x1 = x0;
        imesa->draw_rect.y1 = y0;
        imesa->draw_rect.x2 = x1;
        imesa->draw_rect.y2 = y1;
    }
   
    imesa->Registers.changed.ni.fScissorsChanged=GL_TRUE;

    /*   imesa->Registers.changed.ni.fDrawCtrl0Changed=GL_TRUE;
         imesa->Registers.changed.ni.fDrawCtrl1Changed=GL_TRUE;*/


    imesa->dirty |= SAVAGE_UPLOAD_BUFFERS;
}


static void savageDDPrintDirty( const char *msg, GLuint state )
{
    fprintf(stderr, "%s (0x%x): %s%s%s%s%s\n",	   
            msg,
            (unsigned int) state,
            (state & SAVAGE_UPLOAD_TEX0IMAGE)  ? "upload-tex0, " : "",
            (state & SAVAGE_UPLOAD_TEX1IMAGE)  ? "upload-tex1, " : "",
            (state & SAVAGE_UPLOAD_CTX)        ? "upload-ctx, " : "",
            (state & SAVAGE_UPLOAD_BUFFERS)    ? "upload-bufs, " : "",
            (state & SAVAGE_UPLOAD_CLIPRECTS)  ? "upload-cliprects, " : ""
            );
}



static void savageUpdateRegister_s4(savageContextPtr imesa)
{
    GLuint *pBCIBase;
    pBCIBase = savageDMAAlloc (imesa, 100);
    /*
     *make sure there is enough room for everything
     */
    /*savageKickDMA(imesa);*/
#define PARAMT 1
#if defined(PARAMT) && PARAMT
#define GLOBAL_REG SAVAGE_GLOBAL_CHANGED
#else
#define GLOBAL_REG (SAVAGE_GLOBAL_CHANGED | SAVAGE_TEXTURE_CHANGED)
#endif
    if (imesa->Registers.changed.uiRegistersChanged & GLOBAL_REG)
    {
        WRITE_CMD(pBCIBase,WAIT_3D_IDLE,GLuint);
    }

    if (imesa->Registers.changed.ni.fTexPalAddrChanged)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXPALADDR_S4, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexPalAddr.ui,GLuint);
    }

    if (imesa->Registers.changed.uiRegistersChanged & 0xFC)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXCTRL0_S4, 6),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexCtrl[0].ui,GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexCtrl[1].ui,GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexAddr[0].ui,GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexAddr[1].ui,GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexBlendCtrl[0].ui,GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexBlendCtrl[1].ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fTexDescrChanged)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXDESCR_S4, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexDescr.ui,GLuint);
        imesa->Registers.TexDescr.s4.newPal = GL_FALSE;
    }

    if (imesa->Registers.changed.ni.fFogCtrlChanged)
    {

        WRITE_CMD(pBCIBase,SET_REGISTER(SAVAGE_FOGCTRL_S4, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.FogCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fDestCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DESTCTRL_S4,1),GLuint);
	WRITE_CMD( pBCIBase ,imesa->Registers.DestCtrl.ui,GLuint);
    }
    
    if (imesa->Registers.changed.ni.fDrawLocalCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DRAWLOCALCTRL_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DrawLocalCtrl.ui,GLuint);
    }
    /*
     * Scissors updates drawctrl0 and drawctrl 1
     */

    if (imesa->Registers.changed.ni.fScissorsChanged)
    {
        if(imesa->scissor)
        {
            imesa->Registers.DrawCtrl0.ni.scissorXStart = imesa->scissor_rect.x1;
            imesa->Registers.DrawCtrl0.ni.scissorYStart = imesa->scissor_rect.y1;
            imesa->Registers.DrawCtrl1.ni.scissorXEnd   = imesa->scissor_rect.x2-1;
            imesa->Registers.DrawCtrl1.ni.scissorYEnd   = imesa->scissor_rect.y2-1;
        }
        else
        {
            imesa->Registers.DrawCtrl0.ni.scissorXStart = imesa->draw_rect.x1;
            imesa->Registers.DrawCtrl0.ni.scissorYStart = imesa->draw_rect.y1;
            imesa->Registers.DrawCtrl1.ni.scissorXEnd   = imesa->draw_rect.x2-1;
            imesa->Registers.DrawCtrl1.ni.scissorYEnd   = imesa->draw_rect.y2-1;
        }
        
        imesa->Registers.changed.ni.fDrawCtrl0Changed=GL_TRUE;
        imesa->Registers.changed.ni.fDrawCtrl1Changed=GL_TRUE;
    }
    if (imesa->Registers.changed.uiRegistersChanged )
    {
        
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DRAWCTRLGLOBAL0_S4,2),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DrawCtrl0.ui,GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DrawCtrl1.ui,GLuint);
        
    }

    if (imesa->Registers.changed.ni.fZBufCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_ZBUFCTRL_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ZBufCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fStencilCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_STENCILCTRL_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.StencilCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fTexBlendColorChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_TEXBLENDCOLOR_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.TexBlendColor.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fZWatermarksChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_ZWATERMARK_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ZWatermarks.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fDestTexWatermarksChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DESTTEXRWWATERMARK_S4,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DestTexWatermarks.ui,GLuint);
    }

    imesa->Registers.changed.uiRegistersChanged = 0;
    imesa->dirty=0;	
    savageDMACommit (imesa, pBCIBase);
}
static void savageUpdateRegister_s3d(savageContextPtr imesa)
{
    GLuint *pBCIBase;
    pBCIBase = savageDMAAlloc (imesa, 100);

    /* Always wait for idle for now.
     * FIXME: On the Savage3D individual fields in registers can be
     * local/global. */
    WRITE_CMD(pBCIBase,WAIT_3D_IDLE,GLuint);

    if (imesa->Registers.changed.ni.fZBufCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_ZBUFCTRL_S3D,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ZBufCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fDestCtrlChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DESTCTRL_S3D,1),GLuint);
	WRITE_CMD( pBCIBase ,imesa->Registers.DestCtrl.ui,GLuint);
    }
    /* Better leave these alone. They don't seem to be needed and I
     * don't know exactly what they ary good for. Changing them may
     * have been responsible for lockups with texturing. */
/*
    if (imesa->Registers.changed.ni.fZWatermarksChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_ZWATERMARK_S3D,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ZWatermarks.ui,GLuint);
    }
    if (imesa->Registers.changed.ni.fDestTexWatermarksChanged)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DESTTEXRWWATERMARK_S3D,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DestTexWatermarks.ui,GLuint);
    }
*/
    if (imesa->Registers.changed.ni.fDrawCtrlChanged)
    {
	/* Same as above. The utah-driver always sets these to true.
	 * Changing them definitely caused lockups with texturing. */
	imesa->Registers.DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
	imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_TRUE;
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_DRAWCTRL_S3D,1),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.DrawCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fScissorsChanged)
    {
        if(imesa->scissor)
        {
            imesa->Registers.ScissorsStart.ni.scissorXStart =
		imesa->scissor_rect.x1;
            imesa->Registers.ScissorsStart.ni.scissorYStart =
		imesa->scissor_rect.y1;
            imesa->Registers.ScissorsEnd.ni.scissorXEnd =
		imesa->scissor_rect.x2-1;
            imesa->Registers.ScissorsEnd.ni.scissorYEnd =
		imesa->scissor_rect.y2-1;
        }
        else
        {
            imesa->Registers.ScissorsStart.ni.scissorXStart =
		imesa->draw_rect.x1;
            imesa->Registers.ScissorsStart.ni.scissorYStart =
		imesa->draw_rect.y1;
            imesa->Registers.ScissorsEnd.ni.scissorXEnd =
		imesa->draw_rect.x2-1;
            imesa->Registers.ScissorsEnd.ni.scissorYEnd =
		imesa->draw_rect.y2-1;
        }
        
        imesa->Registers.changed.ni.fScissorsStartChanged=GL_TRUE;
        imesa->Registers.changed.ni.fScissorsEndChanged=GL_TRUE;
    }
    if (imesa->Registers.changed.uiRegistersChanged & 0x00C00000)
    {
        WRITE_CMD(pBCIBase , SET_REGISTER(SAVAGE_SCSTART_S3D,2),GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ScissorsStart.ui,GLuint);
        WRITE_CMD(pBCIBase , imesa->Registers.ScissorsEnd.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fTex0CtrlChanged)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXCTRL_S3D, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexCtrl[0].ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fFogCtrlChanged)
    {
        WRITE_CMD(pBCIBase,SET_REGISTER(SAVAGE_FOGCTRL_S3D, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.FogCtrl.ui,GLuint);
    }

    if (imesa->Registers.changed.ni.fTex0AddrChanged ||
	imesa->Registers.changed.ni.fTexDescrChanged)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXADDR_S3D, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexAddr[0].ui,GLuint);
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXDESCR_S3D, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexDescr.ui,GLuint);
        imesa->Registers.TexDescr.s3d.newPal = GL_FALSE;
    }

    if (imesa->Registers.changed.ni.fTexPalAddrChanged)
    {
        WRITE_CMD(pBCIBase, SET_REGISTER(SAVAGE_TEXPALADDR_S3D, 1),GLuint);
        WRITE_CMD(pBCIBase, imesa->Registers.TexPalAddr.ui,GLuint);
    }

    imesa->Registers.changed.uiRegistersChanged = 0;
    imesa->dirty=0;
    savageDMACommit (imesa, pBCIBase);
}



/* Push the state into the sarea and/or texture memory.
 */
void savageEmitHwStateLocked( savageContextPtr imesa )
{
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_API)
        savageDDPrintDirty( "\n\n\nsavageEmitHwStateLocked", imesa->dirty );

    if (imesa->dirty & ~SAVAGE_UPLOAD_CLIPRECTS)
    {
        if (imesa->dirty & (SAVAGE_UPLOAD_CTX | SAVAGE_UPLOAD_TEX0  | \
                            SAVAGE_UPLOAD_TEX1 | SAVAGE_UPLOAD_BUFFERS))
        {
   
            SAVAGE_STATE_COPY(imesa);
            /* update state to hw*/
            if (imesa->driDrawable &&imesa->driDrawable->numClipRects ==0 )
            {
                return ;
            }
	    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
		savageUpdateRegister_s4(imesa);
	    else
		savageUpdateRegister_s3d(imesa);
        }

        imesa->sarea->dirty |= (imesa->dirty & 
                                ~(SAVAGE_UPLOAD_TEX1|SAVAGE_UPLOAD_TEX0));
        imesa->dirty &= SAVAGE_UPLOAD_CLIPRECTS;
    }
}



static void savageDDInitState_s4( savageContextPtr imesa )
{
#if 1
    *(GLuint *)&imesa->Registers.DestCtrl          = 1<<7;
#else
    *(GLuint *)&imesa->Registers.DestCtrl          = 0;
#endif
    *(GLuint *)&imesa->Registers.ZBufCtrl          = 0;

    imesa->Registers.ZBufCtrl.s4.zCmpFunc = LCS_Z_LESS;
    imesa->Registers.ZBufCtrl.s4.wToZEn               = GL_TRUE;
    /*imesa->Registers.ZBufCtrl.ni.floatZEn          = GL_TRUE;*/
    *(GLuint *)&imesa->Registers.ZBufOffset        = 0;
    *(GLuint *)&imesa->Registers.FogCtrl           = 0;
    imesa->Registers.FogTable.ni.ulEntry[0]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[1]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[2]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[3]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[4]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[5]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[6]           = 0;
    imesa->Registers.FogTable.ni.ulEntry[7]           = 0;
    *(GLuint *)&imesa->Registers.TexDescr          = 0;
    imesa->Registers.TexAddr[0].ui                 = 0;
    imesa->Registers.TexAddr[1].ui                 = 0;
    imesa->Registers.TexPalAddr.ui                 = 0;
    *(GLuint *)&imesa->Registers.TexCtrl[0]        = 0;
    *(GLuint *)&imesa->Registers.TexCtrl[1]        = 0;
    imesa->Registers.TexBlendCtrl[0].ui            = TBC_NoTexMap;
    imesa->Registers.TexBlendCtrl[1].ui            = TBC_NoTexMap1;
    *(GLuint *)&imesa->Registers.DrawCtrl0         = 0;
#if 1/*def __GL_HALF_PIXEL_OFFSET*/
    *(GLuint *)&imesa->Registers.DrawCtrl1         = 0;
#else
    *(GLuint *)&imesa->Registers.DrawCtrl1         = 1<<11;
#endif
    *(GLuint *)&imesa->Registers.DrawLocalCtrl     = 0;
    *(GLuint *)&imesa->Registers.StencilCtrl       = 0;

    /* Set DestTexWatermarks_31,30 to 01 always.
     *Has no effect if dest. flush is disabled.
     */
#if 0
    *(GLuint *)&imesa->Registers.ZWatermarks       = 0x12000C04;
    *(GLuint *)&imesa->Registers.DestTexWatermarks = 0x40200400;
#else
    *(GLuint *)&imesa->Registers.ZWatermarks       = 0x16001808;
    *(GLuint *)&imesa->Registers.DestTexWatermarks = 0x4f000000;
#endif
    imesa->Registers.DrawCtrl0.ni.DPerfAccelEn = GL_TRUE;

    /* clrCmpAlphaBlendCtrl is needed to get alphatest and
     * alpha blending working properly
     */

    imesa->Registers.TexCtrl[0].s4.dBias                 = 0x08;
    imesa->Registers.TexCtrl[1].s4.dBias                 = 0x08;
    imesa->Registers.TexCtrl[0].s4.texXprEn              = GL_TRUE;
    imesa->Registers.TexCtrl[1].s4.texXprEn              = GL_TRUE;
    imesa->Registers.TexCtrl[0].s4.dMax                  = 0x0f;
    imesa->Registers.TexCtrl[1].s4.dMax                  = 0x0f;
    imesa->Registers.DrawLocalCtrl.ni.drawUpdateEn     = GL_TRUE;
    imesa->Registers.DrawLocalCtrl.ni.srcAlphaMode    = SAM_One;
    imesa->Registers.DrawLocalCtrl.ni.wrZafterAlphaTst = GL_FALSE;
    imesa->Registers.DrawLocalCtrl.ni.flushPdZbufWrites= GL_TRUE;
    imesa->Registers.DrawLocalCtrl.ni.flushPdDestWrites= GL_TRUE;

    imesa->Registers.DrawLocalCtrl.ni.zUpdateEn= GL_TRUE;
    imesa->Registers.DrawCtrl1.ni.ditherEn=GL_TRUE;
    imesa->Registers.DrawCtrl1.ni.cullMode             = BCM_None;

    imesa->LcsCullMode=BCM_None;
    imesa->Registers.TexDescr.s4.palSize               = TPS_256;
}
static void savageDDInitState_s3d( savageContextPtr imesa )
{
#if 1
    imesa->Registers.DestCtrl.ui           = 1<<7;
#else
    imesa->Registers.DestCtrl.ui           = 0;
#endif
    imesa->Registers.ZBufCtrl.ui           = 0;

    imesa->Registers.ZBufCtrl.s3d.zCmpFunc = LCS_Z_LESS & 0x07;
    imesa->Registers.ZBufOffset.ui         = 0;
    imesa->Registers.FogCtrl.ui            = 0;
    memset (imesa->Registers.FogTable.ni.ucEntry, 0, 64);
    imesa->Registers.TexDescr.ui           = 0;
    imesa->Registers.TexAddr[0].ui         = 0;
    imesa->Registers.TexPalAddr.ui         = 0;
    imesa->Registers.TexCtrl[0].ui         = 0;
#if 1/*def __GL_HALF_PIXEL_OFFSET*/
    imesa->Registers.DrawCtrl.ui           = 0;
#else
    imesa->Registers.DrawCtrl.ui           = 1<<1;
#endif
    imesa->Registers.ScissorsStart.ui      = 0;
    imesa->Registers.ScissorsEnd.ui        = 0;

    /* Set DestTexWatermarks_31,30 to 01 always.
     *Has no effect if dest. flush is disabled.
     */
#if 0
    imesa->Registers.ZWatermarks.ui       = 0x12000C04;
    imesa->Registers.DestTexWatermarks.ui = 0x40200400;
#else
    imesa->Registers.ZWatermarks.ui       = 0x16001808;
    imesa->Registers.DestTexWatermarks.ui = 0x4f000000;
#endif

    /* clrCmpAlphaBlendCtrl is needed to get alphatest and
     * alpha blending working properly
     */

    imesa->Registers.TexCtrl[0].s3d.dBias          = 0x08;
    imesa->Registers.TexCtrl[0].s3d.texXprEn       = GL_TRUE;

    imesa->Registers.ZBufCtrl.s3d.drawUpdateEn     = GL_TRUE;
    imesa->Registers.ZBufCtrl.s3d.wrZafterAlphaTst = GL_FALSE;
    imesa->Registers.ZBufCtrl.s3d.zUpdateEn        = GL_TRUE;

    imesa->Registers.DrawCtrl.ni.srcAlphaMode      = SAM_One;
    imesa->Registers.DrawCtrl.ni.flushPdZbufWrites = GL_TRUE;
    imesa->Registers.DrawCtrl.ni.flushPdDestWrites = GL_TRUE;

    imesa->Registers.DrawCtrl.ni.ditherEn          = GL_TRUE;
    imesa->Registers.DrawCtrl.ni.cullMode          = BCM_None;

    imesa->LcsCullMode = BCM_None;
    imesa->Registers.TexDescr.s3d.palSize          = TPS_256;
}
void savageDDInitState( savageContextPtr imesa ) {
    volatile GLuint* pBCIBase;
    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
	savageDDInitState_s4 (imesa);
    else
	savageDDInitState_s3d (imesa);

    /*fprintf(stderr,"DBflag:%d\n",imesa->glCtx->Visual->DBflag);*/
    imesa->Registers.DestCtrl.ni.offset = imesa->savageScreen->backOffset>>11;
    if(imesa->savageScreen->cpp == 2)
    {
        imesa->Registers.DestCtrl.ni.dstPixFmt = 0;
        imesa->Registers.DestCtrl.ni.dstWidthInTile =
            (imesa->savageScreen->width+63)>>6;
    }
    else
    {
        imesa->Registers.DestCtrl.ni.dstPixFmt = 1;
        imesa->Registers.DestCtrl.ni.dstWidthInTile =
            (imesa->savageScreen->width+31)>>5;
    }

    imesa->IsDouble = GL_TRUE;

    imesa->NotFirstFrame = GL_FALSE;
    imesa->Registers.ZBufOffset.ni.offset=imesa->savageScreen->depthOffset>>11;
    if(imesa->savageScreen->zpp == 2)
    {
        imesa->Registers.ZBufOffset.ni.zBufWidthInTiles = 
            (imesa->savageScreen->width+63)>>6;
        imesa->Registers.ZBufOffset.ni.zDepthSelect = 0;
    }
    else
    {   
        imesa->Registers.ZBufOffset.ni.zBufWidthInTiles = 
            (imesa->savageScreen->width+31)>>5;
        imesa->Registers.ZBufOffset.ni.zDepthSelect = 1;      
    }
 
    if (imesa->glCtx->Color._DrawDestMask == BACK_LEFT_BIT) {
        if(imesa->IsFullScreen)
        {
            imesa->toggle = TARGET_BACK;

            imesa->drawMap = (char *)imesa->apertureBase[imesa->toggle];
            imesa->readMap = (char *)imesa->apertureBase[imesa->toggle];
        }
        else
        {
            imesa->drawMap = (char *)imesa->apertureBase[TARGET_BACK];
            imesa->readMap = (char *)imesa->apertureBase[TARGET_BACK];
        }

    } else {
      
        if(imesa->IsFullScreen)
        {
            imesa->toggle = TARGET_BACK;

            imesa->drawMap = (char *)imesa->apertureBase[imesa->toggle];
            imesa->readMap = (char *)imesa->apertureBase[imesa->toggle];
        }
        else
        {
            imesa->drawMap = (char *)imesa->apertureBase[TARGET_BACK];
            imesa->readMap = (char *)imesa->apertureBase[TARGET_BACK];
        }
    }

#if 0
    if(imesa->driDrawable)
    {
        LOCK_HARDWARE(imesa);
    }
    pBCIBase=SAVAGE_GET_BCI_POINTER(imesa,38);	
    *pBCIBase++ = WAIT_3D_IDLE;
    pBCIBase[0] = SET_REGISTER(DST,1);
    pBCIBase[1] = imesa->Registers.DestCtrl.ui;	
    pBCIBase+=2;
   	
    pBCIBase[0] = SET_REGISTER(ZBUFCTRL,1);
    pBCIBase[1] = imesa->Registers.ZBufCtrl.ui;	
    pBCIBase+=2;

    pBCIBase[0] = SET_REGISTER(ZBUFOFF,1);
    pBCIBase[1] = imesa->Registers.ZBufOffset.ui;	
    pBCIBase+=2;
  
    pBCIBase[0] = SET_REGISTER(FOGCTRL,1);
    pBCIBase[1] = imesa->Registers.FogCtrl.ui;	
    pBCIBase+=2;
  
  
    pBCIBase[0] = SET_REGISTER(FOGTABLE,8);
    memcpy((GLvoid *)(pBCIBase+1),(GLvoid *)imesa->Registers.FogTable.ni.ulEntry,32);
    pBCIBase+=9;

    pBCIBase[0] = SET_REGISTER(DRAWLOCALCTRL,1);
    pBCIBase[1] = imesa->Registers.DrawLocalCtrl.ui;	
    pBCIBase+=2;
   
    pBCIBase[0] = SET_REGISTER(DRAWCTRLGLOBAL0,2);
    pBCIBase[1] = imesa->Registers.DrawCtrl0.ui;	
    pBCIBase[2] = imesa->Registers.DrawCtrl1.ui;	
    pBCIBase+=3;

   
    pBCIBase[0] = SET_REGISTER(TEXPALADDR,1);
    pBCIBase[1] = imesa->Registers.TexPalAddr.ui;	
    pBCIBase+=2;


    pBCIBase[0] = SET_REGISTER(TEXCTRL0,6);
    pBCIBase[1] = imesa->Registers.TexCtrl[0].ui;	
    pBCIBase[2] = imesa->Registers.TexCtrl[1].ui;	
   
    pBCIBase[3] = imesa->Registers.TexAddr[0].ui;	
    pBCIBase[4] = imesa->Registers.TexAddr[1].ui;	
    pBCIBase[5] = imesa->Registers.TexBlendCtrl[0].ui;	
    pBCIBase[6] = imesa->Registers.TexBlendCtrl[1].ui;	
    pBCIBase+=7;
   
    pBCIBase[0] = SET_REGISTER(TEXDESCR,1);
    pBCIBase[1] = imesa->Registers.TexDescr.ui;	
    pBCIBase+=2;


    pBCIBase[0] = SET_REGISTER(STENCILCTRL,1);
    pBCIBase[1] = imesa->Registers.StencilCtrl.ui;	
    pBCIBase+=2;

    pBCIBase[0] = SET_REGISTER(ZWATERMARK,1);
    pBCIBase[1] = imesa->Registers.ZWatermarks.ui;	
    pBCIBase+=2;

    pBCIBase[0] = SET_REGISTER(DESTTEXRWWATERMARK,1);
    pBCIBase[1] = imesa->Registers.DestTexWatermarks.ui;	
    pBCIBase+=2;
   
    if(imesa->driDrawable)
    {
        UNLOCK_HARDWARE(imesa);
    }
#else
    if(imesa->driDrawable)
        LOCK_HARDWARE(imesa);

    /* This is the only reg that is not emitted in savageUpdateRegisters.
     * FIXME: Should this be set by the Xserver? */
    pBCIBase = SAVAGE_GET_BCI_POINTER(imesa,3);
    *pBCIBase++ = WAIT_3D_IDLE;
    *pBCIBase++ = SET_REGISTER(SAVAGE_ZBUFOFF_S4,1); /* The same on S3D. */
    *pBCIBase++ = imesa->Registers.ZBufOffset.ui;

    if(imesa->driDrawable)
        UNLOCK_HARDWARE(imesa);
    imesa->Registers.changed.uiRegistersChanged = ~0;
#endif
}


#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE))

void savageDDRenderStart(GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;
    XF86DRIClipRectPtr pbox;
    GLint nbox;

    /* if the screen is overrided by other application. set the scissor.
     * In MulitPass, re-render the screen.
     */
    pbox = dPriv->pClipRects;
    nbox = dPriv->numClipRects;
    if (nbox)
    {
        imesa->currentClip = nbox;
        /* set scissor to the first clip box*/
        savageDDScissor(ctx,pbox->x1,pbox->y1,pbox->x2,pbox->y2);

        savageUpdateCull(ctx);
        savageDDUpdateHwState(ctx); /* update to hardware register*/
    }
    else /* need not render at all*/
    {
        /*ctx->VB->CopyStart = ctx->VB->Count;*/
    }
    LOCK_HARDWARE(SAVAGE_CONTEXT(ctx));
}


void savageDDRenderEnd(GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
       
    UNLOCK_HARDWARE(SAVAGE_CONTEXT(ctx));
    if(!imesa->IsDouble)
    {
        savageSwapBuffers(imesa->driDrawable);
    }
	
}

static void savageDDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   SAVAGE_CONTEXT(ctx)->new_gl_state |= new_state;
}


void savageDDInitStateFuncs(GLcontext *ctx)
{
    ctx->Driver.UpdateState = savageDDInvalidateState;
    ctx->Driver.BlendEquationSeparate = savageDDBlendEquationSeparate;
    ctx->Driver.Fogfv = savageDDFogfv;
    ctx->Driver.Scissor = savageDDScissor;
#if HW_CULL
    ctx->Driver.CullFace = savageDDCullFaceFrontFace;
    ctx->Driver.FrontFace = savageDDCullFaceFrontFace;
#else
    ctx->Driver.CullFace = 0;
    ctx->Driver.FrontFace = 0;
#endif /* end #if HW_CULL */
    ctx->Driver.PolygonMode=NULL;
    ctx->Driver.PolygonStipple = 0;
    ctx->Driver.LineStipple = 0;
    ctx->Driver.LineWidth = 0;
    ctx->Driver.LogicOpcode = 0;
    ctx->Driver.DrawBuffer = savageDDDrawBuffer;
    ctx->Driver.ReadBuffer = savageDDReadBuffer;
    ctx->Driver.ClearColor = savageDDClearColor;

    ctx->Driver.DepthRange = savageDepthRange;
    ctx->Driver.Viewport = savageViewport;
    ctx->Driver.RenderMode = savageRenderMode;

    ctx->Driver.ClearIndex = 0;
    ctx->Driver.IndexMask = 0;

    if (SAVAGE_CONTEXT( ctx )->savageScreen->chipset >= S3_SAVAGE4) {
	ctx->Driver.Enable = savageDDEnable_s4;
	ctx->Driver.AlphaFunc = savageDDAlphaFunc_s4;
	ctx->Driver.DepthFunc = savageDDDepthFunc_s4;
	ctx->Driver.DepthMask = savageDDDepthMask_s4;
	ctx->Driver.BlendFuncSeparate = savageDDBlendFuncSeparate_s4;
	ctx->Driver.ColorMask = savageDDColorMask_s4;
	ctx->Driver.ShadeModel = savageDDShadeModel_s4;
	ctx->Driver.LightModelfv = savageDDLightModelfv_s4;
#if HW_STENCIL
	ctx->Driver.StencilFunc = savageDDStencilFunc;
	ctx->Driver.StencilMask = savageDDStencilMask;
	ctx->Driver.StencilOp = savageDDStencilOp;
#else
	ctx->Driver.StencilFunc = 0;
	ctx->Driver.StencilMask = 0;
	ctx->Driver.StencilOp = 0;
#endif /* end #if HW_STENCIL */
    } else {
	ctx->Driver.Enable = savageDDEnable_s3d;
	ctx->Driver.AlphaFunc = savageDDAlphaFunc_s3d;
	ctx->Driver.DepthFunc = savageDDDepthFunc_s3d;
	ctx->Driver.DepthMask = savageDDDepthMask_s3d;
	ctx->Driver.BlendFuncSeparate = savageDDBlendFuncSeparate_s3d;
	ctx->Driver.ColorMask = savageDDColorMask_s3d;
	ctx->Driver.ShadeModel = savageDDShadeModel_s3d;
	ctx->Driver.LightModelfv = savageDDLightModelfv_s3d;
	ctx->Driver.StencilFunc = 0;
	ctx->Driver.StencilMask = 0;
	ctx->Driver.StencilOp = 0;
    }

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
