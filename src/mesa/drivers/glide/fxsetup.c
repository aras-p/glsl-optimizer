/* $Id: fxsetup.c,v 1.36 2001/09/23 16:50:01 brianp Exp $ */

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
 */

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "enums.h"
#include "tnl/t_context.h"

static void
fxTexValidate(GLcontext * ctx, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   GLint minl, maxl;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxTexValidate(...) Start\n");
   }

   if (ti->validated) {
      if (MESA_VERBOSE & VERBOSE_DRIVER) {
	 fprintf(stderr,
		 "fxmesa: fxTexValidate(...) End (validated=GL_TRUE)\n");
      }
      return;
   }

   ti->tObj = tObj;
   minl = ti->minLevel = tObj->BaseLevel;
   maxl = ti->maxLevel = MIN2(tObj->MaxLevel, tObj->Image[0]->MaxLog2);

   fxTexGetInfo(tObj->Image[minl]->Width, tObj->Image[minl]->Height,
		&(FX_largeLodLog2(ti->info)), &(FX_aspectRatioLog2(ti->info)),
		&(ti->sScale), &(ti->tScale),
		&(ti->int_sScale), &(ti->int_tScale), NULL, NULL);

   if ((tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR))
      fxTexGetInfo(tObj->Image[maxl]->Width, tObj->Image[maxl]->Height,
		   &(FX_smallLodLog2(ti->info)), NULL,
		   NULL, NULL, NULL, NULL, NULL, NULL);
   else
      FX_smallLodLog2(ti->info) = FX_largeLodLog2(ti->info);

   fxTexGetFormat(tObj->Image[minl]->TexFormat->BaseFormat, &(ti->info.format),
		  &(ti->baseLevelInternalFormat));

   switch (tObj->WrapS) {
   case GL_CLAMP_TO_EDGE:
      /* What's this really mean compared to GL_CLAMP? */
   case GL_CLAMP:
      ti->sClamp = 1;
      break;
   case GL_REPEAT:
      ti->sClamp = 0;
      break;
   default:
      ;				/* silence compiler warning */
   }
   switch (tObj->WrapT) {
   case GL_CLAMP_TO_EDGE:
      /* What's this really mean compared to GL_CLAMP? */
   case GL_CLAMP:
      ti->tClamp = 1;
      break;
   case GL_REPEAT:
      ti->tClamp = 0;
      break;
   default:
      ;				/* silence compiler warning */
   }

   ti->validated = GL_TRUE;

   ti->info.data = NULL;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxTexValidate(...) End\n");
   }
}

static void
fxPrintUnitsMode(const char *msg, GLuint mode)
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   mode,
	   (mode & FX_UM_E0_REPLACE) ? "E0_REPLACE, " : "",
	   (mode & FX_UM_E0_MODULATE) ? "E0_MODULATE, " : "",
	   (mode & FX_UM_E0_DECAL) ? "E0_DECAL, " : "",
	   (mode & FX_UM_E0_BLEND) ? "E0_BLEND, " : "",
	   (mode & FX_UM_E1_REPLACE) ? "E1_REPLACE, " : "",
	   (mode & FX_UM_E1_MODULATE) ? "E1_MODULATE, " : "",
	   (mode & FX_UM_E1_DECAL) ? "E1_DECAL, " : "",
	   (mode & FX_UM_E1_BLEND) ? "E1_BLEND, " : "",
	   (mode & FX_UM_E0_ALPHA) ? "E0_ALPHA, " : "",
	   (mode & FX_UM_E0_LUMINANCE) ? "E0_LUMINANCE, " : "",
	   (mode & FX_UM_E0_LUMINANCE_ALPHA) ? "E0_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E0_INTENSITY) ? "E0_INTENSITY, " : "",
	   (mode & FX_UM_E0_RGB) ? "E0_RGB, " : "",
	   (mode & FX_UM_E0_RGBA) ? "E0_RGBA, " : "",
	   (mode & FX_UM_E1_ALPHA) ? "E1_ALPHA, " : "",
	   (mode & FX_UM_E1_LUMINANCE) ? "E1_LUMINANCE, " : "",
	   (mode & FX_UM_E1_LUMINANCE_ALPHA) ? "E1_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E1_INTENSITY) ? "E1_INTENSITY, " : "",
	   (mode & FX_UM_E1_RGB) ? "E1_RGB, " : "",
	   (mode & FX_UM_E1_RGBA) ? "E1_RGBA, " : "",
	   (mode & FX_UM_COLOR_ITERATED) ? "COLOR_ITERATED, " : "",
	   (mode & FX_UM_COLOR_CONSTANT) ? "COLOR_CONSTANT, " : "",
	   (mode & FX_UM_ALPHA_ITERATED) ? "ALPHA_ITERATED, " : "",
	   (mode & FX_UM_ALPHA_CONSTANT) ? "ALPHA_CONSTANT, " : "");
}

static GLuint
fxGetTexSetConfiguration(GLcontext * ctx,
			 struct gl_texture_object *tObj0,
			 struct gl_texture_object *tObj1)
{
   GLuint unitsmode = 0;
   GLuint envmode = 0;
   GLuint ifmt = 0;

