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
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "teximage.h"
#include "texobj.h"
#include "texstore.h"
#include "texformat.h"
#include "swrast/swrast.h"
#include "context.h"
#include "via_context.h"
#include "via_tex.h"
#include "via_state.h"
#include "via_ioctl.h"


viaTextureObjectPtr viaAllocTextureObject(struct gl_texture_object *texObj)
{
    viaTextureObjectPtr t;

    t = (viaTextureObjectPtr)CALLOC_STRUCT(via_texture_object_t);
    if (!t)
        return NULL;

    /* Initialize non-image-dependent parts of the state:
     */
    t->bufAddr = NULL;
    t->dirtyImages = ~0;
    t->actualLevel = 0;
    t->globj = texObj;
    make_empty_list(t);

    return t;
}

static void viaTexImage1D(GLcontext *ctx, GLenum target, GLint level,
                          GLint internalFormat,
                          GLint width, GLint border,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
    if (VIA_DEBUG) fprintf(stderr, "viaTexImage1D - in\n");
    if (t) {
        if (level == 0) {
    	    viaSwapOutTexObj(VIA_CONTEXT(ctx), t);
	    t->actualLevel = 0;
	}
	else
	    t->actualLevel++;
    }
    else {
       	t = viaAllocTextureObject(texObj);
      	if (!t) {
 	        _mesa_error(ctx, GL_OUT_OF_MEMORY, "viaTexImage1D");
            return;
        }
        texObj->DriverData = t;
    }
    _mesa_store_teximage1d(ctx, target, level, internalFormat,
                           width, border, format, type,
                           pixels, packing, texObj, texImage);
    if (VIA_DEBUG) fprintf(stderr, "viaTexImage1D - out\n");
}
static void viaTexSubImage1D(GLcontext *ctx,
                             GLenum target,
                             GLint level,
                             GLint xoffset,
                             GLsizei width,
                             GLenum format, GLenum type,
                             const GLvoid *pixels,
                             const struct gl_pixelstore_attrib *packing,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage)

{
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
    
    if (t) {
        viaSwapOutTexObj(VIA_CONTEXT(ctx), t);
    }
    _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			      format, type, pixels, packing, texObj,
                              texImage);

}


static void viaTexImage2D(GLcontext *ctx, GLenum target, GLint level,
                          GLint internalFormat,
                          GLint width, GLint height, GLint border,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
    if (VIA_DEBUG) fprintf(stderr, "viaTexImage2D - in\n");
    if (t) {
        if (level == 0) {
    	    viaSwapOutTexObj(VIA_CONTEXT(ctx), t);
	    t->actualLevel = 0;
	}
	else
	    t->actualLevel++;
    }
    else {
       	t = viaAllocTextureObject(texObj);
      	if (!t) {
 	        _mesa_error(ctx, GL_OUT_OF_MEMORY, "viaTexImage2D");
            return;
        }
        texObj->DriverData = t;
    }
    _mesa_store_teximage2d(ctx, target, level, internalFormat,
                           width, height, border, format, type,
                           pixels, packing, texObj, texImage);
    if (VIA_DEBUG) fprintf(stderr, "viaTexImage2D - out\n");
}

static void viaTexSubImage2D(GLcontext *ctx,
                             GLenum target,
                             GLint level,
                             GLint xoffset, GLint yoffset,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type,
                             const GLvoid *pixels,
                             const struct gl_pixelstore_attrib *packing,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
    
    if (t) {
        viaSwapOutTexObj(VIA_CONTEXT(ctx), t);
    }
    _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
                              height, format, type, pixels, packing, texObj,
                              texImage);

    if(vmesa->shareCtx)
	vmesa->shareCtx->NewState |= _NEW_TEXTURE;

}

static void viaBindTexture(GLcontext *ctx, GLenum target,
                           struct gl_texture_object *texObj)
{
    if (VIA_DEBUG) fprintf(stderr, "viaBindTexture - in\n");
    if (target == GL_TEXTURE_2D) {
        viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;

        if (!t) {

            t = viaAllocTextureObject(texObj);
      	    if (!t) {
 	            _mesa_error(ctx, GL_OUT_OF_MEMORY, "viaBindTexture");
                return;
            }
            texObj->DriverData = t;
        }
    }
    if (VIA_DEBUG) fprintf(stderr, "viaBindTexture - out\n");
}

static void viaDeleteTexture(GLcontext *ctx, struct gl_texture_object *texObj)
{
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
    if (VIA_DEBUG) fprintf(stderr, "viaDeleteTexture - in\n");    
    if (t) {
        viaContextPtr vmesa = VIA_CONTEXT(ctx);
        if (vmesa) {
	    if (vmesa->dma) { /* imply vmesa is not under destroying */
        	VIA_FLUSH_DMA(vmesa);
	    }
    	    viaDestroyTexObj(vmesa, t);
	}
        texObj->DriverData = 0;
    }
    if (VIA_DEBUG) fprintf(stderr, "viaDeleteTexture - out\n");    

   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, texObj);
}

static GLboolean viaIsTextureResident(GLcontext *ctx,
                                      struct gl_texture_object *texObj)
{
    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;

    return t && t->bufAddr;
}

