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

#include "glheader.h"
#include "buffers.h"
#include "context.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"
#include "dd.h"

#include "mm.h"
#include "via_context.h"
#include "via_state.h"
#include "via_tex.h"
#include "via_vb.h"
#include "via_tris.h"
#include "via_ioctl.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"


static GLuint ROP[16] = {
    HC_HROP_BLACK,    /* GL_CLEAR           0                      	*/
    HC_HROP_DPa,      /* GL_AND             s & d                  	*/
    HC_HROP_PDna,     /* GL_AND_REVERSE     s & ~d  			*/
    HC_HROP_P,        /* GL_COPY            s                       	*/
    HC_HROP_DPna,     /* GL_AND_INVERTED    ~s & d                      */
    HC_HROP_D,        /* GL_NOOP            d  		                */
    HC_HROP_DPx,      /* GL_XOR             s ^ d                       */
    HC_HROP_DPo,      /* GL_OR              s | d                       */
    HC_HROP_DPon,     /* GL_NOR             ~(s | d)                    */
    HC_HROP_DPxn,     /* GL_EQUIV           ~(s ^ d)                    */
    HC_HROP_Dn,       /* GL_INVERT          ~d                       	*/
    HC_HROP_PDno,     /* GL_OR_REVERSE      s | ~d                      */
    HC_HROP_Pn,       /* GL_COPY_INVERTED   ~s                       	*/
    HC_HROP_DPno,     /* GL_OR_INVERTED     ~s | d                      */
    HC_HROP_DPan,     /* GL_NAND            ~(s & d)                    */
    HC_HROP_WHITE     /* GL_SET             1                       	*/
};

static __inline__ GLuint viaPackColor(GLuint format,
                                      GLubyte r, GLubyte g,
                                      GLubyte b, GLubyte a)
{
    switch (format) {
    case 0x10:
        return PACK_COLOR_565(r, g, b);
    case 0x20:
        return PACK_COLOR_8888(a, r, g, b);        
    default:
        if (VIA_DEBUG) fprintf(stderr, "unknown format %d\n", (int)format);
        return PACK_COLOR_8888(a, r, g, b);
   }
}

static void viaAlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;
}

static void viaBlendEquationSeparate(GLcontext *ctx, GLenum rgbMode, GLenum aMode)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);

    /* GL_EXT_blend_equation_separate not supported */
    ASSERT(rgbMode == aMode);

    /* Can only do GL_ADD equation in hardware */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_BLEND_EQ, rgbMode != GL_FUNC_ADD_EXT);

    /* BlendEquation sets ColorLogicOpEnabled in an unexpected
     * manner.
     */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_LOGICOP,
             (ctx->Color.ColorLogicOpEnabled &&
              ctx->Color.LogicOp != GL_COPY));
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

static void viaBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLboolean fallback = GL_FALSE;
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    switch (ctx->Color.BlendSrcRGB) {
    case GL_ZERO:                break;
    case GL_SRC_ALPHA:           break;
    case GL_ONE:                 break;
    case GL_DST_COLOR:           break;
    case GL_ONE_MINUS_DST_COLOR: break;
    case GL_ONE_MINUS_SRC_ALPHA: break;
    case GL_DST_ALPHA:           break;
    case GL_ONE_MINUS_DST_ALPHA: break;
    case GL_SRC_ALPHA_SATURATE:  /*a |= SDM_SRC_SRC_ALPHA; break;*/
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        return;
    }

    switch (ctx->Color.BlendDstRGB) {
    case GL_SRC_ALPHA:           break;
    case GL_ONE_MINUS_SRC_ALPHA: break;
    case GL_ZERO:                break;
    case GL_ONE:                 break;
    case GL_SRC_COLOR:           break;
    case GL_ONE_MINUS_SRC_COLOR: break;
    case GL_DST_ALPHA:           break;
    case GL_ONE_MINUS_DST_ALPHA: break;
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        return;
    }

    FALLBACK(vmesa, VIA_FALLBACK_BLEND_FUNC, fallback);
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

/* Shouldn't be called as the extension is disabled.
 */
