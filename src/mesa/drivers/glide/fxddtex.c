/* $Id: fxddtex.c,v 1.51 2003/10/14 14:56:45 dborca Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "enums.h"
#include "image.h"
#include "teximage.h"
#include "texformat.h"
#include "texstore.h"
#include "texutil.h"


void
fxPrintTextureData(tfxTexInfo * ti)
{
   fprintf(stderr, "Texture Data:\n");
   if (ti->tObj) {
      fprintf(stderr, "\tName: %d\n", ti->tObj->Name);
      fprintf(stderr, "\tBaseLevel: %d\n", ti->tObj->BaseLevel);
      fprintf(stderr, "\tSize: %d x %d\n",
	      ti->tObj->Image[ti->tObj->BaseLevel]->Width,
	      ti->tObj->Image[ti->tObj->BaseLevel]->Height);
   }
   else
      fprintf(stderr, "\tName: UNNAMED\n");
   fprintf(stderr, "\tLast used: %d\n", ti->lastTimeUsed);
   fprintf(stderr, "\tTMU: %ld\n", ti->whichTMU);
   fprintf(stderr, "\t%s\n", (ti->isInTM) ? "In TMU" : "Not in TMU");
   if (ti->tm[0])
      fprintf(stderr, "\tMem0: %x-%x\n", (unsigned) ti->tm[0]->startAddr,
	      (unsigned) ti->tm[0]->endAddr);
   if (ti->tm[1])
      fprintf(stderr, "\tMem1: %x-%x\n", (unsigned) ti->tm[1]->startAddr,
	      (unsigned) ti->tm[1]->endAddr);
   fprintf(stderr, "\tMipmaps: %d-%d\n", ti->minLevel, ti->maxLevel);
   fprintf(stderr, "\tFilters: min %d min %d\n",
	   (int) ti->minFilt, (int) ti->maxFilt);
   fprintf(stderr, "\tClamps: s %d t %d\n", (int) ti->sClamp,
	   (int) ti->tClamp);
   fprintf(stderr, "\tScales: s %f t %f\n", ti->sScale, ti->tScale);
   fprintf(stderr, "\t%s\n",
	   (ti->fixedPalette) ? "Fixed palette" : "Non fixed palette");
   fprintf(stderr, "\t%s\n", (ti->validated) ? "Validated" : "Not validated");
}


/************************************************************************/
/*************************** Texture Mapping ****************************/
/************************************************************************/

static void
fxTexInvalidate(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;

   ti = fxTMGetTexInfo(tObj);
   if (ti->isInTM)
      fxTMMoveOutTM(fxMesa, tObj);	/* TO DO: SLOW but easy to write */

   ti->validated = GL_FALSE;
   fxMesa->new_state |= FX_NEW_TEXTURING;
}

static tfxTexInfo *
fxAllocTexObjData(fxMesaContext fxMesa)
{
   tfxTexInfo *ti;

   if (!(ti = CALLOC(sizeof(tfxTexInfo)))) {
      fprintf(stderr, "%s: ERROR: out of memory !\n", __FUNCTION__);
      fxCloseHardware();
      exit(-1);
   }

   ti->validated = GL_FALSE;
   ti->isInTM = GL_FALSE;

   ti->whichTMU = FX_TMU_NONE;

   ti->tm[FX_TMU0] = NULL;
   ti->tm[FX_TMU1] = NULL;

   ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
   ti->maxFilt = GR_TEXTUREFILTER_BILINEAR;

   ti->sClamp = GR_TEXTURECLAMP_WRAP;
   ti->tClamp = GR_TEXTURECLAMP_WRAP;

   ti->mmMode = GR_MIPMAP_NEAREST;
   ti->LODblend = FXFALSE;

   return ti;
}

