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
#include <unistd.h>
#include <sys/mman.h>

#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "context.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "savagecontext.h"
#include "savageioctl.h"
#include "savage_bci.h"
#include "savagedma.h"

#include "drm.h"
#include <sys/ioctl.h>
#include <sys/timeb.h>

extern GLuint bcicount;
#define DEPTH_SCALE_16 ((1<<16)-1)
#define DEPTH_SCALE_24 ((1<<24)-1)

static void savage_BCI_clear(GLcontext *ctx, drm_savage_clear_t *pclear)
{
	savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
	int nbox = imesa->sarea->nbox;
	drm_clip_rect_t *pbox = imesa->sarea->boxes;
        int i;

	
      	if (nbox > SAVAGE_NR_SAREA_CLIPRECTS)
     		nbox = SAVAGE_NR_SAREA_CLIPRECTS;

	for (i = 0 ; i < nbox ; i++, pbox++) {
		unsigned int x = pbox->x1;
		unsigned int y = pbox->y1;
		unsigned int width = pbox->x2 - x;
		unsigned int height = pbox->y2 - y;
 		uint32_t *bciptr;

		if (pbox->x1 > pbox->x2 ||
		    pbox->y1 > pbox->y2 ||
		    pbox->x2 > imesa->savageScreen->width ||
		    pbox->y2 > imesa->savageScreen->height)
			continue;

	   	if ( pclear->flags & SAVAGE_FRONT ) {
		        bciptr = savageDMAAlloc (imesa, 8);
			WRITE_CMD((bciptr) , 0x4BCC8C00,uint32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->frontOffset,uint32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->frontBitmapDesc,uint32_t);
			WRITE_CMD((bciptr) , pclear->clear_color,uint32_t);
			WRITE_CMD((bciptr) , (y <<16) | x,uint32_t);
			WRITE_CMD((bciptr) , (height << 16) | width,uint32_t);
			savageDMACommit (imesa, bciptr);
		}
		if ( pclear->flags & SAVAGE_BACK ) {
		        bciptr = savageDMAAlloc (imesa, 8);
			WRITE_CMD((bciptr) , 0x4BCC8C00,uint32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->backOffset,uint32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->backBitmapDesc,uint32_t);
			WRITE_CMD((bciptr) , pclear->clear_color,uint32_t);
			WRITE_CMD((bciptr) , (y <<16) | x,uint32_t);
			WRITE_CMD((bciptr) , (height << 16) | width,uint32_t);
			savageDMACommit (imesa, bciptr);
		}
		
		if ( pclear->flags & (SAVAGE_DEPTH |SAVAGE_STENCIL) ) {
		        uint32_t writeMask = 0x0;
#if HW_STENCIL		
		        if(imesa->hw_stencil)
		        {        
		            if(pclear->flags & SAVAGE_STENCIL)
		            {
		          
		                 writeMask |= 0xFF000000;
		            }
		            if(pclear->flags & SAVAGE_DEPTH)
		            {
		                 writeMask |= 0x00FFFFFF;
		            }
                        }
#endif
		        if(imesa->IsFullScreen && imesa->NotFirstFrame &&
			   imesa->savageScreen->chipset >= S3_SAVAGE4)
		        {
		            imesa->regs.s4.zBufCtrl.ni.autoZEnable = GL_TRUE;
                            imesa->regs.s4.zBufCtrl.ni.frameID =
				~imesa->regs.s4.zBufCtrl.ni.frameID;
                            
                            imesa->dirty |= SAVAGE_UPLOAD_CTX;
		        }
		        else
		        {
		            if(imesa->IsFullScreen)
		                imesa->NotFirstFrame = GL_TRUE;
		                
#if HW_STENCIL
			    if(imesa->hw_stencil)
			    {
				bciptr = savageDMAAlloc (imesa, 10);
			        if(writeMask != 0xFFFFFFFF)
			        {
                                    WRITE_CMD((bciptr) , 0x960100D7,uint32_t);
                                    WRITE_CMD((bciptr) , writeMask,uint32_t);
                                }
                            }
			    else
#endif              
			    {
				bciptr = savageDMAAlloc (imesa, 6);
			    }

			    WRITE_CMD((bciptr) , 0x4BCC8C00,uint32_t);
			    WRITE_CMD((bciptr) , imesa->savageScreen->depthOffset,uint32_t);
			    WRITE_CMD((bciptr) , imesa->savageScreen->depthBitmapDesc,uint32_t);
			    WRITE_CMD((bciptr) , pclear->clear_depth,uint32_t);
			    WRITE_CMD((bciptr) , (y <<16) | x,uint32_t);
			    WRITE_CMD((bciptr) , (height << 16) | width,uint32_t);
#if HW_STENCIL			    
			    if(imesa->hw_stencil)
			    {
			        if(writeMask != 0xFFFFFFFF)
			        {
			           WRITE_CMD((bciptr) , 0x960100D7,uint32_t);
                                   WRITE_CMD((bciptr) , 0xFFFFFFFF,uint32_t);  
			        }
			    }
#endif
			    savageDMACommit (imesa, bciptr);
			}
		}
	}
	/* FK: Make sure that the clear stuff is emitted. Otherwise a
	   software fallback may get overwritten by a delayed clear. */
	savageDMAFlush (imesa);
}