static void viaBlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB,
                                 GLenum dfactorRGB, GLenum sfactorA,
                                 GLenum dfactorA)
{
    if (dfactorRGB != dfactorA || sfactorRGB != sfactorA) {
        _mesa_error(ctx, GL_INVALID_OPERATION, "glBlendEquation (disabled)");
    }

    viaBlendFunc(ctx, sfactorRGB, dfactorRGB);
}


static void viaDepthFunc(GLcontext *ctx, GLenum func)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;
}

static void viaDepthMask(GLcontext *ctx, GLboolean flag)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

static void viaPolygonStipple(GLcontext *ctx, const GLubyte *mask)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}


/* =============================================================
 * Hardware clipping
 */
static void viaScissor(GLcontext *ctx, GLint x, GLint y,
                       GLsizei w, GLsizei h)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
    if (ctx->Scissor.Enabled) {
        VIA_FIREVERTICES(vmesa); /* don't pipeline cliprect changes */
        vmesa->uploadCliprects = GL_TRUE;
    }

    vmesa->scissorRect.x1 = x;
    vmesa->scissorRect.y1 = vmesa->driDrawable->h - (y + h);
    vmesa->scissorRect.x2 = x + w;
    vmesa->scissorRect.y2 = vmesa->driDrawable->h - y;
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
}


static void viaLogicOp(GLcontext *ctx, GLenum opcode)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (VIA_DEBUG) fprintf(stderr, "opcode = %x\n", opcode);
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

/* Fallback to swrast for select and feedback.
 */
static void viaRenderMode(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    FALLBACK(vmesa, VIA_FALLBACK_RENDERMODE, (mode != GL_RENDER));
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}


static void viaDrawBuffer(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (mode == GL_FRONT) {
        VIA_FIREVERTICES(vmesa);
        VIA_STATECHANGE(vmesa, VIA_UPLOAD_BUFFERS);
        vmesa->drawMap = (char *)vmesa->driScreen->pFB;
        vmesa->readMap = (char *)vmesa->driScreen->pFB;
	vmesa->drawPitch = vmesa->front.pitch;
	vmesa->readPitch = vmesa->front.pitch;
        viaXMesaSetFrontClipRects(vmesa);
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
        return;
    }
    else if (mode == GL_BACK) {
        VIA_FIREVERTICES(vmesa);
        VIA_STATECHANGE(vmesa, VIA_UPLOAD_BUFFERS);
        vmesa->drawMap = vmesa->back.map;
        vmesa->readMap = vmesa->back.map;
	vmesa->drawPitch = vmesa->back.pitch;
	vmesa->readPitch = vmesa->back.pitch;	
        viaXMesaSetBackClipRects(vmesa);
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
        return;
    }
    else {
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_TRUE);
        return;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
}

static void viaClearColor(GLcontext *ctx, const GLfloat color[4])
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLubyte pcolor[4];
    pcolor[0] = (GLubyte) (255 * color[0]);
    pcolor[1] = (GLubyte) (255 * color[1]);
    pcolor[2] = (GLubyte) (255 * color[2]);
    pcolor[3] = (GLubyte) (255 * color[3]);
    vmesa->ClearColor = viaPackColor(vmesa->viaScreen->bitsPerPixel,
                                     pcolor[0], pcolor[1],
                                     pcolor[2], pcolor[3]);
	
}

/* =============================================================
 * Culling - the via isn't quite as clean here as the rest of
 *           its interfaces, but it's not bad.
 */
static void viaCullFaceFrontFace(GLcontext *ctx, GLenum unused)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

static void viaLineWidth(GLcontext *ctx, GLfloat widthf)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

static void viaPointSize(GLcontext *ctx, GLfloat sz)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

