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
#include "context.h"
#include "texformat.h"

#include "mm.h"
#include "via_context.h"
#include "via_tex.h"
#include "via_state.h"
#include "via_ioctl.h"

GLint texSize8bpp[12][12] = {
    {32,32,32,32,32,32,64,128,256,512,1024,2048},
    {64,64,64,64,64,64,128,256,512,1024,2048,4096},
    {128,128,128,128,128,128,256,512,1024,2048,4096,8192},
    {256,256,256,256,256,256,512,1024,2048,4096,8192,16384},
    {512,512,512,512,512,512,1024,2048,4096,8192,16384,32768},
    {1024,1024,1024,1024,1024,1024,2048,4096,8192,16384,32768,65536},
    {2048,2048,2048,2048,2048,2048,4096,8192,16384,32768,65536,131072},
    {4096,4096,4096,4096,4096,4096,8192,16384,32768,65536,131072,262144},
    {8192,8192,8192,8192,8192,8192,16384,32768,65536,131072,262144,524288},
    {16384,16384,16384,16384,16384,16384,32768,65536,131072,262144,524288,1048576},
    {32768,32768,32768,32768,32768,32768,65536,131072,262144,524288,1048576,2097152},
    {65536,65536,65536,65536,65536,65536,131072,262144,524288,1048576,2097152,4194304
}
};

GLint texSize16bpp[12][12] = {
    {32,32,32,32,32,64,128,256,512,1024,2048,4096},
    {64,64,64,64,64,128,256,512,1024,2048,4096,8192},
    {128,128,128,128,128,256,512,1024,2048,4096,8192,16384},
    {256,256,256,256,256,512,1024,2048,4096,8192,16384,32768},
    {512,512,512,512,512,1024,2048,4096,8192,16384,32768,65536},
    {1024,1024,1024,1024,1024,2048,4096,8192,16384,32768,65536,131072},
    {2048,2048,2048,2048,2048,4096,8192,16384,32768,65536,131072,262144},
    {4096,4096,4096,4096,4096,8192,16384,32768,65536,131072,262144,524288},
    {8192,8192,8192,8192,8192,16384,32768,65536,131072,262144,524288,1048576},
    {16384,16384,16384,16384,16384,32768,65536,131072,262144,524288,1048576,2097152},
    {32768,32768,32768,32768,32768,65536,131072,262144,524288,1048576,2097152,4194304},
    {65536,65536,65536,65536,65536,131072,262144,524288,1048576,2097152,4194304,8388608}
};

GLint texSize32bpp[12][12] = {
    {32,32,32,32,64,128,256,512,1024,2048,4096,8192},
    {64,64,64,64,128,256,512,1024,2048,4096,8192,16384},
    {128,128,128,128,256,512,1024,2048,4096,8192,16384,32768},
    {256,256,256,256,512,1024,2048,4096,8192,16384,32768,65536},
    {512,512,512,512,1024,2048,4096,8192,16384,32768,65536,131072},
    {1024,1024,1024,1024,2048,4096,8192,16384,32768,65536,131072,262144},
    {2048,2048,2048,2048,4096,8192,16384,32768,65536,131072,262144,524288},
    {4096,4096,4096,4096,8192,16384,32768,65536,131072,262144,524288,1048576},
    {8192,8192,8192,8192,16384,32768,65536,131072,262144,524288,1048576,2097152},
    {16384,16384,16384,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304},
    {32768,32768,32768,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608},
    {65536,65536,65536,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216}
};

GLint mipmapTexSize8bpp[12][12] = {
    {32,64,96,128,160,192,256,384,640,1152,2176,4224},
    {96,96,128,160,192,224,320,512,896,1664,3200,6272},
    {224,224,224,256,288,320,480,832,1536,2944,5760,11392},
    {480,480,480,480,512,544,832,1504,2880,5632,11136,22144},
    {992,992,992,992,992,1024,1568,2880,5600,11072,22016,43904},
    {2016,2016,2016,2016,2016,2016,3072,5664,11072,21984,43840,87552},
    {4064,4064,4064,4064,4064,4064,6112,11264,22048,43840,87520,174912},
    {8160,8160,8160,8160,8160,8160,12256,22496,44032,87584,174912,349664},
    {16352,16352,16352,16352,16352,16352,24544,45024,88032,175104,349728,699200},
    {32736,32736,32736,32736,32736,32736,49120,90080,176096,350176,699392,1398304},
    {65504,65504,65504,65504,65504,65504,98272,180192,352224,700384,1398752,2796544},
    {131040,131040,131040,131040,131040,131040,196576,360416,704480,1400800,2797536,5593056
}
};

