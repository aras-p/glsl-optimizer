/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
v * copy of this software and associated documentation files (the "Software"),
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

#include "vblank.h"
#include "drm.h"
#include "xf86drm.h"
#include <sys/ioctl.h>
#include <errno.h>


#define VIA_REG_STATUS          0x400
#define VIA_REG_GEMODE          0x004
#define VIA_REG_SRCBASE         0x030
#define VIA_REG_DSTBASE         0x034
#define VIA_REG_PITCH           0x038      
#define VIA_REG_SRCCOLORKEY     0x01C      
#define VIA_REG_KEYCONTROL      0x02C       
#define VIA_REG_SRCPOS          0x008
#define VIA_REG_DSTPOS          0x00C
#define VIA_REG_GECMD           0x000
#define VIA_REG_DIMENSION       0x010       /* width and height */
#define VIA_REG_FGCOLOR         0x018

#define VIA_GEM_8bpp            0x00000000
#define VIA_GEM_16bpp           0x00000100
#define VIA_GEM_32bpp           0x00000300
#define VIA_GEC_BLT             0x00000001
#define VIA_PITCH_ENABLE        0x80000000
#define VIA_GEC_INCX            0x00000000
#define VIA_GEC_DECY            0x00004000
#define VIA_GEC_INCY            0x00000000
#define VIA_GEC_DECX            0x00008000
#define VIA_GEC_FIXCOLOR_PAT    0x00002000


#define VIA_BLIT_CLEAR 0x00
#define VIA_BLIT_COPY 0xCC
#define VIA_BLIT_FILL 0xF0
#define VIA_BLIT_SET 0xFF
#define VIA_BLITSIZE 96


typedef enum {VIABLIT_TRANSCOPY, VIABLIT_COPY, VIABLIT_FILL} ViaBlitOps;


/*=* John Sheng [2003.5.31] flip *=*/
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
   GLuint i = 0;
   GLuint clear_depth_mask = 0xf << 28;
   GLuint clear_depth = 0;

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
      flag |= VIA_DEPTH;
      clear_depth = (GLuint)(ctx->Depth.Clear * vmesa->ClearDepth);
      clear_depth_mask &= ~vmesa->depth_clear_mask;
      mask &= ~DD_DEPTH_BIT;
   }
    
   if (mask & DD_STENCIL_BIT) {
      if (vmesa->have_hw_stencil) {
	 if (ctx->Stencil.WriteMask[0] == 0xff) {
	    flag |= VIA_DEPTH;
	    clear_depth &= ~0xff;
	    clear_depth |= (ctx->Stencil.Clear & 0xff);
	    clear_depth_mask &= ~vmesa->stencil_clear_mask;
	    mask &= ~DD_STENCIL_BIT;
	 }
	 else {
	    fprintf(stderr, "XXX: Clear stencil writemask %x -- need triangles (or a ROP?)\n", 
		    ctx->Stencil.WriteMask[0]);
	    /* Fixme - clear with triangles */
	 }
      }
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
	    
      if (flag & VIA_FRONT) {
	 if (vmesa->drawType == GLX_PBUFFER_BIT)
	    viaFillFrontPBuffer(vmesa);
	 else
	    viaFillFrontBuffer(vmesa);
      } 
		
      if (flag & VIA_BACK) {
	 viaFillBackBuffer(vmesa);
      }

      if (flag & VIA_DEPTH) {
	 viaFillDepthBuffer(vmesa, clear_depth, clear_depth_mask);
      }		

      UNLOCK_HARDWARE(vmesa);
   }
   
   if (mask)
      _swrast_Clear(ctx, mask, all, cx, cy, cw, ch);
   if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
}

/*
 * Copy the back buffer to the front buffer. 
 */