static void viaBitmap( GLcontext *ctx, GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap ) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
        
    /*=* [DBG] csmash : fix background overlap option menu *=*/
    LOCK_HARDWARE(vmesa);
    viaFlushPrimsLocked(vmesa);
    UNLOCK_HARDWARE(vmesa);
    
    WAIT_IDLE
    /*=* [DBG] csmash : fix segmentation fault *=*/
    /*=* John Sheng [2003.7.18] texenv *=*/
    /*if (!vmesa->drawMap && !vmesa->readMap) {*/
    if (1) {
	if (vmesa->glCtx->Color._DrawDestMask[0] == __GL_BACK_BUFFER_MASK) {
	    viaDrawBuffer(ctx, GL_BACK);
	}
	else {
	    viaDrawBuffer(ctx, GL_FRONT);
	}
    }
    /*=* [DBG] csmash : white option words become brown *=*/
    /*_swrast_Bitmap(ctx, px, py, width, height, unpack, bitmap );*/
    {
	GLboolean fog;
	fog = ctx->Fog.Enabled;
	ctx->Fog.Enabled = GL_FALSE;
	_swrast_Bitmap(ctx, px, py, width, height, unpack, bitmap );
	ctx->Fog.Enabled = fog;
    }
}

/* =============================================================
 * Color masks
 */
static void viaColorMask(GLcontext *ctx,
                         GLboolean r, GLboolean g,
                         GLboolean b, GLboolean a)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

/* Seperate specular not fully implemented on the via.
 */
static void viaLightModelfv(GLcontext *ctx, GLenum pname,
                            const GLfloat *param)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}


/* In Mesa 3.5 we can reliably do native flatshading.
 */
static void viaShadeModel(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

/* =============================================================
 * Fog
 */
static void viaFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

/* =============================================================
 */
static void viaEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    vmesa = vmesa;    
}

/* =============================================================
 */
void viaEmitDrawingRectangle(viaContextPtr vmesa)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    viaScreenPrivate *viaScreen = vmesa->viaScreen;
    int x0 = vmesa->drawX;
    int y0 = vmesa->drawY;
    int x1 = x0 + dPriv->w;
    int y1 = y0 + dPriv->h;
/*    GLuint dr2, dr3, dr4;
*/

    /* Coordinate origin of the window - may be offscreen.
     */
/*   dr4 = vmesa->BufferSetup[VIA_DESTREG_DR4] = ((y0 << 16) |
                                                  (((unsigned)x0) & 0xFFFF));
*/                                

    /* Clip to screen.
     */
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > viaScreen->width - 1) x1 = viaScreen->width - 1;
    if (y1 > viaScreen->height - 1) y1 = viaScreen->height - 1;

    /* Onscreen drawing rectangle.
     */
/*   dr2 = vmesa->BufferSetup[VIA_DESTREG_DR2] = ((y0 << 16) | x0);
    dr3 = vmesa->BufferSetup[VIA_DESTREG_DR3] = (((y1 + 1) << 16) | (x1 + 1));
*/  

    vmesa->dirty |= VIA_UPLOAD_BUFFERS;
}


void viaCalcViewport(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    const GLfloat *v = ctx->Viewport._WindowMap.m;
    GLfloat *m = vmesa->ViewportMatrix.m;
    
    /* See also via_translate_vertex.  SUBPIXEL adjustments can be done
     * via state vars, too.
     */
    m[MAT_SX] =  v[MAT_SX];
    m[MAT_TX] =  v[MAT_TX] + vmesa->drawXoff;
    m[MAT_SY] = - v[MAT_SY];
    m[MAT_TY] = - v[MAT_TY] + vmesa->driDrawable->h;
    /*=* John Sheng [2003.7.2] for visual config & viewperf drv-08 *=*/
    if (vmesa->depth.bpp == 16) {
	m[MAT_SZ] =  v[MAT_SZ] * (1.0 / 0xffff);
	m[MAT_TZ] =  v[MAT_TZ] * (1.0 / 0xffff);
    }
    else {
	m[MAT_SZ] =  v[MAT_SZ] * (1.0 / 0xffffffff);
	m[MAT_TZ] =  v[MAT_TZ] * (1.0 / 0xffffffff);
    }
}

static void viaViewport(GLcontext *ctx,
                        GLint x, GLint y,
                        GLsizei width, GLsizei height)
{
   /* update size of Mesa/software ancillary buffers */
   _mesa_ResizeBuffersMESA();
    viaCalcViewport(ctx);
}

static void viaDepthRange(GLcontext *ctx,
                          GLclampd nearval, GLclampd farval)
{
    viaCalcViewport(ctx);
}

