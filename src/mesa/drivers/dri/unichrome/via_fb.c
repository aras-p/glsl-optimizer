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

#include <assert.h>

#include "via_context.h"
#include "via_ioctl.h"
#include "via_fb.h"
#include "xf86drm.h"
#include <sys/ioctl.h>

GLboolean
via_alloc_draw_buffer(viaContextPtr vmesa, viaBuffer *buf)
{
   drm_via_mem_t mem;
   mem.context = vmesa->hHWContext;
   mem.size = buf->size;
   mem.type = VIDEO;

   if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_ALLOCMEM, &mem)) 
      return GL_FALSE;
    
    
   buf->offset = mem.offset;
   buf->map = (char *)vmesa->driScreen->pFB + mem.offset;
   buf->index = mem.index;
   return GL_TRUE;
}

void
via_free_draw_buffer(viaContextPtr vmesa, viaBuffer *buf)
{
   drm_via_mem_t mem;

   if (!vmesa) return;

   mem.context = vmesa->hHWContext;
   mem.index = buf->index;
   mem.type = VIDEO;
   ioctl(vmesa->driFd, DRM_IOCTL_VIA_FREEMEM, &mem);
   buf->map = NULL;
}


GLboolean
via_alloc_dma_buffer(viaContextPtr vmesa)
{
   drmVIADMAInit init;

   if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
   vmesa->dma = (GLubyte *) malloc(VIA_DMA_BUFSIZ);
    
   /*
    * Check whether AGP DMA has been initialized.
    */
   init.func = VIA_DMA_INITIALIZED;
   vmesa->useAgp = 
      ( 0 == drmCommandWrite(vmesa->driFd, DRM_VIA_DMA_INIT, 
			     &init, sizeof(init)));
   if (vmesa->useAgp) 
      printf("unichrome_dri.so: Using AGP.\n");
   else
      printf("unichrome_dri.so: Using PCI.\n");
      
   if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
   return ((vmesa->dma) ? GL_TRUE : GL_FALSE);
}

void
via_free_dma_buffer(viaContextPtr vmesa)
{
    if (!vmesa) return;
    free(vmesa->dma);
    vmesa->dma = 0;
} 

GLboolean
via_alloc_texture(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    drm_via_mem_t fb;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    fb.context = vmesa->hHWContext;
    fb.size = t->texMem.size;
    fb.type = VIDEO;
    if (VIA_DEBUG) {
	fprintf(stderr, "texture size = %d\n", fb.size);
	fprintf(stderr, "texture type = %d\n", fb.type);
    }
    if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_ALLOCMEM, &fb)) {
	fprintf(stderr, "via_alloc_texture fail\n");
        return GL_FALSE;
    }	
    
    t->texMem.offset = fb.offset;
    t->texMem.index = fb.index;
    if (VIA_DEBUG) fprintf(stderr, "texture index = %d\n", (GLuint)fb.index);
    
    t->bufAddr = (unsigned char *)(fb.offset + (GLuint)vmesa->driScreen->pFB);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}
/*=* John Sheng [2003.5.31]  agp tex *=*/
GLboolean
via_alloc_texture_agp(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    drm_via_mem_t fb;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    fb.context = vmesa->hHWContext;
    fb.size = t->texMem.size;
    fb.type = AGP;
    if (VIA_DEBUG) {
	fprintf(stderr, "texture_agp size = %d\n", fb.size);
	fprintf(stderr, "texture type = %d\n", fb.type);
    }
    if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_ALLOCMEM, &fb)) {
	fprintf(stderr, "via_alloc_texture_agp fail\n");
        return GL_FALSE;
    }	
    
    t->texMem.offset = fb.offset;
    t->texMem.index = fb.index;
    if (VIA_DEBUG) fprintf(stderr, "texture agp index = %d\n", (GLuint)fb.index);
    
    t->bufAddr = (unsigned char *)((GLuint)vmesa->viaScreen->agpLinearStart + fb.offset); 	
    /*=* John Sheng [2003.5.31]  agp tex *=*/
    t->inAGP = GL_TRUE;
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

void
via_free_texture(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    drm_via_mem_t fb;
    if (VIA_DEBUG) {
	fprintf(stderr, "via_free_texture: index = %d\n",
            t->texMem.index);
	fprintf(stderr, "via_free_texture: size = %d\n",
            t->texMem.size);
    }
    if (!vmesa) {
	fprintf(stderr, "!mesa\n");
	return;
    }
    
    fb.context = vmesa->hHWContext;
    fb.index = t->texMem.index;
    
    /*=* John Sheng [2003.5.31]  agp tex *=*/
    if(t->inAGP)
	fb.type = AGP;
    else
        fb.type = VIDEO;
	    
    if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_FREEMEM, &fb)) {
	if(vmesa->shareCtx) {
	    fb.context = ((viaContextPtr)((GLcontext *)(vmesa->shareCtx)->DriverCtx))->hHWContext;
	    if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_FREEMEM, &fb)) {
		fprintf(stderr, "via_free_texture fail\n");
	    }
	}
	else
	    fprintf(stderr, "via_free_texture fail\n");
    }

    t->bufAddr = NULL;
}
