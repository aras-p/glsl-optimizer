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
#include "buffers.h"
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

#include "xmlpool.h"

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
    savageBlendFunc_s4(ctx);
}
static void savageDDAlphaFunc_s3d(GLcontext *ctx, GLenum func, GLfloat ref)
{
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
    u_int32_t drawLocalCtrl = imesa->regs.s4.drawLocalCtrl.ui;
    u_int32_t drawCtrl0 = imesa->regs.s4.drawCtrl0.ui;
    u_int32_t drawCtrl1 = imesa->regs.s4.drawCtrl1.ui;

    /* set up draw control register (including blending, alpha
     * test, and shading model)
     */

    imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = 0;

    /*
     * blend modes
     */
    if(ctx->Color.BlendEnabled){
        switch (ctx->Color.BlendDstRGB)
        {
            case GL_ZERO:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
                break;

            case GL_ONE:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_One;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_COLOR:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_SrcClr;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_COLOR:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_1SrcClr;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_SrcAlpha;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_1SrcAlpha;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_One;
                }
                else
                {
                    imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode= DAM_DstAlpha;
                }
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
                }
                else
                {
                    imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode=DAM_1DstAlpha;
                    imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites= GL_TRUE;
                }
                break;
        }

        switch (ctx->Color.BlendSrcRGB)
        {
            case GL_ZERO:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_Zero;
                break;

            case GL_ONE:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_One;
                break;

            case GL_DST_COLOR:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_DstClr;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_COLOR:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_1DstClr;
                imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_SrcAlpha;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_1SrcAlpha;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_One;
                }
                else
                {
                    imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode= SAM_DstAlpha;
                    imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites= GL_TRUE;
                }
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)          
                {
                    imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_Zero;
                }
                else
                {
                    imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode=SAM_1DstAlpha;
                    imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites= GL_TRUE;
                }
                break;
        }
    }
    else
    {
        imesa->regs.s4.drawLocalCtrl.ni.dstAlphaMode = DAM_Zero;
        imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode = SAM_One;
    }

    /* alpha test*/

    if(ctx->Color.AlphaEnabled) 
    {
        ACmpFunc a;
	GLubyte alphaRef;

	CLAMPED_FLOAT_TO_UBYTE(alphaRef,ctx->Color.AlphaRef);
         
        switch(ctx->Color.AlphaFunc)  { 
	case GL_NEVER: a = CF_Never; break;
	case GL_ALWAYS: a = CF_Always; break;
	case GL_LESS: a = CF_Less; break; 
	case GL_LEQUAL: a = CF_LessEqual; break;
	case GL_EQUAL: a = CF_Equal; break;
	case GL_GREATER: a = CF_Greater; break;
	case GL_GEQUAL: a = CF_GreaterEqual; break;
	case GL_NOTEQUAL: a = CF_NotEqual; break;
	default:return;
        }   
      
	imesa->regs.s4.drawCtrl1.ni.alphaTestEn = GL_TRUE;
	imesa->regs.s4.drawCtrl1.ni.alphaTestCmpFunc = a;
	imesa->regs.s4.drawCtrl0.ni.alphaRefVal = alphaRef;
    }
    else
    {
	imesa->regs.s4.drawCtrl1.ni.alphaTestEn      = GL_FALSE;
    }

    /* Set/Reset Z-after-alpha*/

    imesa->regs.s4.drawLocalCtrl.ni.wrZafterAlphaTst =
	imesa->regs.s4.drawCtrl1.ni.alphaTestEn;
    /*imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn =
        ~drawLocalCtrl.ni.wrZafterAlphaTst;*/

    if (drawLocalCtrl != imesa->regs.s4.drawLocalCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
    if (drawCtrl0 != imesa->regs.s4.drawCtrl0.ui ||
	drawCtrl1 != imesa->regs.s4.drawCtrl1.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}
static void savageBlendFunc_s3d(GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    u_int32_t drawCtrl = imesa->regs.s3d.drawCtrl.ui;
    u_int32_t zBufCtrl = imesa->regs.s3d.zBufCtrl.ui;

    /* set up draw control register (including blending, alpha
     * test, dithering, and shading model)
     */

    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = 0;

    /*
     * blend modes
     */
    if(ctx->Color.BlendEnabled){
        switch (ctx->Color.BlendDstRGB)
        {
            case GL_ZERO:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_Zero;
                break;

            case GL_ONE:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_One;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_COLOR:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_SrcClr;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_COLOR:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_1SrcClr;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_SrcAlpha;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_1SrcAlpha;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_One;
                }
                else
                {
                    imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_DstAlpha;
                }
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_Zero;
                }
                else
                {
                    imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_1DstAlpha;
                    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }

        switch (ctx->Color.BlendSrcRGB)
        {
            case GL_ZERO:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_Zero;
                break;

            case GL_ONE:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_One;
                break;

            case GL_DST_COLOR:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_DstClr;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_ONE_MINUS_DST_COLOR:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_1DstClr;
                imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                break;

            case GL_SRC_ALPHA:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_SrcAlpha;
                break;

            case GL_ONE_MINUS_SRC_ALPHA:
                imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_1SrcAlpha;
                break;

            case GL_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)
                {
                    imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_One;
                }
                else
                {
                    imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_DstAlpha;
                    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;

            case GL_ONE_MINUS_DST_ALPHA:
                if (imesa->glCtx->Visual.alphaBits == 0)          
                {
                    imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_Zero;
                }
                else
                {
                    imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_1DstAlpha;
                    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
                }
                break;
        }
    }
    else
    {
        imesa->regs.s3d.drawCtrl.ni.dstAlphaMode = DAM_Zero;
        imesa->regs.s3d.drawCtrl.ni.srcAlphaMode = SAM_One;
    }

    /* alpha test*/

    if(ctx->Color.AlphaEnabled) 
    {
        ACmpFunc a;
	GLubyte alphaRef;

	CLAMPED_FLOAT_TO_UBYTE(alphaRef,ctx->Color.AlphaRef);
         
        switch(ctx->Color.AlphaFunc)  { 
	case GL_NEVER: a = CF_Never; break;
	case GL_ALWAYS: a = CF_Always; break;
	case GL_LESS: a = CF_Less; break; 
	case GL_LEQUAL: a = CF_LessEqual; break;
	case GL_EQUAL: a = CF_Equal; break;
	case GL_GREATER: a = CF_Greater; break;
	case GL_GEQUAL: a = CF_GreaterEqual; break;
	case GL_NOTEQUAL: a = CF_NotEqual; break;
	default:return;
        }   

	imesa->regs.s3d.drawCtrl.ni.alphaTestEn = GL_TRUE;
	imesa->regs.s3d.drawCtrl.ni.alphaTestCmpFunc = a;
	imesa->regs.s3d.drawCtrl.ni.alphaRefVal = alphaRef;
    }
    else
    {
	imesa->regs.s3d.drawCtrl.ni.alphaTestEn = GL_FALSE;
    }

    /* Set/Reset Z-after-alpha*/

    imesa->regs.s3d.zBufCtrl.ni.wrZafterAlphaTst =
	imesa->regs.s3d.drawCtrl.ni.alphaTestEn;

    if (drawCtrl != imesa->regs.s3d.drawCtrl.ui ||
	zBufCtrl != imesa->regs.s3d.zBufCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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
    ZCmpFunc zmode;
    u_int32_t drawLocalCtrl = imesa->regs.s4.drawLocalCtrl.ui;
    u_int32_t zBufCtrl = imesa->regs.s4.zBufCtrl.ui;
    u_int32_t zWatermarks = imesa->regs.s4.zWatermarks.ui; /* FIXME: in DRM */

    /* set up z-buffer control register (global)
     * set up z-buffer offset register (global)
     * set up z read/write watermarks register (global)
     */

    switch(func)  { /* reversed (see savageCalcViewport) */
    case GL_NEVER: zmode = CF_Never; break;
    case GL_ALWAYS: zmode = CF_Always; break;
    case GL_LESS: zmode = CF_Greater; break; 
    case GL_LEQUAL: zmode = CF_GreaterEqual; break;
    case GL_EQUAL: zmode = CF_Equal; break;
    case GL_GREATER: zmode = CF_Less; break;
    case GL_GEQUAL: zmode = CF_LessEqual; break;
    case GL_NOTEQUAL: zmode = CF_NotEqual; break;
    default:return;
    } 
    if (ctx->Depth.Test)
    {

	imesa->regs.s4.zBufCtrl.ni.zCmpFunc = zmode;
	imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn = ctx->Depth.Mask;
	imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites = GL_TRUE;
#if 1
	imesa->regs.s4.zWatermarks.ni.wLow = 0;
#endif

	imesa->regs.s4.zBufCtrl.ni.zBufEn = GL_TRUE;
    }
    else if (imesa->glCtx->Stencil.Enabled && imesa->hw_stencil)
    {
        /* Need to keep Z on for Stencil. */
	imesa->regs.s4.zBufCtrl.ni.zCmpFunc = CF_Always;
	imesa->regs.s4.zBufCtrl.ni.zBufEn   = GL_TRUE;
	imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn = GL_FALSE;
	imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
	imesa->regs.s4.zWatermarks.ni.wLow        = 8;
    }
    else
    {

        if (imesa->regs.s4.drawLocalCtrl.ni.drawUpdateEn == GL_FALSE)
        {
            imesa->regs.s4.zBufCtrl.ni.zCmpFunc = CF_Always;
            imesa->regs.s4.zBufCtrl.ni.zBufEn   = GL_TRUE;
        }
        else

            /* DRAWUPDATE_REQUIRES_Z_ENABLED*/
        {
	    imesa->regs.s4.zBufCtrl.ni.zBufEn         = GL_FALSE;
        }
	imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn = GL_FALSE;
	imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
	imesa->regs.s4.zWatermarks.ni.wLow        = 8;
    }

    if (drawLocalCtrl != imesa->regs.s4.drawLocalCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
    if (zBufCtrl != imesa->regs.s4.zBufCtrl.ui ||
	zWatermarks != imesa->regs.s4.zWatermarks.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}
static void savageDDDepthFunc_s3d(GLcontext *ctx, GLenum func)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    ZCmpFunc zmode;
    u_int32_t drawCtrl = imesa->regs.s3d.drawCtrl.ui;
    u_int32_t zBufCtrl = imesa->regs.s3d.zBufCtrl.ui;
    u_int32_t zWatermarks = imesa->regs.s3d.zWatermarks.ui; /* FIXME: in DRM */

    /* set up z-buffer control register (global)
     * set up z-buffer offset register (global)
     * set up z read/write watermarks register (global)
     */
    switch(func)  { /* reversed (see savageCalcViewport) */
    case GL_NEVER: zmode = CF_Never; break;
    case GL_ALWAYS: zmode = CF_Always; break;
    case GL_LESS: zmode = CF_Greater; break; 
    case GL_LEQUAL: zmode = CF_GreaterEqual; break;
    case GL_EQUAL: zmode = CF_Equal; break;
    case GL_GREATER: zmode = CF_Less; break;
    case GL_GEQUAL: zmode = CF_LessEqual; break;
    case GL_NOTEQUAL: zmode = CF_NotEqual; break;
    default:return;
    } 
    if (ctx->Depth.Test)
    {
	imesa->regs.s3d.zBufCtrl.ni.zBufEn = GL_TRUE;
	imesa->regs.s3d.zBufCtrl.ni.zCmpFunc = zmode;
	imesa->regs.s3d.zBufCtrl.ni.zUpdateEn = ctx->Depth.Mask;
	
	imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_TRUE;
#if 1
	imesa->regs.s3d.zWatermarks.ni.wLow = 0;
#endif
    }
    else
    {
	if (imesa->regs.s3d.zBufCtrl.ni.drawUpdateEn == GL_FALSE) {
	    imesa->regs.s3d.zBufCtrl.ni.zCmpFunc = CF_Always;
            imesa->regs.s3d.zBufCtrl.ni.zBufEn = GL_TRUE;
	}
        else

            /* DRAWUPDATE_REQUIRES_Z_ENABLED*/
        {
	    imesa->regs.s3d.zBufCtrl.ni.zBufEn = GL_FALSE;
        }
	imesa->regs.s3d.zBufCtrl.ni.zUpdateEn = GL_FALSE;
	imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_FALSE;
	imesa->regs.s3d.zWatermarks.ni.wLow = 8;
    }
  
    if (drawCtrl != imesa->regs.s3d.drawCtrl.ui ||
	zBufCtrl != imesa->regs.s3d.zBufCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
    if (zWatermarks != imesa->regs.s3d.zWatermarks.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}

static void savageDDDepthMask_s4(GLcontext *ctx, GLboolean flag)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    u_int32_t drawLocalCtrl = imesa->regs.s4.drawLocalCtrl.ui;

    if (flag)
    {
	imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites = GL_TRUE;
    }
    else
    {
	imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites = GL_FALSE;
    }
    savageDDDepthFunc_s4(ctx,ctx->Depth.Func);

    if (drawLocalCtrl != imesa->regs.s4.drawLocalCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
}
static void savageDDDepthMask_s3d(GLcontext *ctx, GLboolean flag)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    u_int32_t drawCtrl = imesa->regs.s3d.drawCtrl.ui;

    if (flag)
    {
	imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_TRUE;
    }
    else
    {
	imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_FALSE;
    }
    savageDDDepthFunc_s3d(ctx,ctx->Depth.Func);

    if (drawCtrl != imesa->regs.s3d.drawCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
}




/* =============================================================
 * Hardware clipping
 */


static void savageDDScissor( GLcontext *ctx, GLint x, GLint y, 
                             GLsizei w, GLsizei h )
{ 
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    imesa->scissor_rect.x1 = MAX2(imesa->drawX+x,imesa->draw_rect.x1);
    imesa->scissor_rect.y1 = MAX2(imesa->drawY+imesa->driDrawable->h -(y+h),
                                  imesa->draw_rect.y1);
    imesa->scissor_rect.x2 = MIN2(imesa->drawX+x+w,imesa->draw_rect.x2);
    imesa->scissor_rect.y2 = MIN2(imesa->drawY+imesa->driDrawable->h - y,
                                  imesa->draw_rect.y2);
    

    imesa->scissorChanged=GL_TRUE;

    imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
}



static void savageDDDrawBuffer(GLcontext *ctx, GLenum mode )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    u_int32_t destCtrl = imesa->regs.s4.destCtrl.ui;

    /*
     * _DrawDestMask is easier to cope with than <mode>.
     */
    switch ( ctx->Color._DrawDestMask[0] ) {
    case DD_FRONT_LEFT_BIT:
        imesa->IsDouble = GL_FALSE;
      
	imesa->drawMap = (char *)imesa->apertureBase[TARGET_FRONT];
	imesa->readMap = (char *)imesa->apertureBase[TARGET_FRONT];
	imesa->regs.s4.destCtrl.ni.offset = imesa->savageScreen->frontOffset>>11;
        imesa->NotFirstFrame = GL_FALSE;
        savageXMesaSetFrontClipRects( imesa );
	FALLBACK( ctx, SAVAGE_FALLBACK_DRAW_BUFFER, GL_FALSE );
	break;
    case DD_BACK_LEFT_BIT:
        imesa->IsDouble = GL_TRUE;
        imesa->drawMap = (char *)imesa->apertureBase[TARGET_BACK];
        imesa->readMap = (char *)imesa->apertureBase[TARGET_BACK];
	imesa->regs.s4.destCtrl.ni.offset = imesa->savageScreen->backOffset>>11;
        imesa->NotFirstFrame = GL_FALSE;
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

    if (destCtrl != imesa->regs.s4.destCtrl.ui)
        imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
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

   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + imesa->drawX + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + imesa->driDrawable->h + imesa->drawY + SUBPIXEL_Y;
   /* Depth range is reversed (far: 0, near: 1) so that float depth
    * compensates for loss of accuracy of far coordinates. */
   if (imesa->float_depth && imesa->savageScreen->zpp == 2) {
       /* The Savage 16-bit floating point depth format can't encode
	* numbers < 2^-16. Make sure all depth values stay greater
	* than that. */
       m[MAT_SZ] = - v[MAT_SZ] * imesa->depth_scale * (65535.0/65536.0);
       m[MAT_TZ] = 1.0 - v[MAT_TZ] * imesa->depth_scale * (65535.0/65536.0);
   } else {
       m[MAT_SZ] = - v[MAT_SZ] * imesa->depth_scale;
       m[MAT_TZ] = 1.0 - v[MAT_TZ] * imesa->depth_scale;
   }

   imesa->SetupNewInputs = ~0;
}

static void savageViewport( GLcontext *ctx, 
			    GLint x, GLint y, 
			    GLsizei width, GLsizei height )
{
   /* update size of Mesa/software ancillary buffers */
   _mesa_ResizeBuffersMESA();
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
    imesa->new_state |= SAVAGE_NEW_CULL;
}
#endif /* end #if HW_CULL */

static void savageUpdateCull( GLcontext *ctx )
{
#if HW_CULL
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLuint cullMode;
    if (ctx->Polygon.CullFlag &&
	imesa->raster_primitive >= GL_TRIANGLES &&
	ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK)
	cullMode = imesa->LcsCullMode;
    else
	cullMode = BCM_None;
    if (imesa->savageScreen->chipset >= S3_SAVAGE4) {
	if (imesa->regs.s4.drawCtrl1.ni.cullMode != cullMode) {
	    imesa->regs.s4.drawCtrl1.ni.cullMode = cullMode;
	    imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
	}
    } else {
	if (imesa->regs.s3d.drawCtrl.ni.cullMode != cullMode) {
	    imesa->regs.s3d.drawCtrl.ni.cullMode = cullMode;
	    imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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
	imesa->regs.s4.drawLocalCtrl.ni.drawUpdateEn = GL_TRUE;
    }
    else
    {
	imesa->regs.s4.drawLocalCtrl.ni.drawUpdateEn = GL_FALSE;
    }
    imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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
	imesa->regs.s3d.zBufCtrl.ni.drawUpdateEn = GL_TRUE;
    }
    else
    {
	imesa->regs.s3d.zBufCtrl.ni.drawUpdateEn = GL_FALSE;
    }
    imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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
    u_int32_t drawLocalCtrl = imesa->regs.s4.drawLocalCtrl.ui;

    if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR &&
	ctx->Light.Enabled) {
	imesa->regs.s4.drawLocalCtrl.ni.specShadeEn = GL_TRUE;
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_TRUE);*/
    } else {
	imesa->regs.s4.drawLocalCtrl.ni.specShadeEn = GL_FALSE;
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_FALSE);*/
    }

    if (drawLocalCtrl != imesa->regs.s4.drawLocalCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
}
static void savageUpdateSpecular_s3d(GLcontext *ctx) {
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    u_int32_t drawCtrl = imesa->regs.s3d.drawCtrl.ui;

    if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR &&
	ctx->Light.Enabled) {
	imesa->regs.s3d.drawCtrl.ni.specShadeEn = GL_TRUE;
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_TRUE);*/
    } else {
	imesa->regs.s3d.drawCtrl.ni.specShadeEn = GL_FALSE;
	/*FALLBACK (ctx, SAVAGE_FALLBACK_SPECULAR, GL_FALSE);*/
    }

    if (drawCtrl != imesa->regs.s3d.drawCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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
    u_int32_t drawLocalCtrl = imesa->regs.s4.drawLocalCtrl.ui;

    if (mod == GL_SMOOTH)  
    {    
	imesa->regs.s4.drawLocalCtrl.ni.flatShadeEn = GL_FALSE;
    }
    else
    {
	imesa->regs.s4.drawLocalCtrl.ni.flatShadeEn = GL_TRUE;
    }

    if (drawLocalCtrl != imesa->regs.s4.drawLocalCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
}
static void savageDDShadeModel_s3d(GLcontext *ctx, GLuint mod)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    u_int32_t drawCtrl = imesa->regs.s3d.drawCtrl.ui;

    if (mod == GL_SMOOTH)  
    {    
	imesa->regs.s3d.drawCtrl.ni.flatShadeEn = GL_FALSE;
    }
    else
    {
	imesa->regs.s3d.drawCtrl.ni.flatShadeEn = GL_TRUE;
    }

    if (drawCtrl != imesa->regs.s3d.drawCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
}


/* =============================================================
 * Fog
 * The fogCtrl register has the same position and the same layout
 * on savage3d and savage4. No need for two separate functions.
 */

static void savageDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    GLuint  fogClr;
    u_int32_t fogCtrl = imesa->regs.s4.fogCtrl.ui;

    /*if ((ctx->Fog.Enabled) &&(pname == GL_FOG_COLOR))*/
    if (ctx->Fog.Enabled)
    {
        fogClr = (((GLubyte)(ctx->Fog.Color[0]*255.0F) << 16) |
                  ((GLubyte)(ctx->Fog.Color[1]*255.0F) << 8) |
                  ((GLubyte)(ctx->Fog.Color[2]*255.0F) << 0));
	imesa->regs.s4.fogCtrl.ni.fogEn  = GL_TRUE;
        /*cheap fog*/
	imesa->regs.s4.fogCtrl.ni.fogMode  = GL_TRUE;
	imesa->regs.s4.fogCtrl.ni.fogClr = fogClr;    
    }    
    else
    {
        /*No fog*/
        
	imesa->regs.s4.fogCtrl.ni.fogEn     = 0;
	imesa->regs.s4.fogCtrl.ni.fogMode   = 0;
    }

    if (fogCtrl != imesa->regs.s4.fogCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}


static void savageDDStencilFunc(GLcontext *ctx, GLenum func, GLint ref,
                                GLuint mask)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    SCmpFunc a=0;
    u_int32_t zBufCtrl = imesa->regs.s4.zBufCtrl.ui;
    u_int32_t stencilCtrl = imesa->regs.s4.stencilCtrl.ui;

    imesa->regs.s4.zBufCtrl.ni.stencilRefVal = ctx->Stencil.Ref[0];
    imesa->regs.s4.stencilCtrl.ni.readMask  = ctx->Stencil.ValueMask[0];

    switch (ctx->Stencil.Function[0])
    {
    case GL_NEVER: a = CF_Never; break;
    case GL_ALWAYS: a = CF_Always; break;
    case GL_LESS: a = CF_Less; break; 
    case GL_LEQUAL: a = CF_LessEqual; break;
    case GL_EQUAL: a = CF_Equal; break;
    case GL_GREATER: a = CF_Greater; break;
    case GL_GEQUAL: a = CF_GreaterEqual; break;
    case GL_NOTEQUAL: a = CF_NotEqual; break;
    default:
        break;
    }

    imesa->regs.s4.stencilCtrl.ni.cmpFunc = a;

    if (zBufCtrl != imesa->regs.s4.zBufCtrl.ui ||
	stencilCtrl != imesa->regs.s4.stencilCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}

static void savageDDStencilMask(GLcontext *ctx, GLuint mask)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);

    if (imesa->regs.s4.stencilCtrl.ni.writeMask != ctx->Stencil.WriteMask[0]) {
	imesa->regs.s4.stencilCtrl.ni.writeMask = ctx->Stencil.WriteMask[0];
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
    }
}