void viaPrintDirty(const char *msg, GLuint state)
{
    if (VIA_DEBUG)
	fprintf(stderr, "%s (0x%x): %s%s%s%s\n",
            msg,
            (unsigned int) state,
            (state & VIA_UPLOAD_TEX0)   ? "upload-tex0, " : "",
            (state & VIA_UPLOAD_TEX1)   ? "upload-tex1, " : "",
            (state & VIA_UPLOAD_CTX)    ? "upload-ctx, " : "",
            (state & VIA_UPLOAD_BUFFERS)    ? "upload-bufs, " : ""
            );
}


void viaInitState(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    vmesa->regCmdA = HC_ACMD_HCmdA;
    vmesa->regCmdB = HC_ACMD_HCmdB | HC_HVPMSK_X | HC_HVPMSK_Y | HC_HVPMSK_Z;
    vmesa->regEnable = HC_HenCW_MASK;

    if (vmesa->glCtx->Color._DrawDestMask[0] == __GL_BACK_BUFFER_MASK) {
        vmesa->drawMap = vmesa->back.map;
        vmesa->readMap = vmesa->back.map;
    }
    else {
        vmesa->drawMap = (char *)vmesa->driScreen->pFB;
        vmesa->readMap = (char *)vmesa->driScreen->pFB;
    }
}

/**
 * Convert S and T texture coordinate wrap modes to hardware bits.
 */
static u_int32_t
get_wrap_mode( GLenum sWrap, GLenum tWrap )
{
    u_int32_t v = 0;


    switch( sWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Srepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Sclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Smirror;
	break;
    }

    switch( tWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Trepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Tclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Tmirror;
	break;
    }
    
    return v;
}


