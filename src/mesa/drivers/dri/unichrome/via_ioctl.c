#/*
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
#include <unistd.h>

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "via_context.h"
#include "via_ioctl.h"
#include "via_state.h"

#include "drm.h"
#include <sys/ioctl.h>

GLuint FrameCount = 0;
GLuint dmaLow = 0;
/*=* John Sheng [2003.5.31] flip *=*/
GLuint nFirstSwap = GL_TRUE;
GLuint nFirstFlip = GL_TRUE;
#define SetReg2DAGP(nReg, nData) {              	\
    *((GLuint *)(vb)) = ((nReg) >> 2) | 0xF0000000;     \
    *((GLuint *)(vb) + 1) = (nData);          		\
    vb = ((GLuint *)vb) + 2;                  		\
    vmesa->dmaLow +=8;					\
}

#define DEPTH_SCALE ((1 << 16) - 1)

static void viaClear(GLcontext *ctx, GLbitfield mask, GLboolean all,
                     GLint cx, GLint cy, GLint cw, GLint ch)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    const GLuint colorMask = *((GLuint *)&ctx->Color.ColorMask);
    int flag = 0;
    GLuint scrn = 0, i = 0, side = 0;
    scrn = vmesa->saam & S_MASK;
    side = vmesa->saam & P_MASK;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
#endif
    VIA_FIREVERTICES(vmesa);

    if ((mask & DD_FRONT_LEFT_BIT) && colorMask == ~0) {
	flag |= VIA_FRONT;
        mask &= ~DD_FRONT_LEFT_BIT;
    }

    if ((mask & DD_BACK_LEFT_BIT) && colorMask == ~0) {
	flag |= VIA_BACK;	
        mask &= ~DD_BACK_LEFT_BIT;
    }

    if (mask & DD_DEPTH_BIT) {
        if (ctx->Depth.Mask)
  	    flag |= VIA_DEPTH;	    
        mask &= ~DD_DEPTH_BIT;
    }
    
    if (mask & DD_STENCIL_BIT) {
    	if (ctx->Stencil.Enabled)
  	    flag |= VIA_STENCIL;	    
        mask &= ~DD_STENCIL_BIT;
    }
    
    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT) {
	flag |= VIA_FRONT;
	flag &= ~VIA_BACK;
    }
    
    if (flag) {
	LOCK_HARDWARE(vmesa);
        /* flip top to bottom */
        cy = dPriv->h - cy - ch;
        cx += vmesa->drawX;
        cy += vmesa->drawY;
        
	if (vmesa->numClipRects) {
            int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, vmesa->numClipRects);
            drm_clip_rect_t *box = vmesa->pClipRects;
            drm_clip_rect_t *b = vmesa->sarea->boxes;
            int n = 0;
	    
    	    if (!vmesa->saam) {
		if (!all) {
#ifdef DEBUG    
		    if (VIA_DEBUG) fprintf(stderr,"!all");
#endif        
            	    for (; i < nr; i++) {
                	GLint x = box[i].x1;
                	GLint y = box[i].y1;
                	GLint w = box[i].x2 - x;
                	GLint h = box[i].y2 - y;

                	if (x < cx) w -= cx - x, x = cx;
                	if (y < cy) h -= cy - y, y = cy;
                	if (x + w > cx + cw) w = cx + cw - x;
                	if (y + h > cy + ch) h = cy + ch - y;
                	if (w <= 0) continue;
                	if (h <= 0) continue;

                	b->x1 = x;
                	b->y1 = y;
                	b->x2 = x + w;
                	b->y2 = y + h;
                	b++;
                	n++;
            	    }
        	}
        	else {
            	    for (; i < nr; i++) {
                	*b++ = *(drm_clip_rect_t *)&box[i];
                	n++;
            	    }
        	}
        	vmesa->sarea->nbox = n;
	    }
	    else {
		GLuint scrn = 0;
		scrn = vmesa->saam & S_MASK;

		if (scrn == S0 || scrn == S1) {
		    if (!all) {
            		for (; i < nr; i++) {
                	    GLint x = box[i].x1;
                	    GLint y = box[i].y1;
                	    GLint w = box[i].x2 - x;
                	    GLint h = box[i].y2 - y;

                	    if (x < cx) w -= cx - x, x = cx;
                	    if (y < cy) h -= cy - y, y = cy;
                	    if (x + w > cx + cw) w = cx + cw - x;
                	    if (y + h > cy + ch) h = cy + ch - y;
                	    if (w <= 0) continue;
                	    if (h <= 0) continue;

                	    b->x1 = x;
                	    b->y1 = y;
                	    b->x2 = x + w;
                	    b->y2 = y + h;
                	    b++;
                	    n++;
            		}
        	    }
        	    else {
            		for (; i < nr; i++) {
                	    *b++ = *(drm_clip_rect_t *)&box[i];
                	    n++;
            		}
        	    }
        	    vmesa->sarea->nbox = n;		
		}
		/* between */
		else {
		    if (!all) {
            		for (; i < nr; i++) {
                	    GLint x = box[i].x1;
                	    GLint y = box[i].y1;
                	    GLint w = box[i].x2 - x;
                	    GLint h = box[i].y2 - y;

                	    if (x < cx) w -= cx - x, x = cx;
                	    if (y < cy) h -= cy - y, y = cy;
                	    if (x + w > cx + cw) w = cx + cw - x;
                	    if (y + h > cy + ch) h = cy + ch - y;
                	    if (w <= 0) continue;
                	    if (h <= 0) continue;

                	    b->x1 = x;
                	    b->y1 = y;
                	    b->x2 = x + w;
                	    b->y2 = y + h;
                	    b++;
                	    n++;
            		}
			
        	    }
        	    else {
            		for (; i < nr; i++) {
                	    *b++ = *(drm_clip_rect_t *)&box[n];
                	    n++;
            		}
        	    }
		    *b++ = *(drm_clip_rect_t *)vmesa->pSaamRects;
        	    vmesa->sarea->nbox = n;
		}
	    }
	    
	    {
		if (flag & VIA_FRONT) {

		    if (vmesa->drawType == GLX_PBUFFER_BIT)
			viaFillFrontPBuffer(vmesa);
		    else
			viaFillFrontBuffer(vmesa);
		    
		    if (vmesa->saam && (scrn == (S0 | S1))) {
			nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, vmesa->numSaamRects);
        		box = vmesa->pSaamRects;
        		b = vmesa->sarea->boxes;
        		n = 0;

			for (i = 0; i < nr; i++) {
                	    *b++ = *(drm_clip_rect_t *)&box[n];
                	    n++;
            		}

			vmesa->sarea->nbox = n;			
			viaFillFrontBufferSaam(vmesa);
		    }
		} 
		
		if (flag & VIA_BACK) {
		    viaFillBackBuffer(vmesa);
		}

		if (flag & VIA_DEPTH) {
		    double depth_clamp, range = 0xffffffff;
		    if (vmesa->hasStencil == 0) {
			if (vmesa->depthBits == 32) {
			    depth_clamp = ((double)ctx->Depth.Clear)*range;
			    viaFillDepthBuffer(vmesa, (GLuint)depth_clamp);
			}
			else {
			    depth_clamp = ((double)ctx->Depth.Clear)*range;
			    viaFillDepthBuffer(vmesa, (GLuint)depth_clamp);
			}
		    }
		    else {
			depth_clamp = ((double)ctx->Depth.Clear)*range;
			viaFillStencilDepthBuffer(vmesa, (GLuint)depth_clamp);
		    }
		}		
		/*=* [DBG] Fix tuxracer depth error *=*/
		else if (flag & VIA_STENCIL) {
		    viaFillStencilBuffer(vmesa, (GLuint)ctx->Stencil.Clear);
		}
	    }
        }
        UNLOCK_HARDWARE(vmesa);
        vmesa->uploadCliprects = GL_TRUE;
    }

    if (mask)
        _swrast_Clear(ctx, mask, all, cx, cy, cw, ch);
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
}

/*
 * Copy the back buffer to the front buffer. 
 */
void viaCopyBuffer(const __DRIdrawablePrivate *dPriv)
{
    viaContextPtr vmesa;
    drm_clip_rect_t *pbox;
    int nbox, i;
    GLuint scrn = 0, side = 0;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);        
