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

void savageGetGeneralDmaBufferLocked( savageContextPtr mmesa ); 

void savageFlushVertices( savageContextPtr mmesa ); 

void savageFlushGeneralLocked( savageContextPtr imesa );
void savageWaitAgeLocked( savageContextPtr imesa, int age );
void savageWaitAge( savageContextPtr imesa, int age );

unsigned int savageEmitEventLocked( savageContextPtr imesa, unsigned int flags );
unsigned int savageEmitEvent( savageContextPtr imesa, unsigned int flags );
void savageWaitEvent( savageContextPtr imesa, unsigned int event);

void savageFlushCmdBufLocked( savageContextPtr imesa, GLboolean discard );
void savageFlushCmdBuf( savageContextPtr imesa, GLboolean discard );

void savageDmaFinish( savageContextPtr imesa );

void savageRegetLockQuiescent( savageContextPtr imesa );

void savageDDInitIoctlFuncs( GLcontext *ctx );

void savageSwapBuffers( __DRIdrawablePrivate *dPriv );

#define WAIT_IDLE_EMPTY do { \
    savageWaitEvent(imesa, \
		    savageEmitEvent(imesa, SAVAGE_WAIT_3D|SAVAGE_WAIT_2D)); \
} while (0)

#define FLUSH_BATCH(imesa) do { \
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG) \
        fprintf (stderr, "FLUSH_BATCH in %s\n", __FUNCTION__); \
    savageFlushVertices(imesa); \
    savageFlushCmdBuf(imesa, GL_FALSE); \
} while (0)

extern void savageGetDMABuffer( savageContextPtr imesa );

static __inline
u_int32_t *savageAllocVtxBuf( savageContextPtr imesa, GLuint words )
{
   struct savage_vtxbuf_t *buffer = imesa->vtxBuf;
   u_int32_t *head;

   if (buffer == &imesa->dmaVtxBuf) {
       if (!buffer->total) {
	   LOCK_HARDWARE(imesa);
	   savageGetDMABuffer(imesa);
	   UNLOCK_HARDWARE(imesa);
       } else if (buffer->used + words > buffer->total) {
	   if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	       fprintf (stderr, "... flushing DMA buffer in %s\n",
			__FUNCTION__);
	   savageFlushVertices( imesa );
	   LOCK_HARDWARE(imesa);
	   savageFlushCmdBufLocked(imesa, GL_TRUE); /* discard DMA buffer */
	   savageGetDMABuffer(imesa);
	   UNLOCK_HARDWARE(imesa);
       }
   } else if (buffer->used + words > buffer->total) {
       if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	   fprintf (stderr, "... flushing client vertex buffer in %s\n",
		    __FUNCTION__);
       savageFlushVertices( imesa );
       LOCK_HARDWARE(imesa);
       savageFlushCmdBufLocked(imesa, GL_FALSE); /* free clientVtxBuf */
       UNLOCK_HARDWARE(imesa);
   }

   head = &buffer->buf[buffer->used];

   buffer->used += words;
   return head;
}

static __inline
drm_savage_cmd_header_t *savageAllocCmdBuf( savageContextPtr imesa, GLuint bytes )
{
    drm_savage_cmd_header_t *ret;
    GLuint qwords = ((bytes + 7) >> 3) + 1; /* round up */
    assert (qwords < imesa->cmdBuf.size);
    if (imesa->cmdBuf.write - imesa->cmdBuf.base + qwords > imesa->cmdBuf.size) {
	savageFlushCmdBuf(imesa, GL_FALSE);
    }
    ret = (drm_savage_cmd_header_t *)imesa->cmdBuf.write;
    imesa->cmdBuf.write += qwords;
    return ret;
}

#endif
