
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "image.h"
#include "texutil.h"


void fxPrintTextureData(tfxTexInfo *ti)
{
  fprintf(stderr, "Texture Data:\n");
  if (ti->tObj) {
    fprintf(stderr, "\tName: %d\n", ti->tObj->Name);
    fprintf(stderr, "\tBaseLevel: %d\n", ti->tObj->BaseLevel);
    fprintf(stderr, "\tSize: %d x %d\n", 
	    ti->tObj->Image[ti->tObj->BaseLevel]->Width,
	    ti->tObj->Image[ti->tObj->BaseLevel]->Height);
  } else
    fprintf(stderr, "\tName: UNNAMED\n");
  fprintf(stderr, "\tLast used: %d\n", ti->lastTimeUsed);
  fprintf(stderr, "\tTMU: %ld\n", ti->whichTMU);
  fprintf(stderr, "\t%s\n", (ti->isInTM)?"In TMU":"Not in TMU");
  if (ti->tm[0]) 
    fprintf(stderr, "\tMem0: %x-%x\n", (unsigned) ti->tm[0]->startAddr, 
	    (unsigned) ti->tm[0]->endAddr);
  if (ti->tm[1]) 
    fprintf(stderr, "\tMem1: %x-%x\n", (unsigned) ti->tm[1]->startAddr, 
	    (unsigned) ti->tm[1]->endAddr);
  fprintf(stderr, "\tMipmaps: %d-%d\n", ti->minLevel, ti->maxLevel);
  fprintf(stderr, "\tFilters: min %d min %d\n",
          (int) ti->minFilt, (int) ti->maxFilt);
  fprintf(stderr, "\tClamps: s %d t %d\n", (int) ti->sClamp, (int) ti->tClamp);
  fprintf(stderr, "\tScales: s %f t %f\n", ti->sScale, ti->tScale);
  fprintf(stderr, "\tInt Scales: s %d t %d\n", 
	  ti->int_sScale/0x800000, ti->int_tScale/0x800000);
  fprintf(stderr, "\t%s\n", (ti->fixedPalette)?"Fixed palette":"Non fixed palette");
  fprintf(stderr, "\t%s\n", (ti->validated)?"Validated":"Not validated");
}


/************************************************************************/
/*************************** Texture Mapping ****************************/
/************************************************************************/

static void fxTexInvalidate(GLcontext *ctx, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti;

  ti=fxTMGetTexInfo(tObj);
  if (ti->isInTM)   fxTMMoveOutTM(fxMesa,tObj); /* TO DO: SLOW but easy to write */

  ti->validated=GL_FALSE;
  fxMesa->new_state|=FX_NEW_TEXTURING;
  ctx->Driver.RenderStart = fxSetupFXUnits;
}

static tfxTexInfo *fxAllocTexObjData(fxMesaContext fxMesa)
{
  tfxTexInfo *ti;
  int i;

  if(!(ti=CALLOC(sizeof(tfxTexInfo)))) {
    fprintf(stderr,"fx Driver: out of memory !\n");
    fxCloseHardware();
    exit(-1);
  }

  ti->validated=GL_FALSE;
  ti->isInTM=GL_FALSE;

  ti->whichTMU=FX_TMU_NONE;

  ti->tm[FX_TMU0]=NULL;
  ti->tm[FX_TMU1]=NULL;

  ti->minFilt=GR_TEXTUREFILTER_POINT_SAMPLED;
  ti->maxFilt=GR_TEXTUREFILTER_BILINEAR;

  ti->sClamp=GR_TEXTURECLAMP_WRAP;
  ti->tClamp=GR_TEXTURECLAMP_WRAP;

  ti->mmMode=GR_MIPMAP_NEAREST;
  ti->LODblend=FXFALSE;

  for(i=0;i<MAX_TEXTURE_LEVELS;i++) {
    ti->mipmapLevel[i].data=NULL;
  }

  return ti;
}