static void savageDDStencilOp(GLcontext *ctx, GLenum fail, GLenum zfail,
                              GLenum zpass)
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    u_int32_t stencilCtrl = imesa->regs.s4.stencilCtrl.ui;

    switch (ctx->Stencil.FailFunc[0])
    {
    case GL_KEEP:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Keep;
	break;
    case GL_ZERO:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Zero;
	break;
    case GL_REPLACE:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Equal;
	break;
    case GL_INCR:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_IncClamp;
	break;
    case GL_DECR:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_DecClamp;
	break;
    case GL_INVERT:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Invert;
	break;
    case GL_INCR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Inc;
	break;
    case GL_DECR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.failOp = STC_FAIL_Dec;
	break;
    }


    switch (ctx->Stencil.ZFailFunc[0])
    {
    case GL_KEEP:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Keep;
	break;
    case GL_ZERO:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Zero;
	break;
    case GL_REPLACE:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Equal;
	break;
    case GL_INCR:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_IncClamp;
	break;
    case GL_DECR:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_DecClamp;
	break;
    case GL_INVERT:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Invert;
	break;
    case GL_INCR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Inc;
	break;
    case GL_DECR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.passZfailOp = STC_FAIL_Dec;
	break;
    }

    switch (ctx->Stencil.ZPassFunc[0])
    {
    case GL_KEEP:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Keep;
	break;
    case GL_ZERO:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Zero;
	break;
    case GL_REPLACE:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Equal;
	break;
    case GL_INCR:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_IncClamp;
	break;
    case GL_DECR:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_DecClamp;
	break;
    case GL_INVERT:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Invert;
	break;
    case GL_INCR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Inc;
	break;
    case GL_DECR_WRAP_EXT:
	imesa->regs.s4.stencilCtrl.ni.passZpassOp = STC_FAIL_Dec;
	break;
    }

    if (stencilCtrl != imesa->regs.s4.stencilCtrl.ui)
	imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
}


