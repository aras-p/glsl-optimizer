/* -*- mode: C; tab-width:8;  -*-

             fxsetup.c - 3Dfx VooDoo rendering mode setup functions
*/

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * See the file fxapi.c for more informations about authors
 *
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "enums.h"

static void fxTexValidate(GLcontext *ctx, struct gl_texture_object *tObj)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  GLint minl,maxl;

  if (MESA_VERBOSE&VERBOSE_DRIVER) 
     fprintf(stderr,"fxmesa: fxTexValidate(...) Start\n");


  if(ti->validated) {
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
	fprintf(stderr,"fxmesa: fxTexValidate(...) End (validated=GL_TRUE)\n");
     }
    return;
  }

  minl=ti->minLevel=tObj->BaseLevel;
  maxl=ti->maxLevel=MIN2(tObj->MaxLevel,tObj->Image[0]->MaxLog2);

  
  fxTexGetInfo(tObj->Image[minl]->Width,tObj->Image[minl]->Height,
	       &(FX_largeLodLog2(ti->info)),&(FX_aspectRatioLog2(ti->info)),
	       &(ti->sScale),&(ti->tScale),
	       &(ti->int_sScale),&(ti->int_tScale),	       
	       NULL,NULL);

  if((tObj->MinFilter!=GL_NEAREST) && (tObj->MinFilter!=GL_LINEAR))
    fxTexGetInfo(tObj->Image[maxl]->Width,tObj->Image[maxl]->Height,
		 &(FX_smallLodLog2(ti->info)),NULL,
		 NULL,NULL,
		 NULL,NULL,
		 NULL,NULL);
  else
    FX_smallLodLog2(ti->info)=FX_largeLodLog2(ti->info);

  fxTexGetFormat(tObj->Image[minl]->Format,&(ti->info.format),&(ti->baseLevelInternalFormat));

  ti->validated=GL_TRUE;

  ti->info.data=NULL;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxTexValidate(...) End\n");
  }
}

static void fxPrintUnitsMode( const char *msg, GLuint mode )
{
   fprintf(stderr, 
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   mode,
	   (mode & FX_UM_E0_REPLACE)         ? "E0_REPLACE, " : "",
	   (mode & FX_UM_E0_MODULATE)        ? "E0_MODULATE, " : "",
	   (mode & FX_UM_E0_DECAL)           ? "E0_DECAL, " : "",
	   (mode & FX_UM_E0_BLEND)           ? "E0_BLEND, " : "",
	   (mode & FX_UM_E1_REPLACE)         ? "E1_REPLACE, " : "",
	   (mode & FX_UM_E1_MODULATE)        ? "E1_MODULATE, " : "",
	   (mode & FX_UM_E1_DECAL)           ? "E1_DECAL, " : "",
	   (mode & FX_UM_E1_BLEND)           ? "E1_BLEND, " : "",
	   (mode & FX_UM_E0_ALPHA)           ? "E0_ALPHA, " : "",
	   (mode & FX_UM_E0_LUMINANCE)       ? "E0_LUMINANCE, " : "",
	   (mode & FX_UM_E0_LUMINANCE_ALPHA) ? "E0_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E0_INTENSITY)       ? "E0_INTENSITY, " : "",
	   (mode & FX_UM_E0_RGB)             ? "E0_RGB, " : "",
	   (mode & FX_UM_E0_RGBA)            ? "E0_RGBA, " : "",
	   (mode & FX_UM_E1_ALPHA)           ? "E1_ALPHA, " : "",
	   (mode & FX_UM_E1_LUMINANCE)       ? "E1_LUMINANCE, " : "",
	   (mode & FX_UM_E1_LUMINANCE_ALPHA) ? "E1_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E1_INTENSITY)       ? "E1_INTENSITY, " : "",
	   (mode & FX_UM_E1_RGB)             ? "E1_RGB, " : "",
	   (mode & FX_UM_E1_RGBA)            ? "E1_RGBA, " : "",
	   (mode & FX_UM_COLOR_ITERATED)     ? "COLOR_ITERATED, " : "",
	   (mode & FX_UM_COLOR_CONSTANT)     ? "COLOR_CONSTANT, " : "",
	   (mode & FX_UM_ALPHA_ITERATED)     ? "ALPHA_ITERATED, " : "",
	   (mode & FX_UM_ALPHA_CONSTANT)     ? "ALPHA_CONSTANT, " : "");
}

GLuint fxGetTexSetConfiguration(GLcontext *ctx,
				struct gl_texture_object *tObj0,
				struct gl_texture_object *tObj1)
{
  GLuint unitsmode=0;
  GLuint envmode=0;
  GLuint ifmt=0;

  if((ctx->Light.ShadeModel==GL_SMOOTH) ||
     (ctx->Point.SmoothFlag) ||
     (ctx->Line.SmoothFlag) ||
     (ctx->Polygon.SmoothFlag))
    unitsmode|=FX_UM_ALPHA_ITERATED;
  else
    unitsmode|=FX_UM_ALPHA_CONSTANT;

  if(ctx->Light.ShadeModel==GL_SMOOTH)
    unitsmode|=FX_UM_COLOR_ITERATED;
  else
    unitsmode|=FX_UM_COLOR_CONSTANT;

  if(tObj0) {
    tfxTexInfo *ti0=(tfxTexInfo *)tObj0->DriverData;

    switch(ti0->baseLevelInternalFormat) {
    case GL_ALPHA:
      ifmt|=FX_UM_E0_ALPHA;
      break;
    case GL_LUMINANCE:
      ifmt|=FX_UM_E0_LUMINANCE;
      break;
    case GL_LUMINANCE_ALPHA:
      ifmt|=FX_UM_E0_LUMINANCE_ALPHA;
      break;
    case GL_INTENSITY:
      ifmt|=FX_UM_E0_INTENSITY;
      break;
    case GL_RGB:
      ifmt|=FX_UM_E0_RGB;
      break;
    case GL_RGBA:
      ifmt|=FX_UM_E0_RGBA;
      break;
    }

    switch(ctx->Texture.Unit[0].EnvMode) {
    case GL_DECAL:
      envmode|=FX_UM_E0_DECAL;
      break;
    case GL_MODULATE:
      envmode|=FX_UM_E0_MODULATE;
      break;
    case GL_REPLACE:
      envmode|=FX_UM_E0_REPLACE;
      break;
    case GL_BLEND:
      envmode|=FX_UM_E0_BLEND;
      break;
    case GL_ADD:
      envmode|=FX_UM_E0_ADD;
      break;
    default:
      /* do nothing */
      break;
    }
  }