void viaCopyBuffer(const __DRIdrawablePrivate *dPriv)
{
   viaContextPtr vmesa;
   drm_clip_rect_t *pbox;
   int nbox, i;
   GLboolean missed_target;
   int64_t ust;

   if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);        
   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   vmesa = (viaContextPtr)dPriv->driContextPriv->driverPrivate;
    
   VIA_FIREVERTICES(vmesa);

   driWaitForVBlank( dPriv, & vmesa->vbl_seq, vmesa->vblank_flags, & missed_target );
   LOCK_HARDWARE(vmesa);
    
   pbox = vmesa->pClipRects;
   nbox = vmesa->numClipRects;

   if (VIA_DEBUG) fprintf(stderr, "%s %d cliprects (%d)\n", 
			  __FUNCTION__, nbox, vmesa->drawType);
    
	
   if (vmesa->drawType == GLX_PBUFFER_BIT) {
      viaDoSwapPBuffers(vmesa);
      if (VIA_DEBUG) fprintf(stderr, "%s SwapPBuffers\n", __FUNCTION__);    
   }
   else {
      for (i = 0; i < nbox; ) {
	 int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
	 drm_clip_rect_t *b = (drm_clip_rect_t *)vmesa->sarea->boxes;

	 vmesa->sarea->nbox = nr - i;

	 for (; i < nr; i++)
	    *b++ = pbox[i];
	 viaDoSwapBuffers(vmesa);
	 if (VIA_DEBUG) fprintf(stderr, "%s SwapBuffers\n", __FUNCTION__);    
      }
   }
   UNLOCK_HARDWARE(vmesa);

   vmesa->swap_count++;
   (*vmesa->get_ust)( & ust );
   if ( missed_target ) {
      vmesa->swap_missed_count++;
      vmesa->swap_missed_ust = ust - vmesa->swap_ust;
   }
    
   vmesa->swap_ust = ust;

   if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);        
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
    GLboolean missed_target;
    int retcode;

    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);        
    assert(dPriv);
    assert(dPriv->driContextPriv);
    assert(dPriv->driContextPriv->driverPrivate);

    ctx = vmesa->glCtx;
    
    VIA_FIREVERTICES(vmesa);

    /* Now wait for the vblank:
     */
    retcode = driWaitForVBlank( dPriv, &vmesa->vbl_seq, 
				vmesa->vblank_flags, &missed_target );
    if ( missed_target ) {
       vmesa->swap_missed_count++;
       (void) (*vmesa->get_ust)( &vmesa->swap_missed_ust );
    }

    LOCK_HARDWARE(vmesa);

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
    	if (!vmesa->nDoneFirstFlip) {
	    *((volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x43c)) = 0x00fe0000;
	    *((volatile GLuint *)((GLuint)vmesa->regMMIOBase + 0x440)) = 0x00001004;
	    vmesa->nDoneFirstFlip = GL_TRUE;
	}
	*((GLuint *)((GLuint)vmesa->regMMIOBase + 0x214)) = nBackBase;
    }
    /* Auto Swap */
    else {
	viaFlushPrimsLocked(vmesa);
	vb = viaCheckDma(vmesa, 8 * 4);
	if (!vmesa->nDoneFirstFlip) {
    	    *vb++ = HALCYON_HEADER2;
    	    *vb++ = 0x00fe0000;
    	    *vb++ = 0x0000000e;
    	    *vb++ = 0x0000000e;
	    vmesa->dmaLow += 16;

    	    vmesa->nDoneFirstFlip = GL_FALSE;
	}
	nBackBase = (vmesa->back.offset );

	*vb++ = HALCYON_HEADER2;
	*vb++ = 0x00fe0000;
	*vb++ = (HC_SubA_HFBBasL << 24) | (nBackBase & 0xFFFFF8) | 0x2;
	*vb++ = (HC_SubA_HFBDrawFirst << 24) |
                          ((nBackBase & 0xFF000000) >> 24) | 0x0100;
	vmesa->dmaLow += 16;
	viaFlushPrimsLocked(vmesa);
    }
    
    UNLOCK_HARDWARE(vmesa);

    vmesa->swap_count++;
    (void) (*vmesa->get_ust)( &vmesa->swap_ust );
   
    memcpy(&buffer_tmp, &vmesa->back, sizeof(viaBuffer));
    memcpy(&vmesa->back, &vmesa->front, sizeof(viaBuffer));
    memcpy(&vmesa->front, &buffer_tmp, sizeof(viaBuffer));
    
    if(vmesa->currentPage) {
	vmesa->currentPage = 0;
	if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {
		ctx->Driver.DrawBuffer(ctx, GL_BACK);
	}
	else {
		ctx->Driver.DrawBuffer(ctx, GL_FRONT);
	}
    }
    else {
	vmesa->currentPage = 1;
	if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {
		ctx->Driver.DrawBuffer(ctx, GL_BACK);
	}
	else {
		ctx->Driver.DrawBuffer(ctx, GL_FRONT);
	}
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);        

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
   drm_via_flush_sys_t sysCmd;
   GLuint *vb = viaCheckDma(vmesa, 0);
   int i;

   if (*(GLuint *)vmesa->driHwLock != (DRM_LOCK_HELD|vmesa->hHWContext) &&
       *(GLuint *)vmesa->driHwLock != (DRM_LOCK_HELD|DRM_LOCK_CONT|vmesa->hHWContext))
      fprintf(stderr, "%s called without lock held\n", __FUNCTION__);

   if (vmesa->dmaLow == DMA_OFFSET) {
      return;
   }
   if (vmesa->dmaLow > (VIA_DMA_BUFSIZ - 256))
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
    
   sysCmd.offset = 0x0;
   sysCmd.size = vmesa->dmaLow;
   sysCmd.index = (GLuint)vmesa->dma;
   sysCmd.discard = 0;
    
   sarea->vertexPrim = vmesa->hwPrimitive;

   if (!nbox) {
      sysCmd.size = 0;
   }
   else if (nbox > VIA_NR_SAREA_CLIPRECTS) {
      /* XXX: not handled ? */
   }

   if (!nbox) {
      sarea->nbox = 0;
      sysCmd.discard = 1;
      flush_sys(vmesa, &sysCmd);
   }
   else {
      for (i = 0; i < nbox; ) {
	 int nr = MIN2(i + VIA_NR_SAREA_CLIPRECTS, nbox);
	 drm_clip_rect_t *b = sarea->boxes;

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
	       sysCmd.size = 0;
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

	 if (nr == nbox) {
	    sysCmd.discard = 1;
	    flush_sys(vmesa, &sysCmd);
	 }
      }
   }
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
   /* Reset vmesa vars:
    */
   vmesa->dmaLow = DMA_OFFSET;
   vmesa->dmaAddr = (unsigned char *)vmesa->dma;
   vmesa->dmaHigh = VIA_DMA_BUFSIZ;
}

