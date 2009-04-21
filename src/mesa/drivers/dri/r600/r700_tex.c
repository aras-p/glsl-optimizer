/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/texformat.h"
#include "main/texstore.h"
#include "texmem.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/macros.h"
#include "xmlpool.h"

#include "radeon_common.h"

#include "r600_context.h"
#include "r700_chip.h"

#if 0 /* to be enabled */
#include "r700_state.h"
#endif /* to be enabled */

#include "r700_tex.h"

GLuint r700GetTexObjSize(void)  
{
    return sizeof(r700TexObj);
}

/* to be enable */
void r700SetTexBuffer(__DRIcontext *pDRICtx, GLint target,
			     __DRIdrawable *dPriv)
{
}

/* to be enable */
void r700SetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
			      GLint format, __DRIdrawable *dPriv)
{
}

/* to be enable */
void r700SetTexOffset(__DRIcontext *pDRICtx, GLint texname,
			     unsigned long long offset, GLint depth,
			     GLuint pitch)
{
}

#if 0 /* to be enabled */
static GLboolean r700GetTexFormat(struct gl_texture_object *tObj, GLuint mesa_format)
{
    r700TexObjPtr t = (r700TexObjPtr) tObj->DriverData;

    t->texture_state.SQ_TEX_RESOURCE4.u32All &= ~( SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask
                                                  |SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask
                                                  |SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask
                                                  |SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask );

    switch (mesa_format) /* This is mesa format. */
    {
    case MESA_FORMAT_RGBA8888:        
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
                
        break;
    case MESA_FORMAT_RGBA8888_REV: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask); 
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        
        break;
    case MESA_FORMAT_ARGB8888: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ARGB8888_REV: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);     
        break;
 
    case MESA_FORMAT_RGB888: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_RGB565: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_5_6_5,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask); 
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_RGB565_REV: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_5_6_5,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ARGB4444: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_4_4_4_4,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ARGB4444_REV: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_4_4_4_4,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);   
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ARGB1555: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_1_5_5_5,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask); 
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ARGB1555_REV: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_1_5_5_5,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_AL88: 
    case MESA_FORMAT_AL88_REV: /* TODO : Check this. */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break; 
    case MESA_FORMAT_RGB332: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_3_3_2,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask); 
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break; 
    case MESA_FORMAT_A8: /* ZERO, ZERO, ZERO, X */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_L8: /* X, X, X, ONE */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_I8: /* X, X, X, X */
    case MESA_FORMAT_CI8:
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;    
    /* YUV422 TODO conversion */  /* X, Y, Z, ONE, G8R8_G8B8 */
    /*
    case MESA_FORMAT_YCBCR:
        t->texture_state.SQ_TEX_RESOURCE1.bitfields.DATA_FORMAT = ;
        break;
    */
    /* VUY422 TODO conversion */  /* X, Y, Z, ONE, G8R8_G8B8 */
    /*
    case MESA_FORMAT_YCBCR_REV: 
        t->texture_state.SQ_TEX_RESOURCE1.bitfields.DATA_FORMAT = ;
        break;
    */
    case MESA_FORMAT_RGB_DXT1: /* not supported yet */
        
        break;
    case MESA_FORMAT_RGBA_DXT1: /* not supported yet */
        
        break;
    case MESA_FORMAT_RGBA_DXT3: /* not supported yet */
        
        break;
    case MESA_FORMAT_RGBA_DXT5: /* not supported yet */
        
        break;
    case MESA_FORMAT_RGBA_FLOAT32: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_32_32_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);   
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_RGBA_FLOAT16: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_16_16_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_RGB_FLOAT32: /* X, Y, Z, ONE */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_32_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);   
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_RGB_FLOAT16: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_16_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);   
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ALPHA_FLOAT32: /* ZERO, ZERO, ZERO, X */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_ALPHA_FLOAT16: /* ZERO, ZERO, ZERO, X */        
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_LUMINANCE_FLOAT32: /* X, X, X, ONE */        
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_LUMINANCE_FLOAT16: /* X, X, X, ONE */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);  
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16: 
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);    
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);     
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16_FLOAT,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);    
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
        break;
    case MESA_FORMAT_Z16: 
    case MESA_FORMAT_Z24_S8:
    case MESA_FORMAT_Z32:
        switch (mesa_format)
        {
            case MESA_FORMAT_Z16:
                SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_16,
                         SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);                
                break;
            case MESA_FORMAT_Z24_S8:
                SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_24_8,
                         SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);                
                break;
            case MESA_FORMAT_Z32: 
                SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_32,
                         SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);                
        };
        switch (tObj->DepthMode) 
        {
        case GL_LUMINANCE:  /* X, X, X, ONE */          

            t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_1 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
            break;
        case GL_INTENSITY:  /* X, X, X, X */

            t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
            break;
        case GL_ALPHA:     /* ZERO, ZERO, ZERO, X */
            t->texture_state.SQ_TEX_RESOURCE4.u32All |=
                   (SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift)
                  |(SQ_SEL_0 << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift)
                  |(SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift);
            break;
        default:
            return GL_FALSE;
        } 
        break;
    default:
        /* Not supported format */
        return GL_FALSE;        
    };
 
    return GL_TRUE;
}