void
fxDDTexBind(GLcontext * ctx, GLenum target, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(%d, %x)\n", __FUNCTION__, tObj->Name,
	      (GLuint) tObj->DriverData);
   }

   if (target != GL_TEXTURE_2D)
      return;

   if (!tObj->DriverData) {
      tObj->DriverData = fxAllocTexObjData(fxMesa);
   }

   ti = fxTMGetTexInfo(tObj);

   fxMesa->texBindNumber++;
   ti->lastTimeUsed = fxMesa->texBindNumber;

   fxMesa->new_state |= FX_NEW_TEXTURING;
}

void
fxDDTexEnv(GLcontext * ctx, GLenum target, GLenum pname,
	   const GLfloat * param)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      if (param)
	 fprintf(stderr, "%s(%x, %x)\n", __FUNCTION__, pname, (GLint) (*param));
      else
	 fprintf(stderr, "%s(%x)\n", __FUNCTION__, pname);
   }

   /* apply any lod biasing right now */
   if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
      grTexLodBiasValue(GR_TMU0, *param);

      if (fxMesa->haveTwoTMUs) {
	 grTexLodBiasValue(GR_TMU1, *param);
      }

   }

   fxMesa->new_state |= FX_NEW_TEXTURING;
}

void
fxDDTexParam(GLcontext * ctx, GLenum target, struct gl_texture_object *tObj,
	     GLenum pname, const GLfloat * params)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLenum param = (GLenum) (GLint) params[0];
   tfxTexInfo *ti;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxDDTexParam(%d, %x, %s, %s)\n",
                      tObj->Name, (GLuint) tObj->DriverData,
                      _mesa_lookup_enum_by_nr(pname),
                      _mesa_lookup_enum_by_nr(param));
   }

   if (target != GL_TEXTURE_2D)
      return;

   if (!tObj->DriverData)
      tObj->DriverData = fxAllocTexObjData(fxMesa);

   ti = fxTMGetTexInfo(tObj);

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
	 if (fxMesa->haveTwoTMUs) {
	    ti->mmMode = GR_MIPMAP_NEAREST;
	    ti->LODblend = FXTRUE;
	 }
	 else {
	    ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
	    ti->LODblend = FXFALSE;
	 }
	 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 break; /* ZZZ: we may have to fall through here for V3 */
      case GL_NEAREST_MIPMAP_NEAREST:
	 ti->mmMode = GR_MIPMAP_NEAREST;
	 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 ti->LODblend = FXFALSE;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
        /* ZZZ: HACK ALERT! trilinear is bugged! mipmap blending produce
                incorrect filtered colors for the smallest mipmap levels. */
#if 0
	 if (fxMesa->haveTwoTMUs) {
	    ti->mmMode = GR_MIPMAP_NEAREST;
	    ti->LODblend = FXTRUE;
	 }
	 else {
	    ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
	    ti->LODblend = FXFALSE;
	 }
	 ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
	 break; /* ZZZ: we may have to fall through here for V3 */
#endif
      case GL_LINEAR_MIPMAP_NEAREST:
	 ti->mmMode = GR_MIPMAP_NEAREST;
	 ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
	 ti->LODblend = FXFALSE;
	 break;
      default:
	 break;
      }
      fxTexInvalidate(ctx, tObj);
      break;

   case GL_TEXTURE_MAG_FILTER:
      switch (param) {
      case GL_NEAREST:
	 ti->maxFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 break;
      case GL_LINEAR:
	 ti->maxFilt = GR_TEXTUREFILTER_BILINEAR;
	 break;
      default:
	 break;
      }
      fxTexInvalidate(ctx, tObj);
      break;

   case GL_TEXTURE_WRAP_S:
      switch (param) {
      case GL_MIRRORED_REPEAT:
         ti->sClamp = GR_TEXTURECLAMP_MIRROR_EXT;
         break;
      case GL_CLAMP_TO_EDGE: /* CLAMP discarding border */
      case GL_CLAMP:
	 ti->sClamp = GR_TEXTURECLAMP_CLAMP;
	 break;
      case GL_REPEAT:
	 ti->sClamp = GR_TEXTURECLAMP_WRAP;
	 break;
      default:
	 break;
      }
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;

   case GL_TEXTURE_WRAP_T:
      switch (param) {
      case GL_MIRRORED_REPEAT:
         ti->tClamp = GR_TEXTURECLAMP_MIRROR_EXT;
         break;
      case GL_CLAMP_TO_EDGE: /* CLAMP discarding border */
      case GL_CLAMP:
	 ti->tClamp = GR_TEXTURECLAMP_CLAMP;
	 break;
      case GL_REPEAT:
	 ti->tClamp = GR_TEXTURECLAMP_WRAP;
	 break;
      default:
	 break;
      }
      fxMesa->new_state |= FX_NEW_TEXTURING;
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
      fxTexInvalidate(ctx, tObj);
      break;
   case GL_TEXTURE_MAX_LEVEL:
      fxTexInvalidate(ctx, tObj);
      break;

   default:
      break;
   }
}

