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

#include <X11/Xlibint.h>
#include <stdio.h>
#include "savageioctl.h"
#include "savagedma.h"
#include "savage_bci.h"
#include <time.h>
#include <unistd.h>

/* Commit does not depend on whether we use real DMA or fake it via the BCI */
void savageDMACommit (savageContextPtr imesa, void *endPtr) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    GLuint end = (GLuint)endPtr;

    /* make sure that enough space was allocated */
    assert (end <= dmaBuff->allocEnd);

    dmaBuff->allocEnd = dmaBuff->end = end;

    /* TODO: check commands, either here or in flush */
}

#if SAVAGE_CMD_DMA
/* flag =
         0  return -1 if no available page
	 1  wait until a page be available */
static GLuint getDMAPage (savageContextPtr imesa, int flag) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    GLuint page;
    GLuint eventTag1;

    if (dmaBuff->kickFlag == GL_FALSE)
	return dmaBuff->usingPage;

    page = dmaBuff->usingPage + 1;

    /* overflow */
    if (page >= (dmaBuff->buf->size * dmaBuff->buf->type)/DMA_PAGE_SIZE)
	page = 0;

    eventTag1 = GET_EVENTTAG;
    if ( eventTag1 == page) { /* is kicking off */
	if (flag == 1)
	    while (GET_EVENTTAG == page); /* FIXME: add a max loop count? */
	else
	    return -1;
    }

    /* ok, that's it */
    dmaBuff->usingPage = page;
    dmaBuff->start = dmaBuff->end = dmaBuff->allocEnd =
	(dmaBuff->buf->linear + DMA_PAGE_SIZE * page);
    dmaBuff->kickFlag = GL_FALSE;

    return page;
}

/* Allocate space in a real DMA buffer */
void *savageDMAAlloc (savageContextPtr imesa, GLuint size) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;

    /* make sure that everything has been filled in and committed */
    assert (dmaBuff->end == dmaBuff->allocEnd);

    size *= sizeof (u_int32_t); /* size in bytes */
    if (dmaBuff->kickFlag == GL_TRUE) {
	if (size > DMA_PAGE_SIZE)
	    return NULL;
	getDMAPage (imesa, 1);
    } else if (dmaBuff->end + size >=
	       dmaBuff->buf->linear + DMA_PAGE_SIZE*(dmaBuff->usingPage+1)) {
	/* need kick off */
	savageDMAFlush (imesa);
	getDMAPage (imesa, 1);
    }
    dmaBuff->allocEnd = dmaBuff->end + size;
    return (void *)dmaBuff->end;
}

/* Flush DMA buffer via DMA */
void savageDMAFlush (savageContextPtr imesa) {
    volatile u_int32_t* BCIbase;
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    u_int32_t phyAddress;
    GLuint dmaCount, dmaCount1, remain;
    int i;

    /* make sure that everything has been filled in and committed */
    assert (dmaBuff->allocEnd == dmaBuff->end);

    if (dmaBuff->kickFlag == GL_TRUE) /* has been kicked off? */
      return;
    if (dmaBuff->start == dmaBuff->end) /* no command? */
      return;

    /* get bci base */
    BCIbase = (volatile u_int32_t *)SAVAGE_GET_BCI_POINTER(imesa,4);

    /* set the eventtag */
    *BCIbase = (dmaBuff->usingPage & 0xffffL) | (CMD_UpdateShadowStat << 27)
	| (1 << 22);
    *BCIbase = 0x96010051;  /* set register x51*/
    /* set the DMA buffer address */
    phyAddress = (dmaBuff->buf->phyaddress + dmaBuff->usingPage*DMA_PAGE_SIZE)
	& MDT_SRCADD_ALIGMENT;
    if (dmaBuff->buf->location == DRM_SAVAGE_MEM_LOCATION_AGP)
	*BCIbase = (phyAddress) | MDT_SRC_AGP;
    else
        *BCIbase = (phyAddress) | MDT_SRC_PCI;

    /* pad with noops to multiple of 32 bytes */
    dmaCount = (GLuint)(dmaBuff->end - dmaBuff->start);
    dmaCount1 = (dmaCount + 31UL) & ~31UL;
    remain = (dmaCount1 - dmaCount) >> 2;
    for (i = 0; i < remain; i++) {
        *((u_int32_t *)dmaBuff->end) = 0x40000000L;
        dmaBuff->end+=4;
    }
    dmaCount = (dmaCount1 >> 3) - 1;
    dmaBuff->allocEnd = dmaBuff->end;

    /* kick off */
    *BCIbase = (0xA8000000L)|dmaCount;
    dmaBuff->kickFlag = GL_TRUE;
}