GLint mipmapTexSize16bpp[12][12] = {
    {32,64,96,128,160,224,352,608,1120,2144,4192,8288},
    {96,96,128,160,192,288,480,864,1632,3168,6240,12384},
    {224,224,224,256,288,448,800,1504,2912,5728,11360,22624},
    {480,480,480,480,512,800,1472,2848,5600,11104,22112,44128},
    {992,992,992,992,992,1536,2848,5568,11040,21984,43872,87648},
    {2016,2016,2016,2016,2016,3040,5632,11040,21952,43808,87520,174944},
    {4064,4064,4064,4064,4064,6112,11232,22016,43808,87488,174880,349664},
    {8160,8160,8160,8160,8160,12256,22496,44000,87552,174880,349632,699168},
    {16352,16352,16352,16352,16352,24544,45024,88032,175072,349696,699168,1398208},
    {32736,32736,32736,32736,32736,49120,90080,176096,350176,699360,1398272,2796320},
    {65504,65504,65504,65504,65504,98272,180192,352224,700384,1398752,2796512,5592576},
    {131040,131040,131040,131040,131040,196576,360416,704480,1400800,2797536,5593056,11185120}
};

GLint mipmapTexSize32bpp[12][12] = {
    {32,64,96,128,192,320,576,1088,2112,4160,8256,16448},
    {96,96,128,160,256,448,832,1600,3136,6208,12352,24640},
    {224,224,224,256,416,768,1472,2880,5696,11328,22592,45120},
    {480,480,480,480,768,1440,2816,5568,11072,22080,44096,88128},
    {992,992,992,992,1504,2816,5536,11008,21952,43840,87616,175168},
    {2016,2016,2016,2016,3040,5600,11008,21920,43776,87488,174912,349760},
    {4064,4064,4064,4064,6112,11232,21984,43776,87456,174848,349632,699200},
    {8160,8160,8160,8160,12256,22496,44000,87520,174848,349600,699136,1398208},
    {16352,16352,16352,16352,24544,45024,88032,175072,349664,699136,1398176,2796288},
    {32736,32736,32736,32736,49120,90080,176096,350176,699360,1398240,2796288,5592480},
    {65504,65504,65504,65504,98272,180192,352224,700384,1398752,2796512,5592544,11184896},
    {131040,131040,131040,131040,196576,360416,704480,1400800,2797536,5593056,11185120,22369760}
};

static int logbase2(int n)
{
    GLint i = 1;
    GLint log2 = 0;

    if (n < 0) {
        return -1;
    }

    while (n > i) {
        i *= 2;
        log2++;
    }

    if (i != n) {
        return -1;
    }
    else {
        return log2;
    }
}