void viaChooseTextureState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];
    /*=* John Sheng [2003.7.18] texture combine *=*/

    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (texUnit0->_ReallyEnabled || texUnit1->_ReallyEnabled) {
	if (VIA_DEBUG) {
	    fprintf(stderr, "Texture._ReallyEnabled - in\n");    
	    fprintf(stderr, "texUnit0->_ReallyEnabled = %x\n",texUnit0->_ReallyEnabled);
	}

	if (VIA_DEBUG) {
            struct gl_texture_object *texObj0 = texUnit0->_Current;
            struct gl_texture_object *texObj1 = texUnit1->_Current;

	    fprintf(stderr, "env mode: 0x%04x / 0x%04x\n", texUnit0->EnvMode, texUnit1->EnvMode);

	    if ( (texObj0 != NULL) && (texObj0->Image[0][0] != NULL) )
	      fprintf(stderr, "format 0: 0x%04x\n", texObj0->Image[0][0]->Format);
		    
	    if ( (texObj1 != NULL) && (texObj1->Image[0][0] != NULL) )
	      fprintf(stderr, "format 1: 0x%04x\n", texObj1->Image[0][0]->Format);
	}


        if (texUnit0->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit0->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];

	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->_ReallyEnabled\n");    
            if (texImage->Border) {
                FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, GL_TRUE);
                return;
            }
            vmesa->regCmdB |= HC_HVPMSK_S | HC_HVPMSK_T | HC_HVPMSK_W | HC_HVPMSK_Cs;
            vmesa->regEnable |= HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK;
   
            switch (texObj->MinFilter) {
            case GL_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                break;
            case GL_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                break;
            case GL_NEAREST_MIPMAP_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Nearest;
                break;
            case GL_LINEAR_MIPMAP_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Nearest;
                break;
            case GL_NEAREST_MIPMAP_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Linear;
                break;
            case GL_LINEAR_MIPMAP_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Linear;
                break;
            default:
                break;
            }

    	    if (texObj->MagFilter) {
                vmesa->regHTXnTB_0 |= HC_HTXnFLSe_Linear |
                                      HC_HTXnFLTe_Linear;
            }
            else {
                vmesa->regHTXnTB_0 |= HC_HTXnFLSe_Nearest |
                                      HC_HTXnFLTe_Nearest;
            }

	    vmesa->regHTXnMPMD_0 &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD_0 |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode %x\n",texUnit0->EnvMode);    

	    viaTexCombineState( vmesa, texUnit0->_CurrentCombine, 0 );
        }
	else {
	/* Should turn Cs off if actually no Cs */
	}

        if (texUnit1->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit1->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];

            if (texImage->Border) {
                FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, GL_TRUE);
                return;
            }

            switch (texObj->MinFilter) {
            case GL_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                break;
            case GL_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                break;
            case GL_NEAREST_MIPMAP_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Nearest;
                break ;
            case GL_LINEAR_MIPMAP_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Nearest;
                break ;
            case GL_NEAREST_MIPMAP_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Linear;
                break ;
            case GL_LINEAR_MIPMAP_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Linear;
                break ;
            default:
                break;
            }

	    switch(texObj->MagFilter) {
		case GL_NEAREST:
            	    vmesa->regHTXnTB_1 |= HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
            	    break;
        	case GL_LINEAR:
            	    vmesa->regHTXnTB_1 |= HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
            	    break;
	    }
	    
	    vmesa->regHTXnMPMD_1 &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD_1 |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    viaTexCombineState( vmesa, texUnit1->_CurrentCombine, 1 );
        }
        vmesa->dirty |= VIA_UPLOAD_TEXTURE;
	
	if (VIA_DEBUG) {
	    fprintf( stderr, "Csat_0 / Cop_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLCsat_0, vmesa->regHTXnTBLCop_0 );
	    fprintf( stderr, "Asat_0        = 0x%08x\n",
		     vmesa->regHTXnTBLAsat_0 );
	    fprintf( stderr, "RCb_0 / RAa_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLRCb_0, vmesa->regHTXnTBLRAa_0 );
	    fprintf( stderr, "RCa_0 / RCc_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLRCa_0, vmesa->regHTXnTBLRCc_0 );
	    fprintf( stderr, "RCbias_0      = 0x%08x\n",
		     vmesa->regHTXnTBLRCbias_0 );
	}
    }
    else {
	if (ctx->Fog.Enabled)
    	    vmesa->regCmdB &= (~(HC_HVPMSK_S | HC_HVPMSK_T));	
	else	    
    	    vmesa->regCmdB &= (~(HC_HVPMSK_S | HC_HVPMSK_T | HC_HVPMSK_W));
        vmesa->regEnable &= (~(HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK));
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    
}

void viaChooseColorState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLenum s = ctx->Color.BlendSrcRGB;
    GLenum d = ctx->Color.BlendDstRGB;

    /* The HW's blending equation is:
     * (Ca * FCa + Cbias + Cb * FCb) << Cshift
     */
     if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    

    if (ctx->Color.BlendEnabled) {
        vmesa->regEnable |= HC_HenABL_MASK;
        /* Ca  -- always from source color.
         */
        vmesa->regHABLCsat = HC_HABLCsat_MASK | HC_HABLCa_OPC |
                             HC_HABLCa_Csrc;
        /* Aa  -- always from source alpha.
         */
        vmesa->regHABLAsat = HC_HABLAsat_MASK | HC_HABLAa_OPA |
                             HC_HABLAa_Asrc;
        /* FCa -- depend on following condition.
         * FAa -- depend on following condition.
         */
        switch (s) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_SRC_ALPHA_SATURATE:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (f, f, f, 1), f = min(As, 1 - Ad) = min(As, 1 - 1) = 0
                     * So (f, f, f, 1) = (0, 0, 0, 1)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (f, f, f, 1), f = min(As, 1 - Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_mimAsrcInvAdst;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
            }
            break;
        }

        /* Op is add.
         */

        /* bias is 0.
         */
        vmesa->regHABLCsat |= HC_HABLCbias_HABLRCbias;
        vmesa->regHABLAsat |= HC_HABLAbias_HABLRAbias;

        /* Cb  -- always from destination color.
         */
        vmesa->regHABLCop = HC_HABLCb_OPC | HC_HABLCb_Cdst;
        /* Ab  -- always from destination alpha.
         */
        vmesa->regHABLAop = HC_HABLAb_OPA | HC_HABLAb_Adst;
        /* FCb -- depend on following condition.
         */
        switch (d) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
                }
            }
            break;
        default:
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        }

        if (vmesa->viaScreen->bitsPerPixel <= 16)
            vmesa->regEnable &= ~HC_HenDT_MASK;

        vmesa->dirty |= (VIA_UPLOAD_BLEND | VIA_UPLOAD_ENABLE);
    }
    else {
        vmesa->regEnable &= (~HC_HenABL_MASK);
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }

    if (ctx->Color.AlphaEnabled) {
        vmesa->regEnable |= HC_HenAT_MASK;
        vmesa->regHATMD = (((GLchan)ctx->Color.AlphaRef) & 0xFF) |
            ((ctx->Color.AlphaFunc - GL_NEVER) << 8);
        vmesa->dirty |= (VIA_UPLOAD_ALPHATEST | VIA_UPLOAD_ENABLE);
    }
    else {
        vmesa->regEnable &= (~HC_HenAT_MASK);
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }

    if (ctx->Color.DitherFlag && (vmesa->viaScreen->bitsPerPixel < 32)) {
        if (ctx->Color.BlendEnabled) {
            vmesa->regEnable &= ~HC_HenDT_MASK;
        }
        else {
            vmesa->regEnable |= HC_HenDT_MASK;
        }
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }

    if (ctx->Color.ColorLogicOpEnabled) 
        vmesa->regHROP = ROP[ctx->Color.LogicOp & 0xF];
    else
        vmesa->regHROP = HC_HROP_P;

    vmesa->regHFBBMSKL = (*(GLuint *)&ctx->Color.ColorMask[0]) & 0xFFFFFF;
    vmesa->regHROP |= ((*(GLuint *)&ctx->Color.ColorMask[0]) >> 24) & 0xFF;
    vmesa->dirty |= VIA_UPLOAD_MASK_ROP;

    if ((GLuint)((GLuint *)&ctx->Color.ColorMask[0]) & 0xFF000000)
        vmesa->regEnable |= HC_HenAW_MASK;
    else
        vmesa->regEnable &= (~HC_HenAW_MASK);
    vmesa->dirty |= VIA_UPLOAD_ENABLE;  
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaChooseFogState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Fog.Enabled) {
        GLubyte r, g, b, a;

        vmesa->regCmdB |= (HC_HVPMSK_Cd | HC_HVPMSK_Cs | HC_HVPMSK_W);
        vmesa->regEnable |= HC_HenFOG_MASK;

        /* Use fog equation 0 (OpenGL's default) & local fog.
         */
        vmesa->regHFogLF = 0x0;

        r = (GLubyte)(ctx->Fog.Color[0] * 255.0F);
        g = (GLubyte)(ctx->Fog.Color[1] * 255.0F);
        b = (GLubyte)(ctx->Fog.Color[2] * 255.0F);
        a = (GLubyte)(ctx->Fog.Color[3] * 255.0F);
        vmesa->regHFogCL = (r << 16) | (g << 8) | b;
        vmesa->regHFogCH = a;
        vmesa->dirty |= (VIA_UPLOAD_FOG | VIA_UPLOAD_ENABLE);
    }
    else {
        if (!ctx->Texture._EnabledUnits) {
            vmesa->regCmdB &= ~ HC_HVPMSK_W;
    	    vmesa->regCmdB &= ~ HC_HVPMSK_Cs;
	}	    
        vmesa->regEnable &= ~HC_HenFOG_MASK;
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }
}