/* Init real DMA */
int savageDMAInit (savageContextPtr imesa)
{
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    drm_savage_alloc_cont_mem_t * req;
    int i;
    long ret;

    req = (drm_savage_alloc_cont_mem_t *)
	malloc (sizeof(drm_savage_alloc_cont_mem_t));
    if (!req)
	return GL_FALSE;

    req->type = DRM_SAVAGE_MEM_PAGE;
    req->linear = 0;

    /* try agp first */
    req->phyaddress = imesa->sarea->agp_offset;
    if (req->phyaddress) {
	if (drmMap (imesa->driFd,
		    req->phyaddress,
		    DRM_SAVAGE_DMA_AGP_SIZE,
		    (drmAddressPtr)&req->linear) < 0) {
	    fprintf (stderr, "AGP map error.\n");
	    goto dma;
	}
	if (0) fprintf (stderr,"Using AGP dma|\n");
	req->location = DRM_SAVAGE_MEM_LOCATION_AGP;
	req->size = DRM_SAVAGE_DMA_AGP_SIZE/DRM_SAVAGE_MEM_PAGE;
    }

  dma:
    if (!req->linear) {
	req->size = DMA_BUFFER_SIZE/DRM_SAVAGE_MEM_PAGE;
	for (i = 0; i < DMA_TRY_COUNT; i++) {
	    if ((ret = savageAllocDMABuffer (imesa, req)) != 0)
		break;
	    req->size = req->size/2;
	}
      
	if (ret <= 0) {
	    fprintf(stderr, "Can't alloc DMA memory(system and agp)\n");
	    return GL_FALSE;
	}
	req->location = DRM_SAVAGE_MEM_LOCATION_PCI;
    }

    dmaBuff->buf = req;

    dmaBuff->start = dmaBuff->end = dmaBuff->allocEnd = req->linear;
    dmaBuff->usingPage = 0;
    dmaBuff->kickFlag = GL_FALSE;

    return GL_TRUE;
}

/* Close real DMA */
int savageDMAClose (savageContextPtr imesa)
{
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    drm_savage_alloc_cont_mem_t * req = dmaBuff->buf;

    if(req->location == DRM_SAVAGE_MEM_LOCATION_PCI)
	savageFreeDMABuffer (imesa, req);
    else { /* AGP memory */
	drmUnmap ((drmAddress)req->linear, req->size*req->type);
	drmRmMap (imesa->driFd, req->phyaddress);
    }
    free (req);

    return GL_TRUE;
}
#else
/* Allocate space in faked DMA buffer */
void *savageDMAAlloc (savageContextPtr imesa, GLuint size) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;

    /* make sure that everything has been filled in and committed */
    assert (dmaBuff->end == dmaBuff->allocEnd);

    size *= sizeof (u_int32_t); /* size in bytes */
    if (dmaBuff->end + size >= dmaBuff->buf->linear + DMA_PAGE_SIZE) {
	/* need kick off */
	savageDMAFlush (imesa);
    }
    dmaBuff->allocEnd = dmaBuff->end + size;
    return (void *)dmaBuff->end;
}