static void compute_tex_image_offset(
	struct gl_texture_object *tObj,
	GLuint face,
	GLint level,
	GLint* curOffset)
{
    r700TexObjPtr t = (r700TexObjPtr) tObj->DriverData;
    const struct gl_texture_image* texImage;
    GLuint blitWidth = R700_BLIT_WIDTH_BYTES;
    GLuint texelBytes;
    GLuint size;
    GLuint pitch;

    texImage = tObj->Image[0][level + t->base.firstLevel];
    if (!texImage)
    {
	    return;
    }

    texelBytes = texImage->TexFormat->TexelBytes;

    pitch = (texImage->Width + R700_TEXEL_PITCH_ALIGNMENT_MASK) & ~R700_TEXEL_PITCH_ALIGNMENT_MASK;

    /* find image size in bytes */
    if (texImage->IsCompressed) 
    {
        /* not supported yet */
    } 
    else if (tObj->Target == GL_TEXTURE_RECTANGLE_NV) 
    {
        if( (ARRAY_LINEAR_ALIGNED << SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift)
            == (t->texture_state.SQ_TEX_RESOURCE0.u32All & SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask) )
        {
            pitch = (texImage->Width * texelBytes + 255) & ~255;
        }
        else
        {
            if(0 == level)
            {
                pitch = (pitch * texelBytes + 63) & ~63;
            }
            else
            {
                pitch = texImage->Width * texelBytes;
            }
        }
        size  =  pitch * texImage->Height;
        blitWidth = 64 / texelBytes;
        pitch /= texelBytes;
    } 
    else 
    {
        if( (ARRAY_LINEAR_ALIGNED << SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift)
            == (t->texture_state.SQ_TEX_RESOURCE0.u32All & SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask) )
        {
            pitch = (texImage->Width * texelBytes + 255) & ~255;            
        }
        else
        {
            if(0 == level)
            {
                pitch = (pitch * texelBytes + 31) & ~31;
            }
            else
            {
                pitch = texImage->Width * texelBytes;
            }
        }
        size  =  pitch * texImage->Height * texImage->Depth;
        blitWidth = MAX2(texImage->Width, 64 / texelBytes);
        pitch /= texelBytes;
    }
    assert(size > 0);

    if( (0 == level) || (1 == level) ) /* 0 for BASE_ADDRESS, 1 for MIP_ADDRESS */
    {
        *curOffset = (*curOffset + R700_TEXTURE_ALIGNMENT_MASK) & ~R700_TEXTURE_ALIGNMENT_MASK;
    }

    if (texelBytes) 
    {
        /* fix x and y coords up later together with offset */
        t->texel_pitch[face][level]        = pitch;
        t->level_offset[face][level]       = *curOffset;   
        t->byte_per_texel                  = texelBytes;
        t->src_width_in_pexel[face][level] = texImage->Width;
        t->src_hight_in_pexel[face][level] = texImage->Height;
    } 
    else 
    {
        /* Do it like one byte texel. */
        pitch = (size + R700_TEXEL_PITCH_ALIGNMENT_MASK) & ~R700_TEXEL_PITCH_ALIGNMENT_MASK;
        t->texel_pitch[face][level]        = pitch;
        t->level_offset[face][level]       = *curOffset; 
        t->byte_per_texel                  = 1;
        t->src_width_in_pexel[face][level] = size;
        t->src_hight_in_pexel[face][level] = 1;
    }

    *curOffset += size;
}
#endif /* to be enabled */
 
void r700DestroyTexObj(context_t context, r700TexObjPtr t) 
{
    /* TODO : nuke r700 chip texture and sampler pointer. */
    //int i;

    //for (i = 0; i < rmesa->ctx->Const.MaxTextureUnits; i++) 
    //{
        //if (rmesa->state.texture.unit[i].texobj == t) {
        //          rmesa->state.texture.unit[i].texobj = NULL;
        //}
    //}
}
 