void
fxDDTexDel(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(%d, %p)\n", __FUNCTION__, tObj->Name, (void *) ti);
   }

   if (!ti)
      return;

   fxTMFreeTexture(fxMesa, tObj);

   FREE(ti);
   tObj->DriverData = NULL;
}

/*
 * Return true if texture is resident, false otherwise.
 */
GLboolean
fxDDIsTextureResident(GLcontext *ctx, struct gl_texture_object *tObj)
{
 tfxTexInfo *ti = fxTMGetTexInfo(tObj);
 return (ti && ti->isInTM);
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

   ASSERT(!table->FloatTable);

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
fxDDTexPalette(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (tObj) {
      /* per-texture palette */
      tfxTexInfo *ti;
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "%s(%d, %x)\n", __FUNCTION__,
		 tObj->Name, (GLuint) tObj->DriverData);
      }
      if (!tObj->DriverData)
	 tObj->DriverData = fxAllocTexObjData(fxMesa);
      ti = fxTMGetTexInfo(tObj);
      convertPalette(ti->palette.data, &tObj->Palette);
      fxTexInvalidate(ctx, tObj);
   }
   else {
      /* global texture palette */
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "%s(global)\n", __FUNCTION__);
      }
      convertPalette(fxMesa->glbPalette.data, &ctx->Texture.Palette);
      fxMesa->new_state |= FX_NEW_TEXTURING;
   }
}


void
fxDDTexUseGlbPalette(GLcontext * ctx, GLboolean state)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(%d)\n", __FUNCTION__, state);
   }

   if (state) {
      fxMesa->haveGlobalPaletteTexture = 1;

      grTexDownloadTable(GR_TEXTABLE_PALETTE, &(fxMesa->glbPalette));
   }
   else {
      fxMesa->haveGlobalPaletteTexture = 0;

      if ((ctx->Texture.Unit[0]._Current == ctx->Texture.Unit[0].Current2D) &&
	  (ctx->Texture.Unit[0]._Current != NULL)) {
	 struct gl_texture_object *tObj = ctx->Texture.Unit[0]._Current;

	 if (!tObj->DriverData)
	    tObj->DriverData = fxAllocTexObjData(fxMesa);

	 fxTexInvalidate(ctx, tObj);
      }
   }
}


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