#endif
    assert(dPriv);
    assert(dPriv->driContextPriv);
    assert(dPriv->driContextPriv->driverPrivate);

    vmesa = (viaContextPtr)dPriv->driContextPriv->driverPrivate;
    
    VIA_FIREVERTICES(vmesa);
    LOCK_HARDWARE(vmesa);
    
    scrn = vmesa->saam & S_MASK;
    side = vmesa->saam & P_MASK;
    
    pbox = vmesa->pClipRects;
    nbox = vmesa->numClipRects;

#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s %d cliprects (%d), SAAM (%d)\n", 
    	__FUNCTION__, nbox, vmesa->drawType, vmesa->saam);
#endif
    
	
    if (vmesa->drawType == GLX_PBUFFER_BIT) {
	viaDoSwapPBuffers(vmesa);
#ifdef DEBUG    
        if (VIA_DEBUG) fprintf(stderr, "%s SwapPBuffers\n", __FUNCTION__);    
#endif	/*=* [DBG] for pbuffer *=*/
	/*viaDoSwapBufferSoftFront(vmesa);*/
    }
    else {
	GLuint scrn = 0;
	scrn = vmesa->saam & S_MASK;
	if (!vmesa->saam) {
	    for (i = 0; i < nbox; ) {
    		int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
    		drm_clip_rect_t *b = (drm_clip_rect_t *)vmesa->sarea->boxes;

    		vmesa->sarea->nbox = nr - i;

    		for (; i < nr; i++)
        	    *b++ = pbox[i];
		viaDoSwapBuffers(vmesa);
#ifdef DEBUG    
		if (VIA_DEBUG) fprintf(stderr, "%s SwapBuffers\n", __FUNCTION__);    
#endif
	    }
	}
	else if (scrn == S0 || scrn == S1) {
	    for (i = 0; i < nbox; ) {
		int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, vmesa->numClipRects);
    		drm_clip_rect_t *b = (drm_clip_rect_t *)vmesa->sarea->boxes;

    		vmesa->sarea->nbox = nr - i;
		
    		for (; i < nr; i++) {
        	    *b++ = pbox[i];
		}
		viaDoSwapBuffers(vmesa);
	    }
	}
	/* between */
	else {
	    for (i = 0; i < nbox; ) {
    		int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
    		drm_clip_rect_t *b = (drm_clip_rect_t *)vmesa->sarea->boxes;

    		vmesa->sarea->nbox = nr - i;

    		for (; i < nr; i++)
        	    *b++ = pbox[i];
		viaDoSwapBuffers(vmesa);
	    }

	    pbox = vmesa->pSaamRects;
	    nbox = vmesa->numSaamRects;

	    for (i = 0; i < nbox; ) {
    		int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, vmesa->numSaamRects);
    		drm_clip_rect_t *b = (drm_clip_rect_t *)vmesa->sarea->boxes;

    		vmesa->sarea->nbox = nr - i;

    		for (; i < nr; i++)
        	    *b++ = pbox[i];

		viaDoSwapBuffersSaam(vmesa);
	    }
	}
    }
    UNLOCK_HARDWARE(vmesa);
    vmesa->uploadCliprects = GL_TRUE;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);        
#endif
}

/*
 * XXX implement when full-screen extension is done.
 */
void viaPageFlip(const __DRIdrawablePrivate *dPriv)
{
    /*=* John Sheng [2003.5.31] flip *=*/
    viaContextPtr vmesa = (viaContextPtr)dPriv->driContextPriv->driverPrivate;
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*56);
    GLuint nBackBase;
    viaBuffer buffer_tmp;
    GLcontext *ctx;

#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);        
#endif
    assert(dPriv);
    assert(dPriv->driContextPriv);
    assert(dPriv->driContextPriv->driverPrivate);

    ctx = vmesa->glCtx;
    
    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	return;
    
    /* Page Flip*/
    if(GL_FALSE) {
	viaFlushPrimsLocked(vmesa);
	while ((*(volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x200) & 0x2) != 0x2);
	while (*(volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x200) & 0x2);
	nBackBase = vmesa->back.offset >> 1;
	
	/*if (nFirstFlip) {
    	    *vb++ = HALCYON_HEADER2;
    	    *vb++ = 0x00fe0000;
    	    *vb++ = 0x00001004;
    	    *vb++ = 0x00001004;
	    vmesa->dmaLow += 16;

    	    nFirstFlip = GL_FALSE;
	}
	SetReg2DAGP(0x214, nBackBase);
	viaFlushPrimsLocked(vmesa);*/
    	if (nFirstFlip) {
	    *((volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x43c)) = 0x00fe0000;
	    *((volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x440)) = 0x00001004;
	    nFirstFlip = GL_FALSE;
	}
	*((GLuint *)((GLuint)vmesa->regMMIOBase + 0x214)) = nBackBase;
    }
    /* Auto Swap */
    else {
	viaFlushPrimsLocked(vmesa);
	vb = viaCheckDma(vmesa, vmesa->sarea->nbox*56);
	if (nFirstSwap) {
    	    *vb++ = HALCYON_HEADER2;
    	    *vb++ = 0x00fe0000;
    	    *vb++ = 0x0000000e;
    	    *vb++ = 0x0000000e;
	    vmesa->dmaLow += 16;

    	    nFirstSwap = GL_FALSE;
	}
	nBackBase = (vmesa->back.offset << 1);

	*vb++ = HALCYON_HEADER2;
	*vb++ = 0x00fe0000;
	*vb++ = (HC_SubA_HFBBasL << 24) | (nBackBase & 0xFFFFF8) | 0x2;
	*vb++ = (HC_SubA_HFBDrawFirst << 24) |
                          ((nBackBase & 0xFF000000) >> 24) | 0x0100;
	vmesa->dmaLow += 16;
	viaFlushPrimsLocked(vmesa);
    }
    
    
    memcpy(&buffer_tmp, &vmesa->back, sizeof(viaBuffer));
    memcpy(&vmesa->back, &vmesa->front, sizeof(viaBuffer));
    memcpy(&vmesa->front, &buffer_tmp, sizeof(viaBuffer));
    
    if(vmesa->currentPage) {
	vmesa->currentPage = 0;
	if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
		ctx->Driver.DrawBuffer(ctx, GL_BACK);
	}
	else {
		ctx->Driver.DrawBuffer(ctx, GL_FRONT);
	}
    }
    else {
	vmesa->currentPage = 1;
	if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
		ctx->Driver.DrawBuffer(ctx, GL_BACK);
	}
	else {
		ctx->Driver.DrawBuffer(ctx, GL_FRONT);
	}
    }
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);        
#endif

}

/* This waits for *everybody* to finish rendering -- overkill.
 */
void viaDmaFinish(viaContextPtr vmesa)
{
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);        
#endif
    VIA_FIREVERTICES(vmesa);
    LOCK_HARDWARE(vmesa);
    UNLOCK_HARDWARE(vmesa);
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
}

void viaRegetLockQuiescent(viaContextPtr vmesa)
{
    drmUnlock(vmesa->driFd, vmesa->hHWContext);
}

static int intersect_rect(drm_clip_rect_t *out,
                          drm_clip_rect_t *a,
                          drm_clip_rect_t *b)
{
    *out = *a;

    if (b->x1 > out->x1) out->x1 = b->x1;
    if (b->x2 < out->x2) out->x2 = b->x2;
    if (out->x1 >= out->x2) return 0;

    if (b->y1 > out->y1) out->y1 = b->y1;
    if (b->y2 < out->y2) out->y2 = b->y2;
    if (out->y1 >= out->y2) return 0;

    return 1;
}