void viaChooseDepthState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (ctx->Depth.Test) {
        vmesa->regCmdB |= HC_HVPMSK_Z;
        vmesa->regEnable |= HC_HenZT_MASK;
        if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);
	vmesa->regHZWTMD = (ctx->Depth.Func - GL_NEVER) << 16;
        vmesa->dirty |= (VIA_UPLOAD_DEPTH | VIA_UPLOAD_ENABLE);
	
    }
    else {
        /* Still need to send parameter Z.
         */
        
	vmesa->regCmdB |= HC_HVPMSK_Z;
        vmesa->regEnable &= ~HC_HenZT_MASK;
        
        /*=* [DBG] racer : can't display cars in car selection menu *=*/
	/*if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);*/
	vmesa->regEnable &= (~HC_HenZW_MASK);

        vmesa->dirty |= VIA_UPLOAD_ENABLE;                  
    }
}

void viaChooseLightState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Light.ShadeModel == GL_SMOOTH) {
        vmesa->regCmdA |= HC_HShading_Gouraud;
        vmesa->regCmdB |= HC_HVPMSK_Cd;
    }
    else {
        vmesa->regCmdA &= ~HC_HShading_Gouraud;
        vmesa->regCmdB |= HC_HVPMSK_Cd;
    }
}

void viaChooseLineState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Line.SmoothFlag) {
        vmesa->regEnable |= HC_HenAA_MASK;
    }
    else {
        if (!ctx->Polygon.SmoothFlag) {
            vmesa->regEnable &= ~HC_HenAA_MASK;
        }
    }

    if (ctx->Line.StippleFlag) {
        vmesa->regEnable |= HC_HenLP_MASK;
        vmesa->regHLP = ctx->Line.StipplePattern;
        vmesa->regHLPRF = ctx->Line.StippleFactor;
        vmesa->dirty |= VIA_UPLOAD_LINESTIPPLE;
    }
    else {
        vmesa->regEnable &= ~HC_HenLP_MASK;
    }
    vmesa->dirty |= VIA_UPLOAD_ENABLE;
}