  if(tObj1) {
    tfxTexInfo *ti1=(tfxTexInfo *)tObj1->DriverData;

    switch(ti1->baseLevelInternalFormat) {
    case GL_ALPHA:
      ifmt|=FX_UM_E1_ALPHA;
      break;
    case GL_LUMINANCE:
      ifmt|=FX_UM_E1_LUMINANCE;
      break;
    case GL_LUMINANCE_ALPHA:
      ifmt|=FX_UM_E1_LUMINANCE_ALPHA;
      break;
    case GL_INTENSITY:
      ifmt|=FX_UM_E1_INTENSITY;
      break;
    case GL_RGB:
      ifmt|=FX_UM_E1_RGB;
      break;
    case GL_RGBA:
      ifmt|=FX_UM_E1_RGBA;
      break;
    default:
      /* do nothing */
      break;
    }

    switch(ctx->Texture.Unit[1].EnvMode) {
    case GL_DECAL:
      envmode|=FX_UM_E1_DECAL;
      break;
    case GL_MODULATE:
      envmode|=FX_UM_E1_MODULATE;
      break;
    case GL_REPLACE:
      envmode|=FX_UM_E1_REPLACE;
      break;
    case GL_BLEND:
      envmode|=FX_UM_E1_BLEND;
      break;
    case GL_ADD:
      envmode|=FX_UM_E1_ADD;
      break;
    default:
      /* do nothing */
      break;
    }
  }

  unitsmode|=(ifmt | envmode);

  if (MESA_VERBOSE & (VERBOSE_DRIVER|VERBOSE_TEXTURE))
     fxPrintUnitsMode("unitsmode", unitsmode);

  return unitsmode;
}

/************************************************************************/
/************************* Rendering Mode SetUp *************************/
/************************************************************************/

/************************* Single Texture Set ***************************/

static void fxSetupSingleTMU(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;

  if(!ti->tmi.isInTM) {
    if(ti->LODblend)
      fxTMMoveInTM(fxMesa,tObj,FX_TMU_SPLIT);
    else {
      if(fxMesa->haveTwoTMUs) {
	if(fxMesa->freeTexMem[FX_TMU0]>grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&(ti->info)))
	  fxTMMoveInTM(fxMesa,tObj,FX_TMU0);
	else
	  fxTMMoveInTM(fxMesa,tObj,FX_TMU1);
      } else
	fxTMMoveInTM(fxMesa,tObj,FX_TMU0);
    }
  }

  if(ti->LODblend && ti->tmi.whichTMU == FX_TMU_SPLIT) {
    if((ti->info.format==GR_TEXFMT_P_8) && (!fxMesa->haveGlobalPaletteTexture)) {
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
	  fprintf(stderr,"fxmesa: uploading texture palette\n");
       }
      FX_grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&(ti->palette));
      FX_grTexDownloadTable(GR_TMU1,GR_TEXTABLE_PALETTE,&(ti->palette));
    }

    grTexClampMode(GR_TMU0,ti->sClamp,ti->tClamp);
    grTexClampMode(GR_TMU1,ti->sClamp,ti->tClamp);
    grTexFilterMode(GR_TMU0,ti->minFilt,ti->maxFilt);
    grTexFilterMode(GR_TMU1,ti->minFilt,ti->maxFilt);
    grTexMipMapMode(GR_TMU0,ti->mmMode,ti->LODblend);
    grTexMipMapMode(GR_TMU1,ti->mmMode,ti->LODblend);

    grTexSource(GR_TMU0,ti->tmi.tm[FX_TMU0]->startAddress,
		GR_MIPMAPLEVELMASK_ODD,&(ti->info));
    grTexSource(GR_TMU1,ti->tmi.tm[FX_TMU1]->startAddress,
		GR_MIPMAPLEVELMASK_EVEN,&(ti->info));
  } else {
    if((ti->info.format==GR_TEXFMT_P_8) && (!fxMesa->haveGlobalPaletteTexture)) {
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
	  fprintf(stderr,"fxmesa: uploading texture palette\n");
       }
      FX_grTexDownloadTable(ti->tmi.whichTMU,GR_TEXTABLE_PALETTE,&(ti->palette));
    }

    /* KW: The alternative is to do the download to the other tmu.  If
     * we get to this point, I think it means we are thrashing the
     * texture memory, so perhaps it's not a good idea.  
     */
    if (ti->LODblend && (MESA_VERBOSE&VERBOSE_DRIVER))
       fprintf(stderr, "fxmesa: not blending texture - only on one tmu\n");


    grTexClampMode(ti->tmi.whichTMU,ti->sClamp,ti->tClamp);
    grTexFilterMode(ti->tmi.whichTMU,ti->minFilt,ti->maxFilt);
    grTexMipMapMode(ti->tmi.whichTMU,ti->mmMode,FXFALSE);

    grTexSource(ti->tmi.whichTMU,ti->tmi.tm[ti->tmi.whichTMU]->startAddress,
		GR_MIPMAPLEVELMASK_BOTH,&(ti->info));
  }
}

static void fxSelectSingleTMUSrc(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxSelectSingleTMUSrc(%d,%d)\n",tmu,LODblend);
   }

  if(LODblend) {
    grTexCombine(GR_TMU0,
		 GR_COMBINE_FUNCTION_BLEND,
		 GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
		 GR_COMBINE_FUNCTION_BLEND,
		 GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
		 FXFALSE,FXFALSE);

    grTexCombine(GR_TMU1,
		 GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		 GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		 FXFALSE,FXFALSE);

    fxMesa->tmuSrc=FX_TMU_SPLIT;
  } else {
    if(tmu==FX_TMU0) {
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   FXFALSE,FXFALSE);
      
      fxMesa->tmuSrc=FX_TMU0;
    } else {
      grTexCombine(GR_TMU1,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   FXFALSE,FXFALSE);
    
      /* GR_COMBINE_FUNCTION_SCALE_OTHER doesn't work ?!? */
    
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_BLEND,GR_COMBINE_FACTOR_ONE,
		   GR_COMBINE_FUNCTION_BLEND,GR_COMBINE_FACTOR_ONE,
		   FXFALSE,FXFALSE);
    
      fxMesa->tmuSrc=FX_TMU1;
    }
  }
}

