/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_tex.c,v 1.7 2002/11/05 17:46:10 tsi Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *
 */

#include "image.h"
#include "texutil.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "tdfx_context.h"
#include "tdfx_tex.h"
#include "tdfx_texman.h"



static int
logbase2(int n)
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


/*
 * Compute various texture image parameters.
 * Input:  w, h - source texture width and height
 * Output:  lodlevel - Glide lod level token for the larger texture dimension
 *          aspectratio - Glide aspect ratio token
 *          sscale - S scale factor used during triangle setup
 *          tscale - T scale factor used during triangle setup
 *          wscale - OpenGL -> Glide image width scale factor
 *          hscale - OpenGL -> Glide image height scale factor
 *
 * Sample results:
 *      w    h       lodlevel               aspectRatio
 *     128  128  GR_LOD_LOG2_128 (=7)  GR_ASPECT_LOG2_1x1 (=0)
 *      64   64  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_1x1 (=0)
 *      64   32  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_2x1 (=1)
 *      32   64  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_1x2 (=-1)
 *      32   32  GR_LOD_LOG2_32 (=5)   GR_ASPECT_LOG2_1x1 (=0)
 */
static void
tdfxTexGetInfo(const GLcontext *ctx, int w, int h,
               GrLOD_t *lodlevel, GrAspectRatio_t *aspectratio,
               float *sscale, float *tscale,
               int *wscale, int *hscale)
{
    int logw, logh, ar, lod, ws, hs;
    float s, t;

    ASSERT(w >= 1);
    ASSERT(h >= 1);

    logw = logbase2(w);
    logh = logbase2(h);
    ar = logw - logh;  /* aspect ratio = difference in log dimensions */

    /* Hardware only allows a maximum aspect ratio of 8x1, so handle
       |ar| > 3 by scaling the image and using an 8x1 aspect ratio */
    if (ar >= 0) {
        ASSERT(width >= height);
        lod = logw;
        s = 256.0;
        ws = 1;
        if (ar <= GR_ASPECT_LOG2_8x1) {
            t = 256 >> ar;
            hs = 1;
        }
        else {
            /* have to stretch image height */
            t = 32.0;
            hs = 1 << (ar - 3);
        }
    }
    else {
        ASSERT(width < height);
        lod = logh;
        t = 256.0;
        hs = 1;
        if (ar >= GR_ASPECT_LOG2_1x8) {
            s = 256 >> -ar;
            ws = 1;
        }
        else {
            /* have to stretch image width */
            s = 32.0;
            ws = 1 << (-ar - 3);
        }
    }

    if (ar < GR_ASPECT_LOG2_1x8)
        ar = GR_ASPECT_LOG2_1x8;
    else if (ar > GR_ASPECT_LOG2_8x1)
        ar = GR_ASPECT_LOG2_8x1;

    if (lodlevel)
        *lodlevel = (GrLOD_t) lod;
    if (aspectratio)
        *aspectratio = (GrAspectRatio_t) ar;
    if (sscale)
        *sscale = s;
    if (tscale)
        *tscale = t;
    if (wscale)
        *wscale = ws;
    if (hscale)
        *hscale = hs;
}


/*
 * We need to call this when a texture object's minification filter
 * or texture image sizes change.
 */
static void RevalidateTexture(GLcontext *ctx, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    GLint minl, maxl;

    if (!ti)
       return;

    minl = maxl = tObj->BaseLevel;

    if (tObj->Image[minl]) {
       maxl = MIN2(tObj->MaxLevel, tObj->Image[minl]->MaxLog2);

       /* compute largeLodLog2, aspect ratio and texcoord scale factors */
       tdfxTexGetInfo(ctx, tObj->Image[minl]->Width, tObj->Image[minl]->Height,
                      &ti->info.largeLodLog2,
                      &ti->info.aspectRatioLog2,
                      &(ti->sScale), &(ti->tScale), NULL, NULL);
    }

    if (tObj->Image[maxl] && (tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR)) {
        /* mipmapping: need to compute smallLodLog2 */
        tdfxTexGetInfo(ctx, tObj->Image[maxl]->Width,
                       tObj->Image[maxl]->Height,
                       &ti->info.smallLodLog2, NULL,
                       NULL, NULL, NULL, NULL);
    }
    else {
        /* not mipmapping: smallLodLog2 = largeLodLog2 */
        ti->info.smallLodLog2 = ti->info.largeLodLog2;
        maxl = minl;
    }

    ti->minLevel = minl;
    ti->maxLevel = maxl;
    ti->info.data = NULL;
}