#if 0 /* to be enabled */
static void r700SetTexImages(context_t *context, struct gl_texture_object *tObj)
{
    r700TexObjPtr t = (r700TexObjPtr) tObj->DriverData;
    const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
    GLint curOffset;
    GLint i, texelBytes;
    GLint numLevels;
    GLint log2Width, log2Height, log2Depth;
    GLuint uTexelPitch;
 
    if (!t->image_override) 
    {
        if(GL_FALSE == r700GetTexFormat(tObj, baseImage->TexFormat->MesaFormat) )
        {
            _mesa_problem(NULL, "unexpected texture format in %s", __FUNCTION__);
            return;
        }        
    } 
 
    texelBytes = baseImage->TexFormat->TexelBytes;

    switch (tObj->Target)
    {
        case GL_TEXTURE_1D: 
            SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, SQ_TEX_DIM_1D, DIM_shift, DIM_mask);            
            SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);            
            break;
        case GL_TEXTURE_2D: 
        case GL_TEXTURE_RECTANGLE_NV:
            SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);          
            SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);
            break;
        case GL_TEXTURE_3D:
            SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, SQ_TEX_DIM_3D, DIM_shift, DIM_mask); 
            SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, tObj->Image[0][t->base.firstLevel]->Depth - 1, 
                     TEX_DEPTH_shift, TEX_DEPTH_mask);            
            break;
        case GL_TEXTURE_CUBE_MAP:  
            SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, SQ_TEX_DIM_CUBEMAP, DIM_shift, DIM_mask);                       
            SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);
            break;
        default:
            _mesa_problem(NULL, "unexpected texture target type in %s", __FUNCTION__);
            return;
    }
 
    /* Compute which mipmap levels we really want to send to the hardware.
     */
    driCalculateTextureFirstLastLevel((driTextureObject *) t);
    log2Width = tObj->Image[0][t->base.firstLevel]->WidthLog2;
    log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;
    log2Depth = tObj->Image[0][t->base.firstLevel]->DepthLog2;
 
    numLevels = t->base.lastLevel - t->base.firstLevel + 1;
 
    assert(numLevels <= RADEON_MAX_TEXTURE_LEVELS);
 
    /* Calculate mipmap offsets and dimensions for blitting (uploading)
     * The idea is that we lay out the mipmap levels within a block of
     * memory organized as a rectangle of width BLIT_WIDTH_BYTES.
     */
    t->tile_bits = 0;
 
    curOffset = 0;
 
    if (tObj->Target == GL_TEXTURE_CUBE_MAP) 
    {
        ASSERT(log2Width == log2Height);
        
        for(i = 0; i < numLevels; i++) 
        {
            /* i is hw level */
            GLuint face;
            for(face = 0; face < 6; face++)
            {
                compute_tex_image_offset(tObj, face, i, &curOffset);
            }
        }
    } 
    else 
    {
        for (i = 0; i < numLevels; i++)
        {
            /* i is hw level */
            compute_tex_image_offset(tObj, 0, i, &curOffset);
        }
    }
 
    /* Align the total size of texture memory block.
     */
    t->base.totalSize = (curOffset + R700_TEXTURE_ALIGNMENT_MASK) & ~R700_TEXTURE_ALIGNMENT_MASK;
 
    t->pitch = 0;
 
    /* TODO : baseImage->IsCompressed, tObj->Target == GL_TEXTURE_RECTANGLE_NV */
    
    uTexelPitch = (tObj->Image[0][t->base.firstLevel]->Width + R700_TEXEL_PITCH_ALIGNMENT_MASK)
                 & ~R700_TEXEL_PITCH_ALIGNMENT_MASK;

    SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, (uTexelPitch/8)-1, PITCH_shift, PITCH_mask); 
    SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, tObj->Image[0][t->base.firstLevel]->Width  - 1,
             TEX_WIDTH_shift, TEX_WIDTH_mask);
    SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, tObj->Image[0][t->base.firstLevel]->Height - 1,
             TEX_HEIGHT_shift, TEX_HEIGHT_mask);
}

static void r700UploadSubImage(context_t    *context, 
                               r700TexObjPtr t,
			                   GLint         hwlevel, /* relative level to first real level. */
			                   GLint         x, 
                               GLint         y,                                
			                   GLuint        face)
{
    struct gl_texture_image *texImage = NULL;
    GLuint offset;
    GLint imageWidth, imageHeight;
    GLint ret;
    const int level = hwlevel + t->base.firstLevel;

    unsigned char *pSrc;

    ASSERT(face < 6);

    /* Ensure we have a valid texture to upload */
    if ((hwlevel < 0) || (hwlevel >= RADEON_MAX_TEXTURE_LEVELS)) 
    {
        _mesa_problem(NULL, "bad texture level in %s", __FUNCTION__);
        return;
    }

    texImage = t->base.tObj->Image[face][level];

    if (!texImage) 
    {		
	    return;
    }
    if (!texImage->Data) 
    {
        return;
    }

    if (t->base.tObj->Target == GL_TEXTURE_RECTANGLE_NV) 
    {
        /* TODO :
        assert(level == 0);
        assert(hwlevel == 0);

        r300UploadRectSubImage(rmesa, t, texImage, x, y, width, height);
        */
        return;
    } 
    else if (texImage->IsClientData) 
    {
        /* TODO :
        r300UploadGARTClientSubImage(rmesa, t, texImage, hwlevel, x, y,
			             width, height);
        */
        return;
    } 

    imageWidth = texImage->Width;
    imageHeight = texImage->Height;

    /* use hwlevel for hwsurf. */
    offset = t->bufAddr + t->level_offset[face][hwlevel]; 

    pSrc = (unsigned char*)(texImage->Data);

    (context->chipobj.LoadMemSurf)(context,
                         offset, /* gpu addr */
                         t->texel_pitch[face][hwlevel], /* dst_pitch_in_pixel */
                         t->src_width_in_pexel[face][hwlevel], /*src_width_in_pixel */
                         t->src_hight_in_pexel[face][hwlevel], /* height */
                         t->byte_per_texel, /* byte_per_pixel */
                         pSrc);  /* source data */
}

