/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatex.c,v 1.14 2002/10/30 12:51:36 alanh Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"
#include "mgatris.h"
#include "mgaioctl.h"

#include "enums.h"
#include "simple_list.h"
/*#include "mem.h"*/
#include "macros.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"

#include "swrast/swrast.h"

#define TEX_0 1
#define TEX_1 2

/*
 * mgaDestroyTexObj
 * Free all memory associated with a texture and NULL any pointers
 * to it.
 */
void
mgaDestroyTexObj( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
   if ( !t ) return;

   /* free the texture memory */
   if (t->MemBlock) {
      mmFreeMem( t->MemBlock );
      t->MemBlock = 0;

      if (mmesa && t->age > mmesa->dirtyAge)
	 mmesa->dirtyAge = t->age;
   }

   /* free mesa's link */
   if (t->tObj)
      t->tObj->DriverData = NULL;

   /* see if it was the driver's current object */
   if (mmesa) {
      if (t->bound & TEX_0) mmesa->CurrentTexObj[0] = 0;
      if (t->bound & TEX_1) mmesa->CurrentTexObj[1] = 0;
   }
	
   remove_from_list(t);
   free( t );
}


/*
 * mgaSetTexWrappings
 */
static void mgaSetTexWrapping( mgaTextureObjectPtr t,
			       GLenum sWrap,
			       GLenum tWrap )
{
   GLuint val = 0;

   if (sWrap != GL_REPEAT)
      val |= TMC_clampu_enable;

   if (tWrap != GL_REPEAT)
      val |= TMC_clampv_enable;

   t->setup.texctl &= ~(TMC_clampu_enable|TMC_clampv_enable);
   t->setup.texctl |= val;
}


/*
 * mgaSetTexFilter
 */
static void mgaSetTexFilter(mgaTextureObjectPtr t, GLenum minf, GLenum magf)
{
   GLuint val = 0;

   switch (minf) {
   case GL_NEAREST: val = TF_minfilter_nrst; break;
   case GL_LINEAR: val = TF_minfilter_bilin; break;
   case GL_NEAREST_MIPMAP_NEAREST: val = TF_minfilter_mm1s; break;
   case GL_LINEAR_MIPMAP_NEAREST: val = TF_minfilter_mm4s; break;
   case GL_NEAREST_MIPMAP_LINEAR: val = TF_minfilter_mm2s; break;
   case GL_LINEAR_MIPMAP_LINEAR: val = TF_minfilter_mm8s; break;
   default: val = TF_minfilter_nrst; break;
   }

   switch (magf) {
   case GL_NEAREST: val |= TF_magfilter_nrst; break;
   case GL_LINEAR: val |= TF_magfilter_bilin; break;
   default: val |= TF_magfilter_nrst; break;
   }

   /* See OpenGL 1.2 specification */
   if (magf == GL_LINEAR && (minf == GL_NEAREST_MIPMAP_NEAREST ||
			     minf == GL_NEAREST_MIPMAP_LINEAR)) {
      val |= (0x20 << TF_fthres_SHIFT); /* c = 0.5 */
   } else {
      val |= (0x10 << TF_fthres_SHIFT); /* c = 0 */
   }


   t->setup.texfilter &= (TF_minfilter_MASK |
			  TF_magfilter_MASK |
			  TF_fthres_MASK);
   t->setup.texfilter |= val;
}

/*
 * mgaSetTexBorderColor
 */
static void mgaSetTexBorderColor(mgaTextureObjectPtr t, GLubyte color[4])
{
   t->setup.texbordercol = MGAPACKCOLOR8888(color[0],color[1],
					    color[2],color[3]);
}