   if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
       (ctx->Point.SmoothFlag) ||
       (ctx->Line.SmoothFlag) ||
       (ctx->Polygon.SmoothFlag)) unitsmode |= FX_UM_ALPHA_ITERATED;
   else
      unitsmode |= FX_UM_ALPHA_CONSTANT;

   if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
      unitsmode |= FX_UM_COLOR_ITERATED;
   else
      unitsmode |= FX_UM_COLOR_CONSTANT;



   /* 
      OpenGL Feeds Texture 0 into Texture 1
      Glide Feeds Texture 1 into Texture 0
    */
   if (tObj0) {
      tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);

      switch (ti0->baseLevelInternalFormat) {
      case GL_ALPHA:
	 ifmt |= FX_UM_E0_ALPHA;
	 break;
      case GL_LUMINANCE:
	 ifmt |= FX_UM_E0_LUMINANCE;
	 break;
      case GL_LUMINANCE_ALPHA:
	 ifmt |= FX_UM_E0_LUMINANCE_ALPHA;
	 break;
      case GL_INTENSITY:
	 ifmt |= FX_UM_E0_INTENSITY;
	 break;
      case GL_RGB:
	 ifmt |= FX_UM_E0_RGB;
	 break;
      case GL_RGBA:
	 ifmt |= FX_UM_E0_RGBA;
	 break;
      }

      switch (ctx->Texture.Unit[0].EnvMode) {
      case GL_DECAL:
	 envmode |= FX_UM_E0_DECAL;
	 break;
      case GL_MODULATE:
	 envmode |= FX_UM_E0_MODULATE;
	 break;
      case GL_REPLACE:
	 envmode |= FX_UM_E0_REPLACE;
	 break;
      case GL_BLEND:
	 envmode |= FX_UM_E0_BLEND;
	 break;
      case GL_ADD:
	 envmode |= FX_UM_E0_ADD;
	 break;
      default:
	 /* do nothing */
	 break;
      }
   }

   if (tObj1) {
      tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);

      switch (ti1->baseLevelInternalFormat) {
      case GL_ALPHA:
	 ifmt |= FX_UM_E1_ALPHA;
	 break;
      case GL_LUMINANCE:
	 ifmt |= FX_UM_E1_LUMINANCE;
	 break;
      case GL_LUMINANCE_ALPHA:
	 ifmt |= FX_UM_E1_LUMINANCE_ALPHA;
	 break;
      case GL_INTENSITY:
	 ifmt |= FX_UM_E1_INTENSITY;
	 break;
      case GL_RGB:
	 ifmt |= FX_UM_E1_RGB;
	 break;
      case GL_RGBA:
	 ifmt |= FX_UM_E1_RGBA;
	 break;
      default:
	 /* do nothing */
	 break;
      }

      switch (ctx->Texture.Unit[1].EnvMode) {
      case GL_DECAL:
	 envmode |= FX_UM_E1_DECAL;
	 break;
      case GL_MODULATE:
	 envmode |= FX_UM_E1_MODULATE;
	 break;
      case GL_REPLACE:
	 envmode |= FX_UM_E1_REPLACE;
	 break;
      case GL_BLEND:
	 envmode |= FX_UM_E1_BLEND;
	 break;
      case GL_ADD:
	 envmode |= FX_UM_E1_ADD;
	 break;
      default:
	 /* do nothing */
	 break;
      }
   }

   unitsmode |= (ifmt | envmode);

   if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fxPrintUnitsMode("unitsmode", unitsmode);

   return unitsmode;
}

/************************************************************************/
/************************* Rendering Mode SetUp *************************/
/************************************************************************/

/************************* Single Texture Set ***************************/

static void
fxSetupSingleTMU_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   int tmu;

   /* Make sure we're not loaded incorrectly */
   if (ti->isInTM) {
      if (ti->LODblend) {
	 if (ti->whichTMU != FX_TMU_SPLIT)
	    fxTMMoveOutTM(fxMesa, tObj);
      }
      else {
	 if (ti->whichTMU == FX_TMU_SPLIT)
	    fxTMMoveOutTM(fxMesa, tObj);
      }
   }

   /* Make sure we're loaded correctly */
   if (!ti->isInTM) {
      if (ti->LODblend)
	 fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU_SPLIT);
      else {
	 if (fxMesa->haveTwoTMUs) {
	    if (fxMesa->freeTexMem[FX_TMU0] >
		FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_BOTH,
						  &(ti->info))) {
	       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU1);
	    }
	 }
	 else
	    fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
      }
   }

   if (ti->LODblend && ti->whichTMU == FX_TMU_SPLIT) {
      if ((ti->info.format == GR_TEXFMT_P_8)
	  && (!fxMesa->haveGlobalPaletteTexture)) {
	 if (MESA_VERBOSE & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxmesa: uploading texture palette\n");
	 }
	 FX_grTexDownloadTable_NoLock(GR_TMU0, GR_TEXTABLE_PALETTE,
				      &(ti->palette));
	 FX_grTexDownloadTable_NoLock(GR_TMU1, GR_TEXTABLE_PALETTE,
				      &(ti->palette));
      }

      FX_grTexClampMode_NoLock(GR_TMU0, ti->sClamp, ti->tClamp);
      FX_grTexClampMode_NoLock(GR_TMU1, ti->sClamp, ti->tClamp);
      FX_grTexFilterMode_NoLock(GR_TMU0, ti->minFilt, ti->maxFilt);
      FX_grTexFilterMode_NoLock(GR_TMU1, ti->minFilt, ti->maxFilt);
      FX_grTexMipMapMode_NoLock(GR_TMU0, ti->mmMode, ti->LODblend);
      FX_grTexMipMapMode_NoLock(GR_TMU1, ti->mmMode, ti->LODblend);

      FX_grTexSource_NoLock(GR_TMU0, ti->tm[FX_TMU0]->startAddr,
			    GR_MIPMAPLEVELMASK_ODD, &(ti->info));
      FX_grTexSource_NoLock(GR_TMU1, ti->tm[FX_TMU1]->startAddr,
			    GR_MIPMAPLEVELMASK_EVEN, &(ti->info));
   }
   else {
      if (ti->whichTMU == FX_TMU_BOTH)
	 tmu = FX_TMU0;
      else
	 tmu = ti->whichTMU;

      if ((ti->info.format == GR_TEXFMT_P_8)
	  && (!fxMesa->haveGlobalPaletteTexture)) {
	 if (MESA_VERBOSE & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxmesa: uploading texture palette\n");
	 }
	 FX_grTexDownloadTable_NoLock(tmu, GR_TEXTABLE_PALETTE,
				      &(ti->palette));
      }

      /* KW: The alternative is to do the download to the other tmu.  If
       * we get to this point, I think it means we are thrashing the
       * texture memory, so perhaps it's not a good idea.  
       */
      if (ti->LODblend && (MESA_VERBOSE & VERBOSE_DRIVER))
	 fprintf(stderr, "fxmesa: not blending texture - only on one tmu\n");

      FX_grTexClampMode_NoLock(tmu, ti->sClamp, ti->tClamp);
      FX_grTexFilterMode_NoLock(tmu, ti->minFilt, ti->maxFilt);
      FX_grTexMipMapMode_NoLock(tmu, ti->mmMode, FXFALSE);

      FX_grTexSource_NoLock(tmu, ti->tm[tmu]->startAddr,
			    GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
   }
}