int r700UploadTexImages(GLcontext * ctx, struct gl_texture_object *tObj, GLuint face)
{
    context_t *context = R700_CONTEXT(ctx);
    r700TexObjPtr t    = (r700TexObjPtr) tObj->DriverData; 

    int       heap;
    const int numLevels = t->base.lastLevel - t->base.firstLevel + 1;

    if (t->image_override)
    {
        return 0;
    }

    if (t->base.totalSize == 0)
    {
        return 0;
    }
    /* TODO */
    /*LOCK_HARDWARE(&rmesa->radeon);*/

    if (t->base.memBlock == NULL) 
    {
        heap = RADEON_LOCAL_TEX_HEAP;
        if( GL_FALSE == (context->chipobj.AllocMemSurf)(context,
                                         &(t->base.memBlock),
                                         &(t->base.heap),
                                         &heap, /* prefered_heap, also return the actual heap used. */
                                         t->base.totalSize) )
        {
            /* TODO */
            /* UNLOCK_HARDWARE(&rmesa->radeon); */
            return -1;
        }

        /* Set the base offset of the texture image */
        t->bufAddr = context->screen->texOffset[heap] + t->base.memBlock->ofs;
        t->offset  = t->bufAddr;

        /*
        if (!(t->base.tObj->Image[0][0]->IsClientData)) 
        {            
            t->offset |= t->tile_bits;
        }
        */
    }

    /* Let the world know we've used this memory recently.
     */
    driUpdateTextureLRU((driTextureObject *) t);

    /* TODO */
    /* UNLOCK_HARDWARE(&rmesa->radeon); */

    /* Upload any images that are new */
    if (t->my_dirty_images[face]) 
    {
        int i;
        for(i = 0; i < numLevels; i++) 
        {
            if( (t->my_dirty_images[face] & (1 << (i + t->base.firstLevel))) !=0) 
            {
                r700UploadSubImage(context, 
                                   t, 
                                   i, /* i is hw level */
                                   0, 
                                   0,                                       
                                   face);
            }
        }
        t->base.dirty_images[face] = 0;
        t->my_dirty_images[face] = 0;
    }

    /* TODO : 3D, CUBE */
    t->texture_state.SQ_TEX_RESOURCE2.u32All = t->bufAddr / 256;
    if( (t->base.lastLevel - t->base.firstLevel) > 0 )
    {
        t->texture_state.SQ_TEX_RESOURCE3.u32All  = (t->bufAddr + t->level_offset[0][1]) / 256; /* MIP_ADDRESS */

        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, t->base.firstLevel, BASE_LEVEL_shift, BASE_LEVEL_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE5.u32All, t->base.lastLevel, LAST_LEVEL_shift, LAST_LEVEL_mask);        
    }

	return 0;
}
 
static GLboolean r700EnableTexture2D(GLcontext * ctx, int unit)
{
    context_t *context = R700_CONTEXT(ctx);
    struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
    struct gl_texture_object *tObj = texUnit->_Current;
    r700TexObjPtr t = (r700TexObjPtr) tObj->DriverData;
 
    ASSERT(tObj->Target == GL_TEXTURE_2D || tObj->Target == GL_TEXTURE_1D);
 
    if (t->base.dirty_images[0]) 
    {        
        r700SetTexImages(context, tObj);
        r700UploadTexImages(ctx, tObj, 0);
        if (!t->base.memBlock && !t->image_override)
        {
            return GL_FALSE;
        }
    } 
    return GL_TRUE;
}

/* try to find a format which will only need a memcopy */
static const struct gl_texture_format *r700Choose8888TexFormat(GLenum srcFormat,
							       GLenum srcType)
{
    struct gl_texture_format * gtfRet;

    const GLuint ui = 1;
    const GLubyte littleEndian = *((const GLubyte *)&ui);

    if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
        (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
        (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
        (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian)) 
    {
        gtfRet = &_mesa_texformat_rgba8888;
    } 
    else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
	       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
	       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
	       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) 
    {
        gtfRet =  &_mesa_texformat_rgba8888_rev;
    } 
    else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
				        srcType == GL_UNSIGNED_INT_8_8_8_8)) 
    {
        gtfRet = &_mesa_texformat_argb8888_rev;
    } 
    else if (srcFormat == GL_BGRA && ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
				        srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) 
    {
        gtfRet = &_mesa_texformat_argb8888;
    } 
    else
    {
        gtfRet = _dri_texformat_argb8888;
    }

    return gtfRet;
}

