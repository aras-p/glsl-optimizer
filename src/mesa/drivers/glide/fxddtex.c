/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

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
  fprintf(stderr, "\tTMU: %d\n", ti->whichTMU);
  fprintf(stderr, "\t%s\n", (ti->isInTM)?"In TMU":"Not in TMU");
  if (ti->tm[0]) 
    fprintf(stderr, "\tMem0: %x-%x\n", ti->tm[0]->startAddr, 
	    ti->tm[0]->endAddr);
  if (ti->tm[1]) 
    fprintf(stderr, "\tMem1: %x-%x\n", ti->tm[1]->startAddr, 
	    ti->tm[1]->endAddr);
  fprintf(stderr, "\tMipmaps: %d-%d\n", ti->minLevel, ti->maxLevel);
  fprintf(stderr, "\tFilters: min %d min %d\n", ti->minFilt, ti->maxFilt);
  fprintf(stderr, "\tClamps: s %d t %d\n", ti->sClamp, ti->tClamp);
  fprintf(stderr, "\tScales: s %f t %f\n", ti->sScale, ti->tScale);
  fprintf(stderr, "\tInt Scales: s %d t %d\n", 
	  ti->int_sScale/0x800000, ti->int_tScale/0x800000);
  fprintf(stderr, "\t%s\n", (ti->fixedPalette)?"Fixed palette":"Non fixed palette");
  fprintf(stderr, "\t%s\n", (ti->validated)?"Validated":"Not validated");
}


/************************************************************************/
/*************************** Texture Mapping ****************************/
/************************************************************************/

void fxTexInvalidate(GLcontext *ctx, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti;

  fxTMMoveOutTM(fxMesa,tObj); /* TO DO: SLOW but easy to write */

  ti=fxTMGetTexInfo(tObj);
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
    ti->mipmapLevel[i].used=GL_FALSE;
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

void fxDDTexEnv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      if(param)
	 fprintf(stderr,"fxmesa: texenv(%x,%x)\n",pname,(GLint)(*param));
      else
	 fprintf(stderr,"fxmesa: texenv(%x)\n",pname);
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
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti=fxTMGetTexInfo(tObj);

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDTexDel(%d,%x)\n",tObj->Name,(GLuint)ti);
  }

  if(!ti)
    return;

  fxTMFreeTexture(fxMesa,tObj);

  FREE(ti);
  tObj->DriverData=NULL;

  ctx->NewState|=NEW_TEXTURING;
}

void fxDDTexPalette(GLcontext *ctx, struct gl_texture_object *tObj)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  int i;
  FxU32 r,g,b,a;
  tfxTexInfo *ti;

  if(tObj) {  
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
	fprintf(stderr,"fxmesa: fxDDTexPalette(%d,%x)\n",tObj->Name,(GLuint)tObj->DriverData);
     }

    if(tObj->Palette.Format!=GL_RGBA) {
#ifndef FX_SILENT
      fprintf(stderr,"fx Driver: unsupported palette format in texpalette()\n");
#endif
      return;
    }

    if(tObj->Palette.Size>256) {
#ifndef FX_SILENT
      fprintf(stderr,"fx Driver: unsupported palette size in texpalette()\n");
#endif
      return;
    }

    if (!tObj->DriverData)
      tObj->DriverData=fxAllocTexObjData(fxMesa);
  
    ti=fxTMGetTexInfo(tObj);

    for(i=0;i<tObj->Palette.Size;i++) {
      r=tObj->Palette.Table[i*4];
      g=tObj->Palette.Table[i*4+1];
      b=tObj->Palette.Table[i*4+2];
      a=tObj->Palette.Table[i*4+3];
      ti->palette.data[i]=(a<<24)|(r<<16)|(g<<8)|b;
    }

    fxTexInvalidate(ctx,tObj);
  } else {
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
	fprintf(stderr,"fxmesa: fxDDTexPalette(global)\n");
     }
    if(ctx->Texture.Palette.Format!=GL_RGBA) {
#ifndef FX_SILENT
      fprintf(stderr,"fx Driver: unsupported palette format in texpalette()\n");
#endif
      return;
    }

    if(ctx->Texture.Palette.Size>256) {
#ifndef FX_SILENT
      fprintf(stderr,"fx Driver: unsupported palette size in texpalette()\n");
#endif
      return;
    }

    for(i=0;i<ctx->Texture.Palette.Size;i++) {
      r=ctx->Texture.Palette.Table[i*4];
      g=ctx->Texture.Palette.Table[i*4+1];
      b=ctx->Texture.Palette.Table[i*4+2];
      a=ctx->Texture.Palette.Table[i*4+3];
      fxMesa->glbPalette.data[i]=(a<<24)|(r<<16)|(g<<8)|b;
    }

    fxMesa->new_state|=FX_NEW_TEXTURING;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }
}