void fxDDTexBind(GLcontext *ctx, GLenum target, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDTexBind(%d,%x)\n",tObj->Name,(GLuint)tObj->DriverData);
  }

  if(target!=GL_TEXTURE_2D)
    return;

  if (!tObj->DriverData) {
    tObj->DriverData=fxAllocTexObjData(fxMesa);
  }

  ti=fxTMGetTexInfo(tObj);

  fxMesa->texBindNumber++;
  ti->lastTimeUsed=fxMesa->texBindNumber;

  fxMesa->new_state|=FX_NEW_TEXTURING;
  ctx->Driver.RenderStart = fxSetupFXUnits;
}

void fxDDTexEnv(GLcontext *ctx, GLenum target, GLenum pname, const GLfloat *param)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      if(param)
	 fprintf(stderr,"fxmesa: texenv(%x,%x)\n",pname,(GLint)(*param));
      else
	 fprintf(stderr,"fxmesa: texenv(%x)\n",pname);
   }

   /* apply any lod biasing right now */
   if (pname==GL_TEXTURE_LOD_BIAS_EXT) {
     FX_grTexLodBiasValue(GR_TMU0,*param);

     if(fxMesa->haveTwoTMUs) {
       FX_grTexLodBiasValue(GR_TMU1,*param);
     }

   }

   fxMesa->new_state|=FX_NEW_TEXTURING;
   ctx->Driver.RenderStart = fxSetupFXUnits;
}

void fxDDTexParam(GLcontext *ctx, GLenum target, struct gl_texture_object *tObj,
                  GLenum pname, const GLfloat *params)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLenum param=(GLenum)(GLint)params[0];
  tfxTexInfo *ti;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDTexParam(%d,%x,%x,%x)\n",tObj->Name,(GLuint)tObj->DriverData,pname,param);
  }

  if(target!=GL_TEXTURE_2D)
    return;

  if (!tObj->DriverData)
    tObj->DriverData=fxAllocTexObjData(fxMesa);

  ti=fxTMGetTexInfo(tObj);

  switch(pname) {

  case GL_TEXTURE_MIN_FILTER:
    switch(param) {
    case GL_NEAREST:
      ti->mmMode=GR_MIPMAP_DISABLE;
      ti->minFilt=GR_TEXTUREFILTER_POINT_SAMPLED;
      ti->LODblend=FXFALSE;
      break;
    case GL_LINEAR:
      ti->mmMode=GR_MIPMAP_DISABLE;
      ti->minFilt=GR_TEXTUREFILTER_BILINEAR;
      ti->LODblend=FXFALSE;
      break;
    case GL_NEAREST_MIPMAP_NEAREST:
      ti->mmMode=GR_MIPMAP_NEAREST;
      ti->minFilt=GR_TEXTUREFILTER_POINT_SAMPLED;
      ti->LODblend=FXFALSE;
      break;
    case GL_LINEAR_MIPMAP_NEAREST:
      ti->mmMode=GR_MIPMAP_NEAREST;
      ti->minFilt=GR_TEXTUREFILTER_BILINEAR;
      ti->LODblend=FXFALSE;
      break;
    case GL_NEAREST_MIPMAP_LINEAR:
      if(fxMesa->haveTwoTMUs) {
        ti->mmMode=GR_MIPMAP_NEAREST;
        ti->LODblend=FXTRUE;
      } else {
        ti->mmMode=GR_MIPMAP_NEAREST_DITHER;
        ti->LODblend=FXFALSE;
      }
      ti->minFilt=GR_TEXTUREFILTER_POINT_SAMPLED;
      break;
    case GL_LINEAR_MIPMAP_LINEAR:
      if(fxMesa->haveTwoTMUs) {
        ti->mmMode=GR_MIPMAP_NEAREST;
        ti->LODblend=FXTRUE;
      } else {
        ti->mmMode=GR_MIPMAP_NEAREST_DITHER;
        ti->LODblend=FXFALSE;
      }
      ti->minFilt=GR_TEXTUREFILTER_BILINEAR;
      break;
    default:
      break;
    }
    fxTexInvalidate(ctx,tObj);
    break;

  case GL_TEXTURE_MAG_FILTER:
    switch(param) {
    case GL_NEAREST:
      ti->maxFilt=GR_TEXTUREFILTER_POINT_SAMPLED;
      break;
    case GL_LINEAR:
      ti->maxFilt=GR_TEXTUREFILTER_BILINEAR;
      break;
    default:
      break;
    }
    fxTexInvalidate(ctx,tObj);
    break;

  case GL_TEXTURE_WRAP_S:
    switch(param) {
    case GL_CLAMP:
      ti->sClamp=GR_TEXTURECLAMP_CLAMP;
      break;
    case GL_REPEAT:
      ti->sClamp=GR_TEXTURECLAMP_WRAP;
      break;
    default:
      break;
    }
    fxMesa->new_state|=FX_NEW_TEXTURING;
    ctx->Driver.RenderStart = fxSetupFXUnits;
    break;

  case GL_TEXTURE_WRAP_T:
    switch(param) {
    case GL_CLAMP:
      ti->tClamp=GR_TEXTURECLAMP_CLAMP;
      break;
    case GL_REPEAT:
      ti->tClamp=GR_TEXTURECLAMP_WRAP;
      break;
    default:
      break;
    }
    fxMesa->new_state|=FX_NEW_TEXTURING;
    ctx->Driver.RenderStart = fxSetupFXUnits;
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
    fxTexInvalidate(ctx,tObj);
    break;
  case GL_TEXTURE_MAX_LEVEL:
    fxTexInvalidate(ctx,tObj);
    break;

  default:
    break;
  }
}