static tdfxTexInfo *
fxAllocTexObjData(tdfxContextPtr fxMesa)
{
    tdfxTexInfo *ti;

    if (!(ti = CALLOC(sizeof(tdfxTexInfo)))) {
        _mesa_problem(NULL, "tdfx driver: out of memory");
        return NULL;
    }

    ti->isInTM = GL_FALSE;

    ti->whichTMU = TDFX_TMU_NONE;

    ti->tm[TDFX_TMU0] = NULL;
    ti->tm[TDFX_TMU1] = NULL;

    ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
    ti->magFilt = GR_TEXTUREFILTER_BILINEAR;

    ti->sClamp = GR_TEXTURECLAMP_WRAP;
    ti->tClamp = GR_TEXTURECLAMP_WRAP;

    ti->mmMode = GR_MIPMAP_NEAREST;
    ti->LODblend = FXFALSE;

    return ti;
}


/*
 * Called via glBindTexture.
 */

void
tdfxDDBindTexture(GLcontext * ctx, GLenum target,
                  struct gl_texture_object *tObj)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxDDTexBind(%d,%p)\n", tObj->Name,
                tObj->DriverData);
    }

    if (target != GL_TEXTURE_2D)
        return;

    if (!tObj->DriverData) {
        tObj->DriverData = fxAllocTexObjData(fxMesa);
    }

    ti = TDFX_TEXTURE_DATA(tObj);
    ti->lastTimeUsed = fxMesa->texBindNumber++;

    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


/*
 * Called via glTexEnv.
 */
void
tdfxDDTexEnv(GLcontext * ctx, GLenum target, GLenum pname,
             const GLfloat * param)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

    if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
        if (param)
            fprintf(stderr, "fxmesa: texenv(%x,%x)\n", pname,
                    (GLint) (*param));
        else
            fprintf(stderr, "fxmesa: texenv(%x)\n", pname);
    }

    /* XXX this is a bit of a hack to force the Glide texture
     * state to be updated.
     */
    fxMesa->TexState.EnvMode[ctx->Texture.CurrentUnit]  = 0;

    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


/*
 * Called via glTexParameter.
 */
