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
#ifdef DEBUG
        if (VIA_DEBUG) fprintf(stderr, "unknown format %d\n", (int)format);
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif

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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
#endif
}

static void viaBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLboolean fallback = GL_FALSE;
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
#endif
    if (ctx->Scissor.Enabled) {
        VIA_FIREVERTICES(vmesa); /* don't pipeline cliprect changes */
        vmesa->uploadCliprects = GL_TRUE;
    }

    vmesa->scissorRect.x1 = x;
    vmesa->scissorRect.y1 = vmesa->driDrawable->h - (y + h);
    vmesa->scissorRect.x2 = x + w;
    vmesa->scissorRect.y2 = vmesa->driDrawable->h - y;
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
}


static void viaLogicOp(GLcontext *ctx, GLenum opcode)
{
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (VIA_DEBUG) fprintf(stderr, "opcode = %x\n", opcode);
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
#endif
}

/* Fallback to swrast for select and feedback.
 */
static void viaRenderMode(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif
    FALLBACK(vmesa, VIA_FALLBACK_RENDERMODE, (mode != GL_RENDER));
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
#endif
}


static void viaDrawBuffer(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
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
	if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
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


static void viaCalcViewport(GLcontext *ctx)
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
    viaCalcViewport(ctx);
}

static void viaDepthRange(GLcontext *ctx,
                          GLclampd nearval, GLclampd farval)
{
    viaCalcViewport(ctx);
}

void viaPrintDirty(const char *msg, GLuint state)
{
#ifdef DEBUG
    if (VIA_DEBUG)
	fprintf(stderr, "%s (0x%x): %s%s%s%s\n",
            msg,
            (unsigned int) state,
            (state & VIA_UPLOAD_TEX0)   ? "upload-tex0, " : "",
            (state & VIA_UPLOAD_TEX1)   ? "upload-tex1, " : "",
            (state & VIA_UPLOAD_CTX)    ? "upload-ctx, " : "",
            (state & VIA_UPLOAD_BUFFERS)    ? "upload-bufs, " : ""
            );
#endif
}


void viaInitState(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    vmesa->regCmdA = HC_ACMD_HCmdA;
    vmesa->regCmdB = HC_ACMD_HCmdB | HC_HVPMSK_X | HC_HVPMSK_Y | HC_HVPMSK_Z;
    vmesa->regEnable = HC_HenCW_MASK;

    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
        vmesa->drawMap = vmesa->back.map;
        vmesa->readMap = vmesa->back.map;
    }
    else {
        vmesa->drawMap = (char *)vmesa->driScreen->pFB;
        vmesa->readMap = (char *)vmesa->driScreen->pFB;
    }
}