void viaFlushPrims(viaContextPtr vmesa)
{
   if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);

   if (vmesa->dmaLow) {
      LOCK_HARDWARE(vmesa); 
      viaFlushPrimsLocked(vmesa);
      UNLOCK_HARDWARE(vmesa);
   }
   if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
}

static void viaFlush(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    VIA_FIREVERTICES(vmesa);
}

static void viaFinish(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    VIA_FIREVERTICES(vmesa);
    WAIT_IDLE(vmesa);
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



GLuint *viaBlit(viaContextPtr vmesa, GLuint bpp,GLuint srcBase,
		GLuint srcPitch,GLuint dstBase,GLuint dstPitch,
		GLuint w,GLuint h,int xdir,int ydir, GLuint blitMode, 
		GLuint color, GLuint nMask, GLuint *vb) 
{

    GLuint dwGEMode = 0, srcY=0, srcX, dstY=0, dstX;
    GLuint cmd;

    if (VIA_DEBUG)
       fprintf(stderr, "%s bpp %d src %x/%x dst %x/%x w %d h %d dir %d,%d mode: %x color: 0x%08x mask 0x%08x\n",
	       __FUNCTION__, bpp, srcBase, srcPitch, dstBase, dstPitch, w,h, xdir, ydir, blitMode, color, nMask);


    if (!w || !h)
        return vb;

    srcX = srcBase & 31;
    dstX = dstBase & 31;
    switch (bpp) {
    case 16:
        dwGEMode |= VIA_GEM_16bpp;
	srcX >>= 1;
	dstX >>= 1;
        break;
    case 32:
        dwGEMode |= VIA_GEM_32bpp;
	srcX >>= 2;
	dstX >>= 2;
	break;
    default:
        dwGEMode |= VIA_GEM_8bpp;
        break;
    }
    SetReg2DAGP(VIA_REG_GEMODE, dwGEMode);

    cmd = 0; 

    if (xdir < 0) {
        cmd |= VIA_GEC_DECX;
        srcX += (w - 1);
        dstX += (w - 1);
    }
    if (ydir < 0) {
        cmd |= VIA_GEC_DECY;
        srcY += (h - 1);
        dstY += (h - 1);
    }

    switch(blitMode) {
    case VIABLIT_TRANSCOPY:
	SetReg2DAGP( VIA_REG_SRCCOLORKEY, color);
	SetReg2DAGP( VIA_REG_KEYCONTROL, 0x4000);
	cmd |= VIA_GEC_BLT | (VIA_BLIT_COPY << 24);
	break;
    case VIABLIT_FILL:
	SetReg2DAGP( VIA_REG_FGCOLOR, color);
	cmd |= VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT | (VIA_BLIT_FILL << 24);
	break;
    default:
	SetReg2DAGP( VIA_REG_KEYCONTROL, 0x0);
	cmd |= VIA_GEC_BLT | (VIA_BLIT_COPY << 24);
    }	

    SetReg2DAGP( 0x2C, nMask);
    SetReg2DAGP( VIA_REG_SRCBASE, (srcBase & ~31) >> 3);
    SetReg2DAGP( VIA_REG_DSTBASE, (dstBase & ~31) >> 3);
    SetReg2DAGP( VIA_REG_PITCH, VIA_PITCH_ENABLE |
	       (srcPitch >> 3) | (((dstPitch) >> 3) << 16));
    SetReg2DAGP( VIA_REG_SRCPOS, ((srcY << 16) | srcX));
    SetReg2DAGP( VIA_REG_DSTPOS, ((dstY << 16) | dstX));
    SetReg2DAGP( VIA_REG_DIMENSION, (((h - 1) << 16) | (w - 1)));
    SetReg2DAGP( VIA_REG_GECMD, cmd);
    SetReg2DAGP( 0x2C, 0x00000000);
    return vb;
}

void viaFillFrontBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight,i; 
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*VIA_BLITSIZE);
    GLuint pixel = (GLuint)vmesa->ClearColor;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    nDestPitch = vmesa->front.pitch;

    for (i = 0; i < vmesa->sarea->nbox ; i++) {        
        nDestWidth = b->x2 - b->x1;
	nDestHeight = b->y2 - b->y1;
	nDestBase = vmesa->viaScreen->fbOffset + 
	    (b->y1* nDestPitch + b->x1 * bytePerPixel);
	vb = viaBlit(vmesa,vmesa->viaScreen->bitsPerPixel, nDestBase, nDestPitch,
		     nDestBase , nDestPitch, nDestWidth, nDestHeight,
		     0,0,VIABLIT_FILL, pixel, 0x0, vb); 
	b++;
    }

    viaFlushPrimsLocked(vmesa);
}

void viaFillFrontPBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offset; 
    GLuint *vb = viaCheckDma(vmesa, VIA_BLITSIZE);
    GLuint pixel = (GLuint)vmesa->ClearColor;

    offset = vmesa->front.offset;
    if (VIA_DEBUG) fprintf(stderr, "Fill PFront offset = %08x\n", offset);
    nDestBase = offset;
    nDestPitch = vmesa->front.pitch;

    nDestWidth = vmesa->driDrawable->w;
    nDestHeight = vmesa->driDrawable->h;

    viaBlit(vmesa,vmesa->viaScreen->bitsPerPixel, nDestBase, nDestPitch,
	    nDestBase , nDestPitch, nDestWidth, nDestHeight,
	    0,0,VIABLIT_FILL, pixel, 0x0, vb); 
	
    viaFlushPrimsLocked(vmesa);
}

void viaFillBackBuffer(viaContextPtr vmesa)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offset; 
    GLcontext *ctx = vmesa->glCtx;
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*VIA_BLITSIZE);
    GLuint pixel = (GLuint)vmesa->ClearColor;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;

    offset = vmesa->back.offset;
    if (VIA_DEBUG) fprintf(stderr, "Fill Back offset = %08x\n", offset);
    nDestBase = offset;
    nDestPitch = vmesa->back.pitch;
    
    if (!ctx->Scissor.Enabled) {
	nDestWidth = (vmesa->back.pitch / bytePerPixel);
	nDestHeight = vmesa->driDrawable->h;
	viaBlit(vmesa,vmesa->viaScreen->bitsPerPixel, nDestBase, nDestPitch,
		nDestBase , nDestPitch, nDestWidth, nDestHeight,
		0,0,VIABLIT_FILL, pixel, 0x0, vb); 
	
    }
    /*=* John Sheng [2003.7.18] texenv *=*/
    else {
	int i;
	drm_clip_rect_t *b = vmesa->sarea->boxes;
	for (i = 0; i < vmesa->sarea->nbox ; i++) {        
	    nDestWidth = b->x2 - b->x1;
	    nDestHeight = b->y2 - b->y1;
	    nDestBase = offset + ((b->y1 - vmesa->drawY) * nDestPitch) + 
		(b->x1 - vmesa->drawX + vmesa->drawXoff) * bytePerPixel;
	    vb = viaBlit(vmesa,vmesa->viaScreen->bitsPerPixel, nDestBase, nDestPitch,
			 nDestBase , nDestPitch, nDestWidth, nDestHeight,
			 0,0,VIABLIT_FILL, pixel, 0x0, vb); 
	    b++;
	}
    }
    if (VIA_DEBUG) {
	fprintf(stderr," width = %08x\n", nDestWidth);	
	fprintf(stderr," height = %08x\n", nDestHeight);	
    }	     
}