void fxDDTexDel(GLcontext *ctx, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  tfxTexInfo *ti = fxTMGetTexInfo(tObj);

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
     fprintf(stderr, "fxmesa: fxDDTexDel(%d,%p)\n", tObj->Name, ti);
  }

  if (!ti)
    return;

  fxTMFreeTexture(fxMesa, tObj);

  FREE(ti);
  tObj->DriverData = NULL;

/* Pushed into core: Set _NEW_TEXTURE whenever a bound texture is
 * deleted (changes bound texture id).
 */
/*    ctx->NewState |= _NEW_TEXTURE; */
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
        r = g = b = tableUB[i*2+0];
        a = tableUB[i*2+1];
        data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      break;
    case GL_RGB:
      for (i = 0; i < width; i++) {
        r = tableUB[i*3+0];
        g = tableUB[i*3+1];
        b = tableUB[i*3+2];
        a = 255;
        data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      break;
    case GL_RGBA:
      for (i = 0; i < width; i++) {
        r = tableUB[i*4+0];
        g = tableUB[i*4+1];
        b = tableUB[i*4+2];
        a = tableUB[i*4+3];
        data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      break;
  }
}


void fxDDTexPalette(GLcontext *ctx, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);

  if (tObj) {
    /* per-texture palette */
    tfxTexInfo *ti;
    if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDTexPalette(%d,%x)\n",
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
    if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDTexPalette(global)\n");
    }
    convertPalette(fxMesa->glbPalette.data, &ctx->Texture.Palette);
    fxMesa->new_state |= FX_NEW_TEXTURING;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }
}


void fxDDTexUseGlbPalette(GLcontext *ctx, GLboolean state)
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDTexUseGlbPalette(%d)\n",state);
  }

  if (state) {
    fxMesa->haveGlobalPaletteTexture = 1;

    FX_grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE, &(fxMesa->glbPalette));
    if (fxMesa->haveTwoTMUs)
       FX_grTexDownloadTable(GR_TMU1, GR_TEXTABLE_PALETTE, &(fxMesa->glbPalette));
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