/* Flush DMA buffer via BCI (faked DMA) */
void savageDMAFlush(savageContextPtr imesa) {
    volatile u_int32_t* BCIbase;
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    u_int32_t *entry;

    /* make sure that everything has been filled in and committed */
    assert (dmaBuff->allocEnd == dmaBuff->end);

    if (dmaBuff->start == dmaBuff->end) /* no command? */
      return;

    /* get bci base */
    BCIbase = (volatile u_int32_t *)SAVAGE_GET_BCI_POINTER(
	imesa, (dmaBuff->end - dmaBuff->start) / sizeof (u_int32_t));

    for (entry = (u_int32_t *)dmaBuff->start;
	 entry < (u_int32_t *)dmaBuff->end; ++entry)
	*BCIbase = *entry;

    dmaBuff->end = dmaBuff->allocEnd = dmaBuff->start;
}

/* Init faked DMA */
int savageDMAInit (savageContextPtr imesa) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    drm_savage_alloc_cont_mem_t * req;

    req = (drm_savage_alloc_cont_mem_t *)
	malloc (sizeof(drm_savage_alloc_cont_mem_t));
    if (!req)
	return GL_FALSE;

    req->linear = (GLuint)malloc (DMA_PAGE_SIZE);
    if (!req->linear) {
	free (req);
	return GL_FALSE;
    }

    dmaBuff->buf = req;

    dmaBuff->start = dmaBuff->end = dmaBuff->allocEnd = req->linear;
    dmaBuff->usingPage = 0;
    dmaBuff->kickFlag = GL_FALSE;

    return GL_TRUE;
}

/* Close faked DMA */
int savageDMAClose (savageContextPtr imesa) {
    DMABufferPtr dmaBuff = &imesa->DMABuf;
    drm_savage_alloc_cont_mem_t * req = dmaBuff->buf;

    free ((void *)req->linear);
    free (req);

    return GL_TRUE;
}

#endif

/* Faked vertex buffers
 *
 * This is a dirty hack, knowing that it will go away soon when real
 * vertex DMA is implemented and eventually moved to the DRM.
 */

static u_int32_t vertex_data[16384]; /* 64KB */
static drmBuf vertex_buffer = {
    0,                       /* idx */
    65536,                   /* total = 64KB */
    0,                       /* used */
    (drmAddress)vertex_data  /* address */
};

void savageFakeVertices (savageContextPtr imesa, drmBufPtr buffer) {
    GLuint vertexStride = imesa->HwVertexSize; /* stride in dwords */
    GLuint vertexSize = imesa->HwVertexSize; /* the real vertex size in dwords */
    GLuint nVertices = buffer->used / (vertexStride*4);
    u_int32_t *data = (u_int32_t*)buffer->address;
    u_int32_t vertexFormat = imesa->DrawPrimitiveCmd & SAVAGE_HW_SKIPFLAGS;
    GLuint i, j, left;

    /* we have the monopoly on vertex buffers ;-) */
    assert (buffer == &vertex_buffer);
    assert (buffer->used % (vertexStride*4) == 0); /* whole vertices */
    assert (nVertices % 3 == 0);                   /* triangle lists */

    /* Flush (pseodo) DMA before accessing the BCI directly. */
    savageDMAFlush(imesa);

    left = nVertices;
    while (left != 0) {
	/* Can emit up to 255 vertices (85 triangles) with one command. */
	GLuint count = left > 255 ? 255 : left;
	/* Don't go through another buffering mechanism, copy to BCI
	 * directly. */
	volatile u_int32_t *vb = SAVAGE_GET_BCI_POINTER(imesa,
						       count*vertexSize + 1);

	WRITE_CMD (vb, SAVAGE_DRAW_PRIMITIVE(
		       count, SAVAGE_HW_TRIANGLE_LIST | vertexFormat, 0),
		   u_int32_t);
	for (i = 0; i < count; ++i) {
	    for (j = 0; j < vertexSize; ++j)
		WRITE_CMD (vb, data[j], u_int32_t);
	    data += vertexStride;
	}
	left -= count;
    }

    /* clear the vertex buffer for the next set of vertices */
    vertex_buffer.used = 0;
}

drmBufPtr savageFakeGetBuffer (savageContextPtr imesa) {
    assert (vertex_buffer.used == 0); /* has been flushed */
    return &vertex_buffer;
}