struct timeb a,b;

static void savage_BCI_swap(savageContextPtr imesa)
{
    int nbox = imesa->sarea->nbox;
    drm_clip_rect_t *pbox = imesa->sarea->boxes;
    int i;
    volatile uint32_t *bciptr;
    
    if (nbox > SAVAGE_NR_SAREA_CLIPRECTS)
        nbox = SAVAGE_NR_SAREA_CLIPRECTS;
    savageDMAFlush (imesa);
    
    if(imesa->IsFullScreen)
    { /* full screen*/
        unsigned int tmp0;
        tmp0 = imesa->savageScreen->frontOffset; 
        imesa->savageScreen->frontOffset = imesa->savageScreen->backOffset;
        imesa->savageScreen->backOffset = tmp0;
        
        if(imesa->toggle == TARGET_BACK)
            imesa->toggle = TARGET_FRONT;
        else
            imesa->toggle = TARGET_BACK; 
        
        imesa->drawMap = (char *)imesa->apertureBase[imesa->toggle];
        imesa->readMap = (char *)imesa->apertureBase[imesa->toggle];
        
        imesa->regs.s4.destCtrl.ni.offset = imesa->savageScreen->backOffset>>11;
        imesa->dirty |= SAVAGE_UPLOAD_CTX;
        bciptr = SAVAGE_GET_BCI_POINTER(imesa,3);
        *(bciptr) = 0x960100B0;
        *(bciptr) = (imesa->savageScreen->frontOffset); 
        *(bciptr) = 0xA0000000;
    } 
    
    else
    {  /* Use bitblt copy from back to front buffer*/
        
        for (i = 0 ; i < nbox; i++, pbox++)
        {
            unsigned int w = pbox->x2 - pbox->x1;
            unsigned int h = pbox->y2 - pbox->y1;
            
            if (pbox->x1 > pbox->x2 ||
                pbox->y1 > pbox->y2 ||
                pbox->x2 > imesa->savageScreen->width ||
                pbox->y2 > imesa->savageScreen->height)
                continue;

            bciptr = SAVAGE_GET_BCI_POINTER(imesa,6);
            
            *(bciptr) = 0x4BCC00C0;
            
            *(bciptr) = imesa->savageScreen->backOffset;
            *(bciptr) = imesa->savageScreen->backBitmapDesc;
            *(bciptr) = (pbox->y1 <<16) | pbox->x1;   /*x0, y0*/
            *(bciptr) = (pbox->y1 <<16) | pbox->x1;
            *(bciptr) = (h << 16) | w;
        }
        
    }
}