void
tdfxDDTexParameter(GLcontext * ctx, GLenum target,
                   struct gl_texture_object *tObj,
                   GLenum pname, const GLfloat * params)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    GLenum param = (GLenum) (GLint) params[0];
    tdfxTexInfo *ti;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxDDTexParam(%d,%p,%x,%x)\n", tObj->Name,
                tObj->DriverData, pname, param);
    }

    if (target != GL_TEXTURE_2D)
        return;

    if (!tObj->DriverData)
        tObj->DriverData = fxAllocTexObjData(fxMesa);

    ti = TDFX_TEXTURE_DATA(tObj);

    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
        switch (param) {
        case GL_NEAREST:
            ti->mmMode = GR_MIPMAP_DISABLE;
            ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
            ti->LODblend = FXFALSE;
            break;
        case GL_LINEAR:
            ti->mmMode = GR_MIPMAP_DISABLE;
            ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
            ti->LODblend = FXFALSE;
            break;
        case GL_NEAREST_MIPMAP_LINEAR:
            if (TDFX_IS_NAPALM(fxMesa)) {
                 if (fxMesa->haveTwoTMUs) {
                     ti->mmMode = GR_MIPMAP_NEAREST;
                     ti->LODblend = FXTRUE;
                 }
                 else {
                     ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
                     ti->LODblend = FXFALSE;
                 }
                 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
                 break;
            }
            /* XXX Voodoo3/Banshee mipmap blending seems to produce
             * incorrectly filtered colors for the smallest mipmap levels.
             * To work-around we fall-through here and use a different filter.
             */
        case GL_NEAREST_MIPMAP_NEAREST:
            ti->mmMode = GR_MIPMAP_NEAREST;
            ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
            ti->LODblend = FXFALSE;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            if (TDFX_IS_NAPALM(fxMesa)) {
                if (fxMesa->haveTwoTMUs) {
                    ti->mmMode = GR_MIPMAP_NEAREST;
                    ti->LODblend = FXTRUE;
                }
                else {
                    ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
                    ti->LODblend = FXFALSE;
                }
                ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
                break;
            }
            /* XXX Voodoo3/Banshee mipmap blending seems to produce
             * incorrectly filtered colors for the smallest mipmap levels.
             * To work-around we fall-through here and use a different filter.
             */
        case GL_LINEAR_MIPMAP_NEAREST:
            ti->mmMode = GR_MIPMAP_NEAREST;
            ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
            ti->LODblend = FXFALSE;
            break;
        default:
            break;
        }
        RevalidateTexture(ctx, tObj);
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_MAG_FILTER:
        switch (param) {
        case GL_NEAREST:
            ti->magFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
            break;
        case GL_LINEAR:
            ti->magFilt = GR_TEXTUREFILTER_BILINEAR;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_WRAP_S:
        switch (param) {
        case GL_CLAMP:
            ti->sClamp = GR_TEXTURECLAMP_CLAMP;
            break;
        case GL_REPEAT:
            ti->sClamp = GR_TEXTURECLAMP_WRAP;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_WRAP_T:
        switch (param) {
        case GL_CLAMP:
            ti->tClamp = GR_TEXTURECLAMP_CLAMP;
            break;
        case GL_REPEAT:
            ti->tClamp = GR_TEXTURECLAMP_WRAP;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_BORDER_COLOR:
        /* TO DO */
        break;
    case GL_TEXTURE_MIN_LOD:
        /* TO DO */
        break;
    case GL_TEXTURE_MAX_LOD:
        /* TO DO */
        break;
    case GL_TEXTURE_BASE_LEVEL:
        RevalidateTexture(ctx, tObj);
        break;
    case GL_TEXTURE_MAX_LEVEL:
        RevalidateTexture(ctx, tObj);
        break;

    default:
        break;
    }
}


/*
 * Called via glDeleteTextures to delete a texture object.
 * Here, we delete the Glide data associated with the texture.
 */
void
tdfxDDDeleteTexture(GLcontext * ctx, struct gl_texture_object *tObj)
{
    if (ctx && ctx->DriverCtx) {
        tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
        tdfxTMFreeTexture(fxMesa, tObj);
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
    }
}


/*
 * Return true if texture is resident, false otherwise.
 */
GLboolean
tdfxDDIsTextureResident(GLcontext *ctx, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    return (GLboolean) (ti && ti->isInTM);
}



/*
 * Convert a gl_color_table texture palette to Glide's format.
 */
static void
convertPalette(FxU32 data[256], const struct gl_color_table *table)
{
    const GLubyte *tableUB = (const GLubyte *) table->Table;
    GLint width = table->Size;
    FxU32 r, g, b, a;
    GLint i;

    ASSERT(table->TableType == GL_UNSIGNED_BYTE);

    switch (table->Format) {
    case GL_INTENSITY:
        for (i = 0; i < width; i++) {
            r = tableUB[i];
            g = tableUB[i];
            b = tableUB[i];
            a = tableUB[i];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    case GL_LUMINANCE:
        for (i = 0; i < width; i++) {
            r = tableUB[i];
            g = tableUB[i];
            b = tableUB[i];
            a = 255;
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    case GL_ALPHA:
        for (i = 0; i < width; i++) {
            r = g = b = 255;
            a = tableUB[i];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    case GL_LUMINANCE_ALPHA:
        for (i = 0; i < width; i++) {
            r = g = b = tableUB[i * 2 + 0];
            a = tableUB[i * 2 + 1];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    case GL_RGB:
        for (i = 0; i < width; i++) {
            r = tableUB[i * 3 + 0];
            g = tableUB[i * 3 + 1];
            b = tableUB[i * 3 + 2];
            a = 255;
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    case GL_RGBA:
        for (i = 0; i < width; i++) {
            r = tableUB[i * 4 + 0];
            g = tableUB[i * 4 + 1];
            b = tableUB[i * 4 + 2];
            a = tableUB[i * 4 + 3];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        break;
    }
}



void
tdfxDDTexturePalette(GLcontext * ctx, struct gl_texture_object *tObj)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

    if (tObj) {
        /* per-texture palette */
        tdfxTexInfo *ti;
        
        /* This might be a proxy texture. */
        if (!tObj->Palette.Table)
            return;
            
        if (!tObj->DriverData)
            tObj->DriverData = fxAllocTexObjData(fxMesa);
        ti = TDFX_TEXTURE_DATA(tObj);
        convertPalette(ti->palette.data, &tObj->Palette);
        /*tdfxTexInvalidate(ctx, tObj);*/
    }
    else {
        /* global texture palette */
        convertPalette(fxMesa->glbPalette.data, &ctx->Texture.Palette);
    }
    fxMesa->new_state |= TDFX_NEW_TEXTURE; /* XXX too heavy-handed */
}


/**********************************************************************/
/**** NEW TEXTURE IMAGE FUNCTIONS                                  ****/
/**********************************************************************/

#if 000
static FxBool TexusFatalError = FXFALSE;
static FxBool TexusError = FXFALSE;

#define TX_DITHER_NONE                                  0x00000000

static void
fxTexusError(const char *string, FxBool fatal)
{
    _mesa_problem(NULL, string);
   /*
    * Just propagate the fatal value up.
    */
    TexusError = FXTRUE;
    TexusFatalError = fatal;
}
#endif


const struct gl_texture_format *
tdfxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                           GLenum srcFormat, GLenum srcType )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   const GLboolean allow32bpt = TDFX_IS_NAPALM(fxMesa);

   switch (internalFormat) {
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return &_mesa_texformat_a8;
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
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return &_mesa_texformat_i8;
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      return &_mesa_texformat_rgb565;
   case 3:
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return (allow32bpt) ? &_mesa_texformat_argb8888
                          : &_mesa_texformat_rgb565;
      break;
   case GL_RGBA2:
   case GL_RGBA4:
      return &_mesa_texformat_argb4444;
   case 4:
   case GL_RGBA:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return allow32bpt ? &_mesa_texformat_argb8888
                        : &_mesa_texformat_argb4444;
   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
   default:
      _mesa_problem(ctx, "unexpected format in tdfxDDChooseTextureFormat");
      return NULL;
   }
}


/*
 * Return the Glide format for the given mesa texture format.
 */
static GrTextureFormat_t
fxGlideFormat(GLint mesaFormat)
{
   switch (mesaFormat) {
   case MESA_FORMAT_I8:
      return GR_TEXFMT_ALPHA_8;
   case MESA_FORMAT_A8:
      return GR_TEXFMT_ALPHA_8;
   case MESA_FORMAT_L8:
      return GR_TEXFMT_INTENSITY_8;
   case MESA_FORMAT_CI8:
      return GR_TEXFMT_P_8;
   case MESA_FORMAT_AL88:
      return GR_TEXFMT_ALPHA_INTENSITY_88;
   case MESA_FORMAT_RGB565:
      return GR_TEXFMT_RGB_565;
   case MESA_FORMAT_ARGB4444:
      return GR_TEXFMT_ARGB_4444;
   case MESA_FORMAT_ARGB1555:
      return GR_TEXFMT_ARGB_1555;
   case MESA_FORMAT_ARGB8888:
      return GR_TEXFMT_ARGB_8888;
   default:
      _mesa_problem(NULL, "Unexpected format in fxGlideFormat");
      return 0;
   }
}


/* Texel-fetch functions for software texturing and glGetTexImage().
 * We should have been able to use some "standard" fetch functions (which
 * may get defined in texutil.c) but we have to account for scaled texture
 * images on tdfx hardware (the 8:1 aspect ratio limit).
 * Hence, we need special functions here.
 */

static void
fetch_intensity8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLubyte *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = *texel;
    rgba[GCOMP] = *texel;
    rgba[BCOMP] = *texel;
    rgba[ACOMP] = *texel;
}


static void
fetch_luminance8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLubyte *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = *texel;
    rgba[GCOMP] = *texel;
    rgba[BCOMP] = *texel;
    rgba[ACOMP] = 255;
}


static void
fetch_alpha8(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLubyte *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;
    i = i * mml->width / texImage->Width;
    j = j * mml->height / texImage->Height;

    texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = 255;
    rgba[GCOMP] = 255;
    rgba[BCOMP] = 255;
    rgba[ACOMP] = *texel;
}


static void
fetch_index8(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    (void) mml;
   /* XXX todo */
}


static void
fetch_luminance8_alpha8(const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + (j * mml->width + i) * 2;
   rgba[RCOMP] = texel[0];
   rgba[GCOMP] = texel[0];
   rgba[BCOMP] = texel[0];
   rgba[ACOMP] = texel[1];
}


static void
fetch_r5g6b5(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 11) & 0x1f) * 255 / 31;
    rgba[GCOMP] = (((*texel) >> 5) & 0x3f) * 255 / 63;
    rgba[BCOMP] = (((*texel) >> 0) & 0x1f) * 255 / 31;
    rgba[ACOMP] = 255;
}