static void
fxSelectSingleTMUSrc_NoLock(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSelectSingleTMUSrc(%d,%d)\n", tmu, LODblend);
   }

   if (LODblend) {
      FX_grTexCombine_NoLock(GR_TMU0,
			     GR_COMBINE_FUNCTION_BLEND,
			     GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
			     GR_COMBINE_FUNCTION_BLEND,
			     GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
			     FXFALSE, FXFALSE);

      if (fxMesa->haveTwoTMUs)
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
      fxMesa->tmuSrc = FX_TMU_SPLIT;
   }
   else {
      if (tmu != FX_TMU1) {
	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
	 if (fxMesa->haveTwoTMUs) {
	    FX_grTexCombine_NoLock(GR_TMU1,
				   GR_COMBINE_FUNCTION_ZERO,
				   GR_COMBINE_FACTOR_NONE,
				   GR_COMBINE_FUNCTION_ZERO,
				   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
	 }
	 fxMesa->tmuSrc = FX_TMU0;
      }
      else {
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	 /* GR_COMBINE_FUNCTION_SCALE_OTHER doesn't work ?!? */

	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_BLEND,
				GR_COMBINE_FACTOR_ONE,
				GR_COMBINE_FUNCTION_BLEND,
				GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);

	 fxMesa->tmuSrc = FX_TMU1;
      }
   }
}

static void
fxSetupTextureSingleTMU_NoLock(GLcontext * ctx, GLuint textureset)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GrCombineLocal_t localc, locala;
   GLuint unitsmode;
   GLint ifmt;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = ctx->Texture.Unit[textureset].Current2D;
   int tmu;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTextureSingleTMU(...) Start\n");
   }

   ti = fxTMGetTexInfo(tObj);

   fxTexValidate(ctx, tObj);

   fxSetupSingleTMU_NoLock(fxMesa, tObj);

   if (ti->whichTMU == FX_TMU_BOTH)
      tmu = FX_TMU0;
   else
      tmu = ti->whichTMU;
   if (fxMesa->tmuSrc != tmu)
      fxSelectSingleTMUSrc_NoLock(fxMesa, tmu, ti->LODblend);

   if (textureset == 0 || !fxMesa->haveTwoTMUs)
      unitsmode = fxGetTexSetConfiguration(ctx, tObj, NULL);
   else
      unitsmode = fxGetTexSetConfiguration(ctx, NULL, tObj);

/*    if(fxMesa->lastUnitsMode==unitsmode) */
/*      return; */

   fxMesa->lastUnitsMode = unitsmode;

   fxMesa->stw_hint_state = 0;
   FX_grHints_NoLock(GR_HINT_STWHINT, 0);

   ifmt = ti->baseLevelInternalFormat;

   if (unitsmode & FX_UM_ALPHA_ITERATED)
      locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (unitsmode & FX_UM_COLOR_ITERATED)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;

   if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxMesa: fxSetupTextureSingleTMU, envmode is %s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[textureset].EnvMode));

   switch (ctx->Texture.Unit[textureset].EnvMode) {
   case GL_DECAL:
      FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
			       GR_COMBINE_FACTOR_NONE,
			       locala, GR_COMBINE_OTHER_NONE, FXFALSE);

      FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_BLEND,
			       GR_COMBINE_FACTOR_TEXTURE_ALPHA,
			       localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      break;
   case GL_MODULATE:
      FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
			       GR_COMBINE_FACTOR_LOCAL,
			       locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);

      if (ifmt == GL_ALPHA)
	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
				  GR_COMBINE_FACTOR_NONE,
				  localc, GR_COMBINE_OTHER_NONE, FXFALSE);
      else
	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_LOCAL,
				  localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      break;
   case GL_BLEND:
      if (MESA_VERBOSE & VERBOSE_DRIVER)
	 fprintf(stderr, "fx Driver: GL_BLEND not yet supported\n");
      break;
   case GL_REPLACE:
      if ((ifmt == GL_RGB) || (ifmt == GL_LUMINANCE))
	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
				  GR_COMBINE_FACTOR_NONE,
				  locala, GR_COMBINE_OTHER_NONE, FXFALSE);
      else
	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_ONE,
				  locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);

      if (ifmt == GL_ALPHA)
	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
				  GR_COMBINE_FACTOR_NONE,
				  localc, GR_COMBINE_OTHER_NONE, FXFALSE);
      else
	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_ONE,
				  localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      break;
   default:
      if (MESA_VERBOSE & VERBOSE_DRIVER)
	 fprintf(stderr, "fx Driver: %x Texture.EnvMode not yet supported\n",
		 ctx->Texture.Unit[textureset].EnvMode);
      break;
   }

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTextureSingleTMU(...) End\n");
   }
}

#if 00
static void
fxSetupTextureSingleTMU(GLcontext * ctx, GLuint textureset)
{
   BEGIN_BOARD_LOCK();
   fxSetupTextureSingleTMU_NoLock(ctx, textureset);
   END_BOARD_LOCK();
}
#endif


/************************* Double Texture Set ***************************/

static void
fxSetupDoubleTMU_NoLock(fxMesaContext fxMesa,
			struct gl_texture_object *tObj0,
			struct gl_texture_object *tObj1)
{
#define T0_NOT_IN_TMU  0x01
#define T1_NOT_IN_TMU  0x02
#define T0_IN_TMU0     0x04
#define T1_IN_TMU0     0x08
#define T0_IN_TMU1     0x10
#define T1_IN_TMU1     0x20

   tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);
   tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);
   GLuint tstate = 0;
   int tmu0 = 0, tmu1 = 1;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupDoubleTMU(...)\n");
   }

   /* We shouldn't need to do this. There is something wrong with
      mutlitexturing when the TMUs are swapped. So, we're forcing
      them to always be loaded correctly. !!! */
   if (ti0->whichTMU == FX_TMU1)
      fxTMMoveOutTM_NoLock(fxMesa, tObj0);
   if (ti1->whichTMU == FX_TMU0)
      fxTMMoveOutTM_NoLock(fxMesa, tObj1);

   if (ti0->isInTM) {
      switch (ti0->whichTMU) {
      case FX_TMU0:
	 tstate |= T0_IN_TMU0;
	 break;
      case FX_TMU1:
	 tstate |= T0_IN_TMU1;
	 break;
      case FX_TMU_BOTH:
	 tstate |= T0_IN_TMU0 | T0_IN_TMU1;
	 break;
      case FX_TMU_SPLIT:
	 tstate |= T0_NOT_IN_TMU;
	 break;
      }
   }
   else
      tstate |= T0_NOT_IN_TMU;

   if (ti1->isInTM) {
      switch (ti1->whichTMU) {
      case FX_TMU0:
	 tstate |= T1_IN_TMU0;
	 break;
      case FX_TMU1:
	 tstate |= T1_IN_TMU1;
	 break;
      case FX_TMU_BOTH:
	 tstate |= T1_IN_TMU0 | T1_IN_TMU1;
	 break;
      case FX_TMU_SPLIT:
	 tstate |= T1_NOT_IN_TMU;
	 break;
      }
   }
   else
      tstate |= T1_NOT_IN_TMU;

   ti0->lastTimeUsed = fxMesa->texBindNumber;
   ti1->lastTimeUsed = fxMesa->texBindNumber;

   /* Move texture maps into TMUs */

   if (!(((tstate & T0_IN_TMU0) && (tstate & T1_IN_TMU1)) ||
	 ((tstate & T0_IN_TMU1) && (tstate & T1_IN_TMU0)))) {
      if (tObj0 == tObj1)
	 fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU_BOTH);
      else {
	 /* Find the minimal way to correct the situation */
	 if ((tstate & T0_IN_TMU0) || (tstate & T1_IN_TMU1)) {
	    /* We have one in the standard order, setup the other */
	    if (tstate & T0_IN_TMU0) {	/* T0 is in TMU0, put T1 in TMU1 */
	       fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
	    }
	    /* tmu0 and tmu1 are setup */
	 }
	 else if ((tstate & T0_IN_TMU1) || (tstate & T1_IN_TMU0)) {
	    /* we have one in the reverse order, setup the other */
	    if (tstate & T1_IN_TMU0) {	/* T1 is in TMU0, put T0 in TMU1 */
	       fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU1);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
	    }
	    tmu0 = 1;
	    tmu1 = 0;
	 }
	 else {			/* Nothing is loaded */
	    fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
	    fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
	    /* tmu0 and tmu1 are setup */
	 }
      }
   }

   if (!fxMesa->haveGlobalPaletteTexture) {
      if (ti0->info.format == GR_TEXFMT_P_8) {
	 if (MESA_VERBOSE & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxmesa: uploading texture palette TMU0\n");
	 }
	 FX_grTexDownloadTable_NoLock(tmu0, GR_TEXTABLE_PALETTE,
				      &(ti0->palette));
      }

      if (ti1->info.format == GR_TEXFMT_P_8) {
	 if (MESA_VERBOSE & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxmesa: uploading texture palette TMU1\n");
	 }
	 FX_grTexDownloadTable_NoLock(tmu1, GR_TEXTABLE_PALETTE,
				      &(ti1->palette));
      }
   }

   FX_grTexSource_NoLock(tmu0, ti0->tm[tmu0]->startAddr,
			 GR_MIPMAPLEVELMASK_BOTH, &(ti0->info));
   FX_grTexClampMode_NoLock(tmu0, ti0->sClamp, ti0->tClamp);
   FX_grTexFilterMode_NoLock(tmu0, ti0->minFilt, ti0->maxFilt);
   FX_grTexMipMapMode_NoLock(tmu0, ti0->mmMode, FXFALSE);

   FX_grTexSource_NoLock(tmu1, ti1->tm[tmu1]->startAddr,
			 GR_MIPMAPLEVELMASK_BOTH, &(ti1->info));
   FX_grTexClampMode_NoLock(tmu1, ti1->sClamp, ti1->tClamp);
   FX_grTexFilterMode_NoLock(tmu1, ti1->minFilt, ti1->maxFilt);
   FX_grTexMipMapMode_NoLock(tmu1, ti1->mmMode, FXFALSE);

#undef T0_NOT_IN_TMU
#undef T1_NOT_IN_TMU
#undef T0_IN_TMU0
#undef T1_IN_TMU0
#undef T0_IN_TMU1
#undef T1_IN_TMU1
}

static void
fxSetupTextureDoubleTMU_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GrCombineLocal_t localc, locala;
   tfxTexInfo *ti0, *ti1;
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[0].Current2D;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[1].Current2D;
   GLuint envmode, ifmt, unitsmode;
   int tmu0 = 0, tmu1 = 1;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTextureDoubleTMU(...) Start\n");
   }

   ti0 = fxTMGetTexInfo(tObj0);
   fxTexValidate(ctx, tObj0);

   ti1 = fxTMGetTexInfo(tObj1);
   fxTexValidate(ctx, tObj1);

   fxSetupDoubleTMU_NoLock(fxMesa, tObj0, tObj1);

   unitsmode = fxGetTexSetConfiguration(ctx, tObj0, tObj1);