/* =============================================================
 */

static void savageDDEnable_s4(GLcontext *ctx, GLenum cap, GLboolean state)
{
   
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    switch(cap) {
        case GL_ALPHA_TEST:
            /* we should consider the disable case*/
            savageBlendFunc_s4(ctx);
            break;
        case GL_BLEND:
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
            savageDDDepthFunc_s4(ctx,ctx->Depth.Func);
            break;
        case GL_SCISSOR_TEST:
            imesa->scissor = state;
            imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
	    savageDDScissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			    ctx->Scissor.Width, ctx->Scissor.Height);
            break;
        case GL_STENCIL_TEST:
	    if (!imesa->hw_stencil)
		FALLBACK (ctx, SAVAGE_FALLBACK_STENCIL, state);
	    else {
		imesa->regs.s4.stencilCtrl.ni.stencilEn = state;
		if (ctx->Stencil.Enabled &&
		    imesa->regs.s4.zBufCtrl.ni.zBufEn != GL_TRUE)
		{
		    /* Stencil buffer requires Z enabled. */
		    imesa->regs.s4.zBufCtrl.ni.zCmpFunc       = CF_Always;
		    imesa->regs.s4.zBufCtrl.ni.zBufEn         = GL_TRUE;
		    imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn = GL_FALSE;
		}
		imesa->dirty |= SAVAGE_UPLOAD_GLOBAL | SAVAGE_UPLOAD_LOCAL;
	    }
            break;
        case GL_FOG:
            savageDDFogfv(ctx,0,0);	
            break;
        case GL_CULL_FACE:
#if HW_CULL
            if (state)
            {
                savageDDCullFaceFrontFace(ctx,0);
            }
            else
            {
		imesa->LcsCullMode = BCM_None;
		imesa->new_state |= SAVAGE_NEW_CULL;
            }
#endif
            break;
        case GL_DITHER:
            if (state)
            {
                if ( ctx->Color.DitherFlag )
                {
                    imesa->regs.s4.drawCtrl1.ni.ditherEn=GL_TRUE;
                }
            }   
            if (!ctx->Color.DitherFlag )
            {
                imesa->regs.s4.drawCtrl1.ni.ditherEn=GL_FALSE;
            }
            imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
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
    switch(cap) {
        case GL_ALPHA_TEST:
            /* we should consider the disable case*/
            savageBlendFunc_s3d(ctx);
            break;
        case GL_BLEND:
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
            savageDDDepthFunc_s3d(ctx,ctx->Depth.Func);
            break;
        case GL_SCISSOR_TEST:
            imesa->scissor = state;
            imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
	    savageDDScissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			    ctx->Scissor.Width, ctx->Scissor.Height);
            break;
        case GL_STENCIL_TEST:
	    FALLBACK (ctx, SAVAGE_FALLBACK_STENCIL, state);
	    break;
        case GL_FOG:
            savageDDFogfv(ctx,0,0);	
            break;
        case GL_CULL_FACE:
#if HW_CULL
            if (state)
            {
                savageDDCullFaceFrontFace(ctx,0);
            }
            else
            {
                imesa->LcsCullMode = BCM_None;
		imesa->new_state |= SAVAGE_NEW_CULL;
            }
#endif
            break;
        case GL_DITHER:
            if (state)
            {
                if ( ctx->Color.DitherFlag )
                {
                    imesa->regs.s3d.drawCtrl.ni.ditherEn=GL_TRUE;
                }
            }
            if (!ctx->Color.DitherFlag )
            {
                imesa->regs.s3d.drawCtrl.ni.ditherEn=GL_FALSE;
            }
            imesa->dirty |= SAVAGE_UPLOAD_LOCAL;
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

    if (imesa->new_state) {
	savageFlushVertices(imesa);
	if (imesa->new_state & SAVAGE_NEW_TEXTURE) {
	    savageUpdateTextureState( ctx );
	}
	if ((imesa->new_state & SAVAGE_NEW_CULL)) {
	    savageUpdateCull(ctx);
	}
	imesa->new_state = 0;
    }
}