static void
fetch_r4g4b4a4(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 12) & 0xf) * 255 / 15;
    rgba[GCOMP] = (((*texel) >> 8) & 0xf) * 255 / 15;
    rgba[BCOMP] = (((*texel) >> 4) & 0xf) * 255 / 15;
    rgba[ACOMP] = (((*texel) >> 0) & 0xf) * 255 / 15;
}


static void
fetch_r5g5b5a1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 11) & 0x1f) * 255 / 31;
    rgba[GCOMP] = (((*texel) >> 6) & 0x1f) * 255 / 31;
    rgba[BCOMP] = (((*texel) >> 1) & 0x1f) * 255 / 31;
    rgba[ACOMP] = (((*texel) >> 0) & 0x01) * 255;
}


static void
fetch_a8r8g8b8(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLuint *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLuint *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 16) & 0xff);
    rgba[GCOMP] = (((*texel) >>  8) & 0xff);
    rgba[BCOMP] = (((*texel)      ) & 0xff);
    rgba[ACOMP] = (((*texel) >> 24) & 0xff);
}


static FetchTexelFunc
fxFetchFunction(GLint mesaFormat)
{
   switch (mesaFormat) {
   case MESA_FORMAT_I8:
      return fetch_intensity8;
   case MESA_FORMAT_A8:
      return fetch_alpha8;
   case MESA_FORMAT_L8:
      return fetch_luminance8;
   case MESA_FORMAT_CI8:
      return fetch_index8;
   case MESA_FORMAT_AL88:
      return fetch_luminance8_alpha8;
   case MESA_FORMAT_RGB565:
      return fetch_r5g6b5;
   case MESA_FORMAT_ARGB4444:
      return fetch_r4g4b4a4;
   case MESA_FORMAT_ARGB1555:
      return fetch_r5g5b5a1;
   case MESA_FORMAT_ARGB8888:
      return fetch_a8r8g8b8;
   default:
      _mesa_problem(NULL, "Unexpected format in fxFetchFunction");
      printf("%d\n", mesaFormat);
      return NULL;
   }
}