static void savageDDClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
			   GLint cx, GLint cy, GLint cw, GLint ch ) 
{
  savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   drm_savage_clear_t clear;
   int i;

   clear.flags = 0;
   clear.clear_color = imesa->ClearColor;

   if(imesa->savageScreen->zpp == 2)
       clear.clear_depth = (GLuint) (ctx->Depth.Clear * DEPTH_SCALE_16);
   else
       clear.clear_depth = (GLuint) (ctx->Depth.Clear * DEPTH_SCALE_24);

   FLUSH_BATCH( imesa );

   if ((mask & DD_FRONT_LEFT_BIT) && ((colorMask&0xffffffUL)==0xffffffUL) ){
      clear.flags |= SAVAGE_FRONT;
      mask &= ~DD_FRONT_LEFT_BIT;
   }

   if ((mask & DD_BACK_LEFT_BIT) && ((colorMask&0xffffffUL)==0xffffffUL) ) {
      clear.flags |= SAVAGE_BACK;
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if ((mask & DD_DEPTH_BIT) && ctx->Depth.Mask) {
      clear.flags |= SAVAGE_DEPTH;
      mask &= ~DD_DEPTH_BIT;
   }
   
   if((mask & DD_STENCIL_BIT) && imesa->hw_stencil)
   {
       clear.flags |= SAVAGE_STENCIL;
       mask &= ~DD_STENCIL_BIT;
   }

   if (clear.flags) {
       LOCK_HARDWARE( imesa );

       /* flip top to bottom */
       cy = dPriv->h-cy-ch;
       cx += imesa->drawX;
       cy += imesa->drawY;

       for (i = 0 ; i < imesa->numClipRects ; ) { 	 
	   int nr = MIN2(i + SAVAGE_NR_SAREA_CLIPRECTS, imesa->numClipRects);
	   drm_clip_rect_t *box = imesa->pClipRects;	 
	   drm_clip_rect_t *b = imesa->sarea->boxes;
	   int n = 0;

	   if (!all) {
	       for ( ; i < nr ; i++) {
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
	   } else {
	       for ( ; i < nr ; i++) {
		   *b++ = *(drm_clip_rect_t *)&box[i];
		   n++;
	       }
	   }

	   imesa->sarea->nbox = n;

	   savage_BCI_clear(ctx,&clear);
       }

       UNLOCK_HARDWARE( imesa );
       imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS|SAVAGE_UPLOAD_CTX;
   }

   if (mask) 
      _swrast_Clear( ctx, mask, all, cx, cy, cw, ch );
}




/*
 * Copy the back buffer to the front buffer. 
 */
void savageSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   savageContextPtr imesa;
   drm_clip_rect_t *pbox;
   int nbox;
   int i;

   GLboolean pending;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   imesa = (savageContextPtr) dPriv->driContextPriv->driverPrivate;
   if (imesa->IsDouble)
       _mesa_notifySwapBuffers( imesa->glCtx );

   FLUSH_BATCH(imesa);

   LOCK_HARDWARE( imesa );
   WAIT_IDLE_EMPTY;
   PAGE_PENDING(pending);

   if(!pending)
   {
   pbox = dPriv->pClipRects;
   nbox = dPriv->numClipRects;

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + SAVAGE_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
      drm_clip_rect_t *b = (drm_clip_rect_t *)imesa->sarea->boxes;

      imesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++) 
	 *b++ = pbox[i];
     savage_BCI_swap(imesa) ;
   }
   }
   UNLOCK_HARDWARE( imesa );
   

}

/* This waits for *everybody* to finish rendering -- overkill.
 */
void savageDmaFinish( savageContextPtr imesa  ) 
{
    savageDMAFlush(imesa);
    WAIT_IDLE_EMPTY;
}


void savageRegetLockQuiescent( savageContextPtr imesa  ) 
{
    

}

void savageWaitAgeLocked( savageContextPtr imesa, int age  ) 
{
}


void savageWaitAge( savageContextPtr imesa, int age  ) 
{
}



void savageFlushVerticesLocked( savageContextPtr imesa )
{
    drmBufPtr buffer = imesa->vertex_dma_buffer;

    if (!buffer)
	return;

    imesa->vertex_dma_buffer = NULL;

    /* Lot's of stuff to do here. For now there is a fake DMA implementation
     * in savagedma.c that emits drawing commands. Cliprects are not handled
     * yet. */
    if (buffer->used) {
	/* State must be updated "per primitive" because hardware
	 * culling must be disabled for unfilled primitives, points
	 * and lines. */
	savageEmitHwStateLocked (imesa);
	savageFakeVertices (imesa, buffer);
    }
}


void savageFlushVertices( savageContextPtr imesa ) 
{
    LOCK_HARDWARE(imesa);
    savageFlushVerticesLocked (imesa);
    UNLOCK_HARDWARE(imesa);
}


int savage_check_copy(int fd)
{
    return 0;
}

static void savageDDFlush( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    LOCK_HARDWARE(imesa);
    savageFlushVerticesLocked (imesa);
    savageDMAFlush (imesa);
    UNLOCK_HARDWARE(imesa);
}