void savageEmitDrawingRectangle( savageContextPtr imesa )
{
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;
    savageScreenPrivate *savageScreen = imesa->savageScreen;
    drm_clip_rect_t *pbox;
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
   
    imesa->scissorChanged = GL_TRUE;

    /*   imesa->regs.ni.changed.ni.fDrawCtrl0Changed=GL_TRUE;
         imesa->regs.ni.changed.ni.fDrawCtrl1Changed=GL_TRUE;*/

    savageCalcViewport (imesa->glCtx);
}


static void savageDDPrintDirty( const char *msg, GLuint state )
{
    fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s%s\n",	   
            msg,
            (unsigned int) state,
            (state & SAVAGE_UPLOAD_LOCAL)      ? "upload-local, " : "",
            (state & SAVAGE_UPLOAD_TEX0)       ? "upload-tex0, " : "",
            (state & SAVAGE_UPLOAD_TEX1)       ? "upload-tex1, " : "",
            (state & SAVAGE_UPLOAD_FOGTBL)     ? "upload-fogtbl, " : "",
            (state & SAVAGE_UPLOAD_GLOBAL)     ? "upload-global, " : "",
            (state & SAVAGE_UPLOAD_TEXGLOBAL)  ? "upload-texglobal, " : "",
            (state & SAVAGE_UPLOAD_CLIPRECTS)  ? "upload-cliprects, " : ""
            );
}


