/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/**
 * @file
 * Contains the shared resources for VMware Xorg driver
 * that sits ontop of the Xorg State Traker.
 *
 * It is initialized in vmw_screen.c.
 *
 * @author Jakob Bornecrantz <jakob@vmware.com>
 */

#ifndef VMW_DRIVER_H_
#define VMW_DRIVER_H_

#include "state_trackers/xorg/xorg_tracker.h"

struct vmw_dma_buffer;

struct vmw_customizer
{
    CustomizerRec base;
    ScrnInfoPtr pScrn;

    int fd;

    void *cursor_priv;

    /* vmw_video.c */
    void *video_priv;
};

static INLINE struct vmw_customizer *
vmw_customizer(CustomizerPtr cust)
{
    return cust ? (struct vmw_customizer *) cust : NULL;
}


/***********************************************************************
 * vmw_video.c
 */

Bool vmw_video_init(struct vmw_customizer *vmw);

Bool vmw_video_close(struct vmw_customizer *vmw);

void vmw_video_stop_all(struct vmw_customizer *vmw);


/***********************************************************************
 * vmw_ioctl.c
 */

int vmw_ioctl_cursor_bypass(struct vmw_customizer *vmw, int xhot, int yhot);

struct vmw_dma_buffer * vmw_ioctl_buffer_create(struct vmw_customizer *vmw,
						uint32_t size,
						unsigned *handle);

void * vmw_ioctl_buffer_map(struct vmw_customizer *vmw,
			    struct vmw_dma_buffer *buf);

void vmw_ioctl_buffer_unmap(struct vmw_customizer *vmw,
			    struct vmw_dma_buffer *buf);

void vmw_ioctl_buffer_destroy(struct vmw_customizer *vmw,
			      struct vmw_dma_buffer *buf);

int vmw_ioctl_supports_streams(struct vmw_customizer *vmw);

int vmw_ioctl_num_streams(struct vmw_customizer *vmw,
			  uint32_t *ntot, uint32_t *nfree);

int vmw_ioctl_unref_stream(struct vmw_customizer *vmw, uint32_t stream_id);

int vmw_ioctl_claim_stream(struct vmw_customizer *vmw, uint32_t *out);


#endif