static int logbase2(int n)
{
  GLint i = 1;
  GLint log2 = 0;

  if (n<0) {
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

/* Need different versions for different cpus.
 */
#define INT_TRICK(l2) (0x800000 * l2)


int fxTexGetInfo(int w, int h, GrLOD_t *lodlevel, GrAspectRatio_t *ar,
                 float *sscale, float *tscale,
                 int *i_sscale, int *i_tscale,
                 int *wscale, int *hscale)
{

  static GrLOD_t lod[9]={GR_LOD_256,GR_LOD_128,GR_LOD_64,GR_LOD_32,
                         GR_LOD_16,GR_LOD_8,GR_LOD_4,GR_LOD_2,GR_LOD_1};

  int logw,logh,ws,hs;
  GrLOD_t l;
  GrAspectRatio_t aspectratio;
  float s,t;
  int is,it;

  logw=logbase2(w);
  logh=logbase2(h);

  switch(logw-logh) {
  case 0:
    aspectratio=GR_ASPECT_1x1;
    l=lod[8-logw];
    s=t=256.0f;
    is=it=INT_TRICK(8);
    ws=hs=1;
    break;
  case 1:
    aspectratio=GR_ASPECT_2x1;
    l=lod[8-logw];
    s=256.0f;
    t=128.0f;
    is=INT_TRICK(8);it=INT_TRICK(7);
    ws=1;
    hs=1;
    break;
  case 2:
    aspectratio=GR_ASPECT_4x1;
    l=lod[8-logw];
    s=256.0f;
    t=64.0f;
    is=INT_TRICK(8);it=INT_TRICK(6);
    ws=1;
    hs=1;
    break;
  case 3:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=1;
    break;
  case 4:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=2;
    break;
  case 5:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=4;
    break;
  case 6:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=8;
    break;
  case 7:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=16;
    break;
  case 8:
    aspectratio=GR_ASPECT_8x1;
    l=lod[8-logw];
    s=256.0f;
    t=32.0f;
    is=INT_TRICK(8);it=INT_TRICK(5);
    ws=1;
    hs=32;
    break;
  case -1:
    aspectratio=GR_ASPECT_1x2;
    l=lod[8-logh];
    s=128.0f;
    t=256.0f;
    is=INT_TRICK(7);it=INT_TRICK(8);
    ws=1;
    hs=1;
    break;
  case -2:
    aspectratio=GR_ASPECT_1x4;
    l=lod[8-logh];
    s=64.0f;
    t=256.0f;
    is=INT_TRICK(6);it=INT_TRICK(8);
    ws=1;
    hs=1;
    break;
  case -3:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=1;
    hs=1;
    break;
  case -4:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=2;
    hs=1;
    break;
  case -5:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=4;
    hs=1;
    break;
  case -6:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=8;
    hs=1;
    break;
  case -7:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=16;
    hs=1;
    break;
  case -8:
    aspectratio=GR_ASPECT_1x8;
    l=lod[8-logh];
    s=32.0f;
    t=256.0f;
    is=INT_TRICK(5);it=INT_TRICK(8);
    ws=32;
    hs=1;
    break;
  default:
    return 0;
    break;
  }

  if(lodlevel)
    (*lodlevel)=l;

  if(ar)
    (*ar)=aspectratio;

  if(sscale)
    (*sscale)=s;

  if(tscale)
    (*tscale)=t;

  if(wscale)
    (*wscale)=ws;

  if(hscale)
    (*hscale)=hs;

  if (i_sscale)
     *i_sscale = is;

  if (i_tscale)
     *i_tscale = it;


  return 1;
}

/*
 * Given an OpenGL internal texture format, return the corresponding
 * Glide internal texture format and base texture format.
 */
void fxTexGetFormat(GLenum glformat, GrTextureFormat_t *tfmt, GLint *ifmt)
{
  switch(glformat) {
    case 1:
    case GL_LUMINANCE:
    case GL_LUMINANCE4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE12:
    case GL_LUMINANCE16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_INTENSITY_8;
      if(ifmt)
        (*ifmt)=GL_LUMINANCE;
      break;
    case 2:
    case GL_LUMINANCE_ALPHA:
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_ALPHA_INTENSITY_88;
      if(ifmt)
        (*ifmt)=GL_LUMINANCE_ALPHA;
      break;
    case GL_INTENSITY:
    case GL_INTENSITY4:
    case GL_INTENSITY8:
    case GL_INTENSITY12:
    case GL_INTENSITY16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_ALPHA_8;
      if(ifmt)
        (*ifmt)=GL_INTENSITY;
      break;
    case GL_ALPHA:
    case GL_ALPHA4:
    case GL_ALPHA8:
    case GL_ALPHA12:
    case GL_ALPHA16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_ALPHA_8;
      if(ifmt)
        (*ifmt)=GL_ALPHA;
      break;
    case 3:
    case GL_RGB:
    case GL_R3_G3_B2:
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_RGB_565;
      if(ifmt)
        (*ifmt)=GL_RGB;
      break;
    case 4:
    case GL_RGBA:
    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGBA8:
    case GL_RGB10_A2:
    case GL_RGBA12:
    case GL_RGBA16:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_ARGB_4444;
      if(ifmt)
        (*ifmt)=GL_RGBA;
      break;
    case GL_RGB5_A1:
       if(tfmt)
         (*tfmt)=GR_TEXFMT_ARGB_1555;
       if(ifmt)
         (*ifmt)=GL_RGBA;
       break;
    case GL_COLOR_INDEX:
    case GL_COLOR_INDEX1_EXT:
    case GL_COLOR_INDEX2_EXT:
    case GL_COLOR_INDEX4_EXT:
    case GL_COLOR_INDEX8_EXT:
    case GL_COLOR_INDEX12_EXT:
    case GL_COLOR_INDEX16_EXT:
      if(tfmt)
        (*tfmt)=GR_TEXFMT_P_8;
      if(ifmt)
        (*ifmt)=GL_RGBA;   /* XXX why is this RGBA? */
      break;
    default:
      fprintf(stderr,
              "fx Driver: unsupported internalFormat in fxTexGetFormat()\n");
      fxCloseHardware();
      exit(-1);
      break;
  }
}

static GLboolean fxIsTexSupported(GLenum target, GLint internalFormat,
                                  const struct gl_texture_image *image)
{
  if(target != GL_TEXTURE_2D)
    return GL_FALSE;

  if(!fxTexGetInfo(image->Width,image->Height,NULL,NULL,NULL,NULL,NULL,NULL,
		   NULL,NULL))
    return GL_FALSE;

  if (image->Border > 0)
    return GL_FALSE;

  return GL_TRUE;
}


/**********************************************************************/
/**** NEW TEXTURE IMAGE FUNCTIONS                                  ****/
/**********************************************************************/

GLboolean fxDDTexImage2D(GLcontext *ctx, GLenum target, GLint level,
                         GLenum format, GLenum type, const GLvoid *pixels,
                         const struct gl_pixelstore_attrib *packing,
                         struct gl_texture_object *texObj,
                         struct gl_texture_image *texImage,
                         GLboolean *retainInternalCopy)
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;

  if (target != GL_TEXTURE_2D)
    return GL_FALSE;

  if (!texObj->DriverData)
    texObj->DriverData = fxAllocTexObjData(fxMesa);

  if (fxIsTexSupported(target, texImage->IntFormat, texImage)) {
    GrTextureFormat_t gldformat;
    tfxTexInfo *ti = fxTMGetTexInfo(texObj);
    tfxMipMapLevel *mml = &ti->mipmapLevel[level];
    GLint dstWidth, dstHeight, wScale, hScale, texelSize, dstStride;
    MesaIntTexFormat intFormat;

    fxTexGetFormat(texImage->IntFormat, &gldformat, NULL);

    fxTexGetInfo(texImage->Width, texImage->Height, NULL,NULL,NULL,NULL,
                 NULL,NULL, &wScale, &hScale);
    
    dstWidth = texImage->Width * wScale;
    dstHeight = texImage->Height * hScale;

    switch (texImage->IntFormat) {
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
        texelSize = 1;
        intFormat = MESA_I8;
        break;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
        texelSize = 1;
        intFormat = MESA_L8;
        break;
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
        texelSize = 1;
        intFormat = MESA_A8;
        break;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
        texelSize = 1;
        intFormat = MESA_C8;
        break;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
        texelSize = 2;
        intFormat = MESA_A8_L8;
        break;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
        texelSize = 2;
        intFormat = MESA_R5_G6_B5;
        break;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
        texelSize = 2;
        intFormat = MESA_A4_R4_G4_B4;
        break;
      case GL_RGB5_A1:
        texelSize = 2;
        intFormat = MESA_A1_R5_G5_B5;
        break;
      default:
        gl_problem(NULL, "tdfx driver: texbuildimagemap() bad format");
        return GL_FALSE;
    }

    _mesa_set_teximage_component_sizes(intFormat, texImage);

    /*printf("teximage:\n");*/
    /* allocate new storage for texture image, if needed */
    if (!mml->data || mml->glideFormat != gldformat ||
        mml->width != dstWidth || mml->height != dstHeight) {
      if (mml->data)
        FREE(mml->data);
      mml->data = MALLOC(dstWidth * dstHeight * texelSize);
      if (!mml->data)
        return GL_FALSE;
      mml->glideFormat = gldformat;
      mml->width = dstWidth;
      mml->height = dstHeight;
      fxTexInvalidate(ctx, texObj);
    }

    dstStride = dstWidth * texelSize;

    /* store the texture image */
    if (!_mesa_convert_teximage(intFormat, dstWidth, dstHeight, mml->data,
                                dstStride,
                                texImage->Width, texImage->Height,
                                format, type, pixels, packing)) {
      return GL_FALSE;
    }
    
    if (ti->validated && ti->isInTM) {
      /*printf("reloadmipmaplevels\n");*/
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
    }
    else {
      /*printf("invalidate2\n");*/
      fxTexInvalidate(ctx,texObj);
    }

    *retainInternalCopy = GL_FALSE;
    return GL_TRUE;
  }
  else {
    gl_problem(NULL, "fx Driver: unsupported texture in fxDDTexImg()\n");
    return GL_FALSE;
  }
}


GLboolean fxDDTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                            GLint xoffset, GLint yoffset,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type, const GLvoid *pixels,
                            const struct gl_pixelstore_attrib *packing,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage)
{
  fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
  tfxTexInfo *ti;
  GLint wscale, hscale, dstStride;
  tfxMipMapLevel *mml;
  GLboolean result;

  if (target != GL_TEXTURE_2D)
    return GL_FALSE;

  if (!texObj->DriverData)
    return GL_FALSE;

  ti = fxTMGetTexInfo(texObj);
  mml = &ti->mipmapLevel[level];

  fxTexGetInfo( texImage->Width, texImage->Height, NULL,NULL,NULL,NULL,
                NULL,NULL, &wscale, &hscale);

  assert(mml->data);  /* must have an existing texture image! */

  switch (mml->glideFormat) {
    case GR_TEXFMT_INTENSITY_8:
      dstStride = mml->width;
      result = _mesa_convert_texsubimage(MESA_I8, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_ALPHA_8:
      dstStride = mml->width;
      result = _mesa_convert_texsubimage(MESA_A8, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_P_8:
      dstStride = mml->width;
      result = _mesa_convert_texsubimage(MESA_C8, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_ALPHA_INTENSITY_88:
      dstStride = mml->width * 2;
      result = _mesa_convert_texsubimage(MESA_A8_L8, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_RGB_565:
      dstStride = mml->width * 2;
      result = _mesa_convert_texsubimage(MESA_R5_G6_B5, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_ARGB_4444:
      dstStride = mml->width * 2;
      result = _mesa_convert_texsubimage(MESA_A4_R4_G4_B4, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    case GR_TEXFMT_ARGB_1555:
      dstStride = mml->width * 2;
      result = _mesa_convert_texsubimage(MESA_A1_R5_G5_B5, xoffset, yoffset,
                                         mml->width, mml->height, mml->data,
                                         dstStride, width, height,
                                         texImage->Width, texImage->Height,
                                         format, type, pixels, packing);
      break;
    default:
      gl_problem(NULL, "tdfx driver: fxTexBuildSubImageMap() bad format");
      result = GL_FALSE;
  }

  if (!result) {
    return GL_FALSE;
  }

  if (ti->validated && ti->isInTM)
    fxTMReloadMipMapLevel(fxMesa, texObj, level);
  else
    fxTexInvalidate(ctx, texObj);

  return GL_TRUE;
}


static void PrintTexture(int w, int h, int c, const GLubyte *data)
{
  int i, j;
  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      if (c==2)
        printf("%02x %02x  ", data[0], data[1]);
      else if (c==3)
        printf("%02x %02x %02x  ", data[0], data[1], data[2]);
      data += c;
    }
    printf("\n");
  }
}


GLvoid *fxDDGetTexImage(GLcontext *ctx, GLenum target, GLint level,
                        const struct gl_texture_object *texObj,
                        GLenum *formatOut, GLenum *typeOut,
                        GLboolean *freeImageOut )
{
  tfxTexInfo *ti;
  tfxMipMapLevel *mml;

  if (target != GL_TEXTURE_2D)
    return NULL;

  if (!texObj->DriverData)
    return NULL;

  ti = fxTMGetTexInfo(texObj);
  mml = &ti->mipmapLevel[level];
  if (mml->data) {
    MesaIntTexFormat mesaFormat;
    GLenum glFormat;
    struct gl_texture_image *texImage = texObj->Image[level];
    GLint srcStride;

    GLubyte *data = (GLubyte *) MALLOC(texImage->Width * texImage->Height * 4);
    if (!data)
      return NULL;

    switch (mml->glideFormat) {
      case GR_TEXFMT_INTENSITY_8:
        mesaFormat = MESA_I8;
        glFormat = GL_INTENSITY;
        srcStride = mml->width;
        break;
      case GR_TEXFMT_ALPHA_INTENSITY_88:
        mesaFormat = MESA_A8_L8;
        glFormat = GL_LUMINANCE_ALPHA;
        srcStride = mml->width;
        break;
      case GR_TEXFMT_ALPHA_8:
        mesaFormat = MESA_A8;
        glFormat = GL_ALPHA;
        srcStride = mml->width;
        break;
      case GR_TEXFMT_RGB_565:
        mesaFormat = MESA_R5_G6_B5;
        glFormat = GL_RGB;
        srcStride = mml->width * 2;
        break;
      case GR_TEXFMT_ARGB_4444:
        mesaFormat = MESA_A4_R4_G4_B4;
        glFormat = GL_RGBA;
        srcStride = mml->width * 2;
        break;
      case GR_TEXFMT_ARGB_1555:
        mesaFormat = MESA_A1_R5_G5_B5;
        glFormat = GL_RGBA;
        srcStride = mml->width * 2;
        break;
      case GR_TEXFMT_P_8:
        mesaFormat = MESA_C8;
        glFormat = GL_COLOR_INDEX;
        srcStride = mml->width;
        break;
      default:
        gl_problem(NULL, "Bad glideFormat in fxDDGetTexImage");
        return NULL;
    }
    _mesa_unconvert_teximage(mesaFormat, mml->width, mml->height, mml->data,
                             srcStride, texImage->Width, texImage->Height,
                             glFormat, data);
    *formatOut = glFormat;
    *typeOut = GL_UNSIGNED_BYTE;
    *freeImageOut = GL_TRUE;
    return data;
  }
  else {
    return NULL;
  }
}


#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_ddtex(void)
{
  return 0;
}

#endif  /* FX */