int
fxTexGetInfo(int w, int h, GrLOD_t * lodlevel, GrAspectRatio_t * ar,
	     float *sscale, float *tscale,
	     int *wscale, int *hscale)
{
   static GrLOD_t lod[12] = {
          GR_LOD_LOG2_1,
          GR_LOD_LOG2_2,
          GR_LOD_LOG2_4,
          GR_LOD_LOG2_8,
          GR_LOD_LOG2_16,
          GR_LOD_LOG2_32,
          GR_LOD_LOG2_64,
          GR_LOD_LOG2_128,
          GR_LOD_LOG2_256,
          GR_LOD_LOG2_512,
          GR_LOD_LOG2_1024,
          GR_LOD_LOG2_2048
   };

   int logw, logh, ws, hs;
   GrLOD_t l;
   GrAspectRatio_t aspectratio;
   float s, t;

   logw = logbase2(w);
   logh = logbase2(h);

   switch (logw - logh) {
   case 0:
      aspectratio = GR_ASPECT_LOG2_1x1;
      l = lod[logw];
      s = t = 256.0f;
      ws = hs = 1;
      break;
   case 1:
      aspectratio = GR_ASPECT_LOG2_2x1;
      l = lod[logw];
      s = 256.0f;
      t = 128.0f;
      ws = 1;
      hs = 1;
      break;
   case 2:
      aspectratio = GR_ASPECT_LOG2_4x1;
      l = lod[logw];
      s = 256.0f;
      t = 64.0f;
      ws = 1;
      hs = 1;
      break;
   case 3:
      aspectratio = GR_ASPECT_LOG2_8x1;
      l = lod[logw];
      s = 256.0f;
      t = 32.0f;
      ws = 1;
      hs = 1;
      break;
   case -1:
      aspectratio = GR_ASPECT_LOG2_1x2;
      l = lod[logh];
      s = 128.0f;
      t = 256.0f;
      ws = 1;
      hs = 1;
      break;
   case -2:
      aspectratio = GR_ASPECT_LOG2_1x4;
      l = lod[logh];
      s = 64.0f;
      t = 256.0f;
      ws = 1;
      hs = 1;
      break;
   case -3:
      aspectratio = GR_ASPECT_LOG2_1x8;
      l = lod[logh];
      s = 32.0f;
      t = 256.0f;
      ws = 1;
      hs = 1;
      break;
   default:
      if ((logw - logh) > 3) {
         aspectratio = GR_ASPECT_LOG2_8x1;
         l = lod[logw];
         s = 256.0f;
         t = 32.0f;
         ws = 1;
         hs = 1 << (logw - logh - 3);
      } else /*if ((logw - logh) < -3)*/ {
         aspectratio = GR_ASPECT_LOG2_1x8;
         l = lod[logh];
         s = 32.0f;
         t = 256.0f;
         ws = 1 << (logh - logw - 3);
         hs = 1;
      }
   }

   if (lodlevel)
      (*lodlevel) = l;

   if (ar)
      (*ar) = aspectratio;

   if (sscale)
      (*sscale) = s;

   if (tscale)
      (*tscale) = t;

   if (wscale)
      (*wscale) = ws;

   if (hscale)
      (*hscale) = hs;


   return 1;
}

static GLboolean
fxIsTexSupported(GLenum target, GLint internalFormat,
		 const struct gl_texture_image *image)
{
   if (target != GL_TEXTURE_2D)
      return GL_FALSE;

   if (!fxTexGetInfo(image->Width, image->Height, NULL, NULL, NULL, NULL, NULL, NULL))
       return GL_FALSE;

   if (image->Border > 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**********************************************************************/
/**** NEW TEXTURE IMAGE FUNCTIONS                                  ****/
/**********************************************************************/

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
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
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
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
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
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
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
   GLchan *indexOut = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;
   i = i * mml->width / texImage->Width;
   j = j * mml->height / texImage->Height;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   *indexOut = *texel;
}


static void
fetch_luminance8_alpha8(const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
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
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_5[(*texel >> 11) & 0x1F];
   rgba[GCOMP] = FX_rgb_scale_6[(*texel >>  5) & 0x3F];
   rgba[BCOMP] = FX_rgb_scale_5[ *texel        & 0x1F];
   rgba[ACOMP] = 255;
}


static void
fetch_r4g4b4a4(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_4[(*texel >> 12) & 0xF];
   rgba[GCOMP] = FX_rgb_scale_4[(*texel >>  8) & 0xF];
   rgba[BCOMP] = FX_rgb_scale_4[(*texel >>  4) & 0xF];
   rgba[ACOMP] = FX_rgb_scale_4[ *texel        & 0xF];
}


static void
fetch_r5g5b5a1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_5[(*texel >> 11) & 0x1F];
   rgba[GCOMP] = FX_rgb_scale_5[(*texel >>  6) & 0x1F];
   rgba[BCOMP] = FX_rgb_scale_5[(*texel >>  1) & 0x1F];
   rgba[ACOMP] = ((*texel) & 0x01) * 255;
}


