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

#ifdef XFree86Server
# include "xf86.h"
# include "xf86_OSproc.h"
# include "xf86_ansic.h"
# define _DRM_MALLOC xalloc
# define _DRM_FREE   xfree
# ifndef XFree86LOADER
#  include <sys/mman.h>
# endif
#else
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <ctype.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/time.h>
#include "imports.h"
#define _DRM_MALLOC MALLOC
#define _DRM_FREE   FREE
#endif

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#ifdef __linux__
#include <sys/sysmacros.h>	/* for makedev() */
#endif
#include "xf86drm.h"
#include "xf86drmVIA.h"
#include "drm.h"
#include "via_common.h"
int drmVIAAgpInit(int fd, int offset, int size)
{
    drm_via_agp_t agp;
    agp.offset = offset;
    agp.size = size;

    if (ioctl(fd, DRM_IOCTL_VIA_AGP_INIT, &agp) < 0) {
         return -errno;
    }
    else {
        return 0;
    }
}

int drmVIAFBInit(int fd, int offset, int size)
{
    drm_via_fb_t fb;
    fb.offset = offset;
    fb.size = size;

    if (ioctl(fd, DRM_IOCTL_VIA_FB_INIT, &fb) < 0) {
	return -errno;
    }
    else
	return 0;
}

int drmVIAInitMAP(int fd, drmVIAInit *info)
{
    drm_via_init_t init;

    memset(&init, 0, sizeof(drm_via_init_t));
    init.func = VIA_INIT_MAP;  
    init.sarea_priv_offset = info->sarea_priv_offset;  
    init.fb_offset = info->fb_offset;
    init.mmio_offset = info->mmio_offset;
    init.agpAddr = info->agpAddr;
   
    if (ioctl(fd, DRM_IOCTL_VIA_MAP_INIT, &init ) < 0) {
        return -errno;
    }
    else
        return 0;
}

int drmVIAAllocateDMA(int fd, drmVIADMABuf *buf)
{
    if (drmAddMap(fd, 0, buf->size,
		   DRM_SHM, 0,
		   &buf->index) < 0) {
		return -errno;
    }
    
    if (drmMap(fd,(drm_handle_t)buf->index,
		buf->size,(drmAddressPtr)(&buf->address)) < 0) {
	return -errno;
    }
    
    memset(buf->address, 0, buf->size);
	
    return 0;
}

int drmVIAReleaseDMA(int fd, drmVIADMABuf *buf)
{	
    if (drmUnmap((drmAddress)(buf->address), buf->size) < 0)
	return -errno;
    
    return 0;
}

int drmVIACmdBuffer(int fd, drmVIACommandBuffer *buf)
{
    if (ioctl(fd, 0x48, buf ) < 0) {
        return -errno;
    }
    else
        return 0;
}
  