void viaFillDepthBuffer(viaContextPtr vmesa, GLuint pixel, GLuint mask)
{
    GLuint nDestBase, nDestPitch, nDestWidth, nDestHeight, offsetX, offset;
    GLuint *vb = viaCheckDma(vmesa, VIA_BLITSIZE);

    offset = vmesa->depth.offset;
    if (VIA_DEBUG)
       fprintf(stderr, "Fill Depth offset = %08x, pixel %x, mask %x\n", offset, pixel, mask);
    nDestBase = offset;
    nDestPitch = vmesa->depth.pitch;
    
    /* KW: bogus offset? */
/*     offsetX = vmesa->drawXoff; */
    offsetX = 0;

    nDestWidth = (vmesa->depth.pitch / vmesa->depth.bpp * 8) - offsetX;
    nDestHeight = vmesa->driDrawable->h;

    viaBlit(vmesa, vmesa->depth.bpp , nDestBase, nDestPitch,
	    nDestBase , nDestPitch, nDestWidth, nDestHeight,
	    0,0,VIABLIT_FILL, pixel, mask, vb); 

    
    if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {
	viaFlushPrimsLocked(vmesa);
    }
}

void viaDoSwapBuffers(viaContextPtr vmesa)
{    
    GLuint *vb = viaCheckDma(vmesa, vmesa->sarea->nbox*VIA_BLITSIZE );
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontWidth, nFrontHeight;
    GLuint nFrontBase, nBackBase;
    drm_clip_rect_t *b = vmesa->sarea->boxes;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint i;
    
    nFrontPitch = vmesa->front.pitch;
    nBackPitch = vmesa->back.pitch;
    
    for (i = 0; i < vmesa->sarea->nbox; i++) {        

	/* Width, Height */
        nFrontWidth = b->x2 - b->x1;
	nFrontHeight = b->y2 - b->y1;
	
	nFrontBase = vmesa->viaScreen->fbOffset + (b->y1* nFrontPitch + 
						   b->x1 * bytePerPixel);
	nBackBase = vmesa->back.offset + ((b->y1 - vmesa->drawY) * nBackPitch) + 
	    (b->x1 - vmesa->drawX + vmesa->drawXoff) * bytePerPixel;
	
	vb = viaBlit(vmesa, bytePerPixel << 3 , nBackBase, nBackPitch,
		     nFrontBase , nFrontPitch, nFrontWidth, nFrontHeight,
		     0,0,VIABLIT_COPY, 0, 0, vb); 
	b++;
    }

    viaFlushPrimsLocked(vmesa);
    if (VIA_DEBUG) fprintf(stderr, "Do Swap Buffer\n");
}


void viaDoSwapPBuffers(viaContextPtr vmesa)
{    
    GLuint *vb = viaCheckDma(vmesa, 64 );
    GLuint nFrontPitch;
    GLuint nBackPitch;
    GLuint nFrontWidth, nFrontHeight;
    GLuint nFrontBase, nBackBase;
    GLuint nFrontOffsetX, nFrontOffsetY, nBackOffsetX, nBackOffsetY;
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
    GLuint EngStatus = *(vmesa->pnGEMode);
    GLuint blitMode; 

    switch(bytePerPixel) {
    case 4:
	blitMode = 0x300;
	break;
    case 2:
	blitMode = 0x100;
	break;
    default:
	blitMode = 0x000;
	break;
    }

    /* Restore mode */
    SetReg2DAGP(0x04, (EngStatus & 0xFFFFFCFF) | blitMode);


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
    if (VIA_DEBUG) fprintf(stderr, "Do Swap PBuffer\n");
}


#define VIA_CMDBUF_MAX_LAG 50000