void fxDDTexUseGlbPalette(GLcontext *ctx, GLboolean state)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDTexUseGlbPalette(%d)\n",state);
  }

  if(state) {
    fxMesa->haveGlobalPaletteTexture=1;

    FX_grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&(fxMesa->glbPalette));
    if (fxMesa->haveTwoTMUs)
       FX_grTexDownloadTable(GR_TMU1,GR_TEXTABLE_PALETTE,&(fxMesa->glbPalette));
  } else {
    fxMesa->haveGlobalPaletteTexture=0;

    if((ctx->Texture.Unit[0].Current==ctx->Texture.Unit[0].CurrentD[2]) &&
       (ctx->Texture.Unit[0].Current!=NULL)) {
      struct gl_texture_object *tObj=ctx->Texture.Unit[0].Current;

      if (!tObj->DriverData)
        tObj->DriverData=fxAllocTexObjData(fxMesa);
  
      fxTexInvalidate(ctx,tObj);
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
      (*ifmt)=GL_RGBA;
    break;
  default:
    fprintf(stderr,"fx Driver: unsupported internalFormat in fxTexGetFormat()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }
}

static int fxIsTexSupported(GLenum target, GLint internalFormat,
                            const struct gl_texture_image *image)
{
  if(target!=GL_TEXTURE_2D)
    return GL_FALSE;

  switch(internalFormat) {
  case GL_INTENSITY:
  case GL_INTENSITY4:
  case GL_INTENSITY8:
  case GL_INTENSITY12:
  case GL_INTENSITY16:
  case 1:
  case GL_LUMINANCE:
  case GL_LUMINANCE4:
  case GL_LUMINANCE8:
  case GL_LUMINANCE12:
  case GL_LUMINANCE16:
  case 2:
  case GL_LUMINANCE_ALPHA:
  case GL_LUMINANCE4_ALPHA4:
  case GL_LUMINANCE6_ALPHA2:
  case GL_LUMINANCE8_ALPHA8:
  case GL_LUMINANCE12_ALPHA4:
  case GL_LUMINANCE12_ALPHA12:
  case GL_LUMINANCE16_ALPHA16:
  case GL_ALPHA:
  case GL_ALPHA4:
  case GL_ALPHA8:
  case GL_ALPHA12:
  case GL_ALPHA16:
  case 3:
  case GL_RGB:
  case GL_R3_G3_B2:
  case GL_RGB4:
  case GL_RGB5:
  case GL_RGB8:
  case GL_RGB10:
  case GL_RGB12:
  case GL_RGB16:
  case 4:
  case GL_RGBA:
  case GL_RGBA2:
  case GL_RGBA4:
  case GL_RGB5_A1:
  case GL_RGBA8:
  case GL_RGB10_A2:
  case GL_RGBA12:
  case GL_RGBA16:
  case GL_COLOR_INDEX:
  case GL_COLOR_INDEX1_EXT:
  case GL_COLOR_INDEX2_EXT:
  case GL_COLOR_INDEX4_EXT:
  case GL_COLOR_INDEX8_EXT:
  case GL_COLOR_INDEX12_EXT:
  case GL_COLOR_INDEX16_EXT:
    break;
  default:
    return GL_FALSE;
  }

  if(image->Width>256)
    return GL_FALSE;

  if(image->Height>256)
    return GL_FALSE;

  if(!fxTexGetInfo(image->Width,image->Height,NULL,NULL,NULL,NULL,NULL,NULL,
		   NULL,NULL))
    return GL_FALSE;

  return GL_TRUE;
}

static void fxTexBuildImageMap(const struct gl_texture_image *image,
                               GLint internalFormat, unsigned short **dest,
                               GLboolean *istranslate)
{
  unsigned short *src;
  unsigned char *data;
  int x,y,w,h,wscale,hscale,idx;

  fxTexGetInfo(image->Width,image->Height,NULL,NULL,NULL,NULL,NULL,NULL,
	       &wscale,&hscale);
  w=image->Width*wscale;
  h=image->Height*hscale;

  data=image->Data;
  switch(internalFormat) {
  case GL_INTENSITY:
  case GL_INTENSITY4:
  case GL_INTENSITY8:
  case GL_INTENSITY12:
  case GL_INTENSITY16:
  case 1:
  case GL_LUMINANCE:
  case GL_LUMINANCE4:
  case GL_LUMINANCE8:
  case GL_LUMINANCE12:
  case GL_LUMINANCE16:
  case GL_ALPHA:
  case GL_ALPHA4:
  case GL_ALPHA8:
  case GL_ALPHA12:
  case GL_ALPHA16:
  case GL_COLOR_INDEX:
  case GL_COLOR_INDEX1_EXT:
  case GL_COLOR_INDEX2_EXT:
  case GL_COLOR_INDEX4_EXT:
  case GL_COLOR_INDEX8_EXT:
  case GL_COLOR_INDEX12_EXT:
  case GL_COLOR_INDEX16_EXT:
    /* Optimized for GLQuake */

    if(wscale==hscale==1) {
      (*istranslate)=GL_FALSE;

      (*dest)=(unsigned short *)data;
    } else {
      unsigned char *srcb;

      (*istranslate)=GL_TRUE;

      if(!(*dest)) {
        if(!((*dest)=src=(unsigned short *)MALLOC(sizeof(unsigned char)*w*h))) {
          fprintf(stderr,"fx Driver: out of memory !\n");
          fxCloseHardware();
          exit(-1);
        }
      } else
        src=(*dest);

      srcb=(unsigned char *)src;

      for(y=0;y<h;y++)
        for(x=0;x<w;x++) {
          idx=(x/wscale+(y/hscale)*(w/wscale));
          srcb[x+y*w]=data[idx];
        }
    }
    break;
  case 2:
  case GL_LUMINANCE_ALPHA:
  case GL_LUMINANCE4_ALPHA4:
  case GL_LUMINANCE6_ALPHA2:
  case GL_LUMINANCE8_ALPHA8:
  case GL_LUMINANCE12_ALPHA4:
  case GL_LUMINANCE12_ALPHA12:
  case GL_LUMINANCE16_ALPHA16:
    (*istranslate)=GL_TRUE;

    if(!(*dest)) {
      if(!((*dest)=src=(unsigned short *)MALLOC(sizeof(unsigned short)*w*h))) {
        fprintf(stderr,"fx Driver: out of memory !\n");
        fxCloseHardware();
        exit(-1);
      }
    } else
      src=(*dest);

    if(wscale==hscale==1) {
      int i=0;
      int lenght=h*w;
      unsigned short a,l;

      while(i++<lenght) {
        l=*data++;
        a=*data++;

        *src++=(a << 8) | l;
      }
    } else {
      unsigned short a,l;

      for(y=0;y<h;y++)
        for(x=0;x<w;x++) {
          idx=(x/wscale+(y/hscale)*(w/wscale))*2;
          l=data[idx];
          a=data[idx+1];

          src[x+y*w]=(a << 8) | l;
        }
    }
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
    (*istranslate)=GL_TRUE;

    if(!(*dest)) {
      if(!((*dest)=src=(unsigned short *)MALLOC(sizeof(unsigned short)*w*h))) {
        fprintf(stderr,"fx Driver: out of memory !\n");
        fxCloseHardware();
        exit(-1);
      }
    } else
      src=(*dest);

    if(wscale==hscale==1) {
      int i=0;
      int lenght=h*w;
      unsigned int r,g,b;

      while(i++<lenght) {
        r=*data++;
        g=*data++;
        b=*data++;

        *src++=((0xf8 & r) << (11-3))  |
          ((0xfc & g) << (5-3+1))      |
          ((0xf8 & b) >> 3); 
      }
    } else {
      unsigned int r,g,b;

      for(y=0;y<h;y++)
        for(x=0;x<w;x++) {
          idx=(x/wscale+(y/hscale)*(w/wscale))*3;
          r=data[idx];
          g=data[idx+1];
          b=data[idx+2];

          src[x+y*w]=((0xf8 & r) << (11-3))  |
            ((0xfc & g) << (5-3+1))      |
            ((0xf8 & b) >> 3); 
        }
    }
    break;
  case 4:
  case GL_RGBA:
  case GL_RGBA2:
  case GL_RGBA4:
  case GL_RGBA8:
  case GL_RGB10_A2:
  case GL_RGBA12:
  case GL_RGBA16:
    (*istranslate)=GL_TRUE;

    if(!(*dest)) {
      if(!((*dest)=src=(unsigned short *)MALLOC(sizeof(unsigned short)*w*h))) {
        fprintf(stderr,"fx Driver: out of memory !\n");
        fxCloseHardware();
        exit(-1);
      }
    } else
      src=(*dest);

    if(wscale==hscale==1) {
      int i=0;
      int lenght=h*w;
      unsigned int r,g,b,a;

      while(i++<lenght) {
        r=*data++;
        g=*data++;
        b=*data++;
        a=*data++;

        *src++=((0xf0 & a) << 8) |
          ((0xf0 & r) << 4)      |
          (0xf0 & g)             |
          ((0xf0 & b) >> 4);
      }
    } else {
      unsigned int r,g,b,a;

      for(y=0;y<h;y++)
        for(x=0;x<w;x++) {
          idx=(x/wscale+(y/hscale)*(w/wscale))*4;
          r=data[idx];
          g=data[idx+1];
          b=data[idx+2];
          a=data[idx+3];

          src[x+y*w]=((0xf0 & a) << 8) |
            ((0xf0 & r) << 4)      |
            (0xf0 & g)             |
            ((0xf0 & b) >> 4);
        }
    }
    break;
  case GL_RGB5_A1:
    (*istranslate)=GL_TRUE;

    if(!(*dest)) {
      if(!((*dest)=src=(unsigned short *)malloc(sizeof(unsigned short)*w*h))) {
        fprintf(stderr,"fx Driver: out of memory !\n");
        fxCloseHardware();
        exit(-1);
      }
    } else
      src=(*dest);

    if(wscale==hscale==1) {
      int i=0;
      int lenght=h*w;
      unsigned  r,g,b,a;

      while(i++<lenght) {
        r=*data++;
        g=*data++;
        b=*data++;
        a=*data++;
        *src++=((0x80 & a) << 8) |
          ((0xf8 & r) << 7)      |
          ((0xf8 & g) << 2)      |
          ((0xf8 & b) >> 3);
      }
    } else {
      unsigned r,g,b,a;

      for(y=0;y<h;y++)
        for(x=0;x<w;x++) {
          idx=(x/wscale+(y/hscale)*(w/wscale))*4;
          r=data[idx];
          g=data[idx+1];
          b=data[idx+2];
          a=data[idx+3];

          src[x+y*w]=((0x80 & a) << 8) |
          ((0xf8 & r) << 7)      |
          ((0xf8 & g) << 2)      |
          ((0xf8 & b) >> 3);
        }
    }
    break;
  default:
    fprintf(stderr,"fx Driver: wrong internalFormat in texbuildimagemap()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }
}

void fxDDTexImg(GLcontext *ctx, GLenum target,
                struct gl_texture_object *tObj, GLint level, GLint internalFormat,
                const struct gl_texture_image *image)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,
	     "fxmesa: (%d) fxDDTexImg(...,level=%d,target=%d,format=%x,width=%d,height=%d...)\n",
	     tObj->Name, level, target, internalFormat, image->Width,
	     image->Height);
  }

  if(target!=GL_TEXTURE_2D)
    return;

  if (!tObj->DriverData)
    tObj->DriverData=fxAllocTexObjData(fxMesa);

  ti=fxTMGetTexInfo(tObj);

  if(fxIsTexSupported(target,internalFormat,image)) {
    GrTextureFormat_t gldformat;
    tfxMipMapLevel *mml=&ti->mipmapLevel[level];

    fxTexGetFormat(internalFormat,&gldformat,NULL);
    
    if(mml->used) {
      if((mml->glideFormat==gldformat) &&
         (mml->width==image->Width) &&
         (mml->height==image->Height)) {
        fxTexBuildImageMap(image,internalFormat,&(mml->data),
                           &(mml->translated));

        if(ti->validated && ti->isInTM)
          fxTMReloadMipMapLevel(fxMesa,tObj,level);
        else
          fxTexInvalidate(ctx,tObj);

        return;
      } else {
        if(mml->translated)
          FREE(mml->data);
        mml->data=NULL;
      }
    }

    mml->glideFormat=gldformat;
    mml->width=image->Width;
    mml->height=image->Height;
    mml->used=GL_TRUE;

    fxTexBuildImageMap(image,internalFormat,&(mml->data),
                       &(mml->translated));

    fxTexInvalidate(ctx,tObj);
  }
#ifndef FX_SILENT
  else
    fprintf(stderr,"fx Driver: unsupported texture in fxDDTexImg()\n");
#endif
}

static void fxTexBuildSubImageMap(const struct gl_texture_image *image,
                                  GLint internalFormat,
                                  GLint xoffset, GLint yoffset, GLint width, GLint height,
                                  unsigned short *destimg)
{
  fxTexGetInfo(image->Width,image->Height,NULL,NULL,NULL,NULL,NULL,NULL,
	       NULL,NULL);

  switch(internalFormat) {
  case GL_INTENSITY:
  case GL_INTENSITY4:
  case GL_INTENSITY8:
  case GL_INTENSITY12:
  case GL_INTENSITY16:
  case 1:
  case GL_LUMINANCE:
  case GL_LUMINANCE4:
  case GL_LUMINANCE8:
  case GL_LUMINANCE12:
  case GL_LUMINANCE16:
  case GL_ALPHA:
  case GL_ALPHA4:
  case GL_ALPHA8:
  case GL_ALPHA12:
  case GL_ALPHA16:
  case GL_COLOR_INDEX:
  case GL_COLOR_INDEX1_EXT:
  case GL_COLOR_INDEX2_EXT:
  case GL_COLOR_INDEX4_EXT:
  case GL_COLOR_INDEX8_EXT:
  case GL_COLOR_INDEX12_EXT:
  case GL_COLOR_INDEX16_EXT:
    {

      int y;
      unsigned char *bsrc,*bdst;

      bsrc=(unsigned char *)(image->Data+(yoffset*image->Width+xoffset));
      bdst=((unsigned char *)destimg)+(yoffset*image->Width+xoffset);
    
      for(y=0;y<height;y++) {
        MEMCPY(bdst,bsrc,width);
        bsrc += image->Width;
        bdst += image->Width;
      }
    }
    break;
  case 2:
  case GL_LUMINANCE_ALPHA:
  case GL_LUMINANCE4_ALPHA4:
  case GL_LUMINANCE6_ALPHA2:
  case GL_LUMINANCE8_ALPHA8:
  case GL_LUMINANCE12_ALPHA4:
  case GL_LUMINANCE12_ALPHA12:
  case GL_LUMINANCE16_ALPHA16:
    {
      int x,y;
      unsigned char *src;
      unsigned short *dst,a,l;
      int simgw,dimgw;

      src=(unsigned char *)(image->Data+(yoffset*image->Width+xoffset)*2);
      dst=destimg+(yoffset*image->Width+xoffset);
    
      simgw=(image->Width-width)*2;
      dimgw=image->Width-width;
      for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
          l=*src++;
          a=*src++;
          *dst++=(a << 8) | l;
        }

        src += simgw;
        dst += dimgw;
      }
    }
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
    {
      int x,y;
      unsigned char *src;
      unsigned short *dst,r,g,b;
      int simgw,dimgw;

      src=(unsigned char *)(image->Data+(yoffset*image->Width+xoffset)*3);
      dst=destimg+(yoffset*image->Width+xoffset);
    
      simgw=(image->Width-width)*3;
      dimgw=image->Width-width;
      for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
          r=*src++;
          g=*src++;
          b=*src++;
          *dst++=((0xf8 & r) << (11-3))  |
            ((0xfc & g) << (5-3+1))      |
            ((0xf8 & b) >> 3); 
        }

        src += simgw;
        dst += dimgw;
      }
    }
    break;
  case 4:
  case GL_RGBA:
  case GL_RGBA2:
  case GL_RGBA4:
  case GL_RGBA8:
  case GL_RGB10_A2:
  case GL_RGBA12:
  case GL_RGBA16:
    {
      int x,y;
      unsigned char *src;
      unsigned short *dst,r,g,b,a;
      int simgw,dimgw;

      src=(unsigned char *)(image->Data+(yoffset*image->Width+xoffset)*4);
      dst=destimg+(yoffset*image->Width+xoffset);
    
      simgw=(image->Width-width)*4;
      dimgw=image->Width-width;
      for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
          r=*src++;
          g=*src++;
          b=*src++;
          a=*src++;
          *dst++=((0xf0 & a) << 8) |
            ((0xf0 & r) << 4)      |
            (0xf0 & g)             |
            ((0xf0 & b) >> 4);
        }

        src += simgw;
        dst += dimgw;
      }
    }
    break;
  case GL_RGB5_A1:
    {
      int x,y;
      unsigned char *src;
      unsigned short *dst,r,g,b,a;
      int simgw,dimgw;

      src=(unsigned char *)(image->Data+(yoffset*image->Width+xoffset)*4);
      dst=destimg+(yoffset*image->Width+xoffset);
    
      simgw=(image->Width-width)*4;
      dimgw=image->Width-width;
      for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
          r=*src++;
          g=*src++;
          b=*src++;
          a=*src++;
          *dst++=
          ((0x80 & a) << 8) |
          ((0xf8 & r) << 7)      |
          ((0xf8 & g) << 2)      |
          ((0xf8 & b) >> 3);
        }

        src += simgw;
        dst += dimgw;
      }
    }
    break; 
  default:
    fprintf(stderr,"fx Driver: wrong internalFormat in fxTexBuildSubImageMap()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }
}
 