void fxSetupTextureSingleTMU(GLcontext *ctx, GLuint textureset)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GrCombineLocal_t localc,locala;
  GLuint unitsmode;
  GLint ifmt;
  tfxTexInfo *ti;
  struct gl_texture_object *tObj=ctx->Texture.Unit[textureset].CurrentD[2];

  if (MESA_VERBOSE&VERBOSE_DRIVER) 
     fprintf(stderr,"fxmesa: fxSetupTextureSingleTMU(...) Start\n");


  ti=(tfxTexInfo *)tObj->DriverData;

  fxTexValidate(ctx,tObj);

  fxSetupSingleTMU(fxMesa,tObj);

  if(fxMesa->tmuSrc!=ti->tmi.whichTMU)
    fxSelectSingleTMUSrc(fxMesa,ti->tmi.whichTMU,ti->LODblend);

  if(textureset==0 || !fxMesa->haveTwoTMUs)
    unitsmode=fxGetTexSetConfiguration(ctx,tObj,NULL);
  else
    unitsmode=fxGetTexSetConfiguration(ctx,NULL,tObj);

  if(fxMesa->lastUnitsMode==unitsmode)
    return;

  fxMesa->lastUnitsMode=unitsmode;

  fxMesa->stw_hint_state = 0;
  FX_grHints(GR_HINT_STWHINT,0);

  ifmt=ti->baseLevelInternalFormat;

  if(unitsmode & FX_UM_ALPHA_ITERATED)
    locala=GR_COMBINE_LOCAL_ITERATED;
  else
    locala=GR_COMBINE_LOCAL_CONSTANT;

  if(unitsmode & FX_UM_COLOR_ITERATED)
    localc=GR_COMBINE_LOCAL_ITERATED;
  else
    localc=GR_COMBINE_LOCAL_CONSTANT;

  if (MESA_VERBOSE & (VERBOSE_DRIVER|VERBOSE_TEXTURE))
     fprintf(stderr, "fxMesa: fxSetupTextureSingleTMU, envmode is %s\n",
	     gl_lookup_enum_by_nr(ctx->Texture.Unit[textureset].EnvMode));

  switch(ctx->Texture.Unit[textureset].EnvMode) {
  case GL_DECAL:
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
		   GR_COMBINE_FACTOR_NONE,
		   locala,
		   GR_COMBINE_OTHER_NONE,
		   FXFALSE);

    grColorCombine(GR_COMBINE_FUNCTION_BLEND,
		   GR_COMBINE_FACTOR_TEXTURE_ALPHA,
		   localc,
		   GR_COMBINE_OTHER_TEXTURE,
		   FXFALSE);
    break;
  case GL_MODULATE:
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		   GR_COMBINE_FACTOR_LOCAL,
		   locala,
		   GR_COMBINE_OTHER_TEXTURE,
		   FXFALSE);

    if(ifmt==GL_ALPHA)
      grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
		     GR_COMBINE_FACTOR_NONE,
		     localc,
		     GR_COMBINE_OTHER_NONE,
		     FXFALSE);
    else
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_LOCAL,
		     localc,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);
    break;
  case GL_BLEND:
#ifndef FX_SILENT
    fprintf(stderr,"fx Driver: GL_BLEND not yet supported\n");
#endif
    /* TO DO (I think that the Voodoo Graphics isn't able to support GL_BLEND) */
    break;
  case GL_REPLACE:
    if((ifmt==GL_RGB) || (ifmt==GL_LUMINANCE))
      grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
		     GR_COMBINE_FACTOR_NONE,
		     locala,
		     GR_COMBINE_OTHER_NONE,
		     FXFALSE);
    else
      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_ONE,
		     locala,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);
    
    if(ifmt==GL_ALPHA)
      grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
		     GR_COMBINE_FACTOR_NONE,
		     localc,
		     GR_COMBINE_OTHER_NONE,
		     FXFALSE);
    else
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_ONE,
		     localc,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);
    break;
  default:
#ifndef FX_SILENT
    fprintf(stderr,"fx Driver: %x Texture.EnvMode not yet supported\n",ctx->Texture.Unit[textureset].EnvMode);
#endif
    break;
  }

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupTextureSingleTMU(...) End\n");
  }
}

/************************* Double Texture Set ***************************/

void fxSetupDoubleTMU(fxMesaContext fxMesa, struct gl_texture_object *tObj0,
		      struct gl_texture_object *tObj1)
{
#define T0_NOT_IN_TMU  0x01
#define T1_NOT_IN_TMU  0x02
#define T0_IN_TMU0     0x04
#define T1_IN_TMU0     0x08
#define T0_IN_TMU1     0x10
#define T1_IN_TMU1     0x20