void viaFlushPrimsLocked(viaContextPtr vmesa)
{
    drm_clip_rect_t *pbox = (drm_clip_rect_t *)vmesa->pClipRects;
    int nbox = vmesa->numClipRects;
    drm_via_sarea_t *sarea = vmesa->sarea;
    drm_via_flush_agp_t agpCmd;
    drm_via_flush_sys_t sysCmd;
    GLuint *vb = viaCheckDma(vmesa, 0);
    int i;

    if (vmesa->dmaLow == DMA_OFFSET) {
    	return;
    }
    if (vmesa->dmaLow > 2097152)
	fprintf(stderr, "buffer overflow in Flush Prims = %d\n",vmesa->dmaLow);
    
    switch (vmesa->dmaLow & 0x1F) {	
    case 8:
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	vmesa->dmaLow += 24;
	break;
    case 16:
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	vmesa->dmaLow += 16;
	break;    
    case 24:
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;
	*vb++ = HC_DUMMY;	
	vmesa->dmaLow += 40;
	break;    
    case 0:
	break;
    default:
	break;
    }
    
    if (vmesa->useAgp) {
	agpCmd.offset = 0x0;
	agpCmd.size = vmesa->dmaLow;
	agpCmd.index = vmesa->dma[vmesa->dmaIndex].index;
	agpCmd.discard = 0;
    } 
    else {
	sysCmd.offset = 0x0;
	sysCmd.size = vmesa->dmaLow;
	sysCmd.index = (GLuint)vmesa->dma[vmesa->dmaIndex].map;
	sysCmd.discard = 0;
    }
    
    sarea->vertexPrim = vmesa->hwPrimitive;

    if (!nbox) {
	if (vmesa->useAgp)
	    agpCmd.size = 0;
	else
	    sysCmd.size = 0;
    }
    else if (nbox > VIA_NR_SAREA_CLIPRECTS) {
        vmesa->uploadCliprects = GL_TRUE;
    }
/*=* John Sheng [2003.5.31] flip *=*/
/*#ifdef DEBUG    
    if (VIA_DEBUG) {
	GLuint i;
	GLuint *data = (GLuint *)vmesa->dmaAddr;
	for (i = 0; i < vmesa->dmaLow; i += 16) {
            fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x\n", *data++);
	}
	fprintf(stderr, "******************************************\n");
    }   
#endif*/
    if (!nbox || !vmesa->uploadCliprects) {
        if (nbox == 1)
            sarea->nbox = 0;
        else
            sarea->nbox = nbox;

	if (vmesa->useAgp) {
    	    agpCmd.discard = 1;
	    flush_agp(vmesa, &agpCmd);
	}
	else {
	    sysCmd.discard = 1;
	    flush_sys(vmesa, &sysCmd);
	}
    }
    else {
	GLuint scrn;
	GLuint side;
	scrn = vmesa->saam & S_MASK;
	side = vmesa->saam & P_MASK;
        
	for (i = 0; i < nbox; ) {
            int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, nbox);
            drm_clip_rect_t *b = sarea->boxes;

            if (!vmesa->saam) {
		if (vmesa->glCtx->Scissor.Enabled) {
            	    sarea->nbox = 0;

            	    for (; i < nr; i++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;
                	if (intersect_rect(b, b, &vmesa->scissorRect)) {
                    	    sarea->nbox++;
                    	    b++;
                	}
            	    }
            	    if (!sarea->nbox) {
                	if (nr < nbox) continue;
                	agpCmd.size = 0;
            	    }
        	}
        	else {
            	    sarea->nbox = nr - i;
            	    for (; i < nr; i++, b++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;
            	    }
        	}
	    }
	    else if (scrn == S0 || scrn == S1){
		if (vmesa->scissor) {
            	    sarea->nbox = 0;
            	    for (; i < nr; i++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;
                	if (intersect_rect(b, b, &vmesa->scissorRect)) {
                    	    sarea->nbox++;
                    	    b++;
                	}
            	    }
            	    if (!sarea->nbox) {
                	if (nr < nbox) continue;
                	agpCmd.size = 0;
            	    }
        	}
        	else {
            	    sarea->nbox = nr - i;
            	    for (; i < nr; i++, b++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;
            	    }
        	}
	    
	    }
	    /* between */
	    else {
		if (vmesa->scissor) {
            	    sarea->nbox = 0;
            	    for (; i < nr; i++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;

                	if (intersect_rect(b, b, &vmesa->scissorRect)) {
                    	    sarea->nbox++;
                    	    b++;
                	}
            	    }
            	    if (!sarea->nbox) {
                	if (nr < nbox) continue;
                	agpCmd.size = 0;
            	    }
        	}
        	else {
            	    sarea->nbox = nr - i;
            	    for (; i < nr; i++, b++) {
                	b->x1 = pbox[i].x1 - vmesa->drawX;
                	b->y1 = pbox[i].y1 - vmesa->drawY;
                	b->x2 = pbox[i].x2 - vmesa->drawX;
                	b->y2 = pbox[i].y2 - vmesa->drawY;
            	    }
        	}
	    }

            if (nr == nbox) {
		if (vmesa->useAgp) {
            	    agpCmd.discard = 1;
		    flush_agp(vmesa, &agpCmd);
		} 
		else {
		    sysCmd.discard = 1;
		    flush_sys(vmesa, &sysCmd);
		}
	    }

	    if (scrn == (S0 | S1)) {
		pbox = (drm_clip_rect_t *)vmesa->pSaamRects;
		nbox = vmesa->numSaamRects;
		for (i = 0; i < nbox; ) {
        	    int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, nbox);
        	    drm_clip_rect_t *b = sarea->boxes;
		    if (vmesa->scissor) {
            		sarea->nbox = 0;
            		for (; i < nr; i++) {
                	    b->x1 = pbox[i].x1 - vmesa->drawXSaam;
                	    b->y1 = pbox[i].y1 - vmesa->drawYSaam;
                	    b->x2 = pbox[i].x2 - vmesa->drawXSaam;
                	    b->y2 = pbox[i].y2 - vmesa->drawYSaam;

                	    if (intersect_rect(b, b, &vmesa->scissorRect)) {
                    		sarea->nbox++;
                    		b++;
                	    }
            		}
            		if (!sarea->nbox) {
                	    if (nr < nbox) continue;
                	    agpCmd.size = 0;
            		}
        	    }
        	    else {
            		sarea->nbox = nr - i;
            		for (; i < nr; i++, b++) {
                	    b->x1 = pbox[i].x1 - vmesa->drawXSaam;
                	    b->y1 = pbox[i].y1 - vmesa->drawYSaam;
                	    b->x2 = pbox[i].x2 - vmesa->drawXSaam;
                	    b->y2 = pbox[i].y2 - vmesa->drawYSaam;
            		}
        	    }
		}
		flush_agp_saam(vmesa, &agpCmd);
	    }
        }
    }
#ifdef DEBUG        
    if (VIA_DEBUG) {
	GLuint i;
	GLuint *data = (GLuint *)vmesa->dmaAddr;
	for (i = 0; i < vmesa->dmaLow; i += 16) {
            fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x  ", *data++);
	    fprintf(stderr, "%08x\n", *data++);
	}
	fprintf(stderr, "******************************************\n");
    }  
#endif
    /* Reset vmesa vars:
     */
    vmesa->dmaLow = DMA_OFFSET;
    if (vmesa->dmaIndex) {
        vmesa->dmaAddr = vmesa->dma[0].map;
        vmesa->dmaHigh = vmesa->dma[0].size;
        vmesa->dmaLastPrim = DMA_OFFSET;
	vmesa->dmaIndex = 0;
    }
    else {
        vmesa->dmaAddr = vmesa->dma[1].map;
        vmesa->dmaHigh = vmesa->dma[1].size;
        vmesa->dmaLastPrim = DMA_OFFSET;
	vmesa->dmaIndex = 1;	
    }
}

void viaFlushPrims(viaContextPtr vmesa)
{
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif
    if (vmesa->dmaLow) {
        LOCK_HARDWARE(vmesa);
        viaFlushPrimsLocked(vmesa);
        UNLOCK_HARDWARE(vmesa);
    }
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
#endif
}

static void viaFlush(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
#endif    
    VIA_FIREVERTICES(vmesa);
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
}

static void viaFinish(GLcontext *ctx)
{
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
#endif
    return;
}

static void viaClearStencil(GLcontext *ctx,  int s)
{
    return;
}

void viaInitIoctlFuncs(GLcontext *ctx)
{
    ctx->Driver.Flush = viaFlush;
    ctx->Driver.Clear = viaClear;
    ctx->Driver.Finish = viaFinish;
    ctx->Driver.ClearStencil = viaClearStencil;
}

void viaFillFrontBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset,i; 
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*48);
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint pixel = (GLuint)vmesa->ClearColor;

    offset = vmesa->viaScreen->fbOffset + (vmesa->drawY * vmesa->front.pitch + vmesa->drawX * bytePerPixel);
#ifdef DEBUG        
    if (VIA_DEBUG) fprintf(stderr, "Fill Front offset = %08x\n", offset);