static void viaSetTexImages(viaContextPtr vmesa,
                            struct gl_texture_object *tObj)
{
    GLuint texFormat;
    viaTextureObjectPtr t = (viaTextureObjectPtr)tObj->DriverData;
    const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
    GLint firstLevel, lastLevel, numLevels;
    GLint log2Width, log2Height, log2Pitch;
    GLint (*texSize)[12][12];
    GLint w, h, p;
    GLint i, j, k, l, m;
    GLint mipmapSize;
    GLuint texBase;
    GLuint basH = 0;
    GLuint widthExp = 0;
    GLuint heightExp = 0;    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__); 
    switch (baseImage->TexFormat->MesaFormat) {
    case MESA_FORMAT_ARGB8888:
	if (t->image[tObj->BaseLevel].internalFormat == GL_RGB)
    	    texFormat = HC_HTXnFM_ARGB0888;
	else
	    texFormat = HC_HTXnFM_ARGB8888;
        break;
    case MESA_FORMAT_ARGB4444:
        texFormat = HC_HTXnFM_ARGB4444; 
        break;
    case MESA_FORMAT_RGB565:
        texFormat = HC_HTXnFM_RGB565;   
        break;
    case MESA_FORMAT_ARGB1555:
        texFormat = HC_HTXnFM_ARGB1555;   
        break;
    case MESA_FORMAT_L8:
        texFormat = HC_HTXnFM_L8;       
        break;
    case MESA_FORMAT_I8:
        texFormat = HC_HTXnFM_T8;       
        break;
    case MESA_FORMAT_CI8:
        texFormat = HC_HTXnFM_Index8;   
        break;
    case MESA_FORMAT_AL88:
        texFormat = HC_HTXnFM_AL88;     
        break;
    /*=* John Sheng [2003.7.18] texenv *=*/
    case MESA_FORMAT_A8:
        texFormat = HC_HTXnFM_A8;     
        break;
    default:
        _mesa_problem(vmesa->glCtx, "Bad texture format in viaSetTexImages");
	fprintf(stderr, "-- TexFormat = %d\n",baseImage->TexFormat->MesaFormat);
	return;
    };

    /* Compute which mipmap levels we really want to send to the hardware.
     * This depends on the base image size, GL_TEXTURE_MIN_LOD,
     * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
     * Yes, this looks overly complicated, but it's all needed.
     */
    if (tObj->MinFilter == GL_LINEAR || tObj->MinFilter == GL_NEAREST) {
        firstLevel = lastLevel = tObj->BaseLevel;
    }
    else {
        firstLevel = tObj->BaseLevel + (GLint)(tObj->MinLod + 0.5);
        firstLevel = MAX2(firstLevel, tObj->BaseLevel);
        lastLevel = tObj->BaseLevel + (GLint)(tObj->MaxLod + 0.5);
        lastLevel = MAX2(lastLevel, tObj->BaseLevel);
        lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
        lastLevel = MIN2(lastLevel, tObj->MaxLevel);
        lastLevel = MAX2(firstLevel, lastLevel);        /* need at least one level */
    }

    /* save these values */
    t->firstLevel = firstLevel;
    t->lastLevel = lastLevel;

    numLevels = lastLevel - firstLevel + 1;
    
    log2Width = tObj->Image[0][firstLevel]->WidthLog2;
    log2Height = tObj->Image[0][firstLevel]->HeightLog2;
    log2Pitch = logbase2(tObj->Image[0][firstLevel]->Width * baseImage->TexFormat->TexelBytes);
    
    
    for (i = 0; i < numLevels; i++) {
    	t->image[i].image = tObj->Image[0][i];
    	t->image[i].internalFormat = baseImage->Format; 
    }

    if (baseImage->TexFormat->TexelBytes == 1) {
        if (numLevels > 1)
            texSize = &mipmapTexSize8bpp;
        else
            texSize = &texSize8bpp;
    }
    else if (baseImage->TexFormat->TexelBytes == 2) {
        if (numLevels > 1)
            texSize = &mipmapTexSize16bpp;
        else
            texSize = &texSize16bpp;
    }    
    else {
        if (numLevels > 1)
            texSize = &mipmapTexSize32bpp;
        else
            texSize = &texSize32bpp;
    }
    t->totalSize = (*texSize)[log2Height][log2Width];
    t->texMem.size = t->totalSize;
    t->maxLevel = i - 1;