/**
 * Check if global registers were changed
 */
static GLboolean savageGlobalRegChanged (savageContextPtr imesa,
					 GLuint first, GLuint last) {
    GLuint i;
    for (i = first - SAVAGE_FIRST_REG; i <= last - SAVAGE_FIRST_REG; ++i) {
	if (((imesa->oldRegs.ui[i] ^ imesa->regs.ui[i]) &
	     imesa->globalRegMask.ui[i]) != 0)
	    return GL_TRUE;
    }
    return GL_FALSE;
}
static void savageEmitOldRegs (savageContextPtr imesa,
			       GLuint first, GLuint last, GLboolean global) {
    GLuint n = last-first+1;
    drm_savage_cmd_header_t *cmd = savageAllocCmdBuf(imesa, n*4);
    cmd->state.cmd = SAVAGE_CMD_STATE;
    cmd->state.global = global;
    cmd->state.count = n;
    cmd->state.start = first;
    memcpy(cmd+1, &imesa->oldRegs.ui[first-SAVAGE_FIRST_REG], n*4);
}
static void savageEmitContiguousRegs (savageContextPtr imesa,
				      GLuint first, GLuint last) {
    GLuint i;
    GLuint n = last-first+1;
    drm_savage_cmd_header_t *cmd = savageAllocCmdBuf(imesa, n*4);
    cmd->state.cmd = SAVAGE_CMD_STATE;
    cmd->state.global = savageGlobalRegChanged(imesa, first, last);
    cmd->state.count = n;
    cmd->state.start = first;
    memcpy(cmd+1, &imesa->regs.ui[first-SAVAGE_FIRST_REG], n*4);
    /* savageAllocCmdBuf may need to flush the cmd buffer and backup
     * the current hardware state. It should see the "old" (current)
     * state that has actually been emitted to the hardware. Therefore
     * this update is done *after* savageAllocCmdBuf. */
    for (i = first - SAVAGE_FIRST_REG; i <= last - SAVAGE_FIRST_REG; ++i)
	imesa->oldRegs.ui[i] = imesa->regs.ui[i];
    if (SAVAGE_DEBUG & DEBUG_STATE)
	fprintf (stderr, "Emitting regs 0x%02x-0x%02x\n", first, last);
}
static void savageEmitChangedRegs (savageContextPtr imesa,
				   GLuint first, GLuint last) {
    GLuint i, firstChanged;
    firstChanged = SAVAGE_NR_REGS;
    for (i = first - SAVAGE_FIRST_REG; i <= last - SAVAGE_FIRST_REG; ++i) {
	if (imesa->oldRegs.ui[i] != imesa->regs.ui[i]) {
	    if (firstChanged == SAVAGE_NR_REGS)
		firstChanged = i;
	} else {
	    if (firstChanged != SAVAGE_NR_REGS) {
		savageEmitContiguousRegs (imesa, firstChanged+SAVAGE_FIRST_REG,
					  i-1+SAVAGE_FIRST_REG);
		firstChanged = SAVAGE_NR_REGS;
	    }
	}
    }
    if (firstChanged != SAVAGE_NR_REGS)
	savageEmitContiguousRegs (imesa, firstChanged+SAVAGE_FIRST_REG,
				  last);
}
static void savageEmitChangedRegChunk (savageContextPtr imesa,
				       GLuint first, GLuint last) {
    GLuint i;
    for (i = first - SAVAGE_FIRST_REG; i <= last - SAVAGE_FIRST_REG; ++i) {
	if (imesa->oldRegs.ui[i] != imesa->regs.ui[i]) {
	    savageEmitContiguousRegs (imesa, first, last);
	    break;
	}
    }
}
static void savageUpdateRegister_s4(savageContextPtr imesa)
{
    /*
     * Scissors updates drawctrl0 and drawctrl 1
     */
    if (imesa->scissorChanged && imesa->savageScreen->driScrnPriv->drmMinor < 1)
    {
        if(imesa->scissor)
        {
            imesa->regs.s4.drawCtrl0.ni.scissorXStart = imesa->scissor_rect.x1;
            imesa->regs.s4.drawCtrl0.ni.scissorYStart = imesa->scissor_rect.y1;
            imesa->regs.s4.drawCtrl1.ni.scissorXEnd = imesa->scissor_rect.x2-1;
            imesa->regs.s4.drawCtrl1.ni.scissorYEnd = imesa->scissor_rect.y2-1;
        }
        else
        {
            imesa->regs.s4.drawCtrl0.ni.scissorXStart = imesa->draw_rect.x1;
            imesa->regs.s4.drawCtrl0.ni.scissorYStart = imesa->draw_rect.y1;
            imesa->regs.s4.drawCtrl1.ni.scissorXEnd = imesa->draw_rect.x2-1;
            imesa->regs.s4.drawCtrl1.ni.scissorYEnd = imesa->draw_rect.y2-1;
        }
	imesa->scissorChanged = GL_FALSE;
    }

    /* the savage4 uses the contiguous range of BCI registers 0x1e-0x39
     * 0x1e-0x27 are local, no need to check them for global changes */
    savageEmitChangedRegs (imesa, 0x1e, 0x39);

    imesa->dirty=0;
}
static void savageUpdateRegister_s3d(savageContextPtr imesa)
{
    if (imesa->scissorChanged && imesa->savageScreen->driScrnPriv->drmMinor < 1)
    {
        if(imesa->scissor)
        {
            imesa->regs.s3d.scissorsStart.ni.scissorXStart =
		imesa->scissor_rect.x1;
            imesa->regs.s3d.scissorsStart.ni.scissorYStart =
		imesa->scissor_rect.y1;
            imesa->regs.s3d.scissorsEnd.ni.scissorXEnd =
		imesa->scissor_rect.x2-1;
            imesa->regs.s3d.scissorsEnd.ni.scissorYEnd =
		imesa->scissor_rect.y2-1;
        }
        else
        {
            imesa->regs.s3d.scissorsStart.ni.scissorXStart =
		imesa->draw_rect.x1;
            imesa->regs.s3d.scissorsStart.ni.scissorYStart =
		imesa->draw_rect.y1;
            imesa->regs.s3d.scissorsEnd.ni.scissorXEnd =
		imesa->draw_rect.x2-1;
            imesa->regs.s3d.scissorsEnd.ni.scissorYEnd =
		imesa->draw_rect.y2-1;
        }
	imesa->scissorChanged = GL_FALSE;
    }

    /* Some temporary hacks to workaround lockups. Not sure if they are
     * still needed. But they work for now. */
    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;
    imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_TRUE;

    /* the savage3d uses two contiguous ranges of BCI registers:
     * 0x18-0x1c and 0x20-0x38. Some texture registers need to be
     * emitted in one chunk or we get some funky rendering errors. */
    /* FIXME: watermark registers aren't programmed correctly ATM */
    savageEmitChangedRegs (imesa, 0x18, 0x19);
    savageEmitChangedRegChunk (imesa, 0x1a, 0x1c);
    savageEmitChangedRegs (imesa, 0x20, 0x36);

    imesa->dirty=0;
}