#endif    
    nDestBase = offset & 0xffffffe0;
    nDestPitch = vmesa->front.pitch;

    for (i = 0; i < vmesa->sarea->nbox ; i++) {        
        nDestWidth = b->x2 - b->x1 - 1;
	nDestHeight = b->y2 - b->y1 - 1;
	
	if (bytePerPixel == 4)
	    offsetX = (b->x1 - vmesa->drawX) + (vmesa->drawX & 7);
	else 
	    offsetX = (b->x1 - vmesa->drawX) + (vmesa->drawX & 15);
    

	if (GL_TRUE) {
    	    /* GEFGCOLOR*/
    	    SetReg2DAGP(0x18, pixel | 0x00000000);
    	    /* GEWD*/
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST*/
    	    SetReg2DAGP(0x0C, (offsetX | ((b->y1 - vmesa->drawY) << 16)));
    	    /* GEDSTBASE*/
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH*/
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT*/
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	}
	b++;
    }

    viaFlushPrimsLocked(vmesa);
}

void viaFillFrontBufferSaam(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset,i; 
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*48);
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint pixel = (GLuint)vmesa->ClearColor;

    offset = vmesa->viaScreen->fbSize + 
	(vmesa->drawYSaam * vmesa->front.pitch + vmesa->drawXSaam * bytePerPixel);
    nDestBase = offset & 0xffffffe0;
    nDestPitch = vmesa->front.pitch;

    for (i = 0; i < vmesa->sarea->nbox ; i++) {        
        nDestWidth = b->x2 - b->x1 - 1;
	nDestHeight = b->y2 - b->y1 - 1;
	
	if (bytePerPixel == 4)
	    offsetX = (b->x1 - vmesa->drawXSaam) + (vmesa->drawXSaam & 7);
	else 
	    offsetX = (b->x1 - vmesa->drawXSaam) + (vmesa->drawXSaam & 15);
    
	if (GL_TRUE) {
    	    /* GEFGCOLOR*/
    	    SetReg2DAGP(0x18, pixel | 0x00000000);
    	    /* GEWD*/
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST*/
    	    SetReg2DAGP(0x0C, (offsetX | ((b->y1 - vmesa->drawYSaam) << 16)));
    	    /* GEDSTBASE*/
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH*/
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT*/
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	}
	b++;
    }

    viaFlushPrimsLocked(vmesa);
}

void viaFillFrontPBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset; 
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*48);
    GLuint pixel = (GLuint)vmesa->ClearColor;

    offset = vmesa->front.offset;
#ifdef DEBUG        
    if (VIA_DEBUG) fprintf(stderr, "Fill PFront offset = %08x\n", offset);
#endif    
    nDestBase = offset;
    nDestPitch = vmesa->front.pitch;

    nDestWidth = vmesa->driDrawable->w - 1;
    nDestHeight = vmesa->driDrawable->h - 1;
	
    offsetX = 0;

    /* GEFGCOLOR*/
    SetReg2DAGP(0x18, pixel | 0x00000000);
    /* GEWD*/
    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    /* GEDST*/
    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    /* GEDSTBASE*/
    SetReg2DAGP(0x34, (nDestBase >> 3));
    /* GEPITCH*/
    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    /* BITBLT*/
    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
    
    viaFlushPrimsLocked(vmesa);
}

void viaFillBackBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset; 
    GLcontext *ctx = vmesa->glCtx;
    GLuint *vb = viaCheckDma(vmesa, 48);
    GLuint pixel = (GLuint)vmesa->ClearColor;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;

    offset = vmesa->back.offset;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "Fill Back offset = %08x\n", offset);
#endif
    nDestBase = offset;
    nDestPitch = vmesa->back.pitch;
    offsetX = vmesa->drawXoff;
    
    {
	if (!ctx->Scissor.Enabled) {
	    
	    nDestWidth = (vmesa->back.pitch / vmesa->viaScreen->bitsPerPixel * 8) - 1;
	    nDestHeight = vmesa->driDrawable->h -1;
	    
	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel | 0x00000000);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	}
	/*=* John Sheng [2003.7.18] texenv *=*/
	else {
	    int i;
	    drm_clip_rect_t *b = vmesa->sarea->boxes;
	    for (i = 0; i < vmesa->sarea->nbox ; i++) {        
    		nDestWidth = b->x2 - b->x1 - 1;
		nDestHeight = b->y2 - b->y1 - 1;
		
		if (bytePerPixel == 4)
		    offsetX = (b->x1 - vmesa->drawX) + (vmesa->drawX & 7);
		else 
		    offsetX = (b->x1 - vmesa->drawX) + (vmesa->drawX & 15);
		
		/* GEFGCOLOR */
    		SetReg2DAGP(0x18, pixel | 0x00000000);
    		/* GEWD */
    		SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    		/* GEDST */
    		SetReg2DAGP(0x0C, ((offsetX + (b->x1 - vmesa->drawX)) | ((b->y1 - vmesa->drawY) << 16)));
    		/* GEDSTBASE */
    		SetReg2DAGP(0x34, (nDestBase >> 3));
    		/* GEPITCH */
    		SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    		/* BITBLT */
    		SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
		b++;
	    }
	}
#ifdef DEBUG    	
	if (VIA_DEBUG) {
    	    fprintf(stderr," width = %08x\n", nDestWidth);	
    	    fprintf(stderr," height = %08x\n", nDestHeight);	
	}	     
#endif
    }
}

void viaFillStencilDepthBuffer(viaContextPtr vmesa, GLuint pixel)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset; 
    GLuint *vb = viaCheckDma(vmesa, 80);
    GLuint nMask;
    
    offset = vmesa->depth.offset;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "Fill Stencil Depth offset = %08x\n", offset);
#endif    
    nDestBase = offset;
    nDestPitch = vmesa->depth.pitch;
    offsetX = vmesa->drawXoff;
    pixel = pixel & 0xffffff00;
    nMask = 0x10000000;
    
    {        
	nDestWidth = (vmesa->depth.pitch / vmesa->depthBits * 8) - 1 - offsetX;
	nDestHeight = vmesa->driDrawable->h -1;
	
	if (vmesa->viaScreen->bitsPerPixel == vmesa->depth.bpp) {
	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, nMask);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x00000000);
	}
	else {
	    GLuint EngStatus = *(vmesa->pnGEMode);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, (EngStatus & 0xFFFFFCFF) | 0x300);
	    /* GEFGCOLOR */
	    SetReg2DAGP(0x18, pixel);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, nMask);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, EngStatus);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x00000000);
	    
	    WAIT_IDLE
	}
    }

    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	viaFlushPrimsLocked(vmesa);
    }
}

void viaFillStencilBuffer(viaContextPtr vmesa, GLuint pixel)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset; 
    GLuint *vb = viaCheckDma(vmesa, 80);
    GLuint nMask;
    
    offset = vmesa->depth.offset;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "Fill Stencil offset = %08x\n", offset);
#endif
    nDestBase = offset;
    nDestPitch = vmesa->depth.pitch;
    offsetX = vmesa->drawXoff;	
    pixel = pixel & 0x000000ff;
    nMask = 0xe0000000;
    
    {        
	nDestWidth = (vmesa->depth.pitch / vmesa->depthBits * 8) - 1 - offsetX;
	nDestHeight = vmesa->driDrawable->h -1;
	
	if (vmesa->viaScreen->bitsPerPixel == vmesa->depth.bpp) {
	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, nMask);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x00000000);
	}
	else {
	    GLuint EngStatus = *(vmesa->pnGEMode);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, (EngStatus & 0xFFFFFCFF) | 0x300);
	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, nMask);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, EngStatus);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x00000000);
	}
    }

    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	viaFlushPrimsLocked(vmesa);
    }
}