static void savageDDFinish( GLcontext *ctx  ) 
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    LOCK_HARDWARE(imesa);
    savageFlushVerticesLocked (imesa);
    savageDmaFinish (imesa);
    UNLOCK_HARDWARE(imesa);
}

#define ALT_STATUS_WORD0 (* (volatile GLuint *)(imesa->MMIO_BASE+0x48c60))
#define STATUS_WORD0 (* (volatile GLuint *)(imesa->MMIO_BASE+0x48c00))
#define MAXFIFO_S4  0x7F00
#define MAXFIFO_S3D 0x7F00

static GLboolean savagePagePending_s4( savageContextPtr imesa ) {
    return (ALT_STATUS_WORD0 & 0x08000000) ? GL_TRUE : GL_FALSE;
}
static GLboolean savagePagePending_s3d( savageContextPtr imesa ) {
    return GL_FALSE;
}
static void savageWaitForFIFO_s4( savageContextPtr imesa, unsigned count ) {
    int loop = 0;
    int slots = MAXFIFO_S4-count;
    while((ALT_STATUS_WORD0 & 0x001fffff) > slots && loop++ < MAXLOOP);
}
static void savageWaitForFIFO_s3d( savageContextPtr imesa, unsigned count ) {
    int loop = 0;
    int slots = MAXFIFO_S3D-count;
    while((STATUS_WORD0 & 0x0001ffff) > slots && loop++ < MAXLOOP);
}
static void savageWaitIdleEmpty_s4( savageContextPtr imesa ) {
    int loop = 0;
    while((ALT_STATUS_WORD0 & 0x00ffffff) != 0x00E00000L && loop++ < MAXLOOP);
}
static void savageWaitIdleEmpty_s3d( savageContextPtr imesa ) {
    int loop = 0;
    while((STATUS_WORD0 & 0x000fffff) != 0x000E0000L && loop++ < MAXLOOP);
}

GLboolean (*savagePagePending)( savageContextPtr imesa ) = NULL;
void (*savageWaitForFIFO)( savageContextPtr imesa, unsigned count ) = NULL;
void (*savageWaitIdleEmpty)( savageContextPtr imesa ) = NULL;


void savageDDInitIoctlFuncs( GLcontext *ctx )
{
   ctx->Driver.Clear = savageDDClear;
   ctx->Driver.Flush = savageDDFlush;
   ctx->Driver.Finish = savageDDFinish;
   if (SAVAGE_CONTEXT( ctx )->savageScreen->chipset >= S3_SAVAGE4) {
       savagePagePending = savagePagePending_s4;
       savageWaitForFIFO = savageWaitForFIFO_s4;
       savageWaitIdleEmpty = savageWaitIdleEmpty_s4;
   } else {
       savagePagePending = savagePagePending_s3d;
       savageWaitForFIFO = savageWaitForFIFO_s3d;
       savageWaitIdleEmpty = savageWaitIdleEmpty_s3d;
   }
}

#if SAVAGE_CMD_DMA
/* Alloc a continuous memory */
/* return: 0 error when kernel alloc pages(can try a half memory size) 
           >0 sucess
	   <0 Other error*/
int  savageAllocDMABuffer(savageContextPtr imesa,  drm_savage_alloc_cont_mem_t *req)
{
  int ret;
  if (req ==NULL)
    return 0;

  if ((ret=ioctl(imesa->driFd, DRM_IOCTL_SAVAGE_ALLOC_CONTINUOUS_MEM, req)) <=0)    
    return ret;
  
  return 1;
  
}

/* get the physics address*/
GLuint savageGetPhyAddress(savageContextPtr imesa,void * pointer)
{

  drm_savage_get_physcis_address_t req;
  int ret;

  req.v_address = (GLuint )pointer;
  ret = ioctl(imesa->driFd, DRM_IOCTL_SAVAGE_GET_PHYSICS_ADDRESS,&req);

  return req.p_address;
}

/* free the buffer got by savageAllocDMABuffe*/
int  savageFreeDMABuffer(savageContextPtr imesa,  drm_savage_alloc_cont_mem_t *req)
{
  GLuint ret;
  if (req ==NULL)
    return 0;
  
  if ((ret=ioctl(imesa->driFd, DRM_IOCTL_SAVAGE_FREE_CONTINUOUS_MEM, req)) <=0)    
    return ret;
  return 1;
  
}
#endif