  tfxTexInfo *ti0=(tfxTexInfo *)tObj0->DriverData;
  tfxTexInfo *ti1=(tfxTexInfo *)tObj1->DriverData;
  GLuint tstate=0;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupDoubleTMU(...)\n");
  }

  if(ti0->tmi.isInTM) {
    if(ti0->tmi.whichTMU==FX_TMU0)
      tstate|=T0_IN_TMU0;
    else if(ti0->tmi.whichTMU==FX_TMU1)
      tstate|=T0_IN_TMU1;
    else {
      fxTMMoveOutTM(fxMesa,tObj0);
      tstate|=T0_NOT_IN_TMU;
    }
  } else
    tstate|=T0_NOT_IN_TMU;

  if(ti1->tmi.isInTM) {
    if(ti1->tmi.whichTMU==FX_TMU0)
      tstate|=T1_IN_TMU0;
    else if(ti1->tmi.whichTMU==FX_TMU1)
      tstate|=T1_IN_TMU1;
    else {
      fxTMMoveOutTM(fxMesa,tObj1);
      tstate|=T1_NOT_IN_TMU;
    }
  } else
    tstate|=T1_NOT_IN_TMU;

  ti0->tmi.lastTimeUsed=fxMesa->texBindNumber;
  ti1->tmi.lastTimeUsed=fxMesa->texBindNumber;

  /* Move texture maps in TMUs */ 

  switch(tstate) {
  case (T0_IN_TMU0 | T1_IN_TMU0):
    fxTMMoveOutTM(fxMesa,tObj1);

    fxTMMoveInTM(fxMesa,tObj1,FX_TMU1);
    break;

  case (T0_IN_TMU1 | T1_IN_TMU1):
    fxTMMoveOutTM(fxMesa,tObj0);

    fxTMMoveInTM(fxMesa,tObj0,FX_TMU0);
    break;

  case (T0_NOT_IN_TMU | T1_NOT_IN_TMU):
    fxTMMoveInTM(fxMesa,tObj0,FX_TMU0);
    fxTMMoveInTM(fxMesa,tObj1,FX_TMU1);
    break;

    /*** T0/T1 ***/

  case (T0_NOT_IN_TMU | T1_IN_TMU0):
    fxTMMoveInTM(fxMesa,tObj0,FX_TMU1);
    break;

  case (T0_NOT_IN_TMU | T1_IN_TMU1):
    fxTMMoveInTM(fxMesa,tObj0,FX_TMU0);
    break;

  case (T0_IN_TMU0 | T1_NOT_IN_TMU):
    fxTMMoveInTM(fxMesa,tObj1,FX_TMU1);
    break;

  case (T0_IN_TMU1 | T1_NOT_IN_TMU):
    fxTMMoveInTM(fxMesa,tObj1,FX_TMU0);
    break;

    /*** Best Case ***/

  case (T0_IN_TMU1 | T1_IN_TMU0):
  case (T0_IN_TMU0 | T1_IN_TMU1):
    break;

  default:
    fprintf(stderr,"fx Driver: internal error in fxSetupDoubleTMU()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }

  if(!fxMesa->haveGlobalPaletteTexture) {
    if(ti0->info.format==GR_TEXFMT_P_8) {
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
	  fprintf(stderr,"fxmesa: uploading texture palette TMU0\n");
       }
      FX_grTexDownloadTable(ti0->tmi.whichTMU,GR_TEXTABLE_PALETTE,&(ti0->palette));
    }

    if(ti1->info.format==GR_TEXFMT_P_8) {
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
	  fprintf(stderr,"fxmesa: uploading texture palette TMU1\n");
       }
      FX_grTexDownloadTable(ti1->tmi.whichTMU,GR_TEXTABLE_PALETTE,&(ti1->palette));
    }
  }

  grTexClampMode(ti0->tmi.whichTMU,ti0->sClamp,ti0->tClamp);
  grTexFilterMode(ti0->tmi.whichTMU,ti0->minFilt,ti0->maxFilt);
  grTexMipMapMode(ti0->tmi.whichTMU,ti0->mmMode,FXFALSE);
  grTexSource(ti0->tmi.whichTMU,ti0->tmi.tm[ti0->tmi.whichTMU]->startAddress,
	      GR_MIPMAPLEVELMASK_BOTH,&(ti0->info));

  grTexClampMode(ti1->tmi.whichTMU,ti1->sClamp,ti1->tClamp);
  grTexFilterMode(ti1->tmi.whichTMU,ti1->minFilt,ti1->maxFilt);
  grTexMipMapMode(ti1->tmi.whichTMU,ti1->mmMode,FXFALSE);
  grTexSource(ti1->tmi.whichTMU,ti1->tmi.tm[ti1->tmi.whichTMU]->startAddress,
	      GR_MIPMAPLEVELMASK_BOTH,&(ti1->info));

#undef T0_NOT_IN_TMU
#undef T1_NOT_IN_TMU
#undef T0_IN_TMU0
#undef T1_IN_TMU0
#undef T0_IN_TMU1
#undef T1_IN_TMU1
}