void viaFillDepthBuffer(viaContextPtr vmesa, GLuint pixel)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset;
    GLuint *vb = viaCheckDma(vmesa, 72);
    offset = vmesa->depth.offset;
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "Fill Depth offset = %08x\n", offset);
#endif
    nDestBase = offset;
    nDestPitch = vmesa->depth.pitch;
    offsetX = vmesa->drawXoff;
    
    {
	nDestWidth = (vmesa->depth.pitch / vmesa->depthBits * 8) - 1 - offsetX;
	nDestHeight = vmesa->driDrawable->h -1;
	
	if (vmesa->viaScreen->bitsPerPixel == vmesa->depth.bpp) {
    	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x0);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	}
	/* depth = 16, color = 32 */
	else if (vmesa->depth.bpp == 16) {
	    GLuint EngStatus = *(vmesa->pnGEMode);
	    
	    /* GEMODE */
    	    SetReg2DAGP(0x04, (EngStatus & 0xFFFFFCFF) | 0x100);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x0);
	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, EngStatus);
	}
	/* depth = 32, color = 16 */
	else {
	    GLuint EngStatus = *(vmesa->pnGEMode);
	    
	    /* GEMODE */
    	    SetReg2DAGP(0x04, (EngStatus & 0xFFFFFCFF) | 0x300);
	    /* GEMASK */
	    SetReg2DAGP(0x2C, 0x0);
    	    /* GEFGCOLOR */
    	    SetReg2DAGP(0x18, pixel);
    	    /* GEWD */
    	    SetReg2DAGP(0x10, nDestWidth | (nDestHeight << 16));
    	    /* GEDST */
    	    SetReg2DAGP(0x0C, (offsetX | (0 << 16)));
    	    /* GEDSTBASE */
    	    SetReg2DAGP(0x34, (nDestBase >> 3));
    	    /* GEPITCH */
    	    SetReg2DAGP(0x38, (((nDestPitch >> 3) << 16) & 0x7FF0000) | 0x80000000);
    	    /* BITBLT */
    	    SetReg2DAGP(0x0, 0x1 | 0x2000 | 0xF0000000);
	    /* GEMODE */
    	    SetReg2DAGP(0x04, EngStatus);
	}
    }
    
    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	viaFlushPrimsLocked(vmesa);
    }
}

void viaDoSwapBuffers(viaContextPtr vmesa)
{    
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*56);
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontWidth, nFrontHeight, nBackWidth, nBackHeight;
    GLuint nFrontBase, nBackBase;
    GLuint nFrontOffsetX, nFrontOffsetY, nBackOffsetX, nBackOffsetY;
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint i;
    
    nFrontPitch = vmesa->front.pitch;
    nBackPitch = vmesa->back.pitch;
    
    /* Caculate Base */
    nFrontBase = vmesa->viaScreen->fbOffset + (vmesa->drawY * nFrontPitch + vmesa->drawX * bytePerPixel);
    nBackBase = vmesa->back.offset;
    /* 128 bit alignment*/
    nFrontBase = nFrontBase & 0xffffffe0;
    
    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	return;
    
    for (i = 0; i < vmesa->sarea->nbox; i++) {        
	/* Width, Height */
        nFrontWidth = nBackWidth = b->x2 - b->x1 - 1;
	nFrontHeight = nBackHeight = b->y2 - b->y1 - 1;
	/* Offset */
	nFrontOffsetX = (b->x1 - vmesa->drawX) + vmesa->drawXoff;
	nFrontOffsetY = b->y1 - vmesa->drawY;
	
	nBackOffsetX = nFrontOffsetX;
	nBackOffsetY = nFrontOffsetY;
	/* GEWD */
	SetReg2DAGP(0x10, nFrontWidth | (nFrontHeight << 16));
        /* GEDST */
        SetReg2DAGP(0x0C, nFrontOffsetX | (nFrontOffsetY << 16));
        /* GESRC */
        SetReg2DAGP(0x08, nBackOffsetX | (nBackOffsetY << 16));
        /* GEDSTBASE */
        SetReg2DAGP(0x34, (nFrontBase >> 3));
        /* GESCRBASE */
        SetReg2DAGP(0x30, (nBackBase >> 3));
        /* GEPITCH */
        SetReg2DAGP(0x38, (((nFrontPitch >> 3) << 16) & 0x7FF0000) | 0x80000000 |
                          ((nBackPitch >> 3) & 0x7FF));
	/* BITBLT */
	SetReg2DAGP(0x0, 0x1 | 0xCC000000);
	b++;
    }

    viaFlushPrimsLocked(vmesa);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "Do Swap Buffer\n");
#endif
}

void viaDoSwapBuffersSaam(viaContextPtr vmesa)
{    
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*56);
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontWidth, nFrontHeight, nBackWidth, nBackHeight;
    GLuint nFrontBase, nBackBase;
    GLuint nFrontOffsetX, nFrontOffsetY, nBackOffsetX, nBackOffsetY;
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint i;
    
    nFrontPitch = vmesa->front.pitch;
    nBackPitch = vmesa->back.pitch;
    
    /* Caculate Base */
    nFrontBase = vmesa->viaScreen->fbSize + (vmesa->drawYSaam * nFrontPitch + vmesa->drawXSaam * bytePerPixel);
    nBackBase = vmesa->back.offset;
    /* 128 bit alignment*/
    nFrontBase = nFrontBase & 0xffffffe0;
    
    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	return;
    
    for (i = 0; i < vmesa->sarea->nbox; i++) {        
	/* Width, Height */
        nFrontWidth = nBackWidth = b->x2 - b->x1 - 1;
	nFrontHeight = nBackHeight = b->y2 - b->y1 - 1;
	/* Offset */
	nFrontOffsetX = (b->x1 - vmesa->drawXSaam) + vmesa->drawXoff;
	nFrontOffsetY = b->y1 - vmesa->drawYSaam;
	
	nBackOffsetX = nFrontOffsetX;
	nBackOffsetY = nFrontOffsetY;
	/* GEWD */
	SetReg2DAGP(0x10, nFrontWidth | (nFrontHeight << 16));
        /* GEDST */
        SetReg2DAGP(0x0C, nFrontOffsetX | (nFrontOffsetY << 16));
        /* GESRC */
        SetReg2DAGP(0x08, nBackOffsetX | (nBackOffsetY << 16));
        /* GEDSTBASE */
        SetReg2DAGP(0x34, (nFrontBase >> 3));
        /* GESCRBASE */
        SetReg2DAGP(0x30, (nBackBase >> 3));
        /* GEPITCH */
        SetReg2DAGP(0x38, (((nFrontPitch >> 3) << 16) & 0x7FF0000) | 0x80000000 |
                          ((nBackPitch >> 3) & 0x7FF));
	/* BITBLT */
	SetReg2DAGP(0x0, 0x1 | 0xCC000000);
	b++;
    }

    viaFlushPrimsLocked(vmesa);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "Do Swap Buffer\n");
#endif
}

void viaDoSwapPBuffers(viaContextPtr vmesa)
{    
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*56);
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontWidth, nFrontHeight;
    GLuint nFrontBase, nBackBase;
    GLuint nFrontOffsetX, nFrontOffsetY, nBackOffsetX, nBackOffsetY;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    
    nFrontPitch = vmesa->front.pitch;
    nBackPitch = vmesa->back.pitch;
    
    /* Caculate Base */
    nFrontBase = vmesa->front.offset;
    nBackBase = vmesa->back.offset;
    
    /* Width, Height */
    nFrontWidth = nFrontPitch / bytePerPixel;
    nFrontHeight = nBackPitch / bytePerPixel;
    
    /* Offset */
    nFrontOffsetX = 0;
    nFrontOffsetY = 0;
    nBackOffsetX = nFrontOffsetX;
    nBackOffsetY = nFrontOffsetY;
    /* GEWD */
    SetReg2DAGP(0x10, nFrontWidth | (nFrontHeight << 16));
    /* GEDST */
    SetReg2DAGP(0x0C, nFrontOffsetX | (nFrontOffsetY << 16));
    /* GESRC */
    SetReg2DAGP(0x08, nBackOffsetX | (nBackOffsetY << 16));
    /* GEDSTBASE */
    SetReg2DAGP(0x34, (nFrontBase >> 3));
    /* GESCRBASE */
    SetReg2DAGP(0x30, (nBackBase >> 3));
    /* GEPITCH */
    SetReg2DAGP(0x38, (((nFrontPitch >> 3) << 16) & 0x7FF0000) | 0x80000000 |
                      ((nBackPitch >> 3) & 0x7FF));
    /* BITBLT */
    SetReg2DAGP(0x0, 0x1 | 0xCC000000);

    viaFlushPrimsLocked(vmesa);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "Do Swap PBuffer\n");
#endif
}