#endif /* to be enabled */

static r700TexObjPtr r700AllocTexObj(struct gl_texture_object *texObj)
{
	r700TexObjPtr t;

	t = CALLOC_STRUCT(r700_tex_obj);
	texObj->DriverData = t;
	if (t != NULL) 
    {
#if 0 /* to be enabled */
		/* Initialize non-image-dependent parts of the state:
		 */
		t->base.tObj = texObj;
		t->border_fallback = GL_FALSE;

		make_empty_list(&t->base);

        /* Init text object to default states. */
        t->texture_state.SQ_TEX_RESOURCE0.u32All              = 0;
        SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, SQ_TEX_DIM_2D, DIM_shift, DIM_mask); 
        SETfield(t->texture_state.SQ_TEX_RESOURCE0.u32All, ARRAY_LINEAR_GENERAL,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift, SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
        CLEARbit(t->texture_state.SQ_TEX_RESOURCE0.u32All, TILE_TYPE_bit);
        
        t->texture_state.SQ_TEX_RESOURCE1.u32All                = 0;
        SETfield(t->texture_state.SQ_TEX_RESOURCE1.u32All, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->texture_state.SQ_TEX_RESOURCE2.u32All                = 0;
        t->texture_state.SQ_TEX_RESOURCE3.u32All                = 0;
        
        t->texture_state.SQ_TEX_RESOURCE4.u32All                   = 0;
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_FORMAT_COMP_UNSIGNED, 
                 FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_FORMAT_COMP_UNSIGNED, 
                 FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_FORMAT_COMP_UNSIGNED, 
                 FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_FORMAT_COMP_UNSIGNED, 
                 FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_NUM_FORMAT_NORM,
                 SQ_TEX_RESOURCE_WORD4_0__NUM_FORMAT_ALL_shift, SQ_TEX_RESOURCE_WORD4_0__NUM_FORMAT_ALL_mask);
        CLEARbit(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_TEX_RESOURCE_WORD4_0__SRF_MODE_ALL_bit);
        CLEARbit(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, SQ_ENDIAN_NONE,
                 SQ_TEX_RESOURCE_WORD4_0__ENDIAN_SWAP_shift, SQ_TEX_RESOURCE_WORD4_0__ENDIAN_SWAP_mask);
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, 1, REQUEST_SIZE_shift, REQUEST_SIZE_mask);
        t->texture_state.SQ_TEX_RESOURCE4.u32All |= SQ_SEL_X << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift
                                                   |SQ_SEL_Y << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift
                                                   |SQ_SEL_Z << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift
                                                   |SQ_SEL_W << SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift;
        SETfield(t->texture_state.SQ_TEX_RESOURCE4.u32All, 0, BASE_LEVEL_shift, BASE_LEVEL_mask); /* mip-maps */
                  
        t->texture_state.SQ_TEX_RESOURCE5.u32All = 0;
              
        t->texture_state.SQ_TEX_RESOURCE6.u32All = 0;
     
        SETfield(t->texture_state.SQ_TEX_RESOURCE6.u32All, SQ_TEX_VTX_VALID_TEXTURE,
                 SQ_TEX_RESOURCE_WORD6_0__TYPE_shift, SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);
    
        /* Initialize sampler registers */
        t->sampler_state.SQ_TEX_SAMPLER0.u32All                           = 0;
        t->sampler_state.SQ_TEX_SAMPLER0.u32All |=
                         SQ_TEX_WRAP << SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_shift
                        |SQ_TEX_WRAP << CLAMP_Y_shift
                        |SQ_TEX_WRAP << CLAMP_Z_shift
                        |SQ_TEX_XY_FILTER_POINT << XY_MAG_FILTER_shift
                        |SQ_TEX_XY_FILTER_POINT << XY_MIN_FILTER_shift
                        |SQ_TEX_Z_FILTER_NONE << Z_FILTER_shift
                        |SQ_TEX_Z_FILTER_NONE << MIP_FILTER_shift                        
                        |SQ_TEX_BORDER_COLOR_TRANS_BLACK << BORDER_COLOR_TYPE_shift;
                        
        t->sampler_state.SQ_TEX_SAMPLER1.u32All = 0x7FF << MAX_LOD_shift;
        
        t->sampler_state.SQ_TEX_SAMPLER2.u32All                          = 0;        
        SETbit(t->sampler_state.SQ_TEX_SAMPLER2.u32All, SQ_TEX_SAMPLER_WORD2_0__TYPE_bit);
#endif /* to be enabled */
	}

	return t;
}

static GLboolean
r700ValidateClientStorage(GLcontext * ctx, GLenum target,
			  GLint internalFormat,
			  GLint srcWidth, GLint srcHeight,
			  GLenum format, GLenum type, const void *pixels,
			  const struct gl_pixelstore_attrib *packing,
			  struct gl_texture_object *texObj,
			  struct gl_texture_image *texImage)
{
    if (!ctx->Unpack.ClientStorage)
    {
        return 0;
    }

