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


#ifndef SAVAGE_IOCTL_H
#define SAVAGE_IOCTL_H

#include "savagecontext.h"
#include "savagedma.h"

void savageGetGeneralDmaBufferLocked( savageContextPtr mmesa ); 

void savageFlushVertices( savageContextPtr mmesa ); 
void savageFlushVerticesLocked( savageContextPtr mmesa );

void savageFlushGeneralLocked( savageContextPtr imesa );
void savageWaitAgeLocked( savageContextPtr imesa, int age );
void savageWaitAge( savageContextPtr imesa, int age );

void savageDmaFinish( savageContextPtr imesa );

void savageRegetLockQuiescent( savageContextPtr imesa );

void savageDDInitIoctlFuncs( GLcontext *ctx );

void savageSwapBuffers( __DRIdrawablePrivate *dPriv );

int savage_check_copy(int fd);

extern GLboolean (*savagePagePending)( savageContextPtr imesa );
extern void (*savageWaitForFIFO)( savageContextPtr imesa, unsigned count );
extern void (*savageWaitIdleEmpty)( savageContextPtr imesa );

#define PAGE_PENDING(result) do { \
    result = savagePagePending(imesa); \
} while (0)
#define WAIT_FOR_FIFO(count) do { \
    savageWaitForFIFO(imesa, count); \
} while (0)
#define WAIT_IDLE_EMPTY do { \
    savageWaitIdleEmpty(imesa); \
} while (0)

#if SAVAGE_CMD_DMA
int  savageAllocDMABuffer(savageContextPtr imesa,  drm_savage_alloc_cont_mem_t *req);
GLuint savageGetPhyAddress(savageContextPtr imesa,void * pointer);
int  savageFreeDMABuffer(savageContextPtr, drm_savage_alloc_cont_mem_t*);
#endif

#define FLUSH_BATCH(imesa) do { \
    if (imesa->vertex_dma_buffer) savageFlushVertices(imesa); \
} while (0)

static __inline
uint32_t *savageAllocDmaLow( savageContextPtr imesa, GLuint bytes )
{
   uint32_t *head;

   if (!imesa->vertex_dma_buffer) {
      LOCK_HARDWARE(imesa);
      imesa->vertex_dma_buffer = savageFakeGetBuffer (imesa);
      UNLOCK_HARDWARE(imesa);
   } else if (imesa->vertex_dma_buffer->used + bytes >
	      imesa->vertex_dma_buffer->total) {
      LOCK_HARDWARE(imesa);
      savageFlushVerticesLocked( imesa );
      imesa->vertex_dma_buffer = savageFakeGetBuffer (imesa);
      UNLOCK_HARDWARE(imesa);
   }

   head = (uint32_t *)((uint8_t *)imesa->vertex_dma_buffer->address +
		       imesa->vertex_dma_buffer->used);

   imesa->vertex_dma_buffer->used += bytes;
   return head;
}

#endif
