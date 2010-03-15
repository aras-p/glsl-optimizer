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
 * Contains the functions for creating dma buffers by calling
 * the kernel via driver specific ioctls.
 *
 * @author Jakob Bornecrantz <jakob@vmware.com>
 */

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H 1
#endif
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include "xf86drm.h"
#include "../core/vmwgfx_drm.h"

#include "vmw_driver.h"
#include "util/u_debug.h"

struct vmw_dma_buffer
{
    void *data;
    unsigned handle;
    uint64_t map_handle;
    unsigned map_count;
    uint32_t size;
};

static int
vmw_ioctl_get_param(struct vmw_customizer *vmw, uint32_t param, uint64_t *out)
{
    struct drm_vmw_getparam_arg gp_arg;
    int ret;

    memset(&gp_arg, 0, sizeof(gp_arg));
    gp_arg.param = param;
    ret = drmCommandWriteRead(vmw->fd, DRM_VMW_GET_PARAM,
	    &gp_arg, sizeof(gp_arg));

    if (ret == 0) {
	*out = gp_arg.value;
    }

    return ret;
}

int
vmw_ioctl_supports_streams(struct vmw_customizer *vmw)
{
    uint64_t value;
    int ret;

    ret = vmw_ioctl_get_param(vmw, DRM_VMW_PARAM_NUM_STREAMS, &value);
    if (ret)
	return ret;

    return value ? 0 : -ENOSYS;
}

int
vmw_ioctl_num_streams(struct vmw_customizer *vmw,
		      uint32_t *ntot, uint32_t *nfree)
{
    uint64_t v1, v2;
    int ret;

    ret = vmw_ioctl_get_param(vmw, DRM_VMW_PARAM_NUM_STREAMS, &v1);
    if (ret)
	return ret;

    ret = vmw_ioctl_get_param(vmw, DRM_VMW_PARAM_NUM_FREE_STREAMS, &v2);
    if (ret)
	return ret;

    *ntot = (uint32_t)v1;
    *nfree = (uint32_t)v2;

    return 0;
}

int
vmw_ioctl_claim_stream(struct vmw_customizer *vmw, uint32_t *out)
{
    struct drm_vmw_stream_arg s_arg;
    int ret;

    ret = drmCommandRead(vmw->fd, DRM_VMW_CLAIM_STREAM,
			 &s_arg, sizeof(s_arg));

    if (ret)
	return -1;

    *out = s_arg.stream_id;
    return 0;
}

int
vmw_ioctl_unref_stream(struct vmw_customizer *vmw, uint32_t stream_id)
{
    struct drm_vmw_stream_arg s_arg;
    int ret;

    memset(&s_arg, 0, sizeof(s_arg));
    s_arg.stream_id = stream_id;

    ret = drmCommandRead(vmw->fd, DRM_VMW_CLAIM_STREAM,
			 &s_arg, sizeof(s_arg));

    return 0;
}

int
vmw_ioctl_cursor_bypass(struct vmw_customizer *vmw, int xhot, int yhot)
{
    struct drm_vmw_cursor_bypass_arg arg;
    int ret;

    memset(&arg, 0, sizeof(arg));
    arg.flags = DRM_VMW_CURSOR_BYPASS_ALL;
    arg.xhot = xhot;
    arg.yhot = yhot;

    ret = drmCommandWrite(vmw->fd, DRM_VMW_CURSOR_BYPASS,
			  &arg, sizeof(arg));

    return ret;
}

struct vmw_dma_buffer *
vmw_ioctl_buffer_create(struct vmw_customizer *vmw, uint32_t size, unsigned *handle)
{
    struct vmw_dma_buffer *buf;
    union drm_vmw_alloc_dmabuf_arg arg;
    struct drm_vmw_alloc_dmabuf_req *req = &arg.req;
    struct drm_vmw_dmabuf_rep *rep = &arg.rep;
    int ret;

    buf = xcalloc(1, sizeof(*buf));
    if (!buf)
	goto err;

    memset(&arg, 0, sizeof(arg));
    req->size = size;
    do {
	ret = drmCommandWriteRead(vmw->fd, DRM_VMW_ALLOC_DMABUF, &arg, sizeof(arg));
    } while (ret == -ERESTART);

    if (ret) {
	debug_printf("IOCTL failed %d: %s\n", ret, strerror(-ret));
	goto err_free;
    }


    buf->data = NULL;
    buf->handle = rep->handle;
    buf->map_handle = rep->map_handle;
    buf->map_count = 0;
    buf->size = size;

    *handle = rep->handle;

    return buf;

err_free:
    xfree(buf);
err:
    return NULL;
}

void
vmw_ioctl_buffer_destroy(struct vmw_customizer *vmw, struct vmw_dma_buffer *buf)
{ 
    struct drm_vmw_unref_dmabuf_arg arg; 

    if (buf->data) { 
	munmap(buf->data, buf->size); 
	buf->data = NULL; 
    } 

    memset(&arg, 0, sizeof(arg)); 
    arg.handle = buf->handle; 
    drmCommandWrite(vmw->fd, DRM_VMW_UNREF_DMABUF, &arg, sizeof(arg)); 

    xfree(buf); 
} 

void *
vmw_ioctl_buffer_map(struct vmw_customizer *vmw, struct vmw_dma_buffer *buf)
{
    void *map;

    if (buf->data == NULL) {
	map = mmap(NULL, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   vmw->fd, buf->map_handle);
	if (map == MAP_FAILED) {
	    debug_printf("%s: Map failed.\n", __FUNCTION__);
	    return NULL;
	}

	buf->data = map;
    }

    ++buf->map_count;

    return buf->data;
}

void
vmw_ioctl_buffer_unmap(struct vmw_customizer *vmw, struct vmw_dma_buffer *buf)
{
    --buf->map_count;
}