    if (ctx->_ImageTransferState ||
        texImage->IsCompressed || texObj->GenerateMipmap)
    {
        return 0;
    }

    /* This list is incomplete, may be different on ppc???
     */
    switch (internalFormat) 
    {
    case GL_RGBA:
        if (format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV) 
        {
            texImage->TexFormat = _dri_texformat_argb8888;
        } 
        else
        {
            return 0;
        }
        break;

    case GL_RGB:
        if (format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5) 
        {
	        texImage->TexFormat = _dri_texformat_rgb565;
        } 
        else
        {
            return 0;
        }
        break;

    case GL_YCBCR_MESA:
        if (format == GL_YCBCR_MESA &&
            type == GL_UNSIGNED_SHORT_8_8_REV_APPLE) 
        {
            texImage->TexFormat = &_mesa_texformat_ycbcr_rev;
        } 
        else if( format == GL_YCBCR_MESA &&
	             (type == GL_UNSIGNED_SHORT_8_8_APPLE || type == GL_UNSIGNED_BYTE)) 
        {
            texImage->TexFormat = &_mesa_texformat_ycbcr;
        } 
        else
        {
            return 0;
        }
        break;

    default:
        return 0;
    }

    /* Could deal with these packing issues, but currently don't:
     */
    if (packing->SkipPixels ||
        packing->SkipRows || packing->SwapBytes || packing->LsbFirst) 
    {
	    return 0;
    }

    GLint srcRowStride = _mesa_image_row_stride(packing, srcWidth, format, type);

    /* Have validated that _mesa_transfer_teximage would be a straight
     * memcpy at this point.  NOTE: future calls to TexSubImage will
     * overwrite the client data.  This is explicitly mentioned in the
     * extension spec.
     */
    texImage->Data = (void *)pixels;
    texImage->IsClientData = GL_TRUE;
    texImage->RowStride = srcRowStride / texImage->TexFormat->TexelBytes;

    return 1;
}

static void r700TexImage1D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage) 
{
}

static void r700TexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage) 
{
#if 0 /* to be enabled */
    r700TexObjPtr r700t = (r700TexObjPtr) texObj->DriverData;

    driTextureObject *t = (driTextureObject *) texObj->DriverData;
    GLuint face;

    /* which cube face or ordinary 2D image */
    switch (target) 
    {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        face = (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        ASSERT(face < 6);
        break;
    default:
        face = 0;
    }

    if (t != NULL) 
    {
	    driSwapOutTextureObject(t);
    } 
    else 
    {
        t = (driTextureObject *) r700AllocTexObj(texObj);
        if (!t) 
        {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
    }

    texImage->IsClientData = GL_FALSE;

    if (r700ValidateClientStorage(ctx, target,
			          internalFormat,
			          width, height,
			          format, type, pixels,
			          packing, texObj, texImage)) 
    {
        /* client maintained surface */
    } 
    else 
    {
        /* Normal path: copy (to cached memory) and eventually upload
         * via another copy to GART memory and then a blit...  Could
         * eliminate one copy by going straight to (permanent) GART.
         *
         * Note, this will call r700ChooseTextureFormat.
         */
        _mesa_store_teximage2d(ctx, target, level, internalFormat,
		               width, height, border, format, type,
		               pixels, &ctx->Unpack, texObj, texImage);

        t->dirty_images[face] |= (1 << level);

        /* mesa dirty_images is not correct, so use own one for now, review it later. */
        r700t->my_dirty_images[face] |= (1 << level);
    }
#endif /* to be enabled */
}

static void r700TexImage3D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat,
			   GLint width, GLint height, GLint depth,
			   GLint border,
			   GLenum format, GLenum type, const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage) 
{
}

static void r700TexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset,
			      GLsizei width,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage) 
{

}

static void r700TexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint yoffset,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage) 
{
}