void viaDoSwapBufferSoft(viaContextPtr vmesa)
{    
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontBase, nBackBase;
    GLuint i, j;
    GLubyte *by, *fy;
    GLuint w;
    
    w = vmesa->viaScreen->width;
    w = BUFFER_ALIGN_WIDTH(w, BUFFER_ALIGNMENT);
    
    if (vmesa->viaScreen->bitsPerPixel == 0x20)
	nFrontPitch = w << 2;
    else
	nFrontPitch = w << 1;
    
    nBackPitch = vmesa->back.pitch;
    
    /* Caculate Base */
    nFrontBase = (GLuint) vmesa->driScreen->pFB;
    nBackBase = ((GLuint) vmesa->back.offset) + ((GLuint) vmesa->driScreen->pFB);
    by = (GLubyte *) nBackBase;
    fy = (GLubyte *) nFrontBase;
    
    viaFlushPrimsLocked(vmesa);
    
    for (i = 0; i < vmesa->driDrawable->h; i++) {
	fy = (GLubyte *)(nFrontBase + i * nFrontPitch);
	for (j = 0; j < nBackPitch; j++) {
	    *((GLubyte*)fy) = *((GLubyte*)by);
	    fy = fy + 1;
	    by = by + 1;
	}
	
    }

}

void viaDoSwapBufferSoftFront(viaContextPtr vmesa)
{    
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontBase, nBackBase;
    GLuint i, j;
    GLubyte *by, *fy;
    GLuint w;
    
    w = vmesa->viaScreen->width;
    w = BUFFER_ALIGN_WIDTH(w, BUFFER_ALIGNMENT);
    
    if (vmesa->viaScreen->bitsPerPixel == 0x20)
	nFrontPitch = w << 2;
    else
	nFrontPitch = w << 1;
	
    nBackPitch = vmesa->front.pitch;
    
    /* Caculate Base */
    nFrontBase = (GLuint) vmesa->driScreen->pFB;
    nBackBase = ((GLuint) vmesa->front.offset) + ((GLuint) vmesa->driScreen->pFB);
    by = (GLubyte *) nBackBase;
    fy = (GLubyte *) nFrontBase;
    
    viaFlushPrimsLocked(vmesa);
    
    for (i = 0; i < vmesa->driDrawable->h; i++) {
	fy = (GLubyte *)(nFrontBase + i * nFrontPitch);
	for (j = 0; j < nBackPitch; j++) {
	    *((GLubyte*)fy) = *((GLubyte*)by);
	    fy = fy + 1;
	    by = by + 1;
	}
	
    }

}
int flush_agp(viaContextPtr vmesa, drm_via_flush_agp_t* agpCmd) 
{   
    GLuint *pnAGPCurrentPhysStart;
    GLuint *pnAGPCurrentPhysEnd;
    GLuint *pnAGPCurrentStart;
    GLuint *pnAGPCurrentEnd;
    volatile GLuint *pnMMIOBase;
    volatile GLuint *pnEngBaseTranSet;
    volatile GLuint *pnEngBaseTranSpace;
    GLuint *agpBase;
    GLuint ofs = vmesa->dma[vmesa->dmaIndex].offset;
    GLuint *vb = (GLuint *)vmesa->dmaAddr; 
    GLuint i = 0;
    
    pnMMIOBase = vmesa->regMMIOBase;
    pnEngBaseTranSet = vmesa->regTranSet;
    pnEngBaseTranSpace = vmesa->regTranSpace;
    *pnEngBaseTranSet = (0x0010 << 16);
    agpBase = vmesa->agpBase;

    if (!agpCmd->size) {
        return -1;
    }	

    {
        volatile GLuint *pnEngBase = vmesa->regEngineStatus;
        int nStatus;
	
        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000)
		break;
	    i++;
        }
    }

    pnAGPCurrentStart = (GLuint *)(ofs + agpCmd->offset);
    pnAGPCurrentEnd = (GLuint *)((GLuint)pnAGPCurrentStart + vmesa->dmaHigh);
    pnAGPCurrentPhysStart = (GLuint *)( (GLuint)pnAGPCurrentStart + (GLuint)agpBase );
    pnAGPCurrentPhysEnd = (GLuint *)( (GLuint)pnAGPCurrentEnd + (GLuint)agpBase );

    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	vmesa->glCtx->Color._DrawDestMask = __GL_FRONT_BUFFER_MASK;
    
    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
	    *vb++ = (HC_SubA_HClipTB << 24) | 0x0;
	    *vb++ = (HC_SubA_HClipLR << 24) | 0x0;
	}
	else {
	    *vb++ = ((HC_SubA_HClipTB << 24) | (0x0 << 12) | vmesa->driDrawable->h);
	    *vb++ = ((HC_SubA_HClipLR << 24) | (vmesa->drawXoff << 12) | (vmesa->driDrawable->w + vmesa->drawXoff));
	}

	{    
    	    GLuint pitch, format, offset;
        
    	    if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	format = HC_HDBFM_ARGB8888;
    	    }           
    	    else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
        	format = HC_HDBFM_RGB565;
    	    }
    	    else
    	    {
    	    	return -1;
    	    }

	    offset = vmesa->back.offset;
	    pitch = vmesa->back.pitch;
	    
    	    *vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
	    *vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    	    *vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    	    *vb++ = 0xcccccccc;
	}
	
	*pnEngBaseTranSpace = (HC_SubA_HAGPBstL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPBendL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpH << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpL << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFFFFFF) |
    	    HC_HAGPBpID_STOP;
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
                ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
                ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24 |
                HC_HAGPCMNT_MASK;
    }
    else {
	GLuint *head;
	GLuint clipL, clipR, clipT, clipB;
	drm_clip_rect_t *b = vmesa->sarea->boxes;
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	head = vb;
	
	*pnEngBaseTranSpace = (HC_SubA_HAGPBstL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPBendL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpH << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpL << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFFFFFF) |
    	    HC_HAGPBpID_STOP;
	
	for (i = 0; i < vmesa->sarea->nbox; i++) {        
	    if (1) {
    		volatile GLuint *pnEngBase = vmesa->regEngineStatus;
    		int nStatus;
		
    		while (1) {
        	    nStatus = *pnEngBase;
		    if ((nStatus & 0xFFFEFFFF) == 0x00020000)
			break;
    		}
	    }
	    
	    clipL = b->x1 + vmesa->drawXoff;
	    clipR = b->x2;	    
	    clipT = b->y1;
	    clipB = b->y2;
#ifdef DEBUG
	    if (VIA_DEBUG) fprintf(stderr, "clip = %d\n", i);
#endif
	    if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {

		*vb = (HC_SubA_HClipTB << 24) | 0x0;
		vb++;
		
		*vb = (HC_SubA_HClipLR << 24) | 0x0;
		vb++;
	    }
	    else {
		*vb = (HC_SubA_HClipTB << 24) | (clipT << 12) | clipB;
		vb++;
		
		*vb = (HC_SubA_HClipLR << 24) | (clipL << 12) | clipR;
		vb++;
	    }

	    {    
    		GLuint pitch, format, offset;
    		GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
        
    		if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	    format = HC_HDBFM_ARGB8888;
    		}           
    		else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
        	    format = HC_HDBFM_RGB565;
    		}           
    		else
    			return -1;
        
		pitch = vmesa->front.pitch;
		offset = vmesa->viaScreen->fbOffset + (vmesa->drawY * pitch + vmesa->drawX * bytePerPixel);
		offset = offset & 0xffffffe0;
            
    		*vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
		*vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    		*vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    		*vb++ = 0xcccccccc;
	    }
	    *pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
                ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
                ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24 |
                HC_HAGPCMNT_MASK;
#ifdef DEBUG		
	    if (VIA_DEBUG) {
		GLuint i;
		GLuint *data = (GLuint *)vmesa->dmaAddr;
		for (i = 0; i < vmesa->dmaLow; i += 16) {
        	    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x\n", *data++);
		}
		fprintf(stderr, "******************************************\n");
	    }
#endif
	    b++;	
	    vb = head;
	}
    }