static void fxSetupTextureDoubleTMU(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GrCombineLocal_t localc,locala;
  tfxTexInfo *ti0,*ti1;
  struct gl_texture_object *tObj0=ctx->Texture.Unit[0].CurrentD[2];
  struct gl_texture_object *tObj1=ctx->Texture.Unit[1].CurrentD[2];
  GLuint envmode,ifmt,unitsmode;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupTextureDoubleTMU(...) Start\n");
  }

  ti0=(tfxTexInfo *)tObj0->DriverData;
  fxTexValidate(ctx,tObj0);

  ti1=(tfxTexInfo *)tObj1->DriverData;
  fxTexValidate(ctx,tObj1);

  fxSetupDoubleTMU(fxMesa,tObj0,tObj1);

  unitsmode=fxGetTexSetConfiguration(ctx,tObj0,tObj1);

  if(fxMesa->lastUnitsMode==unitsmode)
    return;

  fxMesa->lastUnitsMode=unitsmode;

  fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
  FX_grHints(GR_HINT_STWHINT, fxMesa->stw_hint_state);

  envmode=unitsmode & FX_UM_E_ENVMODE;
  ifmt=unitsmode & FX_UM_E_IFMT;

  if(unitsmode & FX_UM_ALPHA_ITERATED)
    locala=GR_COMBINE_LOCAL_ITERATED;
  else
    locala=GR_COMBINE_LOCAL_CONSTANT;

  if(unitsmode & FX_UM_COLOR_ITERATED)
    localc=GR_COMBINE_LOCAL_ITERATED;
  else
    localc=GR_COMBINE_LOCAL_CONSTANT;


  if (MESA_VERBOSE & (VERBOSE_DRIVER|VERBOSE_TEXTURE))
     fprintf(stderr, "fxMesa: fxSetupTextureDoubleTMU, envmode is %s/%s\n",
	     gl_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	     gl_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


  fxMesa->tmuSrc=FX_TMU_BOTH;
  switch(envmode) {
  case (FX_UM_E0_MODULATE | FX_UM_E1_MODULATE):
    {
      GLboolean isalpha[FX_NUM_TMU];

      if(ti0->baseLevelInternalFormat==GL_ALPHA)
	isalpha[ti0->tmi.whichTMU]=GL_TRUE;
      else
	isalpha[ti0->tmi.whichTMU]=GL_FALSE;

      if(ti1->baseLevelInternalFormat==GL_ALPHA)
	isalpha[ti1->tmi.whichTMU]=GL_TRUE;
      else
	isalpha[ti1->tmi.whichTMU]=GL_FALSE;
	
      if(isalpha[FX_TMU1])
	grTexCombine(GR_TMU1,
		     GR_COMBINE_FUNCTION_ZERO,GR_COMBINE_FACTOR_NONE,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     FXTRUE,FXFALSE);
      else
	grTexCombine(GR_TMU1,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     FXFALSE,FXFALSE);

      if(isalpha[FX_TMU0])
	grTexCombine(GR_TMU0,
		     GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_ONE,
		     GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		     FXFALSE,FXFALSE);
      else
	grTexCombine(GR_TMU0,
		     GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		     GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		     FXFALSE,FXFALSE);

      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_LOCAL,
		     localc,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);

      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_LOCAL,
		     locala,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);
      break;
    }
  case (FX_UM_E0_REPLACE | FX_UM_E1_BLEND): /* Only for GLQuake */
    if(ti1->tmi.whichTMU==FX_TMU1) {
      grTexCombine(GR_TMU1,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   FXTRUE,FXFALSE);
		  
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		   FXFALSE,FXFALSE);
    } else {
      grTexCombine(GR_TMU1,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   FXFALSE,FXFALSE);
		  
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
		   FXFALSE,FXFALSE);
    }
	  
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
		   GR_COMBINE_FACTOR_NONE,
		   locala,
		   GR_COMBINE_OTHER_NONE,
		   FXFALSE);

    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		   GR_COMBINE_FACTOR_ONE,
		   localc,
		   GR_COMBINE_OTHER_TEXTURE,
		   FXFALSE);
    break;
  case (FX_UM_E0_REPLACE | FX_UM_E1_MODULATE): /* Quake 2 and 3 */
    if(ti1->tmi.whichTMU==FX_TMU1) {
      grTexCombine(GR_TMU1,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_ZERO,GR_COMBINE_FACTOR_NONE,
		   FXFALSE,FXTRUE);
		  
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		   FXFALSE,FXFALSE);

    } else {
      grTexCombine(GR_TMU1,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		   FXFALSE,FXFALSE);
		  
      grTexCombine(GR_TMU0,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_LOCAL,
		   GR_COMBINE_FUNCTION_BLEND_OTHER,GR_COMBINE_FACTOR_ONE,
		   FXFALSE,FXFALSE);
    }
	  
    if(ti0->baseLevelInternalFormat==GL_RGB)
      grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
		     GR_COMBINE_FACTOR_NONE,
		     locala,
		     GR_COMBINE_OTHER_NONE,
		     FXFALSE);
    else
      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_ONE,
		     locala,
		     GR_COMBINE_OTHER_NONE,
		     FXFALSE);


    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		   GR_COMBINE_FACTOR_ONE,
		   localc,
		   GR_COMBINE_OTHER_TEXTURE,
		   FXFALSE);
    break;
  

  case (FX_UM_E0_MODULATE | FX_UM_E1_ADD): /* Quake 3 Sky */
    {
      GLboolean isalpha[FX_NUM_TMU];

      if(ti0->baseLevelInternalFormat==GL_ALPHA)
	isalpha[ti0->tmi.whichTMU]=GL_TRUE;
      else
	isalpha[ti0->tmi.whichTMU]=GL_FALSE;

      if(ti1->baseLevelInternalFormat==GL_ALPHA)
	isalpha[ti1->tmi.whichTMU]=GL_TRUE;
      else
	isalpha[ti1->tmi.whichTMU]=GL_FALSE;
	
      if(isalpha[FX_TMU1])
	grTexCombine(GR_TMU1,
		     GR_COMBINE_FUNCTION_ZERO,GR_COMBINE_FACTOR_NONE,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     FXTRUE,FXFALSE);
      else
	grTexCombine(GR_TMU1,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
		     FXFALSE,FXFALSE);

      if(isalpha[FX_TMU0])
	grTexCombine(GR_TMU0,
		     GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,
		     GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,GR_COMBINE_FACTOR_ONE,
		     FXFALSE,FXFALSE);
      else
	grTexCombine(GR_TMU0,
		     GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,GR_COMBINE_FACTOR_ONE,
		     GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,GR_COMBINE_FACTOR_ONE,
		     FXFALSE,FXFALSE);

      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_LOCAL,
		     localc,
		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);

      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
		     GR_COMBINE_FACTOR_LOCAL,
		     locala,		     GR_COMBINE_OTHER_TEXTURE,
		     FXFALSE);
      break;
    }
    
  }

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupTextureDoubleTMU(...) End\n");
  }
}

/************************* No Texture ***************************/

static void fxSetupTextureNone(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GrCombineLocal_t localc,locala;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupTextureNone(...)\n");
  }

  if((ctx->Light.ShadeModel==GL_SMOOTH) ||
     (ctx->Point.SmoothFlag) ||
     (ctx->Line.SmoothFlag) ||
     (ctx->Polygon.SmoothFlag))
    locala=GR_COMBINE_LOCAL_ITERATED;
  else
    locala=GR_COMBINE_LOCAL_CONSTANT;
  
  if(ctx->Light.ShadeModel==GL_SMOOTH)
    localc=GR_COMBINE_LOCAL_ITERATED;
  else
    localc=GR_COMBINE_LOCAL_CONSTANT;
  
  grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
		 GR_COMBINE_FACTOR_NONE,
		 locala,
		 GR_COMBINE_OTHER_NONE,
		 FXFALSE);

  grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
		 GR_COMBINE_FACTOR_NONE,
		 localc,
		 GR_COMBINE_OTHER_NONE,
		 FXFALSE);

  fxMesa->lastUnitsMode=FX_UM_NONE;
}

/* See below.
 */
static GLboolean fxMultipassTexture( struct vertex_buffer *, GLuint );



/************************************************************************/
/************************** Texture Mode SetUp **************************/
/************************************************************************/

void fxSetupTexture(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint tex2Denabled;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxSetupTexture(...)\n");
  }

  /* Disable multipass texturing.
   */
  ctx->Driver.MultipassFunc = 0;

  /* Texture Combine, Color Combine and Alpha Combine.
   */  
  tex2Denabled = (ctx->Texture.ReallyEnabled & TEXTURE0_2D);

  if (fxMesa->emulateTwoTMUs)
     tex2Denabled |= (ctx->Texture.ReallyEnabled & TEXTURE1_2D);
  
  switch(tex2Denabled) {
  case TEXTURE0_2D:
    fxSetupTextureSingleTMU(ctx,0);    
    break;
  case TEXTURE1_2D:
    fxSetupTextureSingleTMU(ctx,1);
    break;
  case (TEXTURE0_2D|TEXTURE1_2D):
     if (fxMesa->haveTwoTMUs)
	fxSetupTextureDoubleTMU(ctx);
     else {
	if (MESA_VERBOSE&VERBOSE_DRIVER)
	   fprintf(stderr, "fxmesa: enabling fake multitexture\n");

	fxSetupTextureSingleTMU(ctx,0);
	ctx->Driver.MultipassFunc = fxMultipassTexture;
     }
    break;
  default:
    fxSetupTextureNone(ctx);
    break;
  }
}

/************************************************************************/
/**************************** Blend SetUp *******************************/
/************************************************************************/