void viaChooseTextureState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];
    /*=* John Sheng [2003.7.18] texture combine *=*/
    GLboolean AlphaCombine[3];
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#endif    
    if (texUnit0->_ReallyEnabled || texUnit1->_ReallyEnabled) {
#ifdef DEBUG
	if (VIA_DEBUG) {
	    fprintf(stderr, "Texture._ReallyEnabled - in\n");    
	    fprintf(stderr, "texUnit0->_ReallyEnabled = %x\n",texUnit0->_ReallyEnabled);
	}
#endif

        if (texUnit0->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit0->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];
            GLint r, g, b, a;
#ifdef DEBUG
	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->_ReallyEnabled\n");    
#endif            
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

            if (texObj->WrapS == GL_REPEAT)
                vmesa->regHTXnMPMD_0 = HC_HTXnMPMD_Srepeat;
            else
                vmesa->regHTXnMPMD_0 = HC_HTXnMPMD_Sclamp;

            if (GL_TRUE) {
                if (texObj->WrapT == GL_REPEAT)
                    vmesa->regHTXnMPMD_0 |= HC_HTXnMPMD_Trepeat;
                else
                    vmesa->regHTXnMPMD_0 |= HC_HTXnMPMD_Tclamp;
            }
#ifdef DEBUG
	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode %x\n",texUnit0->EnvMode);    
#endif
            switch (texUnit0->EnvMode) {
            case GL_MODULATE:
		switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = At*Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Adif | HC_HTXnTBLAb_TOPA |
                        HC_HTXnTBLAb_Atex | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    break;
                case GL_LUMINANCE:
                    /* C = Lt*Cf, A = Af
                     * RGB part.
                     * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    /* C = Lt*Cf, A = At*Af
                     * RGB part.
                     * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    break;
                case GL_INTENSITY:
                    /* C = It*Cf, A = It*Af
                     * RGB part.
                     * Ca = It, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = It, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    break;
                case GL_RGB:
                    /* C = Ct*Cf, A = Af
                     * RGB part.
                     * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_MODULATE: GL_RGB\n");    
#endif
                    break;
                case GL_RGBA:
                    /* C = Ct*Cf, A = At*Af
                     * RGB part.
                     * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
			HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias
			| HC_HTXnTBLAshift_No;
                    
		    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_MODULATE: GL_RGBA\n");        
#endif
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = At*Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex | HC_HTXnTBLAb_TOPA |
                            HC_HTXnTBLAb_Adif | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        /* C = Lt*Cf, A = Af
                         * RGB part.
                         * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        /* C = Lt*Cf, A = At*Af
                         * RGB part.
                         * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    case GL_INTENSITY:
                        /* C = It*Cf, A = It*Af
                         * RGB part.
                         * Ca = It, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = It, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    case GL_RGB:
                        /* C = Ct*Cf, A = Af
                         * RGB part.
                         * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_RGBA:
                        /* C = Ct*Cf, A = At*Af
                         * RGB part.
                         * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Dif | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    }
                    break;
                }
                break;
            case GL_DECAL:
		switch (texImage->Format) {
                case GL_ALPHA:
                case GL_LUMINANCE:
                case GL_LUMINANCE_ALPHA:
                case GL_INTENSITY:
                    /* Undefined.
                     */
                    break;
                case GL_RGB:
                    /* C = Ct, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_DECAL: GL_RGB\n");
#endif
                    break;
                case GL_RGBA:
                    /* C = (1-At)*Cf+At*Ct, A = Af --> At*(Ct-Cf)+Cf
                     * RGB part.
                     * Ca = At, Cb = Ct, Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Atex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_Tex |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_DECAL: GL_RGBA\n");
#endif
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                    case GL_LUMINANCE:
                    case GL_LUMINANCE_ALPHA:
                    case GL_INTENSITY:
                        /* Undefined.
                         */
                        break;
                    case GL_RGB:
                        /* C = Ct, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
			if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_COLOR_INDEX: GL_RGB\n");    
#endif
                        break;
                    case GL_RGBA:
                        /* C = (1-At)*Cf+At*Ct, A = Af --> At*(Ct-Cf)+Cf
                         * RGB part.
                         * Ca = At, Cb = Ct, Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Atex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_Tex |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
			if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_COLOR_INDEX: GL_RGBA\n");    
#endif
                        break;
                    }
                    break;
                }
                break;
            case GL_BLEND:
		switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_LUMINANCE:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    /* C = (1-Lt)*Cf+Lt*Cc, A = Af --> Lt*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    /* C = (1-Lt)*Cf+Lt*Cc, A = At*Af --> Lt*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    break;
                case GL_INTENSITY:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                    /* C = (1-It)*Cf+It*Cc, A = (1-It)*Af+It*Ac
                     * --> It*(Cc-Cf)+Cf, It*(Ac-Af)+Af
                     * RGB part.
                     * Ca = It, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = It, Ab = Ac(Reg), Cop = -, Ac = Af, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = (a << 8);
                    break;
                case GL_RGB:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                    /* C = (1-Ct)*Cf+Ct*Cc, A = Af --> Ct*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_BLEND: GL_RGB\n");    
#endif
                    break;
                case GL_RGBA:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                    /* C = (1-Ct)*Cf+Ct*Cc, A = At*Af --> Ct*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_BLEND: GL_RGBA\n");    
#endif
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        /* C = (1-Lt)*Cf+Lt*Cc, A = Af --> Lt*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        /* C = (1-Lt)*Cf+Lt*Cc, A = At*Af --> Lt*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    case GL_INTENSITY:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        /* C = (1-It)*Cf+It*Cc, A = (1-It)*Af+It*Ac
                         * --> It*(Cc-Cf)+Cf, It*(Ac-Af)+Af
                         * RGB part.
                         * Ca = It, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = It, Ab = Ac(Reg), Cop = -, Ac = Af, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = (a << 8);
                        break;
                    case GL_RGB:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        /* C = (1-Ct)*Cf+Ct*Cc, A = Af --> Ct*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_RGBA:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        /* C = (1-Ct)*Cf+Ct*Cc, A = At*Af --> Ct*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        vmesa->regHTXnTBLRFog_0 = 0x0;
                        break;
                    }
                    break;
                }
                break;
            case GL_REPLACE:
		switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_LUMINANCE:
                    /* C = Lt, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    /* C = Lt, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_INTENSITY:
                    /* C = It, A = It
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = It, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = It, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    break;
                case GL_RGB:
                    /* C = Ct, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_REPLACE: GL_RGB\n");    
#endif
                    break;
                case GL_RGBA:
                    /* C = Ct, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
#ifdef DEBUG
		    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode: GL_REPLACE: GL_RGBA\n");    
#endif
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        /* C = Lt, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        /* C = Lt, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_INTENSITY:
                        /* C = It, A = It
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = It, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = It, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_RGB:
                        /* C = Ct, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    case GL_RGBA:
                        /* C = Ct, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_0 = 0x0;
                        break;
                    }
                    break;
                }
                break;
	    /*=* John Sheng [2003.7.18] texture combine *=*/
	    case GL_COMBINE:
		switch (texUnit0->Combine.ModeRGB) {
        	case GL_REPLACE:
            	    switch (texUnit0->Combine.SourceRGB[0]) {
            	    case GL_TEXTURE:
                	switch (texUnit0->Combine.OperandRGB[0]) {
                	case GL_SRC_COLOR:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_ONE_MINUS_SRC_COLOR:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Tex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_SRC_ALPHA:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Atex; 
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_ONE_MINUS_SRC_ALPHA:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Atex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	}
                	break;
            	    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                	switch (texUnit0->Combine.OperandRGB[0]) {
                	case GL_SRC_COLOR:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_HTXnTBLRC; 
                        
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    	    break;
                	case GL_ONE_MINUS_SRC_COLOR:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_HTXnTBLRC;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    	    break;
                	case GL_SRC_ALPHA:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
	                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
    	                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_HTXnTBLRC;
        	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
            	            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                	    break;
                        case GL_ONE_MINUS_SRC_ALPHA:
	                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
    	                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_HTXnTBLRC;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
	                    vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                            break;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
	                switch (texUnit0->Combine.OperandRGB[0]) {
                        case GL_SRC_COLOR:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_COLOR:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        }
                        break;
                    case GL_PREVIOUS :
                        switch (texUnit0->Combine.OperandRGB[0]) {
                        case GL_SRC_COLOR:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_COLOR:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftRGB)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
    	                break;
                    case 2:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;

                case GL_MODULATE:
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    switch (texUnit0->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit0->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC;
                        AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit0->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                        }
                        else {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Atex;
                        }
                        break;
                    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCa_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
	                }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
                        }
                        break;
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
	                }
    	                break;
                    }
	            switch (texUnit0->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
	                if (AlphaCombine[1]==0) {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                	}
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
                        break;
                    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                	else {
                    	    vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                	}
                	break;
                    case GL_PRIMARY_COLOR :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
	                }
    	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
	                break;
                    case GL_PREVIOUS :
	                if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
	                }
    	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
	                }
    	                break;
        	    }
            	    switch ((GLint)(texUnit0->Combine.ScaleShiftRGB)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
    	                break;
        	    case 2:
            	        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                	break;
                    case 4:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
    	                break;
        	    }
            	    break;
	        case GL_ADD:
                case GL_SUBTRACT :
	            if (texUnit0->Combine.ModeRGB==GL_ADD) {
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
                    }
	            else {
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
	            }
    	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
        	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK | HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC;
                    vmesa->regHTXnTBLRCa_0 = ( 255<<16 | 255<<8 |255 );
                    switch (texUnit0->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
    	                AlphaCombine[0]=0;
                        break;
	            case GL_ONE_MINUS_SRC_COLOR:
    	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit0->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_TOPC; 
    	                AlphaCombine[1]=0;
        	        break;
        	    case GL_ONE_MINUS_SRC_COLOR:
            	        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_InvTOPC;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_TOPC;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_InvTOPC;
                        AlphaCombine[1]=1;
                        break;
	            }
                    switch (texUnit0->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
	                }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
	                }
    	                break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
	                }
    	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
	                }
    	                break;
                    }
                    switch (texUnit0->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_HTXnTBLRC;
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCc_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCc_0 = (a << 16) | (a << 8) | a;
	                }
    	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Adif;
                        }
	                break;
    	            }
        	    switch ((GLint)(texUnit0->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;
                case GL_ADD_SIGNED :
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub;
	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
    	            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
        	        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC|
            	        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_HTXnTBLRC;
            	    vmesa->regHTXnTBLRCa_0 = ( 255<<16 | 255<<8 |255 );
                    vmesa->regHTXnTBLRCc_0 = ( 128<<16 | 128<<8 |128 );
                    switch (texUnit0->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit0->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
    	                AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit0->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
                        break;
                    }
            	    switch (texUnit0->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Atex; 
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_HTXnTBLRC; 
                	if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCbias_0 = (r << 16) | (g << 8) | b;
                        }
	                else {
                            vmesa->regHTXnTBLRCbias_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Dif; 
                        }
                	else {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Adif;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
	            break;
                case GL_INTERPOLATE :
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub;
	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
    	            switch (texUnit0->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_TOPC; 
    	                AlphaCombine[0]=0;
        	        break;
            	    case GL_ONE_MINUS_SRC_COLOR:
                	vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit0->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_TOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_InvTOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_TOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=1;
	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_InvTOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit0->Combine.OperandRGB[2]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[2]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_InvTOPC;
                        AlphaCombine[2]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_TOPC;
                        AlphaCombine[2]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_InvTOPC;
                        AlphaCombine[2]=1;
                        break;
                    }
                    switch (texUnit0->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
	                if (AlphaCombine[0]==0) {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                        }
                        else {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
	                break;
    	            case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
        	        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
                        break;
                    }
                    switch (texUnit0->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Tex; 
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Tex;
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Atex;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_HTXnTBLRC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCc_0 = (r << 16) | (g << 8) | b;
                            vmesa->regHTXnTBLRCbias_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCc_0 = (a << 16) | (a << 8) | a;
                            vmesa->regHTXnTBLRCbias_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Dif; 
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Dif;
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Adif;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Adif;
                        }
                        break;
                    }
                    switch (texUnit0->Combine.SourceRGB[2]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                        }
	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCa_0 = (a << 16) | (a << 8) | a;
                        }
	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;
                }
	        switch (texUnit0->Combine.ModeA) {
                case GL_REPLACE:
                    switch (texUnit0->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        switch (texUnit0->Combine.OperandA[0]) {
                	case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_Atex;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        }
                        break;
                    case GL_CONSTANT :
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        switch (texUnit0->Combine.OperandA[0]) {
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            vmesa->regHTXnTBLRFog_0 = a;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_HTXnTBLRAbias;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            vmesa->regHTXnTBLRFog_0 = a;
                            break;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        switch (texUnit0->Combine.OperandA[0]) {
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
	                    HC_HTXnTBLAbias_Adif;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_Adif;
	                    vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
    	                }
        	    break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftA)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_MODULATE:
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                     HC_HTXnTBLAbias_HTXnTBLRAbias;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
	            vmesa->regHTXnTBLRAa_0= 0x0;
                    switch (texUnit0->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_TOPA; 
    	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_InvTOPA; 
                        break;
                    }
                    switch (texUnit0->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Atex; 
                        break;
                    case GL_CONSTANT :
	                CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a<<16;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Adif; 
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                	CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a<<8;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftA)) {
	            case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_ADD:
                case GL_SUBTRACT :
                    if(texUnit0->Combine.ModeA==GL_ADD) {
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add | HC_HTXnTBLAbias_HTXnTBLRAbias;
	            }
    	            else {
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub | HC_HTXnTBLAbias_HTXnTBLRAbias;
                    }
                    vmesa->regHTXnTBLRFog_0 = 0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA;
                    vmesa->regHTXnTBLRAa_0 = 0x0 |  ( 255<<16 );
                    switch (texUnit0->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
    	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
	            switch (texUnit0->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                	vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_TOPA;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_InvTOPA;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftA)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_ADD_SIGNED :
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA|
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLRAa_0 = ( 255<<16 | 0<<8 |128 );
                    switch (texUnit0->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit0->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Inv;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[1]) {
            	    case GL_TEXTURE:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Atex; 
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_HTXnTBLRAbias; 
                        vmesa->regHTXnTBLRFog_0 |= a;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit0->Combine.ScaleShiftA)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
        	case GL_INTERPOLATE :
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 =  0x0;
                    switch (texUnit0->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit0->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAc_TOPA;
                        break;
	            case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAc_InvTOPA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Inv;
                        break;
                    }
                    switch (texUnit0->Combine.OperandA[2]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAa_TOPA;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAa_InvTOPA;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit0->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Atex;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_HTXnTBLRA;
	                vmesa->regHTXnTBLRAa_0 |= a;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_HTXnTBLRAbias;
                        vmesa->regHTXnTBLRFog_0 |= a;
                        break;
	            case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Adif;
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Adif;
                        break;
	            }
                    switch (texUnit0->Combine.SourceA[2]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 16);
                        break;
                    case GL_PRIMARY_COLOR :
	            case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Adif;
                        break;
	            }
                    switch (texUnit0->Combine.ScaleShiftA) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
            	    break;
            	case GL_DOT3_RGB :
            	case GL_DOT3_RGBA :
            	    break;
    	    	}
		break;
	/*=* John Sheng [2003.7.18] texture add *=*/
        case GL_ADD:
            switch(texImage->Format) {
            case GL_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Adif | HC_HTXnTBLAb_TOPA |
                    HC_HTXnTBLAb_Atex | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_LUMINANCE:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
            case GL_LUMINANCE_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_INTENSITY:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Atex |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
		/*=* John Sheng [2003.7.18] texenv *=*/
		/*vmesa->regHTXnTBLRAa_0 = 0x0;*/
                vmesa->regHTXnTBLRAa_0 = (255<<16) | (255<<8) | 255;;
                vmesa->regHTXnTBLRFog_0 = 0x0 | 255<<16;
                break;
            case GL_RGB:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
            case GL_RGBA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_COLOR_INDEX:
                switch(texObj->Palette.Format) {
                case GL_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Adif | HC_HTXnTBLAb_TOPA |
                    HC_HTXnTBLAb_Atex | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                case GL_LUMINANCE:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
                case GL_LUMINANCE_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                case GL_INTENSITY:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Atex |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0 | 255<<16;
                break;
                case GL_RGB:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
                case GL_RGBA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                }
                break;
            }
            break;
	/*=* John Sheng [2003.7.18] texture dot3 *=*/
	    case GL_DOT3_RGB :
    	    case GL_DOT3_RGBA :
            	vmesa->regHTXnTBLCop_0 = HC_HTXnTBLDOT4 | HC_HTXnTBLCop_Add | 
        		                 HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 | 
                                         HC_HTXnTBLCshift_2 | HC_HTXnTBLAop_Add |
                                         HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                                       HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | 
                                       HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                                       HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                                       HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                switch (texUnit0->Combine.OperandRGB[0]) {
                case GL_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
	            break;
                case GL_ONE_MINUS_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                    break;
                }
                switch (texUnit0->Combine.OperandRGB[1]) {
                case GL_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                    break;
                case GL_ONE_MINUS_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                    break;
                }
                switch (texUnit0->Combine.SourceRGB[0]) {
                case GL_TEXTURE:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                    break;
                case GL_CONSTANT :
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                    vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                    break;
	        case GL_PRIMARY_COLOR :
                case GL_PREVIOUS :
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                    break;
                }
                switch (texUnit0->Combine.SourceRGB[1]) {
                case GL_TEXTURE:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                    break;
                case GL_CONSTANT :
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit0->EnvColor[2], b);
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    break;
                case GL_PRIMARY_COLOR :
                case GL_PREVIOUS :
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                    break;
                }
	        break;
            default:
		break;
            }
        }
	else {
	/* Should turn Cs off if actually no Cs */
	}

        if (texUnit1->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit1->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];
            GLint r, g, b, a;

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
	    
            if (texObj->WrapS == GL_REPEAT)
                vmesa->regHTXnMPMD_1 = HC_HTXnMPMD_Srepeat;
            else
                vmesa->regHTXnMPMD_1 = HC_HTXnMPMD_Sclamp;

            if (GL_TRUE) {
                if (texObj->WrapT == GL_REPEAT)
                    vmesa->regHTXnMPMD_1 |= HC_HTXnMPMD_Trepeat;
                else
                    vmesa->regHTXnMPMD_1 |= HC_HTXnMPMD_Tclamp;
            }

            switch (texUnit1->EnvMode) {
            case GL_MODULATE:
		switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = At*Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex | HC_HTXnTBLAb_TOPA |
                        HC_HTXnTBLAb_Acur | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_LUMINANCE:
                    /* C = Lt*Cf, A = Af
                     * RGB part.
                     * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    /* C = Lt*Cf, A = At*Af
                     * RGB part.
                     * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_INTENSITY:
                    /* C = It*Cf, A = It*Af
                     * RGB part.
                     * Ca = It, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = It, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_RGB:
                    /* C = Ct*Cf, A = Af
                     * RGB part.
                     * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_RGBA:
                    /* C = Ct*Cf, A = At*Af
                     * RGB part.
                     * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                        HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
			HC_HTXnTBLAbias_HTXnTBLRAbias
			| HC_HTXnTBLAshift_No;
                    
		    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = At*Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex | HC_HTXnTBLAb_TOPA |
                            HC_HTXnTBLAb_Acur | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        /* C = Lt*Cf, A = Af
                         * RGB part.
                         * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        /* C = Lt*Cf, A = At*Af
                         * RGB part.
                         * Ca = Lt, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    case GL_INTENSITY:
                        /* C = It*Cf, A = It*Af
                         * RGB part.
                         * Ca = It, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = It, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    case GL_RGB:
                        /* C = Ct*Cf, A = Af
                         * RGB part.
                         * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_RGBA:
                        /* C = Ct*Cf, A = At*Af
                         * RGB part.
                         * Ca = Ct, Cb = Cf, Cop = +, Cc = 0, Cbias = 0, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex | HC_HTXnTBLCb_TOPC |
                            HC_HTXnTBLCb_Cur | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    }
                    break;
                }
                break;
            case GL_DECAL:
                switch (texImage->Format) {
                case GL_ALPHA:
                case GL_LUMINANCE:
                case GL_LUMINANCE_ALPHA:
                case GL_INTENSITY:
                    /* Undefined.
                     */
                    break;
                case GL_RGB:
                    /* C = Ct, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_RGBA:
                    /* C = (1-At)*Cf+At*Ct, A = Af --> At*(Ct-Cf)+Cf
                     * RGB part.
                     * Ca = At, Cb = Ct, Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Atex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_Tex |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                    case GL_LUMINANCE:
                    case GL_LUMINANCE_ALPHA:
                    case GL_INTENSITY:
                        /* Undefined.
                         */
                        break;
                    case GL_RGB:
                        /* C = Ct, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_RGBA:
                        /* C = (1-At)*Cf+At*Ct, A = Af --> At*(Ct-Cf)+Cf
                         * RGB part.
                         * Ca = At, Cb = Ct, Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Atex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_Tex |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    }
                    break;
                }
                break;
            case GL_BLEND:
                switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_LUMINANCE:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    /* C = (1-Lt)*Cf+Lt*Cc, A = Af --> Lt*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    /* C = (1-Lt)*Cf+Lt*Cc, A = At*Af --> Lt*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_INTENSITY:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                    /* C = (1-It)*Cf+It*Cc, A = (1-It)*Af+It*Ac
                     * --> It*(Cc-Cf)+Cf, It*(Ac-Af)+Af
                     * RGB part.
                     * Ca = It, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = It, Ab = Ac(Reg), Cop = -, Ac = Af, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Acur;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Sub |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = (a << 8);
                    break;
                case GL_RGB:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                    /* C = (1-Ct)*Cf+Ct*Cc, A = Af --> Ct*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_RGBA:
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                    /* C = (1-Ct)*Cf+Ct*Cc, A = At*Af --> Ct*(Cc-Cf)+Cf
                     * RGB part.
                     * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                    /* Alpha part.
                     * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    vmesa->regHTXnTBLRFog_1 = 0x0;
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        /* C = (1-Lt)*Cf+Lt*Cc, A = Af --> Lt*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        /* C = (1-Lt)*Cf+Lt*Cc, A = At*Af --> Lt*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Lt, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    case GL_INTENSITY:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        /* C = (1-It)*Cf+It*Cc, A = (1-It)*Af+It*Ac
                         * --> It*(Cc-Cf)+Cf, It*(Ac-Af)+Af
                         * RGB part.
                         * Ca = It, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = It, Ab = Ac(Reg), Cop = -, Ac = Af, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Acur;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Sub |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = (a << 8);
                        break;
                    case GL_RGB:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        /* C = (1-Ct)*Cf+Ct*Cc, A = Af --> Ct*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_RGBA:
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        /* C = (1-Ct)*Cf+Ct*Cc, A = At*Af --> Ct*(Cc-Cf)+Cf
                         * RGB part.
                         * Ca = Ct, Cb = Cc(Reg), Cop = -, Cc = Cf, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_Tex |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_HTXnTBLRC |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Cur;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Sub |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        vmesa->regHTXnTBLRCb_1 = (r << 16) | (g << 8) | b;
                        /* Alpha part.
                         * Aa = At, Ab = Af, Cop = +, Ac = 0, Abias = 0, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Acur |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        vmesa->regHTXnTBLRFog_1 = 0x0;
                        break;
                    }
                    break;
                }
                break;
            case GL_REPLACE:
                switch (texImage->Format) {
                case GL_ALPHA:
                    /* C = Cf, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_LUMINANCE:
                    /* C = Lt, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_LUMINANCE_ALPHA:
                    /* C = Lt, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_INTENSITY:
                    /* C = It, A = It
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = It, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = It, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_RGB:
                    /* C = Ct, A = Af
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_RGBA:
                    /* C = Ct, A = At
                     * RGB part.
                     * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                     */
                    vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                        HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                        HC_HTXnTBLCshift_No;
                    vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                    /* Alpha part.
                     * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                     */
                    vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                        HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                        HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLRAa_1 = 0x0;
                    break;
                case GL_COLOR_INDEX:
                    switch (texObj->Palette.Format) {
                    case GL_ALPHA:
                        /* C = Cf, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Cf, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Cur |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_LUMINANCE:
                        /* C = Lt, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_LUMINANCE_ALPHA:
                        /* C = Lt, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Lt, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_INTENSITY:
                        /* C = It, A = It
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = It, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = It, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_RGB:
                        /* C = Ct, A = Af
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = Af, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Acur | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    case GL_RGBA:
                        /* C = Ct, A = At
                         * RGB part.
                         * Ca = 0, Cb = 0, Cop = +, Cc = 0, Cbias = Ct, Cshift = No.
                         */
                        vmesa->regHTXnTBLCsat_1 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                        vmesa->regHTXnTBLCop_1 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex |
                            HC_HTXnTBLCshift_No;
                        vmesa->regHTXnTBLMPfog_1 = HC_HTXnTBLMPfog_Fog;
                        /* Alpha part.
                         * Aa = 0, Ab = 0, Cop = +, Ac = 0, Abias = At, Ashift = No.
                         */
                        vmesa->regHTXnTBLAsat_1 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLCop_1 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex | HC_HTXnTBLAshift_No;
                        vmesa->regHTXnTBLRAa_1 = 0x0;
                        break;
                    }
                    break;
                }
                break;
	    /*=* John Sheng [2003.7.18] texture combine *=*/
	    case GL_COMBINE:
		switch (texUnit1->Combine.ModeRGB) {
        	case GL_REPLACE:
            	    switch (texUnit1->Combine.SourceRGB[0]) {
            	    case GL_TEXTURE:
                	switch (texUnit1->Combine.OperandRGB[0]) {
                	case GL_SRC_COLOR:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Tex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_ONE_MINUS_SRC_COLOR:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Tex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_SRC_ALPHA:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Atex; 
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	case GL_ONE_MINUS_SRC_ALPHA:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Atex;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    break;
                	}
                	break;
            	    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                	switch (texUnit1->Combine.OperandRGB[0]) {
                	case GL_SRC_COLOR:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_HTXnTBLRC; 
                        
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    	    break;
                	case GL_ONE_MINUS_SRC_COLOR:
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                    	    HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    	    HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_HTXnTBLRC;
                    	    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    	    break;
                	case GL_SRC_ALPHA:  
                    	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    	    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                    	    HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
	                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
    	                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_HTXnTBLRC;
        	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
            	            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                	    break;
                        case GL_ONE_MINUS_SRC_ALPHA:
	                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
    	                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_HTXnTBLRC;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
	                    vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                            break;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
	                switch (texUnit1->Combine.OperandRGB[0]) {
                        case GL_SRC_COLOR:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_COLOR:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        }
                        break;
                    case GL_PREVIOUS :
                        switch (texUnit1->Combine.OperandRGB[0]) {
                        case GL_SRC_COLOR:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_COLOR:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Dif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                            HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 |
                            HC_HTXnTBLCb_TOPC | HC_HTXnTBLCb_0 |
                            HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                            vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                            HC_HTXnTBLCbias_InvCbias | HC_HTXnTBLCbias_Adif;
                            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                            break;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftRGB)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
    	                break;
                    case 2:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;

                case GL_MODULATE:
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    switch (texUnit1->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC;
                        AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                        }
                        else {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Atex;
                        }
                        break;
                    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCa_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
	                }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
                        }
                        break;
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
	                }
    	                break;
                    }
	            switch (texUnit1->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
	                if (AlphaCombine[1]==0) {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                	}
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
                        break;
                    case GL_CONSTANT :
			CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                	else {
                    	    vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                	}
                	break;
                    case GL_PRIMARY_COLOR :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
	                }
    	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
	                break;
                    case GL_PREVIOUS :
	                if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
	                }
    	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
	                }
    	                break;
        	    }
            	    switch ((GLint)(texUnit1->Combine.ScaleShiftRGB)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
    	                break;
        	    case 2:
            	        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                	break;
                    case 4:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
    	                break;
        	    }
            	    break;
	        case GL_ADD:
                case GL_SUBTRACT :
	            if (texUnit1->Combine.ModeRGB==GL_ADD) {
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
                    }
	            else {
                        vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub |
                        HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0;
	            }
    	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
        	    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK | HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC;
                    vmesa->regHTXnTBLRCa_0 = ( 255<<16 | 255<<8 |255 );
                    switch (texUnit1->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
    	                AlphaCombine[0]=0;
                        break;
	            case GL_ONE_MINUS_SRC_COLOR:
    	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_TOPC; 
    	                AlphaCombine[1]=0;
        	        break;
        	    case GL_ONE_MINUS_SRC_COLOR:
            	        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_InvTOPC;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_TOPC;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_InvTOPC;
                        AlphaCombine[1]=1;
                        break;
	            }
                    switch (texUnit1->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
	                }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
	                }
    	                break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                    	    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
	                }
    	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
	                }
    	                break;
                    }
                    switch (texUnit1->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_HTXnTBLRC;
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCc_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCc_0 = (a << 16) | (a << 8) | a;
	                }
    	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Adif;
                        }
	                break;
    	            }
        	    switch ((GLint)(texUnit1->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;
                case GL_ADD_SIGNED :
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub;
	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
    	            vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
        	        HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC|
            	        HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_HTXnTBLRC;
            	    vmesa->regHTXnTBLRCa_0 = ( 255<<16 | 255<<8 |255 );
                    vmesa->regHTXnTBLRCc_0 = ( 128<<16 | 128<<8 |128 );
                    switch (texUnit1->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
    	                AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
                        break;
                    }
            	    switch (texUnit1->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Tex; 
                        }
                        else {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Atex; 
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_HTXnTBLRC; 
                	if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLRCbias_0 = (r << 16) | (g << 8) | b;
                        }
	                else {
                            vmesa->regHTXnTBLRCbias_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[1]==0) {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Dif; 
                        }
                	else {
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Adif;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
	            break;
                case GL_INTERPOLATE :
                    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Sub;
	            vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
    	            switch (texUnit1->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
	                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_TOPC; 
    	                AlphaCombine[0]=0;
        	        break;
            	    case GL_ONE_MINUS_SRC_COLOR:
                	vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_TOPC; 
                        AlphaCombine[0]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCb_InvTOPC; 
                        AlphaCombine[0]=1;
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_TOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_InvTOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=0;
                        break;
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_TOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Cbias;
                        AlphaCombine[1]=1;
	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCc_InvTOPC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_InvCbias;
                        AlphaCombine[1]=1;
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[2]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_TOPC; 
                        AlphaCombine[2]=0;
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_InvTOPC;
                        AlphaCombine[2]=0;
                        break;
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_TOPC;
                        AlphaCombine[2]=1;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCa_InvTOPC;
                        AlphaCombine[2]=1;
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
	                if (AlphaCombine[0]==0) {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                        }
                        else {
                    	    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Atex;
                        }
	                break;
    	            case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCb_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        }
        	        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Adif;
                        }
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Tex; 
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Tex;
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Atex;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_HTXnTBLRC;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCc_0 = (r << 16) | (g << 8) | b;
                            vmesa->regHTXnTBLRCbias_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCc_0 = (a << 16) | (a << 8) | a;
                            vmesa->regHTXnTBLRCbias_0 = (a << 16) | (a << 8) | a;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Dif; 
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Dif;
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCc_Adif;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCbias_Adif;
                        }
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[2]) {
                    case GL_TEXTURE:
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                        }
	                else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Atex;
                        }
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                        }
                        else {
                            vmesa->regHTXnTBLRCa_0 = (a << 16) | (a << 8) | a;
                        }
	                break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        if (AlphaCombine[0]==0) {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                        }
                        else {
                            vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Adif;
                        }
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftRGB)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLCshift_2;
                        break;
                    }
                    break;

        	case GL_DOT3_RGB :
        	case GL_DOT3_RGBA :
            	    vmesa->regHTXnTBLCop_0 = HC_HTXnTBLDOT4 | HC_HTXnTBLCop_Add | 
                	                  HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 | 
                                          HC_HTXnTBLCshift_2 | HC_HTXnTBLAop_Add |
                                          HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                    vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                    vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                                           HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | 
                                           HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                                           HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                                           HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    switch (texUnit1->Combine.OperandRGB[0]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
	                break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                        break;
                    }
                    switch (texUnit1->Combine.OperandRGB[1]) {
                    case GL_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                        break;
                    case GL_ONE_MINUS_SRC_COLOR:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                        vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                        break;
	            case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                        break;
                    }
                    switch (texUnit1->Combine.SourceRGB[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                        break;
                    case GL_CONSTANT :
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                        vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                        break;
                    }
	            break;
		    
                }
	        switch (texUnit1->Combine.ModeA) {
                case GL_REPLACE:
                    switch (texUnit1->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        switch (texUnit1->Combine.OperandA[0]) {
                	case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Atex;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_Atex;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        }
                        break;
                    case GL_CONSTANT :
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        switch (texUnit1->Combine.OperandA[0]) {
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_HTXnTBLRAbias;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            vmesa->regHTXnTBLRFog_0 = a;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_HTXnTBLRAbias;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            vmesa->regHTXnTBLRFog_0 = a;
                            break;
                        }
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        switch (texUnit1->Combine.OperandA[0]) {
                        case GL_SRC_ALPHA:  
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
	                    HC_HTXnTBLAbias_Adif;
                            vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
                        case GL_ONE_MINUS_SRC_ALPHA:
                            vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                            HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                            HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                            HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                            vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                            HC_HTXnTBLAbias_Inv | HC_HTXnTBLAbias_Adif;
	                    vmesa->regHTXnTBLRAa_0 = 0x0;
                            break;
    	                }
        	    break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftA)) {
                    case 1:
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_MODULATE:
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                     HC_HTXnTBLAbias_HTXnTBLRAbias;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
	            vmesa->regHTXnTBLRAa_0= 0x0;
                    switch (texUnit1->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_TOPA; 
    	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_InvTOPA; 
                        break;
                    }
                    switch (texUnit1->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Atex; 
                        break;
                    case GL_CONSTANT :
	                CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a<<16;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Adif; 
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                	CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a<<8;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftA)) {
	            case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_ADD:
                case GL_SUBTRACT :
                    if(texUnit1->Combine.ModeA==GL_ADD) {
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add | HC_HTXnTBLAbias_HTXnTBLRAbias;
	            }
    	            else {
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub | HC_HTXnTBLAbias_HTXnTBLRAbias;
                    }
                    vmesa->regHTXnTBLRFog_0 = 0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA;
                    vmesa->regHTXnTBLRAa_0 = 0x0 |  ( 255<<16 );
                    switch (texUnit1->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
	                vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
    	                break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
	            switch (texUnit1->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                	vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_TOPA;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_InvTOPA;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= a;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftA)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
                case GL_ADD_SIGNED :
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub;
                    vmesa->regHTXnTBLRFog_0 = 0x0;
                    vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                        HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA|
                        HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                    vmesa->regHTXnTBLRAa_0 = ( 255<<16 | 0<<8 |128 );
                    switch (texUnit1->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit1->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Inv;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[1]) {
            	    case GL_TEXTURE:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Atex; 
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_HTXnTBLRAbias; 
                        vmesa->regHTXnTBLRFog_0 |= a;
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Adif;
                        break;
                    }
                    switch ((GLint)(texUnit1->Combine.ScaleShiftA)) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
                    break;
        	case GL_INTERPOLATE :
                    vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Sub;
                    vmesa->regHTXnTBLRAa_0 = 0x0;
                    vmesa->regHTXnTBLRFog_0 =  0x0;
                    switch (texUnit1->Combine.OperandA[0]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAb_TOPA; 
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAb_InvTOPA; 
                        break;
                    }
                    switch (texUnit1->Combine.OperandA[1]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAc_TOPA;
                        break;
	            case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAc_InvTOPA;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Inv;
                        break;
                    }
                    switch (texUnit1->Combine.OperandA[2]) {
                    case GL_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAa_TOPA;
                        break;
                    case GL_ONE_MINUS_SRC_ALPHA:
                        vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAa_InvTOPA;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[0]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 8);
                        break;
                    case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAb_Adif;
                        break;
                    }
                    switch (texUnit1->Combine.SourceA[1]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Atex;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_HTXnTBLRA;
	                vmesa->regHTXnTBLRAa_0 |= a;
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_HTXnTBLRAbias;
                        vmesa->regHTXnTBLRFog_0 |= a;
                        break;
	            case GL_PRIMARY_COLOR :
                    case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAc_Adif;
	                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAbias_Adif;
                        break;
	            }
                    switch (texUnit1->Combine.SourceA[2]) {
                    case GL_TEXTURE:
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Atex;
                        break;
                    case GL_CONSTANT :
                        CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[3], a);
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_HTXnTBLRA;
                        vmesa->regHTXnTBLRAa_0 |= (a << 16);
                        break;
                    case GL_PRIMARY_COLOR :
	            case GL_PREVIOUS :
                        vmesa->regHTXnTBLAsat_0 |= HC_HTXnTBLAa_Adif;
                        break;
	            }
                    switch (texUnit1->Combine.ScaleShiftA) {
                    case 1:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_No;
                        break;
                    case 2:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_1;
                        break;
                    case 4:
                        vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAshift_2;
                        break;
                    }
            	    break;
            	case GL_DOT3_RGB :
            	case GL_DOT3_RGBA :
            	    break;
    	    	}
		break;

	/*=* John Sheng [2003.7.18] texture add *=*/
        case GL_ADD:
            switch(texImage->Format) {
            case GL_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Adif | HC_HTXnTBLAb_TOPA |
                    HC_HTXnTBLAb_Atex | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_LUMINANCE:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
            case GL_LUMINANCE_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_INTENSITY:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Atex |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
		/*=* John Sheng [2003.7.18] texenv *=*/
		/*vmesa->regHTXnTBLRAa_0 = 0x0;*/
                vmesa->regHTXnTBLRAa_0 = (255<<16) | (255<<8) | 255;;
                vmesa->regHTXnTBLRFog_0 = 0x0 | 255<<16;
                break;
            case GL_RGB:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
            case GL_RGBA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
            case GL_COLOR_INDEX:
                switch(texObj->Palette.Format) {
                case GL_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_0 | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_0 | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_Dif |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Adif | HC_HTXnTBLAb_TOPA |
                    HC_HTXnTBLAb_Atex | HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                case GL_LUMINANCE:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
                case GL_LUMINANCE_ALPHA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                case GL_INTENSITY:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Atex |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_Adif;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0 | 255<<16;
                break;
                case GL_RGB:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_Adif | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                break;
                case GL_RGBA:
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                    HC_HTXnTBLCa_TOPC | HC_HTXnTBLCa_HTXnTBLRC | HC_HTXnTBLCb_TOPC |
                    HC_HTXnTBLCb_Tex | HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_Dif;
                vmesa->regHTXnTBLCop_0 = HC_HTXnTBLCop_Add |
                    HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 |
                    HC_HTXnTBLCshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLRCa_0 = (255<<16) | (255<<8) | 255;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK |
                    HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_Atex |
                    HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_Adif |
                    HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLCop_0 |= HC_HTXnTBLAop_Add |
                    HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                break;
                }
                break;
            }
            break;
	    /*=* John Sheng [2003.7.18] texture dot3 *=*/
	    case GL_DOT3_RGB :
    	    case GL_DOT3_RGBA :
            	vmesa->regHTXnTBLCop_0 = HC_HTXnTBLDOT4 | HC_HTXnTBLCop_Add | 
        		                 HC_HTXnTBLCbias_Cbias | HC_HTXnTBLCbias_0 | 
                                         HC_HTXnTBLCshift_2 | HC_HTXnTBLAop_Add |
                                         HC_HTXnTBLAbias_HTXnTBLRAbias | HC_HTXnTBLAshift_No;
                vmesa->regHTXnTBLMPfog_0 = HC_HTXnTBLMPfog_Fog;
                vmesa->regHTXnTBLCsat_0 = HC_HTXnTBLCsat_MASK |
                                       HC_HTXnTBLCc_TOPC | HC_HTXnTBLCc_0;
                vmesa->regHTXnTBLRFog_0 = 0x0;
                vmesa->regHTXnTBLAsat_0 = HC_HTXnTBLAsat_MASK | 
                                       HC_HTXnTBLAa_TOPA | HC_HTXnTBLAa_HTXnTBLRA |
                                       HC_HTXnTBLAb_TOPA | HC_HTXnTBLAb_HTXnTBLRA |
                                       HC_HTXnTBLAc_TOPA | HC_HTXnTBLAc_HTXnTBLRA;
                vmesa->regHTXnTBLRAa_0 = 0x0;
                switch (texUnit1->Combine.OperandRGB[0]) {
                case GL_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_TOPC; 
	            break;
                case GL_ONE_MINUS_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_InvTOPC; 
                    break;
                }
                switch (texUnit1->Combine.OperandRGB[1]) {
                case GL_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_TOPC; 
                    break;
                case GL_ONE_MINUS_SRC_COLOR:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_InvTOPC; 
                    break;
                }
                switch (texUnit1->Combine.SourceRGB[0]) {
                case GL_TEXTURE:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Tex; 
                    break;
                case GL_CONSTANT :
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_HTXnTBLRC;
                    vmesa->regHTXnTBLRCa_0 = (r << 16) | (g << 8) | b;
                    break;
	        case GL_PRIMARY_COLOR :
                case GL_PREVIOUS :
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCa_Dif; 
                    break;
                }
                switch (texUnit1->Combine.SourceRGB[1]) {
                case GL_TEXTURE:
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Tex; 
                    break;
                case GL_CONSTANT :
            	    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[0], r);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[1], g);
                    CLAMPED_FLOAT_TO_UBYTE(texUnit1->EnvColor[2], b);
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_HTXnTBLRC;
                    vmesa->regHTXnTBLRCb_0 = (r << 16) | (g << 8) | b;
                    break;
                case GL_PRIMARY_COLOR :
                case GL_PREVIOUS :
                    vmesa->regHTXnTBLCsat_0 |= HC_HTXnTBLCb_Dif; 
                    break;
                }
	        break;
            default:
                break;
            }
        }
        vmesa->dirty |= VIA_UPLOAD_TEXTURE;
    }
    else {
	if (ctx->Fog.Enabled)
    	    vmesa->regCmdB &= (~(HC_HVPMSK_S | HC_HVPMSK_T));	
	else	    
    	    vmesa->regCmdB &= (~(HC_HVPMSK_S | HC_HVPMSK_T | HC_HVPMSK_W));
        vmesa->regEnable &= (~(HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK));
        vmesa->dirty |= VIA_UPLOAD_ENABLE;
    }
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
    
}