void viaChoosePolygonState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Polygon.SmoothFlag) {
        vmesa->regEnable |= HC_HenAA_MASK;
    }
    else {
        if (!ctx->Line.SmoothFlag) {
            vmesa->regEnable &= ~HC_HenAA_MASK;
        }
    }

    if (ctx->Polygon.StippleFlag) {
        vmesa->regEnable |= HC_HenSP_MASK;
        vmesa->dirty |= VIA_UPLOAD_POLYGONSTIPPLE;
    }
    else {
        vmesa->regEnable &= ~HC_HenSP_MASK;
    }

    if (ctx->Polygon.CullFlag) {
        vmesa->regEnable |= HC_HenFBCull_MASK;
    }
    else {
        vmesa->regEnable &= ~HC_HenFBCull_MASK;
    }
    vmesa->dirty |= VIA_UPLOAD_ENABLE;
}

void viaChooseStencilState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    
    if (ctx->Stencil.Enabled) {
        GLuint temp;

        vmesa->regEnable |= HC_HenST_MASK;
        temp = (ctx->Stencil.Ref[0] & 0xFF) << HC_HSTREF_SHIFT;
        temp |= 0xFF << HC_HSTOPMSK_SHIFT;
        temp |= (ctx->Stencil.ValueMask[0] & 0xFF);
        vmesa->regHSTREF = temp;

        temp = (ctx->Stencil.Function[0] - GL_NEVER) << 16;

        switch (ctx->Stencil.FailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSF_DECR;
            break;
        }

        switch (ctx->Stencil.ZFailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZF_DECR;
            break;
        }

        switch (ctx->Stencil.ZPassFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZP_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZP_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZP_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZP_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZP_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZP_DECR;
            break;
        }
        vmesa->regHSTMD = temp;

        vmesa->dirty |= (VIA_UPLOAD_STENCIL | VIA_UPLOAD_STENCIL);
    }
    else {
        vmesa->regEnable &= ~HC_HenST_MASK;
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaChoosePoint(GLcontext *ctx) 
{
    ctx = ctx;
}

void viaChooseLine(GLcontext *ctx) 
{
    ctx = ctx;
}

void viaChooseTriangle(GLcontext *ctx) 
{       
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) {
       fprintf(stderr, "%s - in\n", __FUNCTION__);        
       fprintf(stderr, "GL_CULL_FACE = %x\n", GL_CULL_FACE);    
       fprintf(stderr, "ctx->Polygon.CullFlag = %x\n", ctx->Polygon.CullFlag);       
       fprintf(stderr, "GL_FRONT = %x\n", GL_FRONT);    
       fprintf(stderr, "ctx->Polygon.CullFaceMode = %x\n", ctx->Polygon.CullFaceMode);    
       fprintf(stderr, "GL_CCW = %x\n", GL_CCW);    
       fprintf(stderr, "ctx->Polygon.FrontFace = %x\n", ctx->Polygon.FrontFace);    
    }
    if (ctx->Polygon.CullFlag == GL_TRUE) {
        switch (ctx->Polygon.CullFaceMode) {
        case GL_FRONT:
            if (ctx->Polygon.FrontFace == GL_CCW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_BACK:
            if (ctx->Polygon.FrontFace == GL_CW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_FRONT_AND_BACK:
            return;
        }
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static void viaChooseState(GLcontext *ctx, GLuint newState)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (VIA_DEBUG) fprintf(stderr, "newState = %x\n", newState);        

    if (!(newState & (_NEW_COLOR |
                      _NEW_TEXTURE |
                      _NEW_DEPTH |
                      _NEW_FOG |
                      _NEW_LIGHT |
                      _NEW_LINE |
                      _NEW_POLYGON |
                      _NEW_POLYGONSTIPPLE |
                      _NEW_STENCIL)))
        return;

    vmesa->dirty = 0;
    vmesa->newState = newState;

    if (texUnit0->_ReallyEnabled || texUnit1->_ReallyEnabled || ctx->Fog.Enabled) {
	vmesa->regCmdB |= HC_HVPMSK_Cs;
    }
    else {
    	vmesa->regCmdB &= ~ HC_HVPMSK_Cs;
    }
    
    if (newState & _NEW_TEXTURE)
        viaChooseTextureState(ctx);

    if (newState & _NEW_COLOR)
        viaChooseColorState(ctx);

    if (newState & _NEW_DEPTH)
        viaChooseDepthState(ctx);

    if (newState & _NEW_FOG)
        viaChooseFogState(ctx);

    if (newState & _NEW_LIGHT)
        viaChooseLightState(ctx);

    if (newState & _NEW_LINE)
        viaChooseLineState(ctx);

    if (newState & (_NEW_POLYGON | _NEW_POLYGONSTIPPLE))
        viaChoosePolygonState(ctx);

    if (newState & _NEW_STENCIL)
        viaChooseStencilState(ctx);
    
    viaChooseTriangle(ctx);
            
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);        
}

static void viaInvalidateState(GLcontext *ctx, GLuint newState)
{
    _swrast_InvalidateState(ctx, newState);
    _swsetup_InvalidateState(ctx, newState);
    _ac_InvalidateState(ctx, newState);
    _tnl_InvalidateState(ctx, newState);
    viaChooseState(ctx, newState);
}

void viaInitStateFuncs(GLcontext *ctx)
{
    /* Callbacks for internal Mesa events.
     */
    ctx->Driver.UpdateState = viaInvalidateState;

    /* API callbacks
     */
    ctx->Driver.AlphaFunc = viaAlphaFunc;
    ctx->Driver.BlendEquationSeparate = viaBlendEquationSeparate;
    ctx->Driver.BlendFuncSeparate = viaBlendFuncSeparate;
    ctx->Driver.ClearColor = viaClearColor;
    ctx->Driver.ColorMask = viaColorMask;
    ctx->Driver.CullFace = viaCullFaceFrontFace;
    ctx->Driver.DepthFunc = viaDepthFunc;
    ctx->Driver.DepthMask = viaDepthMask;
    ctx->Driver.DrawBuffer = viaDrawBuffer;
    ctx->Driver.Enable = viaEnable;
    ctx->Driver.Fogfv = viaFogfv;
    ctx->Driver.FrontFace = viaCullFaceFrontFace;
    ctx->Driver.LineWidth = viaLineWidth;
    ctx->Driver.LogicOpcode = viaLogicOp;
    ctx->Driver.PolygonStipple = viaPolygonStipple;
    ctx->Driver.RenderMode = viaRenderMode;
    ctx->Driver.Scissor = viaScissor;
    ctx->Driver.ShadeModel = viaShadeModel;
    ctx->Driver.DepthRange = viaDepthRange;
    ctx->Driver.Viewport = viaViewport;
    ctx->Driver.PointSize = viaPointSize;
    ctx->Driver.LightModelfv = viaLightModelfv;

    /* Pixel path fallbacks.
     */
    ctx->Driver.Accum = _swrast_Accum;
    ctx->Driver.Bitmap = viaBitmap;
    
    ctx->Driver.CopyPixels = _swrast_CopyPixels;
    ctx->Driver.DrawPixels = _swrast_DrawPixels;
    ctx->Driver.ReadPixels = _swrast_ReadPixels;
    ctx->Driver.ResizeBuffers = viaReAllocateBuffers;

    /* Swrast hooks for imaging extensions:
     */
    ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
    ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
    ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
    ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