/* XXX consider supporting GL_INGR_blend_func_separate */
void fxDDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;
  GrAlphaBlendFnc_t sfact,dfact,asfact,adfact;

  /* From the Glide documentation:
     For alpha source and destination blend function factor
     parameters, Voodoo Graphics supports only
     GR_BLEND_ZERO and GR_BLEND_ONE.
  */

  switch(sfactor) {
  case GL_ZERO:
    asfact=sfact=GR_BLEND_ZERO;
    break;
  case GL_ONE:
    asfact=sfact=GR_BLEND_ONE;
    break;
  case GL_DST_COLOR:
    sfact=GR_BLEND_DST_COLOR;
    asfact=GR_BLEND_ONE;
    break;
  case GL_ONE_MINUS_DST_COLOR:
    sfact=GR_BLEND_ONE_MINUS_DST_COLOR;
    asfact=GR_BLEND_ONE;
    break;
  case GL_SRC_ALPHA:
    sfact=GR_BLEND_SRC_ALPHA;
    asfact=GR_BLEND_ONE;
    break;
  case GL_ONE_MINUS_SRC_ALPHA:
    sfact=GR_BLEND_ONE_MINUS_SRC_ALPHA;
    asfact=GR_BLEND_ONE;
    break;
  case GL_DST_ALPHA:
    sfact=GR_BLEND_DST_ALPHA;
    asfact=GR_BLEND_ONE;
    break;
  case GL_ONE_MINUS_DST_ALPHA:
    sfact=GR_BLEND_ONE_MINUS_DST_ALPHA;
    asfact=GR_BLEND_ONE;
    break;
  case GL_SRC_ALPHA_SATURATE:
    sfact=GR_BLEND_ALPHA_SATURATE;
    asfact=GR_BLEND_ONE;
    break;
  case GL_SRC_COLOR:
  case GL_ONE_MINUS_SRC_COLOR:
    /* USELESS */
    asfact=sfact=GR_BLEND_ONE;
    break;
  default:
    asfact=sfact=GR_BLEND_ONE;
    break;
  }

  if((sfact!=us->blendSrcFuncRGB) ||
     (asfact!=us->blendSrcFuncAlpha)) {
    us->blendSrcFuncRGB=sfact;
    us->blendSrcFuncAlpha=asfact;
    fxMesa->new_state |= FX_NEW_BLEND;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }

  switch(dfactor) {
  case GL_ZERO:
    adfact=dfact=GR_BLEND_ZERO;
    break;
  case GL_ONE:
    adfact=dfact=GR_BLEND_ONE;
    break;
  case GL_SRC_COLOR:
    dfact=GR_BLEND_SRC_COLOR;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_ONE_MINUS_SRC_COLOR:
    dfact=GR_BLEND_ONE_MINUS_SRC_COLOR;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_SRC_ALPHA:
    dfact=GR_BLEND_SRC_ALPHA;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_ONE_MINUS_SRC_ALPHA:
    dfact=GR_BLEND_ONE_MINUS_SRC_ALPHA;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_DST_ALPHA:
    dfact=GR_BLEND_DST_ALPHA;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_ONE_MINUS_DST_ALPHA:
    dfact=GR_BLEND_ONE_MINUS_DST_ALPHA;
    adfact=GR_BLEND_ZERO;
    break;
  case GL_SRC_ALPHA_SATURATE:
  case GL_DST_COLOR:
  case GL_ONE_MINUS_DST_COLOR:
    /* USELESS */
    adfact=dfact=GR_BLEND_ZERO;
    break;
  default:
    adfact=dfact=GR_BLEND_ZERO;
    break;
  }

  if((dfact!=us->blendDstFuncRGB) ||
     (adfact!=us->blendDstFuncAlpha)) {
    us->blendDstFuncRGB=dfact;
    us->blendDstFuncAlpha=adfact;
    fxMesa->new_state |= FX_NEW_BLEND;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }
}

void fxSetupBlend(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;

  if(us->blendEnabled)
     grAlphaBlendFunction(us->blendSrcFuncRGB,us->blendDstFuncRGB,
			  us->blendSrcFuncAlpha,us->blendDstFuncAlpha);
  else
     grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
  
/************************************************************************/
/************************** Alpha Test SetUp ****************************/
/************************************************************************/

void fxDDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;
  GrCmpFnc_t newfunc;

  switch(func) {
  case GL_NEVER:
    newfunc=GR_CMP_NEVER;
    break;
  case GL_LESS:
    newfunc=GR_CMP_LESS;
    break;
  case GL_EQUAL:
    newfunc=GR_CMP_EQUAL;
    break;
  case GL_LEQUAL:
    newfunc=GR_CMP_LEQUAL;
    break;
  case GL_GREATER:
    newfunc=GR_CMP_GREATER;
    break;
  case GL_NOTEQUAL:
    newfunc=GR_CMP_NOTEQUAL;
    break;
  case GL_GEQUAL:
    newfunc=GR_CMP_GEQUAL;
    break;
  case GL_ALWAYS:
    newfunc=GR_CMP_ALWAYS;
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxDDAlphaFunc()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }

  if(newfunc!=us->alphaTestFunc) {
    us->alphaTestFunc=newfunc;
    fxMesa->new_state |= FX_NEW_ALPHA;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }

  if(ctx->Color.AlphaRef!=us->alphaTestRefValue) {
    us->alphaTestRefValue=ctx->Color.AlphaRef;
    fxMesa->new_state |= FX_NEW_ALPHA;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }
}

static void fxSetupAlphaTest(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;

  if(us->alphaTestEnabled) {
     grAlphaTestFunction(us->alphaTestFunc);
     grAlphaTestReferenceValue(us->alphaTestRefValue);
  } else
     grAlphaTestFunction(GR_CMP_ALWAYS);
}

/************************************************************************/
/************************** Depth Test SetUp ****************************/
/************************************************************************/