/*    if(fxMesa->lastUnitsMode==unitsmode) */
/*      return; */

   fxMesa->lastUnitsMode = unitsmode;

   fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
   FX_grHints_NoLock(GR_HINT_STWHINT, fxMesa->stw_hint_state);

   envmode = unitsmode & FX_UM_E_ENVMODE;
   ifmt = unitsmode & FX_UM_E_IFMT;

   if (unitsmode & FX_UM_ALPHA_ITERATED)
      locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (unitsmode & FX_UM_COLOR_ITERATED)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;


   if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxMesa: fxSetupTextureDoubleTMU, envmode is %s/%s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


   if ((ti0->whichTMU == FX_TMU1) || (ti1->whichTMU == FX_TMU0)) {
      tmu0 = 1;
      tmu1 = 0;
   }
   fxMesa->tmuSrc = FX_TMU_BOTH;
   switch (envmode) {
   case (FX_UM_E0_MODULATE | FX_UM_E1_MODULATE):
      {
	 GLboolean isalpha[FX_NUM_TMU];

	 if (ti0->baseLevelInternalFormat == GL_ALPHA)
	    isalpha[tmu0] = GL_TRUE;
	 else
	    isalpha[tmu0] = GL_FALSE;

	 if (ti1->baseLevelInternalFormat == GL_ALPHA)
	    isalpha[tmu1] = GL_TRUE;
	 else
	    isalpha[tmu1] = GL_FALSE;

	 if (isalpha[FX_TMU1])
	    FX_grTexCombine_NoLock(GR_TMU1,
				   GR_COMBINE_FUNCTION_ZERO,
				   GR_COMBINE_FACTOR_NONE,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE, FXTRUE, FXFALSE);
	 else
	    FX_grTexCombine_NoLock(GR_TMU1,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	 if (isalpha[FX_TMU0])
	    FX_grTexCombine_NoLock(GR_TMU0,
				   GR_COMBINE_FUNCTION_BLEND_OTHER,
				   GR_COMBINE_FACTOR_ONE,
				   GR_COMBINE_FUNCTION_BLEND_OTHER,
				   GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);
	 else
	    FX_grTexCombine_NoLock(GR_TMU0,
				   GR_COMBINE_FUNCTION_BLEND_OTHER,
				   GR_COMBINE_FACTOR_LOCAL,
				   GR_COMBINE_FUNCTION_BLEND_OTHER,
				   GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);

	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_LOCAL,
				  localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);

	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_LOCAL,
				  locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
	 break;
      }
   case (FX_UM_E0_REPLACE | FX_UM_E1_BLEND):	/* Only for GLQuake */
      if (tmu1 == FX_TMU1) {
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXTRUE, FXFALSE);

	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_LOCAL,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);
      }
      else {
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
				FXFALSE, FXFALSE);
      }

      FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
			       GR_COMBINE_FACTOR_NONE,
			       locala, GR_COMBINE_OTHER_NONE, FXFALSE);

      FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
			       GR_COMBINE_FACTOR_ONE,
			       localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      break;
   case (FX_UM_E0_REPLACE | FX_UM_E1_MODULATE):	/* Quake 2 and 3 */
      if (tmu1 == FX_TMU1) {
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_ZERO,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXTRUE);

	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_LOCAL,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);

      }
      else {
	 FX_grTexCombine_NoLock(GR_TMU1,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL,
				GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	 FX_grTexCombine_NoLock(GR_TMU0,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_LOCAL,
				GR_COMBINE_FUNCTION_BLEND_OTHER,
				GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);
      }

      if (ti0->baseLevelInternalFormat == GL_RGB)
	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
				  GR_COMBINE_FACTOR_NONE,
				  locala, GR_COMBINE_OTHER_NONE, FXFALSE);
      else
	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_ONE,
				  locala, GR_COMBINE_OTHER_NONE, FXFALSE);


      FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
			       GR_COMBINE_FACTOR_ONE,
			       localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      break;


   case (FX_UM_E0_MODULATE | FX_UM_E1_ADD):	/* Quake 3 Sky */
      {
	 GLboolean isalpha[FX_NUM_TMU];

	 if (ti0->baseLevelInternalFormat == GL_ALPHA)
	    isalpha[tmu0] = GL_TRUE;
	 else
	    isalpha[tmu0] = GL_FALSE;

	 if (ti1->baseLevelInternalFormat == GL_ALPHA)
	    isalpha[tmu1] = GL_TRUE;
	 else
	    isalpha[tmu1] = GL_FALSE;

	 if (isalpha[FX_TMU1])
	    FX_grTexCombine_NoLock(GR_TMU1,
				   GR_COMBINE_FUNCTION_ZERO,
				   GR_COMBINE_FACTOR_NONE,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE, FXTRUE, FXFALSE);
	 else
	    FX_grTexCombine_NoLock(GR_TMU1,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE,
				   GR_COMBINE_FUNCTION_LOCAL,
				   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	 if (isalpha[FX_TMU0])
	    FX_grTexCombine_NoLock(GR_TMU0,
				   GR_COMBINE_FUNCTION_SCALE_OTHER,
				   GR_COMBINE_FACTOR_ONE,
				   GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
				   GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);
	 else
	    FX_grTexCombine_NoLock(GR_TMU0,
				   GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
				   GR_COMBINE_FACTOR_ONE,
				   GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
				   GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);

	 FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_LOCAL,
				  localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);

	 FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
				  GR_COMBINE_FACTOR_LOCAL,
				  locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
	 break;
      }
   default:
      fprintf(stderr, "Unexpected dual texture mode encountered\n");
      break;
   }

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTextureDoubleTMU(...) End\n");
   }
}

/************************* No Texture ***************************/

static void
fxSetupTextureNone_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GrCombineLocal_t localc, locala;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTextureNone(...)\n");
   }

   if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
       (ctx->Point.SmoothFlag) ||
       (ctx->Line.SmoothFlag) ||
       (ctx->Polygon.SmoothFlag)) locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;

   FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
			    GR_COMBINE_FACTOR_NONE,
			    locala, GR_COMBINE_OTHER_NONE, FXFALSE);

   FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
			    GR_COMBINE_FACTOR_NONE,
			    localc, GR_COMBINE_OTHER_NONE, FXFALSE);

   fxMesa->lastUnitsMode = FX_UM_NONE;
}

/************************************************************************/
/************************** Texture Mode SetUp **************************/
/************************************************************************/