void
tdfxDDTexImage2D(GLcontext *ctx, GLenum target, GLint level,
               GLint internalFormat, GLint width, GLint height, GLint border,
               GLenum format, GLenum type, const GLvoid *pixels,
               const struct gl_pixelstore_attrib *packing,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint texelBytes;

    /*
    printf("TexImage id=%d int 0x%x  format 0x%x  type 0x%x  %dx%d\n",
           texObj->Name, texImage->IntFormat, format, type,
           texImage->Width, texImage->Height);
    */

    ti = TDFX_TEXTURE_DATA(texObj);
    if (!ti) {
        texObj->DriverData = fxAllocTexObjData(fxMesa);
        if (!texObj->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
        ti = TDFX_TEXTURE_DATA(texObj);
    }

    mml = TDFX_TEXIMAGE_DATA(texImage);
    if (!mml) {
        texImage->DriverData = CALLOC(sizeof(tdfxMipMapLevel));
        if (!texImage->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
        mml = TDFX_TEXIMAGE_DATA(texImage);
    }

    /* Determine width and height scale factors for texture.
     * Remember, Glide is limited to 8:1 aspect ratios.
     */
    tdfxTexGetInfo(ctx,
                   texImage->Width, texImage->Height,
                   NULL,       /* lod level          */
                   NULL,       /* aspect ratio       */
                   NULL, NULL, /* sscale, tscale     */
                   &mml->wScale, &mml->hScale);

    /* rescaled size: */
    mml->width = width * mml->wScale;
    mml->height = height * mml->hScale;


    /* choose the texture format */
    assert(ctx->Driver.ChooseTextureFormat);
    texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                     internalFormat, format, type);
    assert(texImage->TexFormat);
    mml->glideFormat = fxGlideFormat(texImage->TexFormat->MesaFormat);
    ti->info.format = mml->glideFormat;
    texImage->FetchTexel = fxFetchFunction(texImage->TexFormat->MesaFormat);
    texelBytes = texImage->TexFormat->TexelBytes;

    if (mml->width != width || mml->height != height) {
        /* rescale the image to overcome 1:8 aspect limitation */
        GLvoid *tempImage;
        tempImage = MALLOC(width * height * texelBytes);
        if (!tempImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
        /* unpack image, apply transfer ops and store in tempImage */
        _mesa_transfer_teximage(ctx, 2, texImage->Format,
                                texImage->TexFormat,
                                tempImage,
                                width, height, 1, 0, 0, 0,
                                width * texelBytes,
                                0, /* dstImageStride */
                                format, type, pixels, packing);
        assert(!texImage->Data);
        texImage->Data = MESA_PBUFFER_ALLOC(mml->width * mml->height * texelBytes);
        if (!texImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            FREE(tempImage);
            return;
        }
        _mesa_rescale_teximage2d(texelBytes,
                                 mml->width * texelBytes, /* dst stride */
                                 width, height,
                                 mml->width, mml->height,
                                 tempImage /*src*/, texImage->Data /*dst*/ );
        FREE(tempImage);
    }
    else {
        /* no rescaling needed */
      assert(!texImage->Data);
      texImage->Data = MESA_PBUFFER_ALLOC(mml->width * mml->height * texelBytes);
      if (!texImage->Data) {
          _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
          return;
      }
      /* unpack image, apply transfer ops and store in texImage->Data */
      _mesa_transfer_teximage(ctx, 2, texImage->Format,
                              texImage->TexFormat, texImage->Data,
                              width, height, 1, 0, 0, 0,
                              texImage->Width * texelBytes,
                              0, /* dstImageStride */
                              format, type, pixels, packing);
    }

    RevalidateTexture(ctx, texObj);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


void
tdfxDDTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset, GLint yoffset,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage )
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint texelBytes;

    if (!texObj->DriverData) {
        _mesa_problem(ctx, "problem in fxDDTexSubImage2D");
        return;
    }

    ti = TDFX_TEXTURE_DATA(texObj);
    assert(ti);
    mml = TDFX_TEXIMAGE_DATA(texImage);
    assert(mml);

    assert(texImage->Data);	/* must have an existing texture image! */
    assert(texImage->Format);

    texelBytes = texImage->TexFormat->TexelBytes;

    if (mml->wScale != 1 || mml->hScale != 1) {
        /* need to rescale subimage to match mipmap level's rescale factors */
        const GLint newWidth = width * mml->wScale;
        const GLint newHeight = height * mml->hScale;
        GLvoid *scaledImage, *tempImage;
        GLubyte *destAddr;
        tempImage = MALLOC(width * height * texelBytes);
        if (!tempImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
            return;
        }

        _mesa_transfer_teximage(ctx, 2, texImage->Format,/* Tex int format */
                                texImage->TexFormat,     /* dest format */
                                (GLubyte *) tempImage,   /* dest */
                                width, height, 1,        /* subimage size */
                                0, 0, 0,                 /* subimage pos */
                                width * texelBytes,      /* dest row stride */
                                0,                       /* dst image stride */
                                format, type, pixels, packing);

        /* now rescale */
        scaledImage = MALLOC(newWidth * newHeight * texelBytes);
        if (!scaledImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
            FREE(tempImage);
            return;
        }

        /* compute address of dest subimage within the overal tex image */
        destAddr = (GLubyte *) texImage->Data
            + (yoffset * mml->hScale * mml->width
               + xoffset * mml->wScale) * texelBytes;

        _mesa_rescale_teximage2d(texelBytes,
                                 mml->width * texelBytes, /* dst stride */
                                 width, height,
                                 newWidth, newHeight,
                                 tempImage, destAddr);

        FREE(tempImage);
        FREE(scaledImage);
    }
    else {
        /* no rescaling needed */
        _mesa_transfer_teximage(ctx, 2, texImage->Format,  /* Tex int format */
                                texImage->TexFormat,       /* dest format */
                                (GLubyte *) texImage->Data,/* dest */
                                width, height, 1,          /* subimage size */
                                xoffset, yoffset, 0,       /* subimage pos */
                                mml->width * texelBytes, /* dest row stride */
                                0,                       /* dst image stride */
                                format, type, pixels, packing);
    }

    ti->reloadImages = GL_TRUE; /* signal the image needs to be reloaded */
    fxMesa->new_state |= TDFX_NEW_TEXTURE;  /* XXX this might be a bit much */
}



/**********************************************************************/
/**** COMPRESSED TEXTURE IMAGE FUNCTIONS                           ****/
/**********************************************************************/

#if 0000
GLboolean
tdfxDDCompressedTexImage2D( GLcontext *ctx, GLenum target,
                            GLint level, GLsizei imageSize,
                            const GLvoid *data,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage,
                            GLboolean *retainInternalCopy)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    const GLboolean allow32bpt = TDFX_IS_NAPALM(fxMesa);
    GrTextureFormat_t gldformat;
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint dstWidth, dstHeight, wScale, hScale, texelSize;
    MesaIntTexFormat intFormat;
    GLboolean isCompressedFormat;
    GLsizei texSize;

    if (target != GL_TEXTURE_2D || texImage->Border > 0)
        return GL_FALSE;

    if (!texObj->DriverData)
        texObj->DriverData = fxAllocTexObjData(fxMesa);

    ti = TDFX_TEXTURE_DATA(texObj);
    mml = &ti->mipmapLevel[level];

    isCompressedFormat = tdfxDDIsCompressedGlideFormatMacro(texImage->IntFormat);
    if (!isCompressedFormat) {
        _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage2D(format)");
        return GL_FALSE;
    }
    /* Determine the apporpriate GL internal texel format, Mesa internal
     * texel format, and texelSize (bytes) given the user's internal
     * texture format hint.
     */
    tdfxTexGetFormat(texImage->IntFormat, allow32bpt,
                     &gldformat, &intFormat, &texelSize);

    /* Determine width and height scale factors for texture.
     * Remember, Glide is limited to 8:1 aspect ratios.
     */
    tdfxTexGetInfo(ctx,
                   texImage->Width, texImage->Height,
                   NULL,       /* lod level          */
                   NULL,       /* aspect ratio       */
                   NULL, NULL, /* sscale, tscale     */
                   &wScale, &hScale);
    dstWidth = texImage->Width * wScale;
    dstHeight = texImage->Height * hScale;
    /* housekeeping */
    _mesa_set_teximage_component_sizes(intFormat, texImage);

    texSize = tdfxDDCompressedImageSize(ctx,
                                        texImage->IntFormat,
                                        2,
                                        texImage->Width,
                                        texImage->Height,
                                        1);
    if (texSize != imageSize) {
        _mesa_error(ctx, GL_INVALID_VALUE, "glCompressedTexImage2D(texSize)");
        return GL_FALSE;
    }

    /* allocate new storage for texture image, if needed */
    if (!mml->data || mml->glideFormat != gldformat ||
        mml->width != dstWidth || mml->height != dstHeight ||
        texSize != mml->dataSize) {
        if (mml->data) {
            FREE(mml->data);
        }
        mml->data = MALLOC(texSize);
        if (!mml->data) {
            return GL_FALSE;
        }
        mml->texelSize = texelSize;
        mml->glideFormat = gldformat;
        mml->width = dstWidth;
        mml->height = dstHeight;
        tdfxTMMoveOutTM(fxMesa, texObj);
        /*tdfxTexInvalidate(ctx, texObj);*/
    }

    /* save the texture data */
    MEMCPY(mml->data, data, imageSize);

    RevalidateTexture(ctx, texObj);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;

    *retainInternalCopy = GL_FALSE;
    return GL_TRUE;
}