static void r700TexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset, GLint yoffset, GLint zoffset,
		  GLsizei width, GLsizei height, GLsizei depth,
		  GLenum format, GLenum type,
		  const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage) 
{
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *r700NewTextureObject(GLcontext * ctx,
						      GLuint name,
						      GLenum target) 
{
    context_t *context = R700_CONTEXT(ctx);

	struct gl_texture_object *obj;

	obj = _mesa_new_texture_object(ctx, name, target);
	if (!obj)
	{
		return NULL;
	}

    //obj->MaxAnisotropy = context->initialMaxAnisotropy;

	r700AllocTexObj(obj);

	return obj;
}

static void r700BindTexture(GLcontext * ctx, GLenum target,
			    struct gl_texture_object *texObj)  
{
    if ((target == GL_TEXTURE_1D)
        || (target == GL_TEXTURE_2D)
        || (target == GL_TEXTURE_3D)
        || (target == GL_TEXTURE_CUBE_MAP)
        || (target == GL_TEXTURE_RECTANGLE_NV)) 
    {
	    assert(texObj->DriverData != NULL);
    }
}

static void r700DeleteTexture(GLcontext * ctx, struct gl_texture_object *texObj)
{
} 

#if 0 /* to be enabled */
static void r700SetTexMinFilter(r700TexObjPtr t, GLenum minf)
{
    switch (minf)
    {
    case GL_NEAREST:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Point,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                        
               
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_None,
                 MIP_FILTER_shift, MIP_FILTER_mask);        
        break;
    case GL_LINEAR:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Linear,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                         
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_None,
                 MIP_FILTER_shift, MIP_FILTER_mask);                
        break;
    case GL_NEAREST_MIPMAP_NEAREST:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Point,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                        
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_Point,
                 MIP_FILTER_shift, MIP_FILTER_mask);               
        break;
    case GL_LINEAR_MIPMAP_NEAREST:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Linear,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                        
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_Point,
                 MIP_FILTER_shift, MIP_FILTER_mask);               
        break;
    case GL_NEAREST_MIPMAP_LINEAR:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Point,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                         
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_Linear,
                 MIP_FILTER_shift, MIP_FILTER_mask);               
        break;
    case GL_LINEAR_MIPMAP_LINEAR:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Linear,
                 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);                       
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_MipFilter_Linear,
                 MIP_FILTER_shift, MIP_FILTER_mask);                
        break;
    default:
        /* no case */
        break;
    }
}

static void r700SetTexMagFilter(r700TexObjPtr t, GLenum magf)
{
    switch(magf)
    {
    case GL_NEAREST:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Point,
                 XY_MAG_FILTER_shift, XY_MAG_FILTER_mask);                        
                    
        break;
    case GL_LINEAR:
        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, TEX_XYFilter_Linear,
                 XY_MAG_FILTER_shift, XY_MAG_FILTER_mask);                         
                   
        break;
    default:
        break;
    }
}

static unsigned int r700GetWrapMode(GLenum wrapmode)
{
    switch(wrapmode) 
    {
    case GL_REPEAT: 
        return SQ_TEX_WRAP;
    case GL_CLAMP: 
        return SQ_TEX_CLAMP_HALF_BORDER;
    case GL_CLAMP_TO_EDGE: 
        return SQ_TEX_CLAMP_LAST_TEXEL;
    case GL_CLAMP_TO_BORDER: 
        return SQ_TEX_CLAMP_BORDER;
    case GL_MIRRORED_REPEAT: 
        return SQ_TEX_MIRROR_ONCE_HALF_BORDER;
    case GL_MIRROR_CLAMP_EXT: 
        return SQ_TEX_MIRROR;
    case GL_MIRROR_CLAMP_TO_EDGE_EXT: 
        return SQ_TEX_MIRROR_ONCE_BORDER;
    case GL_MIRROR_CLAMP_TO_BORDER_EXT: 
        return SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
    default:
	    _mesa_problem(NULL, "bad wrap mode in %s", __FUNCTION__);
	    return 0;
    }
}
#endif /* to be enabled */

static void r700TexParameter(GLcontext * ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     GLenum pname, const GLfloat * params)  
{
    r700TexObjPtr t = (r700TexObjPtr) texObj->DriverData;
#if 0 /* to be enabled */
    switch (pname) 
    {
    case GL_TEXTURE_MIN_FILTER:
        r700SetTexMinFilter(t, texObj->MinFilter);
        break;
    case GL_TEXTURE_MAG_FILTER:
        r700SetTexMagFilter(t, texObj->MagFilter);
        break;
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:	    
        
        r700SetTexMinFilter(t, texObj->MinFilter);
        r700SetTexMagFilter(t, texObj->MagFilter);

	    break;

    case GL_TEXTURE_WRAP_S:        
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, r700GetWrapMode(texObj->WrapS),
                 SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_shift, SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_mask);
        break;
    case GL_TEXTURE_WRAP_T:
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, r700GetWrapMode(texObj->WrapT), 
                 CLAMP_Y_shift, CLAMP_Y_mask);
        break;
    case GL_TEXTURE_WRAP_R:
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, r700GetWrapMode(texObj->WrapR),
                 CLAMP_Z_shift, CLAMP_Z_mask);
	    break;

    case GL_TEXTURE_BORDER_COLOR:
        /* TODO : set border color regs before rendering. */
        SETfield(t->sampler_state.SQ_TEX_SAMPLER0.u32All, SQ_TEX_BORDER_COLOR_REGISTER,
                 BORDER_COLOR_TYPE_shift, BORDER_COLOR_TYPE_mask); 
	    break;

    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
	    /* TODO : we do support this, add it later. */
	    driSwapOutTextureObject((driTextureObject *) t);
	    break;

    case GL_DEPTH_TEXTURE_MODE:
	    if (!texObj->Image[0][texObj->BaseLevel])
        {
		    return;
        }
	    if (texObj->Image[0][texObj->BaseLevel]->TexFormat->BaseFormat
	        == GL_DEPTH_COMPONENT) 
        {
		    /* TODO : r700SetDepthTexMode(texObj); */
		    break;
	    } 
        else 
        {
            /* If not depth texture, just return. */
		    return;
	    }

    default:        
	    return;
    }