static void
fetch_a8r8g8b8(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
    const GLuint *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLuint *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 16) & 0xff);
    rgba[GCOMP] = (((*texel) >>  8) & 0xff);
    rgba[BCOMP] = (((*texel)      ) & 0xff);
    rgba[ACOMP] = (((*texel) >> 24) & 0xff);
}


static void
PrintTexture(int w, int h, int c, const GLubyte * data)
{
   int i, j;
   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
	 if (c == 2)
	    fprintf(stderr, "%02x %02x  ", data[0], data[1]);
	 else if (c == 3)
	    fprintf(stderr, "%02x %02x %02x  ", data[0], data[1], data[2]);
	 data += c;
      }
      fprintf(stderr, "\n");
   }
}


const struct gl_texture_format *
fxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                         GLenum srcFormat, GLenum srcType )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLboolean allow32bpt = fxMesa->HaveTexFmt;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
      fprintf(stderr, "fxDDChooseTextureFormat(...)\n");
   }

   switch (internalFormat) {
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;
   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      return &_mesa_texformat_rgb565;
   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
     if ( srcFormat == GL_RGB && srcType == GL_UNSIGNED_SHORT_5_6_5 ) {
       return &_mesa_texformat_rgb565;
     }
     /* intentional fall through */
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return (allow32bpt) ? &_mesa_texformat_argb8888
                          : &_mesa_texformat_rgb565;
   case GL_RGBA2:
   case GL_RGBA4:
      return &_mesa_texformat_argb4444;
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
     if ( srcFormat == GL_BGRA ) {
       if ( srcType == GL_UNSIGNED_INT_8_8_8_8_REV ) {
         return &_mesa_texformat_argb8888;
       }
       else if ( srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ) {
         return &_mesa_texformat_argb4444;
       }
       else if ( srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ) {
         return &_mesa_texformat_argb1555;
       }
     }
     /* intentional fall through */
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return (allow32bpt) ? &_mesa_texformat_argb8888
                          : &_mesa_texformat_argb4444;
   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;
#if 0
   /* GL_EXT_texture_compression_s3tc */
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgb_dxt1;
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return &_mesa_texformat_rgba_dxt3;
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;
   /*case GL_COMPRESSED_RGB_FXT1_3DFX:
     case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return blah;*/
#endif
   default:
      _mesa_problem(NULL, "unexpected format in fxDDChooseTextureFormat");
      return NULL;
   }
}


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
#if 0
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
     return GR_TEXFMT_ARGB_CMP_DXT1;
   case MESA_FORMAT_RGBA_DXT3:
     return GR_TEXFMT_ARGB_CMP_DXT3;
   case MESA_FORMAT_RGBA_DXT5:
     return GR_TEXFMT_ARGB_CMP_DXT5;
   /*case MESA_FORMAT_ARGB_CMP_FXT1:
     return GR_TEXFMT_ARGB_CMP_FXT1;
   case MESA_FORMAT_RGB_CMP_FXT1:
     return GL_COMPRESSED_RGBA_FXT1_3DFX;*/
#endif
   default:
      _mesa_problem(NULL, "Unexpected format in fxGlideFormat");
      return 0;
   }
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
#if 0
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
     return fetch_r4g4b4a4;
#endif
   default:
      _mesa_problem(NULL, "Unexpected format in fxFetchFunction");
      return NULL;
   }
}