static void
fxSetupTexture_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint tex2Denabled;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxSetupTexture(...)\n");
   }

   /* Texture Combine, Color Combine and Alpha Combine.
    */
   tex2Denabled = (ctx->Texture._ReallyEnabled & TEXTURE0_2D);

   if (fxMesa->haveTwoTMUs)
      tex2Denabled |= (ctx->Texture._ReallyEnabled & TEXTURE1_2D);

   switch (tex2Denabled) {
   case TEXTURE0_2D:
      fxSetupTextureSingleTMU_NoLock(ctx, 0);
      break;
   case TEXTURE1_2D:
      fxSetupTextureSingleTMU_NoLock(ctx, 1);
      break;
   case (TEXTURE0_2D | TEXTURE1_2D):
      if (fxMesa->haveTwoTMUs)
	 fxSetupTextureDoubleTMU_NoLock(ctx);
      else {
	 if (MESA_VERBOSE & VERBOSE_DRIVER)
	    fprintf(stderr, "fxmesa: enabling fake multitexture\n");

	 fxSetupTextureSingleTMU_NoLock(ctx, 0);
      }
      break;
   default:
      fxSetupTextureNone_NoLock(ctx);
      break;
   }
}

static void
fxSetupTexture(GLcontext * ctx)
{
   BEGIN_BOARD_LOCK();
   fxSetupTexture_NoLock(ctx);
   END_BOARD_LOCK();
}

/************************************************************************/
/**************************** Blend SetUp *******************************/
/************************************************************************/

/* XXX consider supporting GL_INGR_blend_func_separate */
void
fxDDBlendFunc(GLcontext * ctx, GLenum sfactor, GLenum dfactor)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;
   GrAlphaBlendFnc_t sfact, dfact, asfact, adfact;

   /* From the Glide documentation:
      For alpha source and destination blend function factor
      parameters, Voodoo Graphics supports only
      GR_BLEND_ZERO and GR_BLEND_ONE.
    */

   switch (sfactor) {
   case GL_ZERO:
      asfact = sfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      asfact = sfact = GR_BLEND_ONE;
      break;
   case GL_DST_COLOR:
      sfact = GR_BLEND_DST_COLOR;
      asfact = GR_BLEND_ONE;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      sfact = GR_BLEND_ONE_MINUS_DST_COLOR;
      asfact = GR_BLEND_ONE;
      break;
   case GL_SRC_ALPHA:
      sfact = GR_BLEND_SRC_ALPHA;
      asfact = GR_BLEND_ONE;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      sfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
      asfact = GR_BLEND_ONE;
      break;
   case GL_DST_ALPHA:
      sfact = GR_BLEND_DST_ALPHA;
      asfact = GR_BLEND_ONE;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      sfact = GR_BLEND_ONE_MINUS_DST_ALPHA;
      asfact = GR_BLEND_ONE;
      break;
   case GL_SRC_ALPHA_SATURATE:
      sfact = GR_BLEND_ALPHA_SATURATE;
      asfact = GR_BLEND_ONE;
      break;
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
      /* USELESS */
      asfact = sfact = GR_BLEND_ONE;
      break;
   default:
      asfact = sfact = GR_BLEND_ONE;
      break;
   }

   if ((sfact != us->blendSrcFuncRGB) || (asfact != us->blendSrcFuncAlpha)) {
      us->blendSrcFuncRGB = sfact;
      us->blendSrcFuncAlpha = asfact;
      fxMesa->new_state |= FX_NEW_BLEND;
   }

   switch (dfactor) {
   case GL_ZERO:
      adfact = dfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      adfact = dfact = GR_BLEND_ONE;
      break;
   case GL_SRC_COLOR:
      dfact = GR_BLEND_SRC_COLOR;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      dfact = GR_BLEND_ONE_MINUS_SRC_COLOR;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_SRC_ALPHA:
      dfact = GR_BLEND_SRC_ALPHA;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      dfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_DST_ALPHA:
      /* dfact=GR_BLEND_DST_ALPHA; */
      /* We can't do DST_ALPHA */
      dfact = GR_BLEND_ONE;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      /* dfact=GR_BLEND_ONE_MINUS_DST_ALPHA; */
      /* We can't do DST_ALPHA */
      dfact = GR_BLEND_ZERO;
      adfact = GR_BLEND_ZERO;
      break;
   case GL_SRC_ALPHA_SATURATE:
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
      /* USELESS */
      adfact = dfact = GR_BLEND_ZERO;
      break;
   default:
      adfact = dfact = GR_BLEND_ZERO;
      break;
   }

   if ((dfact != us->blendDstFuncRGB) || (adfact != us->blendDstFuncAlpha)) {
      us->blendDstFuncRGB = dfact;
      us->blendDstFuncAlpha = adfact;
      fxMesa->new_state |= FX_NEW_BLEND;
   }
}

static void
fxSetupBlend(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->blendEnabled)
      FX_grAlphaBlendFunction(us->blendSrcFuncRGB, us->blendDstFuncRGB,
			      us->blendSrcFuncAlpha, us->blendDstFuncAlpha);
   else
      FX_grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE,
			      GR_BLEND_ZERO);
}

/************************************************************************/
/************************** Alpha Test SetUp ****************************/
/************************************************************************/

void
fxDDAlphaFunc(GLcontext * ctx, GLenum func, GLchan ref)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;
   GrCmpFnc_t newfunc;

   switch (func) {
   case GL_NEVER:
      newfunc = GR_CMP_NEVER;
      break;
   case GL_LESS:
      newfunc = GR_CMP_LESS;
      break;
   case GL_EQUAL:
      newfunc = GR_CMP_EQUAL;
      break;
   case GL_LEQUAL:
      newfunc = GR_CMP_LEQUAL;
      break;
   case GL_GREATER:
      newfunc = GR_CMP_GREATER;
      break;
   case GL_NOTEQUAL:
      newfunc = GR_CMP_NOTEQUAL;
      break;
   case GL_GEQUAL:
      newfunc = GR_CMP_GEQUAL;
      break;
   case GL_ALWAYS:
      newfunc = GR_CMP_ALWAYS;
      break;
   default:
      fprintf(stderr, "fx Driver: internal error in fxDDAlphaFunc()\n");
      fxCloseHardware();
      exit(-1);
      break;
   }

   if (newfunc != us->alphaTestFunc) {
      us->alphaTestFunc = newfunc;
      fxMesa->new_state |= FX_NEW_ALPHA;
   }

   if (ref != us->alphaTestRefValue) {
      us->alphaTestRefValue = ref;
      fxMesa->new_state |= FX_NEW_ALPHA;
   }
}

