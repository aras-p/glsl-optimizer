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


#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texformat.h"

#include "mm.h"
#include "via_context.h"
#include "via_tex.h"
#include "via_state.h"
#include "via_ioctl.h"
#include "via_fb.h"
/*=* John Sheng [2003.5.31]  agp tex *=*/
GLuint agpFullCount = 0;

void viaDestroyTexObj(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (!t) 
	return;

    /* This is sad - need to sync *in case* we upload a texture
     * to this newly free memory...
     */
    if (t->bufAddr) {
	via_free_texture(vmesa, t);

        if (vmesa && t->age > vmesa->dirtyAge)
            vmesa->dirtyAge = t->age;
    }

    if (t->globj)
        t->globj->DriverData = 0;

    if (vmesa) {
        if (vmesa->CurrentTexObj[0] == t) {
            vmesa->CurrentTexObj[0] = 0;
            vmesa->dirty &= ~VIA_UPLOAD_TEX0;
        }

        if (vmesa->CurrentTexObj[1] == t) {
            vmesa->CurrentTexObj[1] = 0;
            vmesa->dirty &= ~VIA_UPLOAD_TEX1;
        }
    }

    remove_from_list(t);
    free(t);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

void viaSwapOutTexObj(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (t->bufAddr) {
	via_free_texture(vmesa, t);

        if (t->age > vmesa->dirtyAge)
            vmesa->dirtyAge = t->age;
    }

    t->dirtyImages = ~0;
    move_to_tail(&(vmesa->SwappedOut), t);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

/* Upload an image from mesa's internal copy.
 */
static void viaUploadTexLevel(viaTextureObjectPtr t, int level)
{
    const struct gl_texture_image *image = t->image[level].image;
    int i, j;
    if (VIA_DEBUG) {
	fprintf(stderr, "%s - in\n", __FUNCTION__);    
	fprintf(stderr, "width = %d, height = %d \n", image->Width, image->Height);    
    }	
    switch (t->image[level].internalFormat) {
    case GL_RGB:
    {
	if (image->TexFormat->MesaFormat == MESA_FORMAT_ARGB8888) {
	    GLuint *dst = (GLuint *)(t->bufAddr + t->image[level].offset);
	    GLuint *src = (GLuint *)image->Data;
	    if (VIA_DEBUG) fprintf(stderr, "GL_RGB MESA_FORMAT_ARGB8888\n");    
	    if (image->Width < 8) {
		 for (i = 0; i < image->Height ; i++) {
		    for (j = 0; j < image->Width ; j++) {
			dst[j] = *src;
			src++;
		    }
		    dst += 8;
		}
	    }
	    else {
		for (j = 0; j < image->Height * image->Width; j++) {
        	    *dst = *src;
		    dst++;
        	    src++;
    		}
	    }
	    /*memcpy(dst, src, image->Height * image->Width * sizeof(GLuint));*/
	}
	else {
	    GLushort *dst = (GLushort *)(t->bufAddr + t->image[level].offset);
	    GLushort *src = (GLushort *)image->Data;
	    if (VIA_DEBUG) fprintf(stderr, "GL_RGB !MESA_FORMAT_ARGB8888\n");    
	    if (image->Width < 16) {
		 for (i = 0; i < image->Height ; i++) {
		    for (j = 0; j < image->Width ; j++) {
			dst[j] = *src;
			src++;
		    }
		    dst += 16;
		}
	    }
	    else {
		for (j = 0; j < image->Height * image->Width; j++) {
        	    *dst = *src;
		    dst++;
        	    src++;
    		}
	    }
	    /*memcpy(dst, src, image->Height * image->Width * sizeof(GLushort));*/
	}
    }
    break;

    case GL_RGBA:
    {
        if (image->TexFormat->MesaFormat == MESA_FORMAT_ARGB4444) {    

	    GLushort *dst = (GLushort *)(t->bufAddr + t->image[level].offset);
            GLushort *src = (GLushort *)image->Data;
	    if (image->Width < 16) {
		 for (i = 0; i < image->Height ; i++) {
		    for (j = 0; j < image->Width ; j++) {
			dst[j] = *src;
			src++;
		    }
		    dst += 16;
		}
	    }
	    else {
    		for (j = 0; j < image->Height * image->Width; j++) {
            	        *dst = *src; 
            		src++;
			dst++;
        	}
	    }
	    /*memcpy(dst, src, image->Height * image->Width * sizeof(GLushort));*/
	    if (VIA_DEBUG) fprintf(stderr, "GL_RGBA MESA_FORMAT_ARGB4444\n");    
        }
	else if(image->TexFormat->MesaFormat == MESA_FORMAT_ARGB8888) {
            GLuint *dst = (GLuint *)(t->bufAddr + t->image[level].offset);
            GLuint *src = (GLuint *)image->Data;
	    if (VIA_DEBUG) fprintf(stderr, "GL_RGBA !MESA_FORMAT_ARGB4444\n");    
	    if (image->Width < 8) {
		 for (i = 0; i < image->Height ; i++) {
		    for (j = 0; j < image->Width ; j++) {
			dst[j] = *src;
			src++;
		    }
		    dst += 8;
		}
	    }
	    else {
        	for (j = 0; j < image->Height * image->Width; j++) {
            	    *dst = *src;
            	    dst++;
            	    src++;
        	}
	    }
	    /*memcpy(dst, src, image->Height * image->Width * sizeof(GLuint));*/
	}
	else if(image->TexFormat->MesaFormat == MESA_FORMAT_ARGB1555) {
	    GLushort *dst = (GLushort *)(t->bufAddr + t->image[level].offset);
            GLushort *src = (GLushort *)image->Data;
	    if (image->Width < 16) {
		 for (i = 0; i < image->Height ; i++) {
		    for (j = 0; j < image->Width ; j++) {
			dst[j] = *src;
			src++;
		    }
		    dst += 16;
		}
	    }
	    else {
    		for (j = 0; j < image->Height * image->Width; j++) {
            	        *dst = *src; 
            		src++;
			dst++;
        	}
	    }
	    /*memcpy(dst, src, image->Height * image->Width * sizeof(GLushort));*/
	    if (VIA_DEBUG) fprintf(stderr, "GL_RGBA MESA_FORMAT_ARGB1555\n");    
        }
    }
    break;

    case GL_LUMINANCE:
    {
        GLubyte *dst = (GLubyte *)(t->bufAddr + t->image[level].offset);
        GLubyte *src = (GLubyte *)image->Data;

        for (j = 0; j < image->Height * image->Width; j++) {
            *dst = *src;
	    dst++;
            src++;
        }
    }
    break;

    case GL_INTENSITY:
    {
        GLubyte *dst = (GLubyte *)(t->bufAddr + t->image[level].offset);
        GLubyte *src = (GLubyte *)image->Data;

        for (j = 0; j < image->Height * image->Width; j++) {
            *dst = *src;
	    dst++;
            src++;
        }
    }
    break;

    case GL_LUMINANCE_ALPHA:
    {
        GLushort *dst = (GLushort *)(t->bufAddr + t->image[level].offset);
        GLushort *src = (GLushort *)image->Data;

        for (j = 0; j < image->Height * image->Width; j++) {
            *dst = *src;
            dst++;
            src++;
        }
    }
    break;

    case GL_ALPHA:
    {
        GLubyte *dst = (GLubyte *)(t->bufAddr + t->image[level].offset);
        GLubyte *src = (GLubyte *)image->Data;

        for (j = 0; j < image->Height * image->Width; j++) {
            *dst = *src;
            dst++;    
            src++;
        }
    }
    break;

    /* TODO: Translate color indices *now*:
     */
    case GL_COLOR_INDEX:
    {
        GLubyte *dst = (GLubyte *)(t->bufAddr + t->image[level].offset);
        GLubyte *src = (GLubyte *)image->Data;

        for (j = 0; j < image->Height * image->Width; j++) {
            *dst = *src;
            dst++;
            src++;
        }
    }
    break;

    default:;
        if (VIA_DEBUG) fprintf(stderr, "Not supported texture format %s\n",
                _mesa_lookup_enum_by_nr(image->Format));
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

void viaPrintLocalLRU(viaContextPtr vmesa)
{
    viaTextureObjectPtr t;

    foreach (t, &vmesa->TexObjList) {
        if (!t->globj) {
            if (VIA_DEBUG) {
		fprintf(stderr, "offset = %x, index = %x, size = %x\n",
                    t->texMem.offset,
                    t->texMem.index,
                    t->texMem.size);
	    }
	    else {
        	if (VIA_DEBUG) {
		    fprintf(stderr, "offset = %x, siez = %x\n",
                	t->texMem.offset,
                    t->texMem.size);
		}
	    }
	}	    	    
    }
}

void viaPrintGlobalLRU(viaContextPtr vmesa)
{
    int i, j;
    drm_via_tex_region_t *list = vmesa->sarea->texList;

    for (i = 0, j = VIA_NR_TEX_REGIONS; i < VIA_NR_TEX_REGIONS; i++) {
        if (VIA_DEBUG) fprintf(stderr, "list[%d] age %d next %d prev %d\n",
                j, list[j].age, list[j].next, list[j].prev);
        j = list[j].next;
        if (j == VIA_NR_TEX_REGIONS) break;
    }
    if (j != VIA_NR_TEX_REGIONS)
       if (VIA_DEBUG) fprintf(stderr, "Loop detected in global LRU\n");
}

void viaResetGlobalLRU(viaContextPtr vmesa)
{
    drm_via_tex_region_t *list = vmesa->sarea->texList;
    int sz = 1 << vmesa->viaScreen->logTextureGranularity;
    int i;

    /* (Re)initialize the global circular LRU list.  The last element
     * in the array (VIA_NR_TEX_REGIONS) is the sentinal.  Keeping it
     * at the end of the array allows it to be addressed rationally
     * when looking up objects at a particular location in texture
     * memory.
     */
    for (i = 0; (i + 1) * sz <= vmesa->viaScreen->textureSize; i++) {
        list[i].prev = i - 1;
        list[i].next = i + 1;
        list[i].age = 0;
    }

    i--;
    list[0].prev = VIA_NR_TEX_REGIONS;
    list[i].prev = i - 1;
    list[i].next = VIA_NR_TEX_REGIONS;
    list[VIA_NR_TEX_REGIONS].prev = i;
    list[VIA_NR_TEX_REGIONS].next = 0;
    vmesa->sarea->texAge = 0;
}

void viaUpdateTexLRU(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    vmesa->texAge = ++vmesa->sarea->texAge;
    move_to_head(&(vmesa->TexObjList), t);
}

/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent
 * the other client's textures.
 */
void viaTexturesGone(viaContextPtr vmesa,
                     GLuint offset,
                     GLuint size,
                     GLuint inUse)
{
    viaTextureObjectPtr t, tmp;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    foreach_s (t, tmp, &vmesa->TexObjList) {
        viaSwapOutTexObj(vmesa, t);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

/* This is called with the lock held.  May have to eject our own and/or
 * other client's texture objects to make room for the upload.
 */
void viaUploadTexImages(viaContextPtr vmesa, viaTextureObjectPtr t)
{
    int i, j;
    int numLevels;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    LOCK_HARDWARE(vmesa);

     j = 0;
    if (!t->bufAddr) {
        while (1) {

    	    /*=* John Sheng [2003.5.31]  agp tex *=*/
	    if (via_alloc_texture_agp(vmesa, t))
		break;
	    if (via_alloc_texture(vmesa, t))
		break;

	    agpFullCount++; 
            if (vmesa->TexObjList.prev == vmesa->CurrentTexObj[0] ||
                vmesa->TexObjList.prev == vmesa->CurrentTexObj[1]) {
                if (VIA_DEBUG) fprintf(stderr, "Hit bound texture in upload\n");
                viaPrintLocalLRU(vmesa);
                UNLOCK_HARDWARE(vmesa);
                return;
            }

            if (vmesa->TexObjList.prev == &(vmesa->TexObjList)) {
                if (VIA_DEBUG) fprintf(stderr, "Failed to upload texture, sz %d\n", t->totalSize);
                mmDumpMemInfo(vmesa->texHeap);
                UNLOCK_HARDWARE(vmesa);
                return;
            }

            viaSwapOutTexObj(vmesa, vmesa->TexObjList.prev);
        }
	/*=* John Sheng [2003.5.31]  agp tex *=*/
        /*t->bufAddr = (char *)((GLuint)vmesa->driScreen->pFB + t->texMem.offset);*/

        if (t == vmesa->CurrentTexObj[0])
            VIA_STATECHANGE(vmesa, VIA_UPLOAD_TEX0);

        if (t == vmesa->CurrentTexObj[1])
            VIA_STATECHANGE(vmesa, VIA_UPLOAD_TEX1);

        viaUpdateTexLRU(vmesa, t);
	
	j++;
    }

    numLevels = t->lastLevel - t->firstLevel + 1;

    for (i = 0; i < numLevels; i++)
        if (t->dirtyImages & (1 << i))
            viaUploadTexLevel(t, i);

    t->dirtyImages = 0;

    UNLOCK_HARDWARE(vmesa);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}