static GLint mgaChooseTexFormat( mgaContextPtr mmesa,
				 struct gl_texture_image *texImage,
				 GLenum format, GLenum type )
{
   const GLboolean do32bpt = mmesa->default32BitTextures;
   const struct gl_texture_format *texFormat;
   GLint ret;

   if ( 0 )
      fprintf( stderr, "internal=%s format=%s type=%s\n",
	       texImage->IntFormat == 3 ? "GL_RGB (3)" :
	       texImage->IntFormat == 4 ? "GL_RGBA (4)" :
	       _mesa_lookup_enum_by_nr( texImage->IntFormat ),
	       _mesa_lookup_enum_by_nr( format ),
	       _mesa_lookup_enum_by_nr( type ) );

#define SET_FORMAT( r, gl )						\
   do {									\
      ret = (r);							\
      texFormat = &(gl);						\
   } while (0)

#define SET_FORMAT_32BPT( r32, gl32, r16, gl16 )			\
   do {									\
      if ( do32bpt ) {							\
	 ret = (r32);							\
	 texFormat = &(gl32);						\
      } else {								\
	 ret = (r16);							\
	 texFormat = &(gl16);						\
      }									\
   } while (0)

   switch ( texImage->IntFormat ) {
      /* GH: Bias towards GL_RGB, GL_RGBA texture formats.  This has
       * got to be better than sticking them way down the end of this
       * huge list.
       */
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if ( format == GL_BGRA ) {
	 if ( type == GL_UNSIGNED_INT_8_8_8_8_REV ) {
	    SET_FORMAT( TMC_tformat_tw32, _mesa_texformat_argb8888 );
	    break;
	 } else if ( type == GL_UNSIGNED_SHORT_4_4_4_4_REV ) {
	    SET_FORMAT( TMC_tformat_tw12, _mesa_texformat_argb4444 );
	    break;
	 } else if ( type == GL_UNSIGNED_SHORT_1_5_5_5_REV ) {
	    SET_FORMAT( TMC_tformat_tw15, _mesa_texformat_argb1555 );
	    break;
	 }
      }
      SET_FORMAT_32BPT( TMC_tformat_tw32, _mesa_texformat_argb8888,
			TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if ( format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 ) {
	 SET_FORMAT( TMC_tformat_tw16, _mesa_texformat_rgb565 );
	 break;
      }
      SET_FORMAT_32BPT( TMC_tformat_tw32, _mesa_texformat_argb8888,
			TMC_tformat_tw16, _mesa_texformat_rgb565 );
      break;

      /* GH: Okay, keep checking as normal.  Still test for GL_RGB,
       * GL_RGBA formats first.
       */
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      SET_FORMAT_32BPT( TMC_tformat_tw32, _mesa_texformat_argb8888,
			TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case GL_RGBA4:
   case GL_RGBA2:
      SET_FORMAT( TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case GL_RGB5_A1:
      SET_FORMAT( TMC_tformat_tw15, _mesa_texformat_argb1555 );
      break;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      SET_FORMAT_32BPT( TMC_tformat_tw32, _mesa_texformat_argb8888,
			TMC_tformat_tw16, _mesa_texformat_rgb565 );
      break;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      SET_FORMAT( TMC_tformat_tw16, _mesa_texformat_rgb565 );
      break;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      /* FIXME: This will report incorrect component sizes... */
      SET_FORMAT( TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      /* FIXME: This will report incorrect component sizes... */
      SET_FORMAT( TMC_tformat_tw16, _mesa_texformat_rgb565 );
      break;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      /* FIXME: This will report incorrect component sizes... */
      SET_FORMAT( TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      /* FIXME: This will report incorrect component sizes... */
      SET_FORMAT( TMC_tformat_tw12, _mesa_texformat_argb4444 );
      break;

   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      SET_FORMAT( TMC_tformat_tw8, _mesa_texformat_ci8 );
      break;

   default:
      fprintf( stderr, "bad texture format in mgaChooseTexFormat() %d",
	       texImage->IntFormat );
      return -1;
   }

   texImage->TexFormat = texFormat;

   return ret;
}


/*
 * mgaCreateTexObj
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */
static void mgaCreateTexObj(mgaContextPtr mmesa,
			    struct gl_texture_object *tObj)
{
   const GLint baseLevel = tObj->BaseLevel;
   struct gl_texture_image *image = tObj->Image[baseLevel];
   mgaTextureObjectPtr t;
   int i, ofs;
   int LastLevel;
   int s, s2;
   int tformat;

   if (!image) return;

   tObj->DriverData = t = calloc( 1, sizeof( *t ) );
   if (!t) {
      fprintf(stderr, "mgaCreateTexObj: Failed to malloc mgaTextureObject\n" );
      return;
   }

   /* FIXME: Use the real DD interface...
    */
   tformat = mgaChooseTexFormat( mmesa, image, image->Format,
				 GL_UNSIGNED_BYTE );
   t->texelBytes = image->TexFormat->TexelBytes;

   /* We are going to upload all levels that are present, even if
    * later levels wouldn't be used by the current filtering mode.  This
    * allows the filtering mode to change without forcing another upload
    * of the images.
    */
   LastLevel = MGA_TEX_MAXLEVELS-1;

   ofs = 0;
   for ( i = 0 ; i <= LastLevel ; i++ ) {
      if ( !tObj->Image[i] ) {
	 LastLevel = i - 1;
	 break;
      }

      t->offsets[i] = ofs;
      t->dirty_images |= (1<<i);

      ofs += ((MAX2( tObj->Image[i]->Width, 8 ) *
	       MAX2( tObj->Image[i]->Height, 8 ) *
	       t->texelBytes) + 31) & ~31;
   }
   t->totalSize = ofs;
   t->lastLevel = LastLevel;
   t->tObj = tObj;
   t->ctx = mmesa;
   t->age = 0;
   t->bound = 0;
   t->MemBlock = 0;

   insert_at_tail(&(mmesa->SwappedOut), t);


   /* setup hardware register values */
   t->setup.texctl = TMC_takey_1 | TMC_tamask_0 | tformat;

   if (image->WidthLog2 >= 3)
      t->setup.texctl |= ((image->WidthLog2 - 3) << TMC_tpitch_SHIFT);
   else
      t->setup.texctl |= (TMC_tpitchlin_enable |
			  (image->Width << TMC_tpitchext_SHIFT));


   t->setup.texctl2 = TMC_ckstransdis_enable;

   if ( mmesa->glCtx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR )
      t->setup.texctl2 |= TMC_specen_enable;


   t->setup.texfilter = (TF_minfilter_nrst |
			 TF_magfilter_nrst |
			 TF_filteralpha_enable |
			 (0x10 << TF_fthres_SHIFT) |
			 (LastLevel << TF_mapnb_SHIFT));

   /* warp texture registers */
   ofs = MGA_IS_G200(mmesa) ? 28 : 11;
   s = image->Width;
   s2 = image->WidthLog2;
   t->setup.texwidth = (MGA_FIELD(TW_twmask, s - 1) |
			MGA_FIELD(TW_rfw, (10 - s2 - 8) & 63 ) |
			MGA_FIELD(TW_tw, (s2 + ofs ) | 0x40 ));


   s = image->Height;
   s2 = image->HeightLog2;
   t->setup.texheight = (MGA_FIELD(TH_thmask, s - 1) |
			 MGA_FIELD(TH_rfh, (10 - s2 - 8) & 63 ) |
			 MGA_FIELD(TH_th, (s2 + ofs ) | 0x40 ));


   /* set all the register values for filtering, border, etc */
   mgaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
   mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
   mgaSetTexBorderColor( t, tObj->_BorderChan );
}




static void mgaUpdateTextureEnvG200( GLcontext *ctx )
{
   struct gl_texture_object *tObj = ctx->Texture.Unit[0]._Current;
   mgaTextureObjectPtr t;

   if (!tObj || !tObj->DriverData)
      return;

   t = (mgaTextureObjectPtr)tObj->DriverData;

   t->setup.texctl2 &= ~TMC_decalblend_enable;

   switch (ctx->Texture.Unit[0].EnvMode) {
   case GL_REPLACE:
      t->setup.texctl &= ~TMC_tmodulate_enable;
      break;
   case GL_MODULATE:
      t->setup.texctl |= TMC_tmodulate_enable;
      break;
   case GL_DECAL:
      t->setup.texctl &= ~TMC_tmodulate_enable;
      t->setup.texctl2 |= TMC_decalblend_enable;
      break;
   case GL_BLEND:
      FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
      break;
   default:
      break;
   }
}

static void mgaUpdateTextureEnvG400( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint *reg = ((GLuint *)&mmesa->setup.tdualstage0 + unit);
   GLuint source = mmesa->tmu_source[unit];
   struct gl_texture_object *tObj = ctx->Texture.Unit[source]._Current;
   GLenum format;

   if ( tObj != ctx->Texture.Unit[source].Current2D || !tObj ) 
      return;

   format = tObj->Image[tObj->BaseLevel]->Format;

   switch (ctx->Texture.Unit[source].EnvMode) {
   case GL_REPLACE:
      if (format == GL_RGB || format == GL_LUMINANCE) {
	 *reg = (TD0_color_sel_arg1 |
                 TD0_alpha_arg2_diffuse |
		 TD0_alpha_sel_arg2 );
      }
      else if (format == GL_ALPHA) {
         *reg = (TD0_color_sel_arg2 |
                 TD0_color_arg2_diffuse |
                 TD0_alpha_sel_arg1 );
      }
      else {
         *reg = (TD0_color_sel_arg1 |
                 TD0_alpha_sel_arg1 );
      }
      break;

   case GL_MODULATE:
      if (unit == 0) {
	 *reg = ( TD0_color_arg2_diffuse |
		  TD0_color_sel_mul |
		  TD0_alpha_arg2_diffuse |
		  TD0_alpha_sel_mul);
      }
      else {
	 *reg = ( TD0_color_arg2_prevstage |
		  TD0_color_alpha_prevstage |
		  TD0_color_sel_mul |
		  TD0_alpha_arg2_prevstage |
		  TD0_alpha_sel_mul);
      }
      break;
   case GL_DECAL:
      if (format == GL_RGB) {
         if (unit == 0) {
            *reg = (TD0_color_sel_arg1 |
                    TD0_alpha_arg2_diffuse |
                    TD0_alpha_sel_arg2 );
         }
         else {
            *reg = (TD0_color_sel_arg1 |
                    TD0_alpha_arg2_prevstage |
                    TD0_alpha_sel_arg2 );
         }
      }
      else if ( format == GL_RGBA ) {
#if 0
         if (unit == 0) {
            /* this doesn't work */
            *reg = (TD0_color_arg2_diffuse |
                    TD0_color_alpha_currtex |
                    TD0_color_alpha2inv_enable |
                    TD0_color_arg2mul_alpha2 |
                    TD0_color_arg1mul_alpha1 |
                    TD0_color_blend_enable |
                    TD0_color_arg1add_mulout |
                    TD0_color_arg2add_mulout |
                    TD0_color_add_add |
                    TD0_color_sel_mul |
                    TD0_alpha_arg2_diffuse |
                    TD0_alpha_sel_arg2 );
         }
         else {
            *reg = (TD0_color_arg2_prevstage |
                    TD0_color_alpha_currtex |
                    TD0_color_alpha2inv_enable |
                    TD0_color_arg2mul_alpha2 |
                    TD0_color_arg1mul_alpha1 |
                    TD0_color_add_add |
                    TD0_color_sel_add |
                    TD0_alpha_arg2_prevstage |
                    TD0_alpha_sel_arg2 );
         }
#else
         /* s/w fallback, pretty sure we can't do in h/w */
	 FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
	 if ( MGA_DEBUG & DEBUG_VERBOSE_FALLBACK )
	    fprintf( stderr, "FALLBACK: GL_DECAL RGBA texture, unit=%d\n",
		     unit );
#endif
      }
      else {
	 if (unit == 0) {
	    *reg = ( TD0_color_arg2_diffuse |
		     TD0_color_sel_arg2 |
		     TD0_alpha_arg2_diffuse |
		     TD0_alpha_sel_arg2);
	 }
	 else {
	    *reg = ( TD0_color_arg2_prevstage |
		     TD0_color_sel_arg2 |
		     TD0_alpha_arg2_prevstage |
		     TD0_alpha_sel_arg2);
	 }
      }
      break;

   case GL_ADD:
      if (unit == 0) {
         if (format == GL_INTENSITY)
            *reg = ( TD0_color_arg2_diffuse |
                     TD0_color_add_add |
                     TD0_color_sel_add |
                     TD0_alpha_arg2_diffuse |
                     TD0_alpha_add_enable |
                     TD0_alpha_sel_add);
         else if (format == GL_ALPHA)
            *reg = ( TD0_color_arg2_diffuse |
                     TD0_color_sel_mul |
                     TD0_alpha_arg2_diffuse |
                     TD0_alpha_sel_mul);
         else
            *reg = ( TD0_color_arg2_diffuse |
                     TD0_color_add_add |
                     TD0_color_sel_add |
                     TD0_alpha_arg2_diffuse |
                     TD0_alpha_sel_mul);
      }
      else {
         if (format == GL_INTENSITY) {
            *reg = ( TD0_color_arg2_prevstage |
                     TD0_color_add_add |
                     TD0_color_sel_add |
                     TD0_alpha_arg2_prevstage |
                     TD0_alpha_add_enable |
                     TD0_alpha_sel_add);
         }
         else if (format == GL_ALPHA) {
            *reg = ( TD0_color_arg2_prevstage |
                     TD0_color_sel_mul |
                     TD0_alpha_arg2_prevstage |
                     TD0_alpha_sel_mul);
         }
         else {
            *reg = ( TD0_color_arg2_prevstage |
                     TD0_color_alpha_prevstage |
                     TD0_color_add_add |
                     TD0_color_sel_add |
                     TD0_alpha_arg2_prevstage |
                     TD0_alpha_sel_mul);
         }
      }
      break;

   case GL_BLEND:
      if (format == GL_ALPHA) {
	 *reg = ( TD0_color_arg2_diffuse |
		  TD0_color_sel_mul |
		  TD0_alpha_arg2_diffuse |
		  TD0_alpha_sel_mul);
      }
      else {
	 FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
	 if ( MGA_DEBUG & DEBUG_VERBOSE_FALLBACK )
	    fprintf( stderr, "FALLBACK: GL_BLEND envcolor=0x%08x\n",
		     mmesa->envcolor );

         /* Do singletexture GL_BLEND with 'all ones' env-color
          * by using both texture units.  Multitexture gl_blend
          * is a fallback.
          */
         if (unit == 0) {
            /* Part 1: R1 = Rf ( 1 - Rt )
             *         A1 = Af At
             */
            *reg = ( TD0_color_arg2_diffuse |
                     TD0_color_arg1_inv_enable |
                     TD0_color_sel_mul |
                     TD0_alpha_arg2_diffuse |
                     TD0_alpha_sel_arg1);
         } else {
            /* Part 2: R2 = R1 + Rt
             *         A2 = A1
             */
            *reg = ( TD0_color_arg2_prevstage |
                     TD0_color_add_add |
                     TD0_color_sel_add |
                     TD0_alpha_arg2_prevstage |
                     TD0_alpha_sel_arg2);
         }
      }
      break;
   default:
      break;
   }
}



static void mgaUpdateTextureObject( GLcontext *ctx, int hw_unit )
{
   mgaTextureObjectPtr t;
   struct gl_texture_object	*tObj;
   GLuint enabled;
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint gl_unit = mmesa->tmu_source[hw_unit];


   enabled = ctx->Texture.Unit[gl_unit]._ReallyEnabled;
   tObj = ctx->Texture.Unit[gl_unit]._Current;

   if (enabled != TEXTURE_2D_BIT) {
      if (enabled)
	 FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
      return;
   }

   if (tObj->Image[tObj->BaseLevel]->Border > 0) {
      FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
      if ( MGA_DEBUG & DEBUG_VERBOSE_FALLBACK )
	 fprintf( stderr, "FALLBACK: texture border\n" );
      return;
   }

   if ( !tObj->DriverData ) {
      mgaCreateTexObj( mmesa, tObj );
      if ( !tObj->DriverData ) {
	 FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_TRUE );
	 return;		
      }
   }

   t = (mgaTextureObjectPtr)tObj->DriverData;

   if (t->dirty_images)
      mmesa->dirty |= (MGA_UPLOAD_TEX0IMAGE << hw_unit);

   mmesa->CurrentTexObj[hw_unit] = t;
   t->bound |= hw_unit+1;

/*     if (t->MemBlock) */
/*        mgaUpdateTexLRU( mmesa, t ); */

   t->setup.texctl2 &= ~TMC_dualtex_enable;
   if ((ctx->Texture.Unit[0]._ReallyEnabled == TEXTURE_2D_BIT) &&
       (ctx->Texture.Unit[1]._ReallyEnabled == TEXTURE_2D_BIT)) 
      t->setup.texctl2 |= TMC_dualtex_enable;

   t->setup.texctl2 &= ~TMC_specen_enable;
   if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
      t->setup.texctl2 |= TMC_specen_enable;
}






/* The G400 is now programmed quite differently wrt texture environment.
 */
void mgaUpdateTextureState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   FALLBACK( ctx, MGA_FALLBACK_TEXTURE, GL_FALSE );

   if (mmesa->CurrentTexObj[0]) {
      mmesa->CurrentTexObj[0]->bound = 0;
      mmesa->CurrentTexObj[0] = 0;
   }

   if (mmesa->CurrentTexObj[1]) {
      mmesa->CurrentTexObj[1]->bound = 0;
      mmesa->CurrentTexObj[1] = 0;
   }

   if (ctx->Texture.Unit[1]._ReallyEnabled == TEXTURE_2D_BIT) {
      mmesa->tmu_source[0] = 1;
   } else {
      mmesa->tmu_source[0] = 0;
   }

   if (MGA_IS_G400(mmesa)) {
      mgaUpdateTextureObject( ctx, 0 );
      mgaUpdateTextureEnvG400( ctx, 0 );

      mmesa->setup.tdualstage1 = mmesa->setup.tdualstage0;
      
      if ((ctx->Texture.Unit[0]._ReallyEnabled == TEXTURE_2D_BIT) &&
          (ctx->Texture.Unit[1]._ReallyEnabled == TEXTURE_2D_BIT)) {
	 mgaUpdateTextureObject( ctx, 1 );	
	 mgaUpdateTextureEnvG400( ctx, 1 );
	 mmesa->dirty |= MGA_UPLOAD_TEX1;
      }
   } else {
      mgaUpdateTextureObject( ctx, 0 );
      mgaUpdateTextureEnvG200( ctx );
   }

   mmesa->dirty |= MGA_UPLOAD_CONTEXT | MGA_UPLOAD_TEX0;

   mmesa->setup.dwgctl &= DC_opcod_MASK;
   mmesa->setup.dwgctl |= (ctx->Texture.Unit[0]._ReallyEnabled
			   ? DC_opcod_texture_trap
			   : DC_opcod_trap);
}




static void mgaDDTexEnv( GLcontext *ctx, GLenum target,
			 GLenum pname, const GLfloat *param )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);


   if (pname == GL_TEXTURE_ENV_MODE) {
      /* force the texture state to be updated */
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
      MGA_CONTEXT(ctx)->new_state |= (MGA_NEW_TEXTURE |
				      MGA_NEW_ALPHA);
   }
   else if (pname == GL_TEXTURE_ENV_COLOR)
   {
      struct gl_texture_unit *texUnit =
	 &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      GLfloat *fc = texUnit->EnvColor;
      GLubyte c[4];
      GLuint col;

      COPY_4V(c, fc);
      col = mgaPackColor( mmesa->mgaScreen->cpp, c[0], c[1], c[2], c[3] );
      mmesa->envcolor = (c[3]<<24) | (c[0]<<16) | (c[1]<<8) | (c[2]);

      if (mmesa->setup.fcol != col) {
	 FLUSH_BATCH(mmesa);
	 mmesa->setup.fcol = col;
	 mmesa->dirty |= MGA_UPLOAD_CONTEXT;

	 mmesa->blend_flags &= ~MGA_BLEND_ENV_COLOR;

	 /* Actually just require all four components to be
	  * equal.  This permits a single-pass GL_BLEND.
	  *
	  * More complex multitexture/multipass fallbacks
	  * for blend can be done later.
	  */
	 if (mmesa->envcolor != 0x0 && mmesa->envcolor != 0xffffffff)
	    mmesa->blend_flags |= MGA_BLEND_ENV_COLOR;
      }
   }
}


static void mgaTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   mgaTextureObjectPtr t = (mgaTextureObjectPtr) texObj->DriverData;
   if (t) {
      mgaDestroyTexObj( MGA_CONTEXT(ctx), t );
      texObj->DriverData = 0;
   }
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
}