static void
fxSetupAlphaTest(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->alphaTestEnabled) {
      FX_grAlphaTestFunction(us->alphaTestFunc);
      FX_grAlphaTestReferenceValue(us->alphaTestRefValue);
   }
   else
      FX_grAlphaTestFunction(GR_CMP_ALWAYS);
}

/************************************************************************/
/************************** Depth Test SetUp ****************************/
/************************************************************************/

void
fxDDDepthFunc(GLcontext * ctx, GLenum func)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;
   GrCmpFnc_t dfunc;

   switch (func) {
   case GL_NEVER:
      dfunc = GR_CMP_NEVER;
      break;
   case GL_LESS:
      dfunc = GR_CMP_LESS;
      break;
   case GL_GEQUAL:
      dfunc = GR_CMP_GEQUAL;
      break;
   case GL_LEQUAL:
      dfunc = GR_CMP_LEQUAL;
      break;
   case GL_GREATER:
      dfunc = GR_CMP_GREATER;
      break;
   case GL_NOTEQUAL:
      dfunc = GR_CMP_NOTEQUAL;
      break;
   case GL_EQUAL:
      dfunc = GR_CMP_EQUAL;
      break;
   case GL_ALWAYS:
      dfunc = GR_CMP_ALWAYS;
      break;
   default:
      fprintf(stderr, "fx Driver: internal error in fxDDDepthFunc()\n");
      fxCloseHardware();
      exit(-1);
      break;
   }

   if (dfunc != us->depthTestFunc) {
      us->depthTestFunc = dfunc;
      fxMesa->new_state |= FX_NEW_DEPTH;
   }

}

void
fxDDDepthMask(GLcontext * ctx, GLboolean flag)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;

   if (flag != us->depthMask) {
      us->depthMask = flag;
      fxMesa->new_state |= FX_NEW_DEPTH;
   }
}

static void
fxSetupDepthTest(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->depthTestEnabled) {
      FX_grDepthBufferFunction(us->depthTestFunc);
      FX_grDepthMask(us->depthMask);
   }
   else {
      FX_grDepthBufferFunction(GR_CMP_ALWAYS);
      FX_grDepthMask(FXFALSE);
   }
}

/************************************************************************/
/**************************** Color Mask SetUp **************************/
/************************************************************************/

void
fxDDColorMask(GLcontext * ctx,
	      GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   fxMesa->new_state |= FX_NEW_COLOR_MASK;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
}

static void
fxSetupColorMask(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (ctx->Color.DrawBuffer == GL_NONE) {
      FX_grColorMask(FXFALSE, FXFALSE);
   }
   else {
      FX_grColorMask(ctx->Color.ColorMask[RCOMP] ||
		     ctx->Color.ColorMask[GCOMP] ||
		     ctx->Color.ColorMask[BCOMP],
		     ctx->Color.ColorMask[ACOMP] && fxMesa->haveAlphaBuffer);
   }
}




/************************************************************************/
/**************************** Fog Mode SetUp ****************************/
/************************************************************************/

/*
 * This is called during state update in order to update the Glide fog state.
 */
static void
fxSetupFog(GLcontext * ctx)
{
   if (ctx->Fog.Enabled /*&& ctx->FogMode==FOG_FRAGMENT */ ) {
      fxMesaContext fxMesa = FX_CONTEXT(ctx);

      /* update fog color */
      GLubyte col[4];
      col[0] = (unsigned int) (255 * ctx->Fog.Color[0]);
      col[1] = (unsigned int) (255 * ctx->Fog.Color[1]);
      col[2] = (unsigned int) (255 * ctx->Fog.Color[2]);
      col[3] = (unsigned int) (255 * ctx->Fog.Color[3]);
      FX_grFogColorValue(FXCOLOR4(col));

      if (fxMesa->fogTableMode != ctx->Fog.Mode ||
	  fxMesa->fogDensity != ctx->Fog.Density ||
	  fxMesa->fogStart != ctx->Fog.Start ||
	  fxMesa->fogEnd != ctx->Fog.End) {
	 /* reload the fog table */
	 switch (ctx->Fog.Mode) {
	 case GL_LINEAR:
	    guFogGenerateLinear(fxMesa->fogTable, ctx->Fog.Start,
				ctx->Fog.End);
	    break;
	 case GL_EXP:
	    guFogGenerateExp(fxMesa->fogTable, ctx->Fog.Density);
	    break;
	 case GL_EXP2:
	    guFogGenerateExp2(fxMesa->fogTable, ctx->Fog.Density);
	    break;
	 default:
	    ;
	 }
	 fxMesa->fogTableMode = ctx->Fog.Mode;
	 fxMesa->fogDensity = ctx->Fog.Density;
	 fxMesa->fogStart = ctx->Fog.Start;
	 fxMesa->fogEnd = ctx->Fog.End;
      }

      FX_grFogTable(fxMesa->fogTable);
      FX_grFogMode(GR_FOG_WITH_TABLE);
   }
   else {
      FX_grFogMode(GR_FOG_DISABLE);
   }
}

void
fxDDFogfv(GLcontext * ctx, GLenum pname, const GLfloat * params)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_FOG;
}

/************************************************************************/
/************************** Scissor Test SetUp **************************/
/************************************************************************/