GLboolean
tdfxDDCompressedTexSubImage2D( GLcontext *ctx, GLenum target,
                               GLint level, GLint xoffset,
                               GLint yoffset, GLsizei width,
                               GLint height, GLenum format,
                               GLsizei imageSize, const GLvoid *data,
                               struct gl_texture_object *texObj,
                               struct gl_texture_image *texImage )
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;

    /*
     * We punt if we are not replacing the entire image.  This
     * is allowed by the spec.
     */
    if ((xoffset != 0) && (yoffset != 0)
        && (width != texImage->Width)
        && (height != texImage->Height)) {
        return GL_FALSE;
    }

    ti = TDFX_TEXTURE_DATA(texObj);
    mml = &ti->mipmapLevel[level];
    if (imageSize != mml->dataSize) {
        return GL_FALSE;
    }
    MEMCPY(data, mml->data, imageSize);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;

    return GL_TRUE;
}
#endif



#if	0
static void
PrintTexture(int w, int h, int c, const GLubyte * data)
{
    int i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            if (c == 2)
                printf("%02x %02x  ", data[0], data[1]);
            else if (c == 3)
                printf("%02x %02x %02x  ", data[0], data[1], data[2]);
            data += c;
        }
        printf("\n");
    }
}
#endif


GLboolean
tdfxDDTestProxyTexImage(GLcontext *ctx, GLenum target,
                        GLint level, GLint internalFormat,
                        GLenum format, GLenum type,
                        GLint width, GLint height,
                        GLint depth, GLint border)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;

    switch (target) {
    case GL_PROXY_TEXTURE_1D:
        return GL_TRUE;  /* software rendering */
    case GL_PROXY_TEXTURE_2D:
        {
            struct gl_texture_object *tObj;
            tdfxTexInfo *ti;
            int memNeeded;

            tObj = ctx->Texture.Proxy2D;
            if (!tObj->DriverData)
                tObj->DriverData = fxAllocTexObjData(fxMesa);
            ti = TDFX_TEXTURE_DATA(tObj);

            /* assign the parameters to test against */
            tObj->Image[level]->Width = width;
            tObj->Image[level]->Height = height;
            tObj->Image[level]->Border = border;
#if 0
            tObj->Image[level]->IntFormat = internalFormat;
#endif
            if (level == 0) {
               /* don't use mipmap levels > 0 */
               tObj->MinFilter = tObj->MagFilter = GL_NEAREST;
            }
            else {
               /* test with all mipmap levels */
               tObj->MinFilter = GL_LINEAR_MIPMAP_LINEAR;
               tObj->MagFilter = GL_NEAREST;
            }
            RevalidateTexture(ctx, tObj);

            /*
            printf("small lodlog2 0x%x\n", ti->info.smallLodLog2);
            printf("large lodlog2 0x%x\n", ti->info.largeLodLog2);
            printf("aspect ratio 0x%x\n", ti->info.aspectRatioLog2);
            printf("glide format 0x%x\n", ti->info.format);
            printf("data %p\n", ti->info.data);
            printf("lodblend %d\n", (int) ti->LODblend);
            */

            /* determine where texture will reside */
            if (ti->LODblend && !shared->umaTexMemory) {
                /* XXX GR_MIPMAPLEVELMASK_BOTH might not be right, but works */
                memNeeded = fxMesa->Glide.grTexTextureMemRequired(
                                        GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
            }
            else {
                /* XXX GR_MIPMAPLEVELMASK_BOTH might not be right, but works */
                memNeeded = fxMesa->Glide.grTexTextureMemRequired(
                                        GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
            }
            /*
            printf("Proxy test %d > %d\n", memNeeded, shared->totalTexMem[0]);
            */
            if (memNeeded > shared->totalTexMem[0])
                return GL_FALSE;
            else
                return GL_TRUE;
        }
    case GL_PROXY_TEXTURE_3D:
        return GL_TRUE;  /* software rendering */
    default:
        return GL_TRUE;  /* never happens, silence compiler */
    }
}


#if 000
/*
 * This is called from _mesa_GetCompressedTexImage.  We just
 * copy out the compressed data.
 */
void
tdfxDDGetCompressedTexImage( GLcontext *ctx, GLenum target,
                             GLint lod, void *image,
                             const struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage )
{
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;

    if (target != GL_TEXTURE_2D)
        return;

    if (!texObj->DriverData)
        return;

    ti = TDFX_TEXTURE_DATA(texObj);
    mml = &ti->mipmapLevel[lod];
    if (mml->data) {
        MEMCPY(image, mml->data, mml->dataSize);
    }
}
#endif

/*
 * Calculate a specific texture format given a generic
 * texture format.
 */
GLint
tdfxDDSpecificCompressedTexFormat(GLcontext *ctx,
                                  GLint internalFormat,
                                  GLint numDimensions)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

    if (numDimensions != 2) {
        return internalFormat;
    }
   /*
    * If we don't have pointers to the functions, then
    * we drop back to uncompressed format.  The logic
    * in Mesa proper handles this for us.
    *
    * This is just to ease the transition to a Glide with
    * the texus2 library.
    */
    if (!fxMesa->Glide.txImgQuantize || !fxMesa->Glide.txImgDequantizeFXT1) {
        return internalFormat;
    }
    switch (internalFormat) {
    case GL_COMPRESSED_RGB_ARB:
        return GL_COMPRESSED_RGB_FXT1_3DFX;
    case GL_COMPRESSED_RGBA_ARB:
        return GL_COMPRESSED_RGBA_FXT1_3DFX;
    }
    return internalFormat;
}