void viaChooseColorState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLenum s = ctx->Color.BlendSrcRGB;
    GLenum d = ctx->Color.BlendDstRGB;

    /* The HW's blending equation is:
     * (Ca * FCa + Cbias + Cb * FCb) << Cshift
     */
#ifdef DEBUG
     if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#endif

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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#endif
    
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    
    if (VIA_DEBUG) fprintf(stderr, "GL_CULL_FACE = %x\n", GL_CULL_FACE);    
    if (VIA_DEBUG) fprintf(stderr, "ctx->Polygon.CullFlag = %x\n", ctx->Polygon.CullFlag);    
    
    if (VIA_DEBUG) fprintf(stderr, "GL_FRONT = %x\n", GL_FRONT);    
    if (VIA_DEBUG) fprintf(stderr, "ctx->Polygon.CullFaceMode = %x\n", ctx->Polygon.CullFaceMode);    
    if (VIA_DEBUG) fprintf(stderr, "GL_CCW = %x\n", GL_CCW);    
    if (VIA_DEBUG) fprintf(stderr, "ctx->Polygon.FrontFace = %x\n", ctx->Polygon.FrontFace);    
#endif
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
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
}

static void viaChooseState(GLcontext *ctx, GLuint newState)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    
    if (VIA_DEBUG) fprintf(stderr, "newState = %x\n", newState);        
#endif    
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
            
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);        
#endif
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