/* This routine is used in managing the lock state, and therefore can't lock */
void
fxSetScissorValues(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   int xmin, xmax;
   int ymin, ymax, check;

   if (ctx->Scissor.Enabled) {
      xmin = ctx->Scissor.X;
      xmax = ctx->Scissor.X + ctx->Scissor.Width;
      ymin = ctx->Scissor.Y;
      ymax = ctx->Scissor.Y + ctx->Scissor.Height;
      check = 1;
   }
   else {
      xmin = 0;
      ymin = 0;
      xmax = fxMesa->width;
      ymax = fxMesa->height;
      check = 0;
   }
   if (xmin < fxMesa->clipMinX)
      xmin = fxMesa->clipMinX;
   if (xmax > fxMesa->clipMaxX)
      xmax = fxMesa->clipMaxX;
   if (ymin < fxMesa->screen_height - fxMesa->clipMaxY)
      ymin = fxMesa->screen_height - fxMesa->clipMaxY;
   if (ymax > fxMesa->screen_height - fxMesa->clipMinY)
      ymax = fxMesa->screen_height - fxMesa->clipMinY;
   FX_grClipWindow_NoLock(xmin, ymin, xmax, ymax);
}

static void
fxSetupScissor(GLcontext * ctx)
{
   BEGIN_BOARD_LOCK();
   fxSetScissorValues(ctx);
   END_BOARD_LOCK();
}

void
fxDDScissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_SCISSOR;
}

/************************************************************************/
/*************************** Cull mode setup ****************************/
/************************************************************************/


void
fxDDCullFace(GLcontext * ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
}

void
fxDDFrontFace(GLcontext * ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
}


static void
fxSetupCull(GLcontext * ctx)
{
   if (ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_BACK:
	 if (ctx->Polygon.FrontFace == GL_CCW)
	    FX_CONTEXT(ctx)->cullMode = GR_CULL_NEGATIVE;
	 else
	    FX_CONTEXT(ctx)->cullMode = GR_CULL_POSITIVE;
	 break;
      case GL_FRONT:
	 if (ctx->Polygon.FrontFace == GL_CCW)
	    FX_CONTEXT(ctx)->cullMode = GR_CULL_POSITIVE;
	 else
	    FX_CONTEXT(ctx)->cullMode = GR_CULL_NEGATIVE;
	 break;
      case GL_FRONT_AND_BACK:
	 FX_CONTEXT(ctx)->cullMode = GR_CULL_DISABLE;
	 break;
      default:
	 break;
      }
   }
   else
      FX_CONTEXT(ctx)->cullMode = GR_CULL_DISABLE;

   if (FX_CONTEXT(ctx)->raster_primitive == GL_TRIANGLES)
      FX_grCullMode(FX_CONTEXT(ctx)->cullMode);
}


/************************************************************************/
/****************************** DD Enable ******************************/
/************************************************************************/

void
fxDDEnable(GLcontext * ctx, GLenum cap, GLboolean state)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   tfxUnitsState *us = &fxMesa->unitsState;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxDDEnable(...)\n");
   }

   switch (cap) {
   case GL_ALPHA_TEST:
      if (state != us->alphaTestEnabled) {
	 us->alphaTestEnabled = state;
	 fxMesa->new_state |= FX_NEW_ALPHA;
      }
      break;
   case GL_BLEND:
      if (state != us->blendEnabled) {
	 us->blendEnabled = state;
	 fxMesa->new_state |= FX_NEW_BLEND;
      }
      break;
   case GL_DEPTH_TEST:
      if (state != us->depthTestEnabled) {
	 us->depthTestEnabled = state;
	 fxMesa->new_state |= FX_NEW_DEPTH;
      }
      break;
   case GL_DITHER:
      if (state) {
	 FX_grDitherMode(GR_DITHER_4x4);
      }
      else {
	 FX_grDitherMode(GR_DITHER_DISABLE);
      }
      break;
   case GL_SCISSOR_TEST:
      fxMesa->new_state |= FX_NEW_SCISSOR;
      break;
   case GL_SHARED_TEXTURE_PALETTE_EXT:
      fxDDTexUseGlbPalette(ctx, state);
      break;
   case GL_FOG:
      fxMesa->new_state |= FX_NEW_FOG;
      break;
   case GL_CULL_FACE:
      fxMesa->new_state |= FX_NEW_CULL;
      break;
   case GL_LINE_SMOOTH:
   case GL_LINE_STIPPLE:
   case GL_POINT_SMOOTH:
   case GL_POLYGON_SMOOTH:
   case GL_TEXTURE_2D:
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;
   default:
      ;				/* XXX no-op? */
   }
}




/************************************************************************/
/************************** Changes to units state **********************/
/************************************************************************/


/* All units setup is handled under texture setup.
 */
void
fxDDShadeModel(GLcontext * ctx, GLenum mode)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_TEXTURING;
}



/************************************************************************/
/****************************** Units SetUp *****************************/
/************************************************************************/
static void
fx_print_state_flags(const char *msg, GLuint flags)
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & FX_NEW_TEXTURING) ? "texture, " : "",
	   (flags & FX_NEW_BLEND) ? "blend, " : "",
	   (flags & FX_NEW_ALPHA) ? "alpha, " : "",
	   (flags & FX_NEW_FOG) ? "fog, " : "",
	   (flags & FX_NEW_SCISSOR) ? "scissor, " : "",
	   (flags & FX_NEW_COLOR_MASK) ? "colormask, " : "",
	   (flags & FX_NEW_CULL) ? "cull, " : "");
}

void
fxSetupFXUnits(GLcontext * ctx)
{
   fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
   GLuint newstate = fxMesa->new_state;

   if (MESA_VERBOSE & VERBOSE_DRIVER)
      fx_print_state_flags("fxmesa: fxSetupFXUnits", newstate);

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
	 fxSetupFog(ctx);

      if (newstate & FX_NEW_SCISSOR)
	 fxSetupScissor(ctx);

      if (newstate & FX_NEW_COLOR_MASK)
	 fxSetupColorMask(ctx);

      if (newstate & FX_NEW_CULL)
	 fxSetupCull(ctx);

      fxMesa->new_state = 0;
   }
}



#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_setup(void);
int
gl_fx_dummy_function_setup(void)
{
   return 0;
}

#endif /* FX */