static const struct gl_texture_format *
viaChooseTexFormat(GLcontext *ctx, GLint internalFormat,
	           GLenum format, GLenum type)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    (void)format;
    (void)type;
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);    
    if (VIA_DEBUG) fprintf(stderr, "internalFormat:%d format:%d\n", internalFormat, format);    
    switch (internalFormat) {
    case 1:
    case GL_LUMINANCE:
    case GL_LUMINANCE4:    
    case GL_LUMINANCE8:    
    case GL_LUMINANCE12:    
    case GL_LUMINANCE16:    
        return &_mesa_texformat_l8;
    case 2:
    case GL_LUMINANCE_ALPHA:    
    case GL_LUMINANCE4_ALPHA4:        
    case GL_LUMINANCE6_ALPHA2:    
    case GL_LUMINANCE8_ALPHA8:    
    case GL_LUMINANCE12_ALPHA4:    
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:        
        return &_mesa_texformat_al88;
    case GL_R3_G3_B2:	
    case GL_RGB4:    
    case GL_RGB5:
	if (VIA_DEBUG) fprintf(stderr, "2 &_mesa_texformat_arg565\n");    
        return &_mesa_texformat_rgb565;    
    case 3:
    case GL_RGB:
    case GL_RGB8:    
    case GL_RGB10:
    case GL_RGB12:    
    case GL_RGB16:
	if (vmesa->viaScreen->bitsPerPixel == 0x20) {
	    if (VIA_DEBUG) fprintf(stderr,"3 argb8888\n");
	    return &_mesa_texformat_argb8888;
	}	    
	else {
	    if (VIA_DEBUG) fprintf(stderr,"3 rgb565\n");	
            return &_mesa_texformat_rgb565;
	}
    case 4:
	if (vmesa->viaScreen->bitsPerPixel == 0x20) {
	    if (VIA_DEBUG) fprintf(stderr, "4 &_mesa_texformat_argb8888\n");
    	    return &_mesa_texformat_argb8888;
	}
	else {
	    if (VIA_DEBUG) fprintf(stderr, "4 &_mesa_texformat_argb4444\n");
    	    return &_mesa_texformat_argb4444;		    
	}
    case GL_RGBA2:    
    case GL_RGBA4:	    
	if (VIA_DEBUG) fprintf(stderr, "GL_RGBA4 &_mesa_texformat_argb4444\n");    
        return &_mesa_texformat_argb4444;

    case GL_RGB5_A1:
	if (VIA_DEBUG) fprintf(stderr, "GL_RGB5_A1 &_mesa_texformat_argb1555\n");
        return &_mesa_texformat_argb1555;    
    case GL_RGBA:
    case GL_RGBA8:
    case GL_RGBA12:
    case GL_RGBA16:    
    case GL_RGB10_A2:
	if (VIA_DEBUG) fprintf(stderr, "GL_RGBA &_mesa_texformat_argb8888\n");
        return &_mesa_texformat_argb8888;
    case GL_ALPHA:	
    case GL_ALPHA4:    	
    case GL_ALPHA8:    	
    case GL_ALPHA12:
    case GL_ALPHA16:    
        return &_mesa_texformat_a8;    
    case GL_INTENSITY:
    case GL_INTENSITY4:    	
    case GL_INTENSITY8:    	
    case GL_INTENSITY12:
    case GL_INTENSITY16:    
        return &_mesa_texformat_i8;
    case GL_COLOR_INDEX:	
    case GL_COLOR_INDEX1_EXT:	
    case GL_COLOR_INDEX2_EXT:	
    case GL_COLOR_INDEX4_EXT:	
    case GL_COLOR_INDEX8_EXT:	
    case GL_COLOR_INDEX12_EXT:	    
    case GL_COLOR_INDEX16_EXT:
        return &_mesa_texformat_ci8;    
    default:
        _mesa_problem(ctx, "unexpected format in viaChooseTextureFormat");
        return NULL;
    }	
}		   

void viaInitTextureFuncs(struct dd_function_table * functions)
{
    if (VIA_DEBUG) fprintf(stderr, "viaInitTextureFuncs - in\n");
    functions->ChooseTextureFormat = viaChooseTexFormat;
    functions->TexImage1D = viaTexImage1D;
    functions->TexImage2D = viaTexImage2D;
    functions->TexSubImage1D = viaTexSubImage1D;
    functions->TexSubImage2D = viaTexSubImage2D;

    functions->NewTextureObject = _mesa_new_texture_object;
    functions->BindTexture = viaBindTexture;
    functions->DeleteTexture = viaDeleteTexture;
    functions->UpdateTexturePalette = 0;
    functions->IsTextureResident = viaIsTextureResident;

    if (VIA_DEBUG) fprintf(stderr, "viaInitTextureFuncs - out\n");
}

void viaInitTextures(GLcontext *ctx)
{
    GLuint tmp = ctx->Texture.CurrentUnit;
    ctx->Texture.CurrentUnit = 0;
    viaBindTexture(ctx, GL_TEXTURE_1D, ctx->Texture.Unit[0].Current1D);
    viaBindTexture(ctx, GL_TEXTURE_2D, ctx->Texture.Unit[0].Current2D);
    ctx->Texture.CurrentUnit = 1;
    viaBindTexture(ctx, GL_TEXTURE_1D, ctx->Texture.Unit[1].Current1D);
    viaBindTexture(ctx, GL_TEXTURE_2D, ctx->Texture.Unit[1].Current2D);
    ctx->Texture.CurrentUnit = tmp;
}