int flush_sys(viaContextPtr vmesa, drm_via_flush_sys_t* buf) 
{
    GLuint *pnBuf;
    GLuint *pnEnd;
    volatile GLuint *pnMMIOBase;
    GLuint *vb = (GLuint *)vmesa->dmaAddr; 
    GLuint i = 0;
    drmVIACommandBuffer bufI;
    drmVIACmdBufSize bSiz;
    int ret;

    pnMMIOBase = vmesa->regMMIOBase;
    
    pnBuf = (GLuint *)(buf->index + buf->offset);
    pnEnd = (GLuint *)((GLuint)pnBuf + buf->size);	
    
    if (vmesa->glCtx->Color._DrawDestMask[0] == DD_BACK_LEFT_BIT) {
	*vb++ = HC_HEADER2;
	*vb++ = (HC_ParaType_NotTex << 16);
	  
	if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
	    *vb++ = (HC_SubA_HClipTB << 24) | 0x0;
	    *vb++ = (HC_SubA_HClipLR << 24) | 0x0;
	}
	else {
	    *vb++ = ((HC_SubA_HClipTB << 24) | (0x0 << 12) | vmesa->driDrawable->h);
	    /* KW: drawXoff known zero */
/* 	    *vb++ = ((HC_SubA_HClipLR << 24) | (vmesa->drawXoff << 12) | (vmesa->driDrawable->w + vmesa->drawXoff)); */
 	    *vb++ = ((HC_SubA_HClipLR << 24) | vmesa->driDrawable->w); 
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
	
	bufI.buf = (char *) pnBuf;
	bufI.size = buf->size;

	/*
	 * If AGP is enabled, try it. Otherwise or if it fails, use PCI MMIO.
	 */
	
	ret = 1;
	if (vmesa->useAgp) {

	    /*
	     * Prevent command regulator from lagging to far behind by waiting.
	     * Otherwise some applications become unresponsive and jumpy.
	     * The VIA_CMDBUF_MAX_LAG size may be tuned. We could also wait for idle
	     * but that would severely lower performance of some very important
	     * applications. (for example glxgears :).
	     */

	    bSiz.func = VIA_CMDBUF_LAG;
	    bSiz.wait = 1;
	    bSiz.size = VIA_CMDBUF_MAX_LAG;
	    while ( -EAGAIN == (ret = drmCommandWriteRead(vmesa->driFd, DRM_VIA_CMDBUF_SIZE, 
							  &bSiz, sizeof(bSiz))));
	    if (ret) {
		_mesa_error(vmesa->glCtx, GL_INVALID_OPERATION, __FUNCTION__); 
		abort();
	    }
	    while ( -EAGAIN == (ret = drmCommandWrite(vmesa->driFd, DRM_VIA_CMDBUFFER, 
						      &bufI, sizeof(bufI))));
	    if (ret) {
	       abort();
	    }
	}
	if (ret) {
	    if (vmesa->useAgp) WAIT_IDLE(vmesa);
	    
/* 	    for (i = 0; */

	    if (drmCommandWrite(vmesa->driFd, DRM_VIA_PCICMD, &bufI, sizeof(bufI))) {
		_mesa_error(vmesa->glCtx, GL_INVALID_OPERATION, __FUNCTION__);
		abort();
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
	    clipR = b->x2 + vmesa->drawXoff; /* FIXME: is this correct? */
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
	    
	    bufI.buf = (char *) pnBuf;
	    bufI.size = buf->size;
	    ret = 1;
	    if (vmesa->useAgp) {
		bSiz.func = VIA_CMDBUF_LAG;
		bSiz.wait = 1;
		bSiz.size = VIA_CMDBUF_MAX_LAG;
		while ( -EAGAIN == (ret = drmCommandWriteRead(vmesa->driFd, DRM_VIA_CMDBUF_SIZE, 
							      &bSiz, sizeof(bSiz))));
		if (ret) {
		    _mesa_error(vmesa->glCtx, GL_INVALID_OPERATION, __FUNCTION__); 
		    abort();
		}
		while ( -EAGAIN == (ret = drmCommandWrite(vmesa->driFd, DRM_VIA_CMDBUFFER, 
							      &bufI, sizeof(bufI))));
		if (ret) {
		   abort();
		}
	    }
	    
	    if (ret) {
		if (vmesa->useAgp) 
		   WAIT_IDLE(vmesa);

	        if (drmCommandWrite(vmesa->driFd, DRM_VIA_PCICMD, &bufI, sizeof(bufI))) {
		    _mesa_error(vmesa->glCtx, GL_INVALID_OPERATION, __FUNCTION__);
		    abort();
		}
	    }

	    b++;	
	    vb = head;
	}

    }

    return 0;
}
