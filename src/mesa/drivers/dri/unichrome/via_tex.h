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


#ifndef _VIATEX_H
#define _VIATEX_H

#include "mtypes.h"
#include "mm.h"

#include "via_context.h"
#include "via_3d_reg.h"

#define VIA_TEX_MAXLEVELS	10


/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
struct via_texture_object_t {
    struct via_texture_object_t *next, *prev;

    GLuint age;
    struct gl_texture_object *globj;

    int texelBytes;
    int totalSize;

    struct {
	GLuint index;
	GLuint offset;
	GLuint size;
    } texMem;
    unsigned char* bufAddr;
    
    GLuint inAGP;
    GLuint needClearCache;    
    GLuint actualLevel;

    GLuint maxLevel;
    GLuint dirtyImages;

    struct {
        const struct gl_texture_image *image;
        int offset;               /* into bufAddr */
        int height;
        int internalFormat;
    } image[VIA_TEX_MAXLEVELS];

    GLuint dirty;
    
    GLuint regTexFM;
    GLuint regTexWidthLog2[2];
    GLuint regTexHeightLog2[2];
    GLuint regTexBaseH[4];
    struct {
	GLuint baseL;
	GLuint pitchLog2;
    } regTexBaseAndPitch[12];

    GLint firstLevel, lastLevel;  /* upload tObj->Image[first .. lastLevel] */
};              

viaTextureObjectPtr viaAllocTextureObject(struct gl_texture_object *texObj);
GLboolean viaUpdateTextureState(GLcontext *ctx);
void viaInitTextureFuncs(struct dd_function_table * functions);
void viaInitTextures(GLcontext *ctx);

void viaDestroyTexObj(viaContextPtr vmesa, viaTextureObjectPtr t);
void viaSwapOutTexObj(viaContextPtr vmesa, viaTextureObjectPtr t);
void viaUploadTexImages(viaContextPtr vmesa, viaTextureObjectPtr t);

void viaResetGlobalLRU(viaContextPtr vmesa);
void viaTexturesGone(viaContextPtr vmesa,
                     GLuint start, GLuint end,
                     GLuint in_use);

void viaPrintLocalLRU(viaContextPtr vmesa);
void viaPrintGlobalLRU(viaContextPtr vmesa);
void viaUpdateTexLRU(viaContextPtr vmesa, viaTextureObjectPtr t);

#endif