static void mgaTexSubImage2D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset, GLint yoffset,
			       GLsizei width, GLsizei height,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *packing,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
   mgaTextureObjectPtr t = (mgaTextureObjectPtr) texObj->DriverData;
   if (t) {
      mgaDestroyTexObj( MGA_CONTEXT(ctx), t );
      texObj->DriverData = 0;
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);

}




/*
 * mgaTexParameter
 * This just changes variables and flags for a state update, which
 * will happen at the next mgaUpdateTextureState
 */
static void
mgaDDTexParameter( GLcontext *ctx, GLenum target,
		   struct gl_texture_object *tObj,
		   GLenum pname, const GLfloat *params )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaTextureObjectPtr t;

   t = (mgaTextureObjectPtr) tObj->DriverData;

   /* if we don't have a hardware texture, it will be automatically
      created with current state before it is used, so we don't have
      to do anything now */
   if ( !t || !t->bound || target != GL_TEXTURE_2D ) {
      return;
   }

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      FLUSH_BATCH(mmesa);
      mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      FLUSH_BATCH(mmesa);
      mgaSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
      break;

   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_BATCH(mmesa);
      mgaSetTexBorderColor(t,tObj->_BorderChan);
      break;

   default:
      return;
   }

   mmesa->new_state |= MGA_NEW_TEXTURE;
}