void fxDDTexSubImg(GLcontext *ctx, GLenum target,
                   struct gl_texture_object *tObj, GLint level,
                   GLint xoffset, GLint yoffset, GLint width, GLint height,
                   GLint internalFormat, const struct gl_texture_image *image)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxTexInfo *ti;
  GrTextureFormat_t gldformat;
  int wscale,hscale;
  tfxMipMapLevel *mml;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,
	     "fxmesa: (%d) fxDDTexSubImg(level=%d,target=%d,format=%x,width=%d,height=%d)\n",
	     tObj->Name, level, target, internalFormat, image->Width,
	     image->Height);
  }

  if(target!=GL_TEXTURE_2D)
    return;

  if (!tObj->DriverData)
    return;

  ti=fxTMGetTexInfo(tObj);
  mml=&ti->mipmapLevel[level];

  fxTexGetFormat(internalFormat,&gldformat,NULL);

  if(mml->glideFormat!=gldformat) {
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
	fprintf(stderr,"fxmesa:  ti->info.format!=format in fxDDTexSubImg()\n");
     }
    fxDDTexImg(ctx,target,tObj,level,internalFormat,image);

    return;
  }

  fxTexGetInfo(image->Width,image->Height,NULL,NULL,NULL,NULL,NULL,NULL,&wscale,&hscale);

  if((wscale!=1) || (hscale!=1)) {
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
	fprintf(stderr,"fxmesa:  (wscale!=1) || (hscale!=1) in fxDDTexSubImg()\n");
     }
    fxDDTexImg(ctx,target,tObj,level,internalFormat,image);

    return;
  }

  if(mml->translated)
    fxTexBuildSubImageMap(image,internalFormat,xoffset,yoffset,
                          width,height,mml->data);

  if(ti->validated && ti->isInTM)
    fxTMReloadSubMipMapLevel(fxMesa,tObj,level,yoffset,height);
  else
    fxTexInvalidate(ctx,tObj);
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