void
fxDDTexImage2D(GLcontext * ctx, GLenum target, GLint level,
	       GLint internalFormat, GLint width, GLint height, GLint border,
	       GLenum format, GLenum type, const GLvoid * pixels,
	       const struct gl_pixelstore_attrib *packing,
	       struct gl_texture_object *texObj,
	       struct gl_texture_image *texImage)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;
   GLint texelBytes;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDTexImage2D: id=%d int 0x%x  format 0x%x  type 0x%x  %dx%d\n",
                       texObj->Name, texImage->IntFormat, format, type,
                       texImage->Width, texImage->Height);
   }

   if (!fxIsTexSupported(target, internalFormat, texImage)) {
      _mesa_problem(NULL, "fx Driver: unsupported texture in fxDDTexImg()\n");
      return;
   }

   if (!texObj->DriverData) {
      texObj->DriverData = fxAllocTexObjData(fxMesa);
      if (!texObj->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   ti = fxTMGetTexInfo(texObj);

   if (!texImage->DriverData) {
      texImage->DriverData = CALLOC(sizeof(tfxMipMapLevel));
      if (!texImage->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   mml = FX_MIPMAP_DATA(texImage);

   fxTexGetInfo(width, height, NULL, NULL, NULL, NULL,
		&mml->wScale, &mml->hScale);

   mml->width = width * mml->wScale;
   mml->height = height * mml->hScale;


   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);
   texelBytes = texImage->TexFormat->TexelBytes;
   /*if (!fxMesa->HaveTexFmt) assert(texelBytes == 1 || texelBytes == 2);*/

   if (mml->wScale != 1 || mml->hScale != 1) {
      /* rescale image to overcome 1:8 aspect limitation */
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

   mml->glideFormat = fxGlideFormat(texImage->TexFormat->MesaFormat);
   ti->info.format = mml->glideFormat;
   texImage->FetchTexel = fxFetchFunction(texImage->TexFormat->MesaFormat);

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (!(fxMesa->new_state & FX_NEW_TEXTURING) && ti->validated && ti->isInTM) {
      /*fprintf(stderr, "reloadmipmaplevels\n"); */
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   }
   else {
      /*fprintf(stderr, "invalidate2\n"); */
      fxTexInvalidate(ctx, texObj);
   }
}


void
fxDDTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset, GLint yoffset,
		  GLsizei width, GLsizei height,
		  GLenum format, GLenum type, const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;
   GLint texelBytes;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDTexSubImage2D: id=%d\n", texObj->Name);
   }

   if (!texObj->DriverData) {
      _mesa_problem(ctx, "problem in fxDDTexSubImage2D");
      return;
   }

   ti = fxTMGetTexInfo(texObj);
   assert(ti);
   mml = FX_MIPMAP_DATA(texImage);
   assert(mml);

   assert(texImage->Data);	/* must have an existing texture image! */
   assert(texImage->Format);

   texelBytes = texImage->TexFormat->TexelBytes;

   if (mml->wScale != 1 || mml->hScale != 1) {
      /* need to rescale subimage to match mipmap level's rescale factors */
      const GLint newWidth = width * mml->wScale;
      const GLint newHeight = height * mml->hScale;
      GLvoid *tempImage;
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
   }
   else {
      /* no rescaling needed */
      _mesa_transfer_teximage(ctx, 2, texImage->Format,  /* Tex int format */
                              texImage->TexFormat,       /* dest format */
                              (GLubyte *) texImage->Data,/* dest */
                              width, height, 1,          /* subimage size */
                              xoffset, yoffset, 0,       /* subimage pos */
                              mml->width * texelBytes,   /* dest row stride */
                              0,                         /* dst image stride */
                              format, type, pixels, packing);
   }

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (!(fxMesa->new_state & FX_NEW_TEXTURING) && ti->validated && ti->isInTM)
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   else
      fxTexInvalidate(ctx, texObj);
}


#else /* FX */

/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_ddtex(void);
int
gl_fx_dummy_function_ddtex(void)
{
   return 0;
}

#endif /* FX */