#endif /* to be enabled */
}

static void r700CompressedTexImage2D(GLcontext * ctx, GLenum target,
				     GLint level, GLint internalFormat,
				     GLint width, GLint height, GLint border,
				     GLsizei imageSize, const GLvoid * data,
				     struct gl_texture_object *texObj,
				     struct gl_texture_image *texImage)  
{
}

static void r700CompressedTexSubImage2D(GLcontext * ctx, GLenum target,
					GLint level, GLint xoffset,
					GLint yoffset, GLsizei width,
					GLsizei height, GLenum format,
					GLsizei imageSize, const GLvoid * data,
					struct gl_texture_object *texObj,
					struct gl_texture_image *texImage)  
{
}

static GLboolean r700UpdateTextureUnit(GLcontext * ctx, int unit)
{
    return GL_TRUE;
}

static GLboolean r700EnableTextureRect(GLcontext * ctx, int unit)
{
    return GL_TRUE;
}

static GLboolean r700EnableTexture3D(GLcontext * ctx, int unit)
{
    return GL_TRUE;
}

static GLboolean r700EnableTextureCube(GLcontext * ctx, int unit)
{
    return GL_TRUE;
}

static GLboolean r700UpdateTexture(GLcontext * ctx, int unit)
{
#if 0 /* to be enabled */
    context_t         *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700    = (R700_CHIP_CONTEXT*)(context->chipobj.pvChipObj);

    struct gl_texture_unit   *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj    = texUnit->_Current;
	r700TexObjPtr             t       = (r700TexObjPtr) tObj->DriverData;

    if( r700->texture_states.textures[unit] != &(t->texture_state) )
    {
        if(NULL != r700->texture_states.textures[unit])
        {   /* there is an old one. */
        }

        r700->texture_states.textures[unit] = &(t->texture_state);
        r700->texture_states.samplers[unit] = &(t->sampler_state);
        driUpdateTextureLRU((driTextureObject *) t);	/* XXX: should be locked! */
    }
#endif /* to be enabled */

    return GL_TRUE;
}

void r700UpdateTextureState(context_t * context)  
{
#if 0 /* to be enabled */
    GLboolean bRet;
    GLuint    unit;
    GLcontext * ctx = context->ctx;
    struct gl_texture_unit *texUnit;

    for (unit = 0; unit < 8; unit++) 
    {
        texUnit = &ctx->Texture.Unit[unit];

        if (texUnit->_ReallyEnabled & (TEXTURE_RECT_BIT)) 
        {
            bRet = (r700EnableTextureRect(ctx, unit) &&
                    r700UpdateTexture(ctx, unit));
        } 
        else if (texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT)) 
        {
            bRet = (r700EnableTexture2D(ctx, unit) &&
                    r700UpdateTexture(ctx, unit));
        } 
        else if (texUnit->_ReallyEnabled & (TEXTURE_3D_BIT)) 
        {
            bRet = (r700EnableTexture3D(ctx, unit) &&
                    r700UpdateTexture(ctx, unit));
        } 
        else if (texUnit->_ReallyEnabled & (TEXTURE_CUBE_BIT)) 
        {
            bRet = (r700EnableTextureCube(ctx, unit) &&
                    r700UpdateTexture(ctx, unit));
        } 
        else if (texUnit->_ReallyEnabled) 
        {
            bRet = GL_FALSE;
        } 
        else 
        {
            bRet = GL_TRUE;
        }

        if (!bRet) 
        {
	        _mesa_warning(ctx, "failed to update texture state for unit %d.\n", unit);
        }
    }
#endif /* to be enabled */
}

void r700InitTextureFuncs(struct dd_function_table *functions) 
{
	/* Note: we only plug in the functions we implement in the driver
	 * since _mesa_init_driver_functions() was already called.
	 */
	functions->ChooseTextureFormat = radeonChooseTextureFormat_mesa;
	functions->TexImage1D = r700TexImage1D;
	functions->TexImage2D = r700TexImage2D;
	functions->TexImage3D = r700TexImage3D;
	functions->TexSubImage1D = r700TexSubImage1D;
	functions->TexSubImage2D = r700TexSubImage2D;
	functions->TexSubImage3D = r700TexSubImage3D;
	functions->NewTextureObject = r700NewTextureObject;
	functions->BindTexture = r700BindTexture;
	functions->DeleteTexture = r700DeleteTexture;
	functions->IsTextureResident = driIsTextureResident;

	functions->TexParameter = r700TexParameter;

	functions->CompressedTexImage2D = r700CompressedTexImage2D;
	functions->CompressedTexSubImage2D = r700CompressedTexSubImage2D;

	driInitTextureFormats();
}


