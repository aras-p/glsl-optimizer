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

#ifndef _VIAIOCTL_H
#define _VIAIOCTL_H

#include "via_context.h"


void viaEmitPrim(viaContextPtr vmesa);
void viaFlushPrims(viaContextPtr vmesa);
void viaFlushPrimsLocked(viaContextPtr vmesa);

void viaDmaFinish(viaContextPtr vmesa);
void viaRegetLockQuiescent(viaContextPtr vmesa);
void viaInitIoctlFuncs(GLcontext *ctx);
void viaCopyBuffer(const __DRIdrawablePrivate *dpriv);
void viaPageFlip(const __DRIdrawablePrivate *dpriv);
int via_check_copy(int fd);
void viaFillFrontBuffer(viaContextPtr vmesa);
void viaFillFrontPBuffer(viaContextPtr vmesa);
void viaFillBackBuffer(viaContextPtr vmesa);
void viaFillDepthBuffer(viaContextPtr vmesa, GLuint pixel, GLuint mask);
void viaDoSwapBuffers(viaContextPtr vmesa);
void viaDoSwapPBuffers(viaContextPtr vmesa);

int flush_agp(viaContextPtr vmesa, drm_via_flush_agp_t* agpCmd); 
int flush_sys(viaContextPtr vmesa, drm_via_flush_sys_t* buf); 

#define VIA_STATECHANGE(vmesa, flag)                            \
    do {                                                        \
        if (vmesa->dmaLow != vmesa->dmaLastPrim)                \
            viaFlushPrims(vmesa);                               \
        vmesa->dirty |= flag;                                   \
    } while (0)                                                 \


#define VIA_FIREVERTICES(vmesa)                                 \
    do {                                                        \
        if (vmesa->dmaLow) {                                    \
            viaFlushPrims(vmesa);                               \
        }                                                       \
    } while (0)
    

static __inline GLuint *viaCheckDma(viaContextPtr vmesa, int bytes)
{
    if (vmesa->dmaLow + bytes > vmesa->dmaHigh) {
	if (VIA_DEBUG) fprintf(stderr, "buffer overflow in check dma = %d + %d = %d\n", 
			       vmesa->dmaLow, bytes, vmesa->dmaLow + bytes);
	viaFlushPrims(vmesa);
    }

    {
        GLuint *start = (GLuint *)(vmesa->dmaAddr + vmesa->dmaLow);
        return start;
    }
}
#endif