#ifdef DEBUG
    if (VIA_DEBUG) {
        volatile GLuint *pnEngBase = (volatile GLuint *)((GLuint)pnMMIOBase + 0x400);
        int nStatus;
	int i = 0;

        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000) {
		break;
	    }
	    else {
		GLuint j;
		GLuint *data;
		/* dump current command buffer */
		data = (GLuint *)vmesa->dmaAddr;

		if (i == 500000) {
		    fprintf(stderr, "current command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < vmesa->dmaLow; j += 16) {
        	        fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
		/* dump previous command buffer */
		if (vmesa->dmaIndex) {
		    data = (GLuint *)vmesa->dma[0].map;
		}
		else {
		    data = (GLuint *)vmesa->dma[1].map;
		}
		if (i == 500000) {
		    fprintf(stderr, "previous command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < dmaLow; j += 16) {
        		fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
	    }
	    i++;
        }
    }
#endif
    dmaLow = vmesa->dmaLow; 
    return 0;
}

int flush_agp_saam(viaContextPtr vmesa, drm_via_flush_agp_t* agpCmd) 
{   
    GLuint *pnAGPCurrentPhysStart;
    GLuint *pnAGPCurrentPhysEnd;
    GLuint *pnAGPCurrentStart;
    GLuint *pnAGPCurrentEnd;
    volatile GLuint *pnMMIOBase;
    volatile GLuint *pnEngBaseTranSet;
    volatile GLuint *pnEngBaseTranSpace;
    GLuint *agpBase;
    GLuint ofs = vmesa->dma[vmesa->dmaIndex].offset;
    GLuint *vb = (GLuint *)vmesa->dmaAddr; 
    GLuint i = 0;
    
    pnMMIOBase = vmesa->regMMIOBase;
    pnEngBaseTranSet = vmesa->regTranSet;
    pnEngBaseTranSpace = vmesa->regTranSpace;
    *pnEngBaseTranSet = (0x0010 << 16);
    agpBase = vmesa->agpBase;

    if (!agpCmd->size) {
        return -1;
    }	

    {
        volatile GLuint *pnEngBase = vmesa->regEngineStatus;
        int nStatus;
	
        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000)
		break;
	    i++;
        }
    }

    pnAGPCurrentStart = (GLuint *)(ofs + agpCmd->offset);
    pnAGPCurrentEnd = (GLuint *)((GLuint)pnAGPCurrentStart + vmesa->dmaHigh);
    pnAGPCurrentPhysStart = (GLuint *)( (GLuint)pnAGPCurrentStart + (GLuint)agpBase );
    pnAGPCurrentPhysEnd = (GLuint *)( (GLuint)pnAGPCurrentEnd + (GLuint)agpBase );

    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	vmesa->glCtx->Color._DrawDestMask = __GL_FRONT_BUFFER_MASK;
    
    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
	    *vb++ = (HC_SubA_HClipTB << 24) | 0x0;
	    *vb++ = (HC_SubA_HClipLR << 24) | 0x0;
	}
	else {
	    *vb++ = ((HC_SubA_HClipTB << 24) | (0x0 << 12) | vmesa->driDrawable->h);
	    *vb++ = ((HC_SubA_HClipLR << 24) | (vmesa->drawXoff << 12) | (vmesa->driDrawable->w + vmesa->drawXoff));
	}

	{    
    	    GLuint pitch, format, offset;
        
    	    if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	format = HC_HDBFM_ARGB8888;
    	    }           
    	    else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
        	format = HC_HDBFM_RGB565;
    	    }           
    	    else
    	    	return -1;
        
    	    offset = vmesa->back.offset;
	    pitch = vmesa->back.pitch;
            
    	    *vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
	    *vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    	    *vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    	    *vb++ = 0xcccccccc;
	}
	
	*pnEngBaseTranSpace = (HC_SubA_HAGPBstL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPBendL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpH << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpL << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFFFFFF) |
    	    HC_HAGPBpID_STOP;
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
                ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
                ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24 |
                HC_HAGPCMNT_MASK;
    }
    else {
	GLuint *head;
	GLuint clipL, clipR, clipT, clipB;
	drm_clip_rect_t *b = vmesa->sarea->boxes;
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	head = vb;
	
	*pnEngBaseTranSpace = (HC_SubA_HAGPBstL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPBendL << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFFFFFF);
	*pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
    	    ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
    	    ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpH << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFF000000) >> 24;
	*pnEngBaseTranSpace = (HC_SubA_HAGPBpL << 24) |
    	    ((GLuint)((GLbyte *)pnAGPCurrentPhysStart + agpCmd->size - 4) & 0xFFFFFF) |
    	    HC_HAGPBpID_STOP;
	
	for (i = 0; i < vmesa->sarea->nbox; i++) {        
	    if (1) {
    		volatile GLuint *pnEngBase = vmesa->regEngineStatus;
    		int nStatus;
		
    		while (1) {
        	    nStatus = *pnEngBase;
		    if ((nStatus & 0xFFFEFFFF) == 0x00020000)
			break;
    		}
	    }
	    
	    clipL = b->x1 + vmesa->drawXoff;
	    clipR = b->x2;
	    clipT = b->y1;
	    clipB = b->y2;
#ifdef DEBUG
	    if (VIA_DEBUG) fprintf(stderr, "clip = %d\n", i);
#endif
	    if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {

		*vb = (HC_SubA_HClipTB << 24) | 0x0;
		vb++;
		
		*vb = (HC_SubA_HClipLR << 24) | 0x0;
		vb++;
	    }
	    else {
		*vb = (HC_SubA_HClipTB << 24) | (clipT << 12) | clipB;
		vb++;
		
		*vb = (HC_SubA_HClipLR << 24) | (clipL << 12) | clipR;
		vb++;
	    }

	    {    
    		GLuint pitch, format, offset;
    		GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
        
    		if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	    format = HC_HDBFM_ARGB8888;
    		}           
    		else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
        	    format = HC_HDBFM_RGB565;
    		}           
    		else
    			return -1;
        
		pitch = vmesa->front.pitch;
		offset = vmesa->viaScreen->fbOffset + (vmesa->drawY * pitch + vmesa->drawX * bytePerPixel);
		offset = offset & 0xffffffe0;
            
    		*vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
		*vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    		*vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    		*vb++ = 0xcccccccc;
	    }
	    *pnEngBaseTranSpace = (HC_SubA_HAGPCMNT << 24) |
                ((GLuint)(pnAGPCurrentPhysEnd) & 0xFF000000) >> 16 |
                ((GLuint)(pnAGPCurrentPhysStart) & 0xFF000000) >> 24 |
                HC_HAGPCMNT_MASK;
#ifdef DEBUG		
	    if (VIA_DEBUG) {
		GLuint i;
		GLuint *data = (GLuint *)vmesa->dmaAddr;
		for (i = 0; i < vmesa->dmaLow; i += 16) {
        	    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x  ", *data++);
		    fprintf(stderr, "%08x\n", *data++);
		}
		fprintf(stderr, "******************************************\n");
	    }
#endif
	    b++;	
	    vb = head;
	}
    }
    