/*
 * Calculate a specific texture format given a generic
 * texture format.
 */
GLint
tdfxDDBaseCompressedTexFormat(GLcontext *ctx,
                              GLint internalFormat)
{
    switch (internalFormat) {
    case GL_COMPRESSED_RGB_FXT1_3DFX:
        return GL_RGB;
    case GL_COMPRESSED_RGBA_FXT1_3DFX:
        return GL_RGBA;
    }
    return -1;
}

/*
 * Tell us if an image is compressed.  The real work is done
 * in a macro, but we need to have a function to create a
 * function pointer.
 */
GLboolean
tdfxDDIsCompressedFormat(GLcontext *ctx, GLint internalFormat)
{
    return tdfxDDIsCompressedFormatMacro(internalFormat);
}


/*
 * Calculate the image size of a compressed texture.
 *
 * The current compressed format, the FXT1 family, all
 * map 8x32 texel blocks into 128 bits.
 *
 * We return 0 if we can't calculate the size.
 *
 * Glide would report this out to us, but we don't have
 * exactly the right parameters.
 */
GLsizei
tdfxDDCompressedImageSize(GLcontext *ctx,
                          GLenum intFormat,
                          GLuint numDimensions,
                          GLuint width,
                          GLuint height,
                          GLuint depth)
{
    if (numDimensions != 2) {
        return 0;
    }
    switch (intFormat) {
    case GL_COMPRESSED_RGB_FXT1_3DFX:
    case GL_COMPRESSED_RGBA_FXT1_3DFX:
       /*
        * Round height and width to multiples of 4 and 8,
        * divide the resulting product by 32 to get the number
        * of blocks, and multiply by 32 = 128/8 to get the.
        * number of bytes required.  That is to say, just
        * return the product.  Remember that we are returning
        * bytes, not texels, so we have shrunk the texture
        * by a factor of the texel size.
        */
        width = (width   + 0x7) &~ 0x7;
        height = (height + 0x3) &~ 0x3;
        return width * height;
    }
    return 0;
}
