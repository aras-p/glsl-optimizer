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

#ifndef __r700_TEX_H__
#define __r700_TEX_H__

#include "texmem.h"

#include "r700_chip.h"

/* TODO : review this after texture load code. */
#define R700_BLIT_WIDTH_BYTES 1024
/* The BASE_ADDRESS and MIP_ADDRESS fields are 256-byte-aligned */
#define R700_TEXTURE_ALIGNMENT_MASK     0x255
/* Texel pitch is 8 alignment. */
#define R700_TEXEL_PITCH_ALIGNMENT_MASK 0x7

#define R700_MAX_TEXTURE_UNITS 8 /* TODO : should be 16, lets make it work, review later */

typedef struct r700_tex_obj r700TexObj, *r700TexObjPtr;

/* Texture object in locally shared texture space.
 */
struct r700_tex_obj 
{
	driTextureObject base;

	/* r300 tex obj */
	GLuint bufAddr;	
	GLboolean image_override;
	GLuint pitch;		
	GLuint filter;
	GLuint filter_1;
	GLuint pitch_reg;
	GLuint size;		
	GLuint format;
	GLuint offset;		
	GLuint unknown4;
	GLuint unknown5;
	GLboolean border_fallback;
	GLuint tile_bits;
    
    /* r700 texture states */
    TEXTURE_STATE_STRUCT texture_state;
    SAMPLER_STATE_STRUCT sampler_state;

    GLuint texel_pitch[6][RADEON_MAX_TEXTURE_LEVELS];
    GLuint level_offset[6][RADEON_MAX_TEXTURE_LEVELS];
    GLuint byte_per_texel;    
    GLuint src_width_in_pexel[6][RADEON_MAX_TEXTURE_LEVELS];
    GLuint src_hight_in_pexel[6][RADEON_MAX_TEXTURE_LEVELS];

    GLuint my_dirty_images[6]; /* TODO : review */
};

extern void r700SetTexBuffer(__DRIcontext *pDRICtx, GLint target,
			     __DRIdrawable *dPriv);

extern void r700SetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
			      GLint format, __DRIdrawable *dPriv);

extern void r700SetTexOffset(__DRIcontext *pDRICtx, GLint texname,
			     unsigned long long offset, GLint depth,
			     GLuint pitch);

extern GLuint r700GetTexObjSize(void);
extern void r700UpdateTextureState(context_t * context);

extern void r700SetTexOffset(__DRIcontext *pDRICtx, 
                             GLint texname,
			                 unsigned long long offset, 
                             GLint depth,
			                 GLuint pitch);

extern void r700DestroyTexObj(context_t rmesa, r700TexObjPtr t);

extern GLboolean r700ValidateBuffers(GLcontext * ctx);

extern void r700InitTextureFuncs(struct dd_function_table *functions);

#endif /* __r700_TEX_H__ */