#ifdef DEBUG
    if (VIA_DEBUG) {
        volatile GLuint *pnEngBase = (GLuint *)((GLuint)pnMMIOBase + 0x400);
        int nStatus;
	int i = 0;

        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000) {
		break;
	    }
	    else {
		GLuint j;
		GLuint *data;
		/* dump current command buffer */
		data = (GLuint *)vmesa->dmaAddr;

		if (i == 500000) {
		    fprintf(stderr, "current command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < vmesa->dmaLow; j += 16) {
        	        fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
		/* dump previous command buffer */
		if (vmesa->dmaIndex) {
		    data = (GLuint *)vmesa->dma[0].map;
		}
		else {
		    data = (GLuint *)vmesa->dma[1].map;
		}
		if (i == 500000) {
		    fprintf(stderr, "previous command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < dmaLow; j += 16) {
        		fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
	    }
	    i++;
        }
    }
#endif
    dmaLow = vmesa->dmaLow; 
    return 0;
}

int flush_sys(viaContextPtr vmesa, drm_via_flush_sys_t* buf) 

{
    GLuint *pnBuf;
    GLuint *pnEnd;
    volatile GLuint *pnMMIOBase;
    volatile GLuint *pnEngBaseTranSet;
    volatile GLuint *pnEngBaseTranSpace;
    GLuint uCheck2DCmd = GL_TRUE;
    GLuint addr;
    GLuint *vb = (GLuint *)vmesa->dmaAddr; 
    GLuint i = 0;
    
    pnMMIOBase = vmesa->regMMIOBase;
    pnEngBaseTranSet = vmesa->regTranSet;
    pnEngBaseTranSpace = vmesa->regTranSpace;
    
    pnBuf = (GLuint *)(buf->index + buf->offset);
    pnEnd = (GLuint *)((GLuint)pnBuf + buf->size);	
    
    /*=* [DBG] make draw to front buffer *=*/
    if(DRAW_FRONT)
	vmesa->glCtx->Color._DrawDestMask = __GL_FRONT_BUFFER_MASK;
    
    
    /*=* John Sheng [2003.6.20] fix pci *=*/
    {
        volatile GLuint *pnEngBase = vmesa->regEngineStatus;
        int nStatus;
	
        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000)
		break;
	    i++;
        }
    }
    /*=* John Sheng [2003.12.9] Tuxracer & VQ *=*/
    /*=* Disable VQ *=*/
    if (vmesa->VQEnable)
    {
	WAIT_IDLE
	*vmesa->regTranSet = 0x00fe0000;
	*vmesa->regTranSet = 0x00fe0000;
	*vmesa->regTranSpace = 0x00000004;
	*vmesa->regTranSpace = 0x40008c0f;
	*vmesa->regTranSpace = 0x44000000;
	*vmesa->regTranSpace = 0x45080c04;
	*vmesa->regTranSpace = 0x46800408;
	vmesa->VQEnable = 0;
    }
    if (vmesa->glCtx->Color._DrawDestMask == __GL_BACK_BUFFER_MASK) {
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	
	if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
	    *vb++ = (HC_SubA_HClipTB << 24) | 0x0;
	    *vb++ = (HC_SubA_HClipLR << 24) | 0x0;
	}
	else {
	    *vb++ = ((HC_SubA_HClipTB << 24) | (0x0 << 12) | vmesa->driDrawable->h);
	    *vb++ = ((HC_SubA_HClipLR << 24) | (vmesa->drawXoff << 12) | (vmesa->driDrawable->w + vmesa->drawXoff));
	}
	
	/*=* John Sheng [2003.6.16] fix pci path *=*/
	{    
    	    GLuint pitch, format, offset;
        
    	    if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	format = HC_HDBFM_ARGB8888;
    	    }           
    	    else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
        	format = HC_HDBFM_RGB565;
    	    }           
    	    else
    	    	return -1;

	    offset = vmesa->back.offset;
	    pitch = vmesa->back.pitch;
	    
    	    *vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
	    *vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    	    *vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    	    *vb++ = 0xcccccccc;
	}
	
	while (pnBuf != pnEnd) {	
	    if (*pnBuf == HALCYON_HEADER2) {
		pnBuf++;
		if (*pnBuf == HALCYON_SUB_ADDR0) {
		    *pnEngBaseTranSet = *pnBuf;
		    pnBuf++;
		    uCheck2DCmd = GL_FALSE;
		}
		else {
		    *pnEngBaseTranSet = *pnBuf;
		    pnBuf++;
    		    uCheck2DCmd = GL_TRUE;
		}
	    }
	    else if (uCheck2DCmd && ((*pnBuf&HALCYON_HEADER1MASK)==HALCYON_HEADER1)) {
		addr = ((*pnBuf)&0x0000001f) << 2;
		pnBuf++;
		*((GLuint*)((GLuint)pnMMIOBase+addr)) = *pnBuf;
		pnBuf++;
	    }
	    else if ((*pnBuf&HALCYON_FIREMASK) == HALCYON_FIRECMD) {
		*pnEngBaseTranSpace = *pnBuf;
		pnBuf++;
		if ((pnBuf!=pnEnd)&&((*pnBuf&HALCYON_FIREMASK)==HALCYON_FIRECMD))
		    pnBuf++;
		if ((*pnBuf&HALCYON_CMDBMASK) != HC_ACMD_HCmdB)
		    uCheck2DCmd = GL_TRUE;
	    }
	    else {
		*pnEngBaseTranSpace = *pnBuf;
		pnBuf++;
	    }
	}
    }
    else {
	GLuint *head;
	GLuint clipL, clipR, clipT, clipB;
	drm_clip_rect_t *b = vmesa->sarea->boxes;
	GLuint *pnTmp;
	
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	
	head = vb;
	pnTmp = pnBuf;
		
	for (i = 0; i < vmesa->sarea->nbox; i++) {        
	    clipL = b->x1 + vmesa->drawXoff;
	    clipR = b->x2 + vmesa->drawXoff;
	    clipT = b->y1;
	    clipB = b->y2;
	    
	    if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
		*vb = (HC_SubA_HClipTB << 24) | 0x0;
		vb++;
		*vb = (HC_SubA_HClipLR << 24) | 0x0;
		vb++;
	    }
	    else {
		*vb = (HC_SubA_HClipTB << 24) | (clipT << 12) | clipB;
		vb++;
		*vb = (HC_SubA_HClipLR << 24) | (clipL << 12) | clipR;
		vb++;
	    }
	    
	    /*=* John Sheng [2003.6.16] fix pci path *=*/
	    {    
    		GLuint pitch, format, offset;
    		GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
        
    		if (vmesa->viaScreen->bitsPerPixel == 0x20) {
        	    format = HC_HDBFM_ARGB8888;
    		}           
    		else if (vmesa->viaScreen->bitsPerPixel == 0x10) {
    	    	    format = HC_HDBFM_RGB565;
    		}           
    		else
    			return -1;
        
		pitch = vmesa->front.pitch;
		offset = vmesa->viaScreen->fbOffset + (vmesa->drawY * pitch + vmesa->drawX * bytePerPixel);
		offset = offset & 0xffffffe0;
            
    		*vb++ = ((HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF));
		*vb++ = ((HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000)) >> 24);      
    		*vb++ = ((HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch);            
    		*vb++ = 0xcccccccc;
	    }
	    
	    pnBuf = pnTmp;
	    
	    while (pnBuf != pnEnd) {	
		if (*pnBuf == HALCYON_HEADER2) {
		    pnBuf++;
		    if (*pnBuf == HALCYON_SUB_ADDR0) {
			*pnEngBaseTranSet = *pnBuf;
			pnBuf++;
			uCheck2DCmd = GL_FALSE;
		    }
		    else {
			*pnEngBaseTranSet = *pnBuf;
			pnBuf++;
    			uCheck2DCmd = GL_TRUE;
		    }
		}
		else if (uCheck2DCmd && ((*pnBuf&HALCYON_HEADER1MASK)==HALCYON_HEADER1)) {
		    addr = ((*pnBuf)&0x0000001f) << 2;
		    pnBuf++;
		    *((GLuint*)((GLuint)pnMMIOBase+addr)) = *pnBuf;
		    pnBuf++;
		}
		else if ((*pnBuf&HALCYON_FIREMASK) == HALCYON_FIRECMD) {
		    *pnEngBaseTranSpace = *pnBuf;
		    pnBuf++;
		    if ((pnBuf!=pnEnd)&&((*pnBuf&HALCYON_FIREMASK)==HALCYON_FIRECMD))
			pnBuf++;
		    if ((*pnBuf&HALCYON_CMDBMASK) != HC_ACMD_HCmdB)
			uCheck2DCmd = GL_TRUE;
		}
		else {
		    *pnEngBaseTranSpace = *pnBuf;
		    pnBuf++;
		}
	    }
	    b++;	
	    vb = head;
	}
    }
    /*=* John Sheng [2003.6.20] debug pci *=*/
    if (VIA_DEBUG) {
        GLuint *pnEngBase = (GLuint *)((GLuint)pnMMIOBase + 0x400);
        int nStatus;
	int i = 0;

        while (1) {
            nStatus = *pnEngBase;
	    if ((nStatus & 0xFFFEFFFF) == 0x00020000) {
		break;
	    }
	    else {
		GLuint j;
		GLuint *data;
		/*=* John Sheng [2003.12.9] Tuxracer & VQ *=*/
		GLuint k;
		GLuint *ES;

		data = (GLuint *)vmesa->dmaAddr;
		ES = pnEngBase;

		if (i == 500000) {
		    /*=* John Sheng [2003.12.9] Tuxracer & VQ *=*/
		    for (k =0 ; k < 35; k++) {
			fprintf(stderr, "%02xh - %02xh\n", k*4 + 3, k*4);
			fprintf(stderr, "%08x\n", *ES);
			ES++;
		    }
		    fprintf(stderr, "current command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < vmesa->dmaLow; j += 16) {
        	        fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
		if (vmesa->dmaIndex) {
		    data = (GLuint *)vmesa->dma[0].map;
		}
		else {
		    data = (GLuint *)vmesa->dma[1].map;
		}
		if (i == 500000) {
		    fprintf(stderr, "previous command buffer");
		    fprintf(stderr, "i = %d\n", i);
		    for (j = 0; j < dmaLow; j += 16) {
        		fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x  ", *data++);
			fprintf(stderr, "%08x\n", *data++);
		    }
		}
	    }
	    i++;
        }
    }
    dmaLow = vmesa->dmaLow; 
    return 0;
}