/*     t->dirty = VIA_UPLOAD_TEX0 | VIA_UPLOAD_TEX1; */

    if (VIA_DEBUG) {
	fprintf(stderr, "log2Width = %d\n", log2Width);  
	fprintf(stderr, "log2Height = %d\n", log2Height);    
	fprintf(stderr, "log2Pitch = %d\n", log2Pitch);    
	fprintf(stderr, "bytePerTexel = %d\n", baseImage->TexFormat->TexelBytes);
	fprintf(stderr, "total size = %d\n", t->totalSize);
	fprintf(stderr, "actual level = %d\n", t->actualLevel);
        fprintf(stderr, "numlevel = %d\n", numLevels);    
    }	

    {
	w = log2Width;
        h = log2Height;	
	for (i = 0; i < numLevels; i++) {
	    t->image[i].offset = t->totalSize -  (*texSize)[h][w];
	    if (w) w--;
            if (h) h--;
        }
    }    
    
    viaUploadTexImages(vmesa, t);
    
    if (t->bufAddr) {
	if (t->inAGP)
            t->regTexFM = (HC_SubA_HTXnFM << 24) | HC_HTXnLoc_AGP | texFormat;
        else
            t->regTexFM = (HC_SubA_HTXnFM << 24) | HC_HTXnLoc_Local | texFormat;
        
        w = log2Width;
        h = log2Height;
        p = log2Pitch;
        mipmapSize = 0;     

        for (i = 0; i < numLevels; i++) {    
	    if (i == (numLevels - 1))
		mipmapSize = 0;
	    else
		mipmapSize = (*texSize)[h][w];
	    
	    /*=* John Sheng [2003.5.31]  agp tex *=*/
            if (t->inAGP)
                texBase = (GLuint)vmesa->agpBase + t->texMem.offset + t->image[i].offset;
            else
		texBase = t->texMem.offset + t->image[i].offset;
	    if (VIA_DEBUG) {
		fprintf(stderr, "texmem offset = %x\n", t->texMem.offset);    
		fprintf(stderr, "mipmap%d addr = %x\n", i, t->image[i].offset);    
		fprintf(stderr, "mipmap%d size = %d, h = %d, w = %d\n", i, (*texSize)[h][w], h, w);    
		fprintf(stderr, "texBase%d = %x\n", i, texBase);
	    }	
	    t->regTexBaseAndPitch[i].baseL = ((HC_SubA_HTXnL0BasL + i) << 24) | (texBase & 0xFFFFFF);
            
	    if (p < 5) {
                t->regTexBaseAndPitch[i].pitchLog2 = ((HC_SubA_HTXnL0Pit + i) << 24) |
                                                     (0x5 << 20);
	    }
            else {         
                t->regTexBaseAndPitch[i].pitchLog2 = ((HC_SubA_HTXnL0Pit + i) << 24) |
                                                     ((GLuint)p << 20);
	    }			     
            j = i / 3;
            k = 3 - (i % 3);                                              
            basH |= ((texBase & 0xFF000000) >> (k << 3));
            if (k == 1) {
                t->regTexBaseH[j] = ((j + HC_SubA_HTXnL012BasH) << 24) | basH;
                basH = 0;
            }
            
            l = i / 6;
            m = i % 6;
            widthExp |= (((GLuint)w & 0xF) << (m << 2));
            heightExp |= (((GLuint)h & 0xF) << (m << 2));
            if (m == 5) {
                t->regTexWidthLog2[l] = ((l + HC_SubA_HTXnL0_5WE) << 24 | widthExp);
                t->regTexHeightLog2[l] = ((l + HC_SubA_HTXnL0_5HE) << 24 | heightExp);  
                widthExp = 0;
                heightExp = 0;
            }
            if (w) w--;
            if (h) h--;
            if (p) p--;                                           
        }
        
        if (k != 1) {
            t->regTexBaseH[j] = ((j + HC_SubA_HTXnL012BasH) << 24) | basH;      
        }
        if (m != 5) {
            t->regTexWidthLog2[l] = ((l + HC_SubA_HTXnL0_5WE) << 24 | widthExp);
            t->regTexHeightLog2[l] = ((l + HC_SubA_HTXnL0_5HE) << 24 | heightExp);
        }    
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}



static GLboolean viaUpdateTexUnit(GLcontext *ctx, GLuint unit)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

    if (texUnit->_ReallyEnabled == TEXTURE_2D_BIT || 
	texUnit->_ReallyEnabled == TEXTURE_1D_BIT) {

        struct gl_texture_object *tObj = texUnit->_Current;
        viaTextureObjectPtr t = (viaTextureObjectPtr)tObj->DriverData;

        /* Upload teximages (not pipelined)
         */
        if (t->dirtyImages) {
            VIA_FLUSH_DMA(vmesa);
            viaSetTexImages(vmesa, tObj);
            if (!t->bufAddr) {
                return GL_FALSE;
            }
        }

        if (tObj->Image[0][tObj->BaseLevel]->Border > 0) {
            return GL_FALSE;
        }

        /* Update state if this is a different texture object to last
         * time.
         */
        if (vmesa->CurrentTexObj[unit] != t) {
            VIA_FLUSH_DMA(vmesa);
            vmesa->CurrentTexObj[unit] = t;
            viaUpdateTexLRU(vmesa, t); /* done too often */
        }

	return GL_TRUE;
    }
    else if (texUnit->_ReallyEnabled) {
       return GL_FALSE;
    } 
    else {
        vmesa->CurrentTexObj[unit] = 0;
        VIA_FLUSH_DMA(vmesa);
	return GL_TRUE;
    }
}

void viaUpdateTextureState(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    GLuint ok = (viaUpdateTexUnit(ctx, 0) &&
		 viaUpdateTexUnit(ctx, 1));

    FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, !ok);
}