static void
mgaDDBindTexture( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int unit = ctx->Texture.CurrentUnit;

   FLUSH_BATCH(mmesa);

   if (mmesa->CurrentTexObj[unit]) {
      mmesa->CurrentTexObj[unit]->bound &= ~(unit+1);
      mmesa->CurrentTexObj[unit] = 0;
   }

   /* force the texture state to be updated
    */
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
}


static void
mgaDDDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaTextureObjectPtr t = (mgaTextureObjectPtr)tObj->DriverData;

   if ( t ) {
      if (mmesa) {
         if (t->bound) {
            FLUSH_BATCH(mmesa);
            if (t->bound & TEX_0) mmesa->CurrentTexObj[0] = 0;
            if (t->bound & TEX_1) mmesa->CurrentTexObj[1] = 0;
         }
         mmesa->new_state |= MGA_NEW_TEXTURE;
      }

      mgaDestroyTexObj( mmesa, t );
   }
}


static GLboolean
mgaDDIsTextureResident( GLcontext *ctx, struct gl_texture_object *t )
{
   mgaTextureObjectPtr mt = (mgaTextureObjectPtr)t->DriverData;
   return mt && mt->MemBlock;
}


void
mgaDDInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.TexEnv = mgaDDTexEnv;

   ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   ctx->Driver.TexImage1D = _mesa_store_teximage1d;
   ctx->Driver.TexImage2D = mgaTexImage2D;
   ctx->Driver.TexImage3D = _mesa_store_teximage3d;
   ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
   ctx->Driver.TexSubImage2D = mgaTexSubImage2D;
   ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
   ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
   ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
   ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

   ctx->Driver.BindTexture = mgaDDBindTexture;
   ctx->Driver.DeleteTexture = mgaDDDeleteTexture;
   ctx->Driver.TexParameter = mgaDDTexParameter;
   ctx->Driver.UpdateTexturePalette = 0;
   ctx->Driver.IsTextureResident = mgaDDIsTextureResident;
}