void savageEmitOldState( savageContextPtr imesa )
{
    assert(imesa->cmdBuf.write == imesa->cmdBuf.base);
    if (imesa->savageScreen->chipset >= S3_SAVAGE4) {
	savageEmitOldRegs (imesa, 0x1e, 0x39, GL_TRUE);
    } else {
	savageEmitOldRegs (imesa, 0x18, 0x1c, GL_TRUE);
	savageEmitOldRegs (imesa, 0x20, 0x36, GL_FALSE);
    }
}


/* Push the state into the sarea and/or texture memory.
 */
void savageEmitChangedState( savageContextPtr imesa )
{
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_API)
        savageDDPrintDirty( "\n\n\nsavageEmitHwStateLocked", imesa->dirty );

    if (imesa->dirty & ~SAVAGE_UPLOAD_CLIPRECTS)
    {
        if (imesa->dirty & (SAVAGE_UPLOAD_GLOBAL | SAVAGE_UPLOAD_LOCAL |
                            SAVAGE_UPLOAD_TEX0   | SAVAGE_UPLOAD_TEX1  |
			    SAVAGE_UPLOAD_FOGTBL | SAVAGE_UPLOAD_TEXGLOBAL))
        {
	    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
		fprintf (stderr, "... emitting state\n");
	    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
		savageUpdateRegister_s4(imesa);
	    else
		savageUpdateRegister_s3d(imesa);
        }

        imesa->dirty &= SAVAGE_UPLOAD_CLIPRECTS;
    }
}