void fxDDDepthFunc(GLcontext *ctx, GLenum func)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;
  GrCmpFnc_t dfunc;

  switch(func) {
  case GL_NEVER:
    dfunc=GR_CMP_NEVER;
    break;
  case GL_LESS:
    dfunc=GR_CMP_LESS;
    break;
  case GL_GEQUAL:
    dfunc=GR_CMP_GEQUAL;
    break;
  case GL_LEQUAL:
    dfunc=GR_CMP_LEQUAL;
    break;
  case GL_GREATER:
    dfunc=GR_CMP_GREATER;
    break;
  case GL_NOTEQUAL:
    dfunc=GR_CMP_NOTEQUAL;
    break;
  case GL_EQUAL:
    dfunc=GR_CMP_EQUAL;
    break;
  case GL_ALWAYS:
    dfunc=GR_CMP_ALWAYS;
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxDDDepthFunc()\n");
    fxCloseHardware();
    exit(-1);
    break;
  }

  if(dfunc!=us->depthTestFunc) {
    us->depthTestFunc=dfunc;
    fxMesa->new_state |= FX_NEW_DEPTH;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }

}

void fxDDDepthMask(GLcontext *ctx, GLboolean flag)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;

  if(flag!=us->depthMask) {
    us->depthMask=flag;
    fxMesa->new_state |= FX_NEW_DEPTH;
    ctx->Driver.RenderStart = fxSetupFXUnits;
  }
}

void fxSetupDepthTest(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;

  if(us->depthTestEnabled)
     grDepthBufferFunction(us->depthTestFunc);
  else
     grDepthBufferFunction(GR_CMP_ALWAYS);

  grDepthMask(us->depthMask);
}

/************************************************************************/
/**************************** Color Mask SetUp **************************/
/************************************************************************/

GLboolean fxDDColorMask(GLcontext *ctx, 
			GLboolean r, GLboolean g, 
			GLboolean b, GLboolean a )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  fxMesa->new_state |= FX_NEW_COLOR_MASK;
  ctx->Driver.RenderStart = fxSetupFXUnits;
  (void) r; (void) g; (void) b; (void) a;
  return 1;
}

static void fxSetupColorMask(GLcontext *ctx)
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);

  grColorMask(ctx->Color.ColorMask[RCOMP] ||
	      ctx->Color.ColorMask[GCOMP] ||
	      ctx->Color.ColorMask[BCOMP],
	      ctx->Color.ColorMask[ACOMP] && fxMesa->haveAlphaBuffer);
}



/************************************************************************/
/**************************** Fog Mode SetUp ****************************/
/************************************************************************/

void fxFogTableGenerate(GLcontext *ctx)
{
  int i;
  float f,eyez;
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  for(i=0;i<FX_grGetInteger(FX_FOG_TABLE_ENTRIES);i++) {
    eyez=guFogTableIndexToW(i);

    switch(ctx->Fog.Mode) {
    case GL_LINEAR:
      f=(ctx->Fog.End-eyez)/(ctx->Fog.End-ctx->Fog.Start);
      break;
    case GL_EXP:
      f=exp(-ctx->Fog.Density*eyez);  
      break;
    case GL_EXP2:
      f=exp(-ctx->Fog.Density*ctx->Fog.Density*eyez*eyez);
      break;
    default: /* That should never happen */
      f=0.0f;
      break; 
    }

    fxMesa->fogTable[i]=(GrFog_t)((1.0f-CLAMP(f,0.0f,1.0f))*255.0f);
  }
}

void fxSetupFog(GLcontext *ctx, GLboolean forceTableRebuild)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if(ctx->Fog.Enabled && ctx->FogMode==FOG_FRAGMENT) {
    GLubyte col[4];
    grFogMode(GR_FOG_WITH_TABLE);

    col[0]=(unsigned int)(255*ctx->Fog.Color[0]);
    col[1]=(unsigned int)(255*ctx->Fog.Color[1]);
    col[2]=(unsigned int)(255*ctx->Fog.Color[2]); 
    col[3]=(unsigned int)(255*ctx->Fog.Color[3]);

    grFogColorValue(FXCOLOR4(col));

    if(forceTableRebuild ||
       (fxMesa->fogTableMode!=ctx->Fog.Mode) ||
       (fxMesa->fogDensity!=ctx->Fog.Density)) {
      fxFogTableGenerate(ctx);
         
      fxMesa->fogTableMode=ctx->Fog.Mode;
      fxMesa->fogDensity=ctx->Fog.Density;
    }
      
    grFogTable(fxMesa->fogTable);
  } else
    grFogMode(GR_FOG_DISABLE);
}

void fxDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_FOG;
   ctx->Driver.RenderStart = fxSetupFXUnits;
}

/************************************************************************/
/************************** Scissor Test SetUp **************************/
/************************************************************************/

static void fxSetupScissor(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if (ctx->Scissor.Enabled) {
    int ymin, ymax;

    ymin=ctx->Scissor.Y;
    ymax=ctx->Scissor.Y+ctx->Scissor.Height;

    if (ymin<0) ymin=0;

    if (ymax>fxMesa->height) ymax=fxMesa->height;

    grClipWindow(ctx->Scissor.X, 
 		 ymin,
 		 ctx->Scissor.X+ctx->Scissor.Width, 
 		 ymax);
  } else
    grClipWindow(0,0,fxMesa->width,fxMesa->height);
}

void fxDDScissor( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_SCISSOR;
   ctx->Driver.RenderStart = fxSetupFXUnits;
}

/************************************************************************/
/*************************** Cull mode setup ****************************/
/************************************************************************/


void fxDDCullFace(GLcontext *ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
   ctx->Driver.RenderStart = fxSetupFXUnits;   
}

void fxDDFrontFace(GLcontext *ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
   ctx->Driver.RenderStart = fxSetupFXUnits;   
}


void fxSetupCull(GLcontext *ctx)
{
   if(ctx->Polygon.CullFlag) {
      switch(ctx->Polygon.CullFaceMode) {
      case GL_BACK:
	 if(ctx->Polygon.FrontFace==GL_CCW)
	    grCullMode(GR_CULL_NEGATIVE);
	 else
	    grCullMode(GR_CULL_POSITIVE);
	 break;
      case GL_FRONT:
	 if(ctx->Polygon.FrontFace==GL_CCW)
	    grCullMode(GR_CULL_POSITIVE);
	 else
	    grCullMode(GR_CULL_NEGATIVE);
	 break;
      case GL_FRONT_AND_BACK:
	 grCullMode(GR_CULL_DISABLE);
	 break;
      default:
	 break;
      }
   } else
      grCullMode(GR_CULL_DISABLE);
}


/************************************************************************/
/****************************** DD Enable ******************************/
/************************************************************************/

void fxDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  tfxUnitsState *us=&fxMesa->unitsState;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxDDEnable(...)\n");
  }

  switch(cap) {
  case GL_ALPHA_TEST:
    if(state!=us->alphaTestEnabled) {
      us->alphaTestEnabled=state;
      fxMesa->new_state |= FX_NEW_ALPHA;
      ctx->Driver.RenderStart = fxSetupFXUnits;
    }
    break;
  case GL_BLEND:
    if(state!=us->blendEnabled) {
      us->blendEnabled=state;
      fxMesa->new_state |= FX_NEW_BLEND;
      ctx->Driver.RenderStart = fxSetupFXUnits;
    }
    break;
  case GL_DEPTH_TEST:
    if(state!=us->depthTestEnabled) {
      us->depthTestEnabled=state;
      fxMesa->new_state |= FX_NEW_DEPTH;
      ctx->Driver.RenderStart = fxSetupFXUnits;
    }
    break;
  case GL_SCISSOR_TEST:
     fxMesa->new_state |= FX_NEW_SCISSOR;
     ctx->Driver.RenderStart = fxSetupFXUnits;
     break;
  case GL_FOG:
     fxMesa->new_state |= FX_NEW_FOG;
     ctx->Driver.RenderStart = fxSetupFXUnits;
     break;
  case GL_CULL_FACE:
     fxMesa->new_state |= FX_NEW_CULL;
     ctx->Driver.RenderStart = fxSetupFXUnits;
     break;
  case GL_LINE_SMOOTH:
  case GL_POINT_SMOOTH:
  case GL_POLYGON_SMOOTH:
  case GL_TEXTURE_2D:
      fxMesa->new_state |= FX_NEW_TEXTURING;
      ctx->Driver.RenderStart = fxSetupFXUnits;
      break;
  default:
    ;  /* XXX no-op??? */
  }    
}

/************************************************************************/
/******************** Fake Multitexture Support *************************/
/************************************************************************/

/* Its considered cheeky to try to fake ARB multitexture by doing
 * multipass rendering, because it is not possible to emulate the full
 * spec in this way.  The fact is that the voodoo 2 supports only a
 * subset of the possible multitexturing modes, and it is possible to
 * support almost the same subset using multipass blending on the
 * voodoo 1.  In all other cases for both voodoo 1 and 2, we fall back
 * to software rendering, satisfying the spec if not the user.  
 */
static GLboolean fxMultipassTexture( struct vertex_buffer *VB, GLuint pass )
{
   GLcontext *ctx = VB->ctx;
   fxVertex *v = FX_DRIVER_DATA(VB)->verts;
   fxVertex *last = FX_DRIVER_DATA(VB)->last_vert;
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   switch (pass) {
   case 1:
      if (MESA_VERBOSE&(VERBOSE_DRIVER|VERBOSE_PIPELINE|VERBOSE_TEXTURE))
	 fprintf(stderr, "fxmesa: Second texture pass\n");

      for ( ; v != last ; v++) {
	 v->f[S0COORD] = v->f[S1COORD];
	 v->f[T0COORD] = v->f[T1COORD];
      }

      fxMesa->restoreUnitsState = fxMesa->unitsState; 
      fxMesa->tmu_source[0] = 1;

      if (ctx->Depth.Mask) {
	 switch (ctx->Depth.Func) {
	 case GL_NEVER:
	 case GL_ALWAYS:
	    break;
	 default:
	    fxDDDepthFunc( ctx, GL_EQUAL );
	    break;
	 }

	 fxDDDepthMask( ctx, GL_FALSE ); 
      }
      
      if (ctx->Texture.Unit[1].EnvMode == GL_MODULATE) {
	 fxDDEnable( ctx, GL_BLEND, GL_TRUE );
	 fxDDBlendFunc( ctx, GL_DST_COLOR, GL_ZERO );
      }

      fxSetupTextureSingleTMU( ctx, 1 ); 
      fxSetupBlend( ctx );
      fxSetupDepthTest( ctx );
      break;

   case 2:
      /* Restore original state.  
       */
      fxMesa->tmu_source[0] = 0;
      fxMesa->unitsState = fxMesa->restoreUnitsState;
      fxMesa->setupdone &= ~SETUP_TMU0;
      fxSetupTextureSingleTMU( ctx, 0 ); 
      fxSetupBlend( ctx );
      fxSetupDepthTest( ctx );
      break;
   }

   return pass == 1;      
}


/************************************************************************/
/************************** Changes to units state **********************/
/************************************************************************/


/* All units setup is handled under texture setup.
 */
void fxDDShadeModel(GLcontext *ctx, GLenum mode)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_TEXTURING;
   ctx->Driver.RenderStart = fxSetupFXUnits;
}



/************************************************************************/
/****************************** Units SetUp *****************************/
/************************************************************************/
void gl_print_fx_state_flags( const char *msg, GLuint flags )
{
   fprintf(stderr, 
	   "%s: (0x%x) %s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & FX_NEW_TEXTURING)   ? "texture, " : "",
	   (flags & FX_NEW_BLEND)       ? "blend, " : "",
	   (flags & FX_NEW_ALPHA)       ? "alpha, " : "",
	   (flags & FX_NEW_FOG)         ? "fog, " : "",
	   (flags & FX_NEW_SCISSOR)     ? "scissor, " : "",
	   (flags & FX_NEW_COLOR_MASK)  ? "colormask, " : "",
	   (flags & FX_NEW_CULL)        ? "cull, " : "");
}

void fxSetupFXUnits( GLcontext *ctx )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint newstate = fxMesa->new_state;


  if (MESA_VERBOSE&VERBOSE_DRIVER) 
     gl_print_fx_state_flags("fxmesa: fxSetupFXUnits", newstate);


  if (newstate) {
     if (newstate & FX_NEW_TEXTURING)
	fxSetupTexture(ctx);

     if (newstate & FX_NEW_BLEND)
	fxSetupBlend(ctx);

     if (newstate & FX_NEW_ALPHA)
	fxSetupAlphaTest(ctx);
     
     if (newstate & FX_NEW_DEPTH)
	fxSetupDepthTest(ctx);

     if (newstate & FX_NEW_FOG)
	fxSetupFog(ctx,GL_FALSE);

     if (newstate & FX_NEW_SCISSOR)
	fxSetupScissor(ctx);

     if (newstate & FX_NEW_COLOR_MASK)
	fxSetupColorMask(ctx);

     if (newstate & FX_NEW_CULL)
	fxSetupCull(ctx);     

     fxMesa->new_state = 0;
/*       ctx->Driver.RenderStart = 0; */
  }
}



#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_setup(void)
{
  return 0;
}

#endif  /* FX */