static void savageDDInitState_s4( savageContextPtr imesa )
{
#if 1
    imesa->regs.s4.destCtrl.ui          = 1<<7;
#endif

    imesa->regs.s4.zBufCtrl.ni.zCmpFunc = CF_Less;
    imesa->regs.s4.zBufCtrl.ni.wToZEn               = GL_TRUE;
    if (imesa->float_depth) {
	imesa->regs.s4.zBufCtrl.ni.zExpOffset =
	    imesa->savageScreen->zpp == 2 ? 16 : 32;
	imesa->regs.s4.zBufCtrl.ni.floatZEn = GL_TRUE;
    } else {
	imesa->regs.s4.zBufCtrl.ni.zExpOffset = 0;
	imesa->regs.s4.zBufCtrl.ni.floatZEn = GL_FALSE;
    }
    imesa->regs.s4.texBlendCtrl[0].ui            = TBC_NoTexMap;
    imesa->regs.s4.texBlendCtrl[1].ui            = TBC_NoTexMap1;
    imesa->regs.s4.drawCtrl0.ui         = 0;
#if 0
    imesa->regs.s4.drawCtrl1.ni.xyOffsetEn = 1;
#endif

    /* Set DestTexWatermarks_31,30 to 01 always.
     *Has no effect if dest. flush is disabled.
     */
#if 0
    imesa->regs.s4.zWatermarks.ui       = 0x12000C04;
    imesa->regs.s4.destTexWatermarks.ui = 0x40200400;
#else
    imesa->regs.s4.zWatermarks.ui       = 0x16001808;
    imesa->regs.s4.destTexWatermarks.ui = 0x4f000000;
#endif
    imesa->regs.s4.drawCtrl0.ni.dPerfAccelEn = GL_TRUE;

    /* clrCmpAlphaBlendCtrl is needed to get alphatest and
     * alpha blending working properly
     */

    imesa->regs.s4.texCtrl[0].ni.dBias                 = 0x08;
    imesa->regs.s4.texCtrl[1].ni.dBias                 = 0x08;
    imesa->regs.s4.texCtrl[0].ni.texXprEn              = GL_TRUE;
    imesa->regs.s4.texCtrl[1].ni.texXprEn              = GL_TRUE;
    imesa->regs.s4.texCtrl[0].ni.dMax                  = 0x0f;
    imesa->regs.s4.texCtrl[1].ni.dMax                  = 0x0f;
    imesa->regs.s4.drawLocalCtrl.ni.drawUpdateEn     = GL_TRUE;
    imesa->regs.s4.drawLocalCtrl.ni.srcAlphaMode    = SAM_One;
    imesa->regs.s4.drawLocalCtrl.ni.wrZafterAlphaTst = GL_FALSE;
    imesa->regs.s4.drawLocalCtrl.ni.flushPdZbufWrites= GL_TRUE;
    imesa->regs.s4.drawLocalCtrl.ni.flushPdDestWrites= GL_TRUE;

    imesa->regs.s4.drawLocalCtrl.ni.zUpdateEn= GL_TRUE;
    imesa->regs.s4.drawCtrl1.ni.ditherEn = (
	driQueryOptioni(&imesa->optionCache, "color_reduction") ==
	DRI_CONF_COLOR_REDUCTION_DITHER) ? GL_TRUE : GL_FALSE;
    imesa->regs.s4.drawCtrl1.ni.cullMode             = BCM_None;

    imesa->regs.s4.zBufCtrl.ni.stencilRefVal      = 0x00;

    imesa->regs.s4.stencilCtrl.ni.stencilEn       = GL_FALSE;
    imesa->regs.s4.stencilCtrl.ni.cmpFunc         = CF_Always;
    imesa->regs.s4.stencilCtrl.ni.failOp          = STC_FAIL_Keep;
    imesa->regs.s4.stencilCtrl.ni.passZfailOp     = STC_FAIL_Keep;
    imesa->regs.s4.stencilCtrl.ni.passZpassOp     = STC_FAIL_Keep;
    imesa->regs.s4.stencilCtrl.ni.writeMask       = 0xff;
    imesa->regs.s4.stencilCtrl.ni.readMask        = 0xff;

    imesa->LcsCullMode=BCM_None;
    imesa->regs.s4.texDescr.ni.palSize               = TPS_256;

    /* clear the local registers in the global reg mask */
    imesa->globalRegMask.s4.drawLocalCtrl.ui   = 0;
    imesa->globalRegMask.s4.texPalAddr.ui      = 0;
    imesa->globalRegMask.s4.texCtrl[0].ui      = 0;
    imesa->globalRegMask.s4.texCtrl[1].ui      = 0;
    imesa->globalRegMask.s4.texAddr[0].ui      = 0;
    imesa->globalRegMask.s4.texAddr[1].ui      = 0;
    imesa->globalRegMask.s4.texBlendCtrl[0].ui = 0;
    imesa->globalRegMask.s4.texBlendCtrl[1].ui = 0;
    imesa->globalRegMask.s4.texXprClr.ui       = 0;
    imesa->globalRegMask.s4.texDescr.ui        = 0;
}
static void savageDDInitState_s3d( savageContextPtr imesa )
{
#if 1
    imesa->regs.s3d.destCtrl.ui           = 1<<7;
#endif

    imesa->regs.s3d.zBufCtrl.ni.zCmpFunc  = CF_Less;
#if 0
    imesa->regs.s3d.drawCtrl.ni.xyOffsetEn = 1;
#endif

    /* Set DestTexWatermarks_31,30 to 01 always.
     *Has no effect if dest. flush is disabled.
     */
#if 0
    imesa->regs.s3d.zWatermarks.ui       = 0x12000C04;
    imesa->regs.s3d.destTexWatermarks.ui = 0x40200400;
#else
    imesa->regs.s3d.zWatermarks.ui       = 0x16001808;
    imesa->regs.s3d.destTexWatermarks.ui = 0x4f000000;
#endif

    /* clrCmpAlphaBlendCtrl is needed to get alphatest and
     * alpha blending working properly
     */

    imesa->regs.s3d.texCtrl.ni.dBias          = 0x08;
    imesa->regs.s3d.texCtrl.ni.texXprEn       = GL_TRUE;

    imesa->regs.s3d.zBufCtrl.ni.drawUpdateEn     = GL_TRUE;
    imesa->regs.s3d.zBufCtrl.ni.wrZafterAlphaTst = GL_FALSE;
    imesa->regs.s3d.zBufCtrl.ni.zUpdateEn        = GL_TRUE;

    imesa->regs.s3d.drawCtrl.ni.srcAlphaMode      = SAM_One;
    imesa->regs.s3d.drawCtrl.ni.flushPdZbufWrites = GL_TRUE;
    imesa->regs.s3d.drawCtrl.ni.flushPdDestWrites = GL_TRUE;

    imesa->regs.s3d.drawCtrl.ni.ditherEn =  (
	driQueryOptioni(&imesa->optionCache, "color_reduction") ==
	DRI_CONF_COLOR_REDUCTION_DITHER) ? GL_TRUE : GL_FALSE;
    imesa->regs.s3d.drawCtrl.ni.cullMode          = BCM_None;

    imesa->LcsCullMode = BCM_None;
    imesa->regs.s3d.texDescr.ni.palSize          = TPS_256;

    /* clear the local registers in the global reg mask */
    imesa->globalRegMask.s3d.texPalAddr.ui = 0;
    imesa->globalRegMask.s3d.texXprClr.ui  = 0;
    imesa->globalRegMask.s3d.texAddr.ui    = 0;
    imesa->globalRegMask.s3d.texDescr.ui   = 0;
    imesa->globalRegMask.s3d.texCtrl.ui    = 0;

    imesa->globalRegMask.s3d.fogCtrl.ui = 0;

    /* drawCtrl is local with some exceptions */
    imesa->globalRegMask.s3d.drawCtrl.ui = 0;
    imesa->globalRegMask.s3d.drawCtrl.ni.cullMode = 0x3;
    imesa->globalRegMask.s3d.drawCtrl.ni.alphaTestCmpFunc = 0x7;
    imesa->globalRegMask.s3d.drawCtrl.ni.alphaTestEn = 0x1;
    imesa->globalRegMask.s3d.drawCtrl.ni.alphaRefVal = 0xff;

    /* zBufCtrl is local with some exceptions */
    imesa->globalRegMask.s3d.zBufCtrl.ui = 0;
    imesa->globalRegMask.s3d.zBufCtrl.ni.zCmpFunc = 0x7;
    imesa->globalRegMask.s3d.zBufCtrl.ni.zBufEn = 0x1;
}
void savageDDInitState( savageContextPtr imesa ) {
    memset (imesa->regs.ui, 0, SAVAGE_NR_REGS*sizeof(u_int32_t));
    memset (imesa->globalRegMask.ui, 0xff, SAVAGE_NR_REGS*sizeof(u_int32_t));
    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
	savageDDInitState_s4 (imesa);
    else
	savageDDInitState_s3d (imesa);

    /*fprintf(stderr,"DBflag:%d\n",imesa->glCtx->Visual->DBflag);*/
    /* zbufoffset and destctrl have the same position and layout on
     * savage4 and savage3d. */
    imesa->regs.s4.destCtrl.ni.offset = imesa->savageScreen->backOffset>>11;
    if(imesa->savageScreen->cpp == 2)
    {
        imesa->regs.s4.destCtrl.ni.dstPixFmt = 0;
        imesa->regs.s4.destCtrl.ni.dstWidthInTile =
            (imesa->savageScreen->width+63)>>6;
    }
    else
    {
        imesa->regs.s4.destCtrl.ni.dstPixFmt = 1;
        imesa->regs.s4.destCtrl.ni.dstWidthInTile =
            (imesa->savageScreen->width+31)>>5;
    }

    imesa->IsDouble = GL_TRUE;

    imesa->NotFirstFrame = GL_FALSE;
    imesa->regs.s4.zBufOffset.ni.offset=imesa->savageScreen->depthOffset>>11;
    if(imesa->savageScreen->zpp == 2)
    {
        imesa->regs.s4.zBufOffset.ni.zBufWidthInTiles = 
            (imesa->savageScreen->width+63)>>6;
        imesa->regs.s4.zBufOffset.ni.zDepthSelect = 0;
    }
    else
    {   
        imesa->regs.s4.zBufOffset.ni.zBufWidthInTiles = 
            (imesa->savageScreen->width+31)>>5;
        imesa->regs.s4.zBufOffset.ni.zDepthSelect = 1;      
    }
 
    if (imesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {
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

    memcpy (imesa->oldRegs.ui, imesa->regs.ui, SAVAGE_NR_REGS*sizeof(u_int32_t));

    /* Emit the initial state to the (empty) command buffer. */
    assert (imesa->cmdBuf.write == imesa->cmdBuf.base);
    savageEmitOldState(imesa);
    imesa->cmdBuf.start = imesa->cmdBuf.write;
}


#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE))

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
	ctx->Driver.StencilFunc = savageDDStencilFunc;
	ctx->Driver.StencilMask = savageDDStencilMask;
	ctx->Driver.StencilOp = savageDDStencilOp;
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
