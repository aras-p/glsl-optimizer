/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (c) Copyright IBM Corporation 2002
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
 * VA LINUX SYSTEMS, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */
/* $XFree86:$ */

#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"
#include "mgatris.h"
#include "mgaioctl.h"

#include "context.h"
#include "enums.h"
#include "macros.h"
#include "imports.h"

#include "simple_list.h"
#include "texformat.h"

#define MGA_USE_TABLE_FOR_FORMAT
#ifdef MGA_USE_TABLE_FOR_FORMAT
#define TMC_nr_tformat (MESA_FORMAT_YCBCR_REV + 1)
static const unsigned TMC_tformat[ TMC_nr_tformat ] =
{
    [MESA_FORMAT_ARGB8888] = TMC_tformat_tw32 | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_RGB565]   = TMC_tformat_tw16 | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_ARGB4444] = TMC_tformat_tw12 | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_ARGB1555] = TMC_tformat_tw15 | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_AL88]     = TMC_tformat_tw8al | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_I8]       = TMC_tformat_tw8a | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_CI8]      = TMC_tformat_tw8  | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_YCBCR]     = TMC_tformat_tw422uyvy | TMC_takey_1 | TMC_tamask_0,
    [MESA_FORMAT_YCBCR_REV] = TMC_tformat_tw422 | TMC_takey_1 | TMC_tamask_0,
};
#endif

static void
mgaSetTexImages( mgaContextPtr mmesa,
		 const struct gl_texture_object * tObj )
{
    mgaTextureObjectPtr t = (mgaTextureObjectPtr) tObj->DriverData;
    struct gl_texture_image *baseImage = tObj->Image[ tObj->BaseLevel ];
    GLint totalSize;
    GLint width, height;
    GLint i;
    GLint numLevels;
    GLint log2Width, log2Height;
    GLuint txformat = 0;
    GLint ofs;

    /* Set the hardware texture format
     */
#ifndef MGA_USE_TABLE_FOR_FORMAT
    switch (baseImage->TexFormat->MesaFormat) {

	case MESA_FORMAT_ARGB8888: txformat = TMC_tformat_tw32;	break;
	case MESA_FORMAT_RGB565:   txformat = TMC_tformat_tw16; break;
	case MESA_FORMAT_ARGB4444: txformat = TMC_tformat_tw12;	break;
	case MESA_FORMAT_ARGB1555: txformat = TMC_tformat_tw15; break;
	case MESA_FORMAT_AL88:     txformat = TMC_tformat_tw8al; break;
	case MESA_FORMAT_I8:       txformat = TMC_tformat_tw8a; break;
	case MESA_FORMAT_CI8:      txformat = TMC_tformat_tw8;  break;
        case MESA_FORMAT_YCBCR:    txformat  = TMC_tformat_tw422uyvy; break;
        case MESA_FORMAT_YCBCR_REV: txformat = TMC_tformat_tw422; break;

	default:
	_mesa_problem(NULL, "unexpected texture format in %s", __FUNCTION__);
	return;
    }
#else
    if ( (baseImage->TexFormat->MesaFormat >= TMC_nr_tformat)
	 || (TMC_tformat[ baseImage->TexFormat->MesaFormat ] == 0) )
    {
	_mesa_problem(NULL, "unexpected texture format in %s", __FUNCTION__);
	return;
    }

    txformat = TMC_tformat[ baseImage->TexFormat->MesaFormat ];

#endif /* MGA_USE_TABLE_FOR_FORMAT */

   driCalculateTextureFirstLastLevel( (driTextureObject *) t );
   log2Width  = tObj->Image[t->base.firstLevel]->WidthLog2;
   log2Height = tObj->Image[t->base.firstLevel]->HeightLog2;

   width = tObj->Image[t->base.firstLevel]->Width;
   height = tObj->Image[t->base.firstLevel]->Height;

   numLevels = MIN2( t->base.lastLevel - t->base.firstLevel + 1,
                     MGA_IS_G200(mmesa) ? G200_TEX_MAXLEVELS : G400_TEX_MAXLEVELS);


   totalSize = 0;
   for ( i = 0 ; i < numLevels ; i++ ) {
      const struct gl_texture_image * const texImage = 
	  tObj->Image[ i + t->base.firstLevel ];

      if ( (texImage == NULL)
	   || ((i != 0)
	       && ((texImage->Width < 8) || (texImage->Height < 8))) ) {
	 break;
      }

      t->offsets[i] = totalSize;
      t->base.dirty_images[0] |= (1<<i);

      totalSize += ((MAX2( texImage->Width, 8 ) *
                     MAX2( texImage->Height, 8 ) *
                     baseImage->TexFormat->TexelBytes) + 31) & ~31;
   }

   /* save these values */
   numLevels = i;
   t->base.lastLevel = t->base.firstLevel + numLevels - 1;
   t->base.totalSize = totalSize;

   /* setup hardware register values */
   t->setup.texctl &= (TMC_tformat_MASK & TMC_tpitch_MASK 
		       & TMC_tpitchext_MASK);
   t->setup.texctl |= txformat;


   /* Set the texture width.  In order to support non-power of 2 textures and
    * textures larger than 1024 texels wide, "linear" pitch must be used.  For
    * the linear pitch, if the width is 2048, a value of zero is used.
    */

   t->setup.texctl |= TMC_tpitchlin_enable;
   t->setup.texctl |= (width & (2048 - 1)) << TMC_tpitchext_SHIFT;


   /* G400 specifies the number of mip levels in a strange way.  Since there
    * are up to 12 levels, it requires 4 bits.  Three of the bits are at the
    * high end of TEXFILTER.  The other bit is in the middle.  Weird.
    */

   t->setup.texfilter &= TF_mapnb_MASK & TF_mapnbhigh_MASK & TF_reserved_MASK;
   t->setup.texfilter |= (((numLevels-1) & 0x07) << (TF_mapnb_SHIFT));
   t->setup.texfilter |= (((numLevels-1) & 0x08) << (TF_mapnbhigh_SHIFT - 3));

   /* warp texture registers */
   ofs = MGA_IS_G200(mmesa) ? 28 : 11;

   t->setup.texwidth = (MGA_FIELD(TW_twmask, width - 1) |
			MGA_FIELD(TW_rfw, (10 - log2Width - 8) & 63 ) |
			MGA_FIELD(TW_tw, (log2Width + ofs ) | 0x40 ));

   t->setup.texheight = (MGA_FIELD(TH_thmask, height - 1) |
			 MGA_FIELD(TH_rfh, (10 - log2Height - 8) & 63 ) |
			 MGA_FIELD(TH_th, (log2Height + ofs ) | 0x40 ));

   mgaUploadTexImages( mmesa, t );
}


/* ================================================================
 * Texture unit state management
 */

static void mgaUpdateTextureEnvG200( GLcontext *ctx, GLuint unit )
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
      t->texenv_fallback = GL_TRUE;
      break;
   default:
      break;
   }
}


#define MGA_DISABLE		0
#define MGA_REPLACE		1
#define MGA_MODULATE		2
#define MGA_DECAL		3
#define MGA_BLEND		4
#define MGA_ADD			5
#define MGA_MAX_COMBFUNC	6

static const GLuint g400_color_combine[][MGA_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (0),

      /* GL_REPLACE
       * Cv = Cs
       * Av = Af
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),
      
      /* GL_MODULATE
       * Cv = Cf Cs
       * Av = Af
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_mul |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),
      
      /* GL_DECAL
       * Cv = Cs
       * Av = Af
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),
      
      /* GL_BLEND (Cc=0.0)
       * Cv = Cf ( 1 - Cs )
       * Av = Af
       */
      (TD0_color_arg1_inv_enable |
       TD0_color_arg2_diffuse |
       TD0_color_sel_mul |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),
      
      /* GL_ADD
       * Cv = Cf + Cs
       * Av = Af
       */
      (TD0_color_arg2_diffuse |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),
   },
   
   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      (0),
       
      /* GL_REPLACE
       * Cv = Cs
       * Av = Ap
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),
      
      /* GL_MODULATE
       * Cv = Cp Cs
       * Av = Ap
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_mul |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),

      /* GL_DECAL
       * Cv = Cs
       * Av = Ap
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),
      
      /* GL_BLEND (Cc=0.0)
       * Cv = Cp ( 1 - Cs )
       * Av = Ap
       */
      (TD0_color_arg1_inv_enable |
       TD0_color_arg2_prevstage |
       TD0_color_sel_mul |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),
      
      /* GL_ADD
       * Cv = Cp + Cs
       * Av = Ap
       */
      (TD0_color_arg2_prevstage |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),
   },
};

static const GLuint g400_color_alpha_combine[][MGA_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (0),

      /* GL_REPLACE
       * Cv = Cs
       * Av = As
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_sel_arg1),
      
      /* GL_MODULATE
       * Cv = Cf Cs
       * Av = Af As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_mul |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),
      
      /* GL_DECAL
       * tmp = Cf ( 1 - As )
       * Cv = tmp + Cs As
       * Av = Af
       */
      (TD0_color_arg2_diffuse |
       TD0_color_alpha_currtex |
       TD0_color_alpha1inv_enable |
       TD0_color_arg1mul_alpha1 |
       TD0_color_blend_enable |
       TD0_color_arg1add_mulout |
       TD0_color_arg2add_mulout |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),

      /* GL_BLEND (Cc=0.0)
       * Cv = Cf ( 1 - Cs )
       * Av = Af As
       */
      (TD0_color_arg1_inv_enable |
       TD0_color_arg2_diffuse |
       TD0_color_sel_mul |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),
      
      /* GL_ADD
       * Cv = Cf + Cs
       * Av = Af As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),
   },
   
   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      (0),
       
      /* GL_REPLACE
       * Cv = Cs
       * Av = As
       */
      (TD0_color_sel_arg1 |
       TD0_alpha_sel_arg1),
      
      /* GL_MODULATE
       * Cv = Cp Cs
       * Av = Ap As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_mul |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),

      /* GL_DECAL
       * tmp = Cp ( 1 - As )
       * Cv = tmp + Cs As
       * Av = Ap
       */
      (TD0_color_arg2_prevstage |
       TD0_color_alpha_currtex |
       TD0_color_alpha1inv_enable |
       TD0_color_arg1mul_alpha1 |
       TD0_color_blend_enable |
       TD0_color_arg1add_mulout |
       TD0_color_arg2add_mulout |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),
      
      /* GL_BLEND (Cc=0.0)
       * Cv = Cp ( 1 - Cs )
       * Av = Ap As
       */
      (TD0_color_arg1_inv_enable |
       TD0_color_arg2_prevstage |
       TD0_color_sel_mul |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),
      
      /* GL_ADD
       * Cv = Cp + Cs
       * Av = Ap As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_add_add |
       TD0_color_sel_add |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),
   },
};

static const GLuint g400_alpha_combine[][MGA_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (0),

      /* GL_REPLACE
       * Cv = Cf
       * Av = As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_arg2 |
       TD0_alpha_sel_arg1),
      
      /* GL_MODULATE
       * Cv = Cf
       * Av = Af As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),

      /* GL_DECAL (undefined)
       * Cv = Cf
       * Av = Af
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_arg2),

      /* GL_BLEND
       * Cv = Cf
       * Av = Af As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),

      /* GL_ADD
       * Cv = Cf
       * Av = Af As
       */
      (TD0_color_arg2_diffuse |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_diffuse |
       TD0_alpha_sel_mul),
   },

   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      (0),

      /* GL_REPLACE
       * Cv = Cp
       * Av = As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_arg2 |
       TD0_alpha_sel_arg1),
      
      /* GL_MODULATE
       * Cv = Cp
       * Av = Ap As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),

      /* GL_DECAL (undefined)
       * Cv = Cp
       * Av = Ap
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_arg2),

      /* GL_BLEND
       * Cv = Cp
       * Av = Ap As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),

      /* GL_ADD
       * Cv = Cp
       * Av = Ap As
       */
      (TD0_color_arg2_prevstage |
       TD0_color_sel_arg2 |
       TD0_alpha_arg2_prevstage |
       TD0_alpha_sel_mul),
   },
};

static void mgaUpdateTextureEnvG400( GLcontext *ctx, GLuint unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   const int source = mmesa->tmu_source[unit];
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[source];
   const struct gl_texture_object *tObj = texUnit->_Current;
   GLuint *reg = ((GLuint *)&mmesa->setup.tdualstage0 + unit);
   mgaTextureObjectPtr t;
   GLenum format;

   if ( !tObj ||
        (tObj != ctx->Texture.Unit[source].Current2D &&
         tObj != ctx->Texture.Unit[source].CurrentRect) )
      return;

   format = tObj->Image[tObj->BaseLevel]->Format;

   t = (mgaTextureObjectPtr) tObj->DriverData;

   switch (ctx->Texture.Unit[source].EnvMode) {
   case GL_REPLACE:
      if (format == GL_ALPHA) {
         *reg = g400_alpha_combine[unit][MGA_REPLACE];
      } else if (format == GL_RGB || format == GL_LUMINANCE) {
         *reg = g400_color_combine[unit][MGA_REPLACE];
      } else {
         *reg = g400_color_alpha_combine[unit][MGA_REPLACE];
      }
      break;

   case GL_MODULATE:
      if (format == GL_ALPHA) {
         *reg = g400_alpha_combine[unit][MGA_MODULATE];
      } else if (format == GL_RGB || format == GL_LUMINANCE) {
         *reg = g400_color_combine[unit][MGA_MODULATE];
      } else {
         *reg = g400_color_alpha_combine[unit][MGA_MODULATE];
      }
      break;

   case GL_DECAL:
      if (format == GL_RGB) {
         *reg = g400_color_combine[unit][MGA_DECAL];
      } else if (format == GL_RGBA) {
         *reg = g400_color_alpha_combine[unit][MGA_DECAL];
         if (ctx->Texture._EnabledUnits != 0x03) {
            /* Linear blending mode needs dual texturing enabled */
            *(reg+1) = (TD0_color_arg2_prevstage |
                        TD0_color_sel_arg2 |
                        TD0_alpha_arg2_prevstage |
                        TD0_alpha_sel_arg2);
            mmesa->dualtex_env = GL_TRUE;
         }
      } else {
         /* Undefined */
         *reg = g400_alpha_combine[unit][MGA_DECAL];
      }
      break;

   case GL_ADD:
      if (format == GL_ALPHA) {
         *reg = g400_alpha_combine[unit][MGA_ADD];
      } else if (format == GL_RGB || format == GL_LUMINANCE) {
         *reg = g400_color_combine[unit][MGA_ADD];
      } else if (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) {
         *reg = g400_color_alpha_combine[unit][MGA_ADD];
      } else if (format == GL_INTENSITY) {
         /* Cv = Cf + Cs
          * Av = Af + As
          */
         if (unit == 0) {
            *reg = (TD0_color_arg2_diffuse |
                    TD0_color_add_add |
                    TD0_color_sel_add |
                    TD0_alpha_arg2_diffuse |
                    TD0_alpha_add_enable |
                    TD0_alpha_sel_add);
         } else {
            *reg = (TD0_color_arg2_prevstage |
                    TD0_color_add_add |
                    TD0_color_sel_add |
                    TD0_alpha_arg2_prevstage |
                    TD0_alpha_add_enable |
                    TD0_alpha_sel_add);
         }
      }
      break;

   case GL_BLEND:
      if (format == GL_ALPHA) {
         *reg = g400_alpha_combine[unit][MGA_BLEND];
      } else {
         if (mmesa->blend_flags & MGA_BLEND_RGB_ZERO) {
            if (format == GL_RGB || format == GL_LUMINANCE) {
               *reg = g400_color_combine[unit][MGA_BLEND];
            } else if (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) {
               *reg = g400_color_alpha_combine[unit][MGA_BLEND];
            } else if (format == GL_INTENSITY) {
               if (mmesa->blend_flags & MGA_BLEND_ALPHA_ZERO) {
                  /* Cv = Cf ( 1 - Cs )
                   * Av = Af ( 1 - As )
                   */
                  if (unit == 0) {
                     *reg = (TD0_color_arg1_inv_enable |
                             TD0_color_arg2_diffuse |
                             TD0_color_sel_mul |
                             TD0_alpha_arg1_inv_enable |
                             TD0_alpha_arg2_diffuse |
                             TD0_alpha_sel_mul);
                  } else {
                     *reg = (TD0_color_arg1_inv_enable |
                             TD0_color_arg2_prevstage |
                             TD0_color_sel_mul |
                             TD0_alpha_arg1_inv_enable |
                             TD0_alpha_arg2_prevstage |
                             TD0_alpha_sel_mul);
                  }
               } else if (mmesa->blend_flags & MGA_BLEND_ALPHA_ONE &&
                          ctx->Texture._EnabledUnits != 0x03) {
                  /* C1 = Cf ( 1 - Cs )
                   * A1 = Af ( 1 - As )
                   */
                  *reg = (TD0_color_arg1_inv_enable |
                          TD0_color_arg2_diffuse |
                          TD0_color_sel_mul |
                          TD0_alpha_arg1_inv_enable |
                          TD0_alpha_arg2_diffuse |
                          TD0_alpha_sel_mul);
                  /* Cv = C1
                   * Av = A1 + As
                   */
                  *(reg+1) = (TD0_color_arg2_prevstage |
                              TD0_color_sel_arg2 |
                              TD0_alpha_arg2_prevstage |
                              TD0_alpha_add_enable |
                              TD0_alpha_sel_add);
                  mmesa->dualtex_env = GL_TRUE;
               } else {
                  t->texenv_fallback = GL_TRUE;
               }
            }
         } else if (mmesa->blend_flags & MGA_BLEND_RGB_ONE &&
                    ctx->Texture._EnabledUnits != 0x03) {
            if (format == GL_RGB || format == GL_LUMINANCE) {
                  /* C1 = Cf ( 1 - Cs )
                   * A1 = Af
                   */
                  *reg = (TD0_color_arg1_inv_enable |
                          TD0_color_arg2_diffuse |
                          TD0_color_sel_mul |
                          TD0_alpha_arg2_diffuse |
                          TD0_alpha_sel_arg2);
                  /* Cv = C1 + Cs
                   * Av = A1
                   */
                  *(reg+1) = (TD0_color_arg2_prevstage |
                              TD0_color_add_add |
                              TD0_color_sel_add |
                              TD0_alpha_arg2_prevstage |
                              TD0_alpha_sel_arg2);
                  mmesa->dualtex_env = GL_TRUE;
            } else if (format == GL_RGBA || format == GL_LUMINANCE_ALPHA) {
                  /* C1 = Cf ( 1 - Cs )
                   * A1 = Af As
                   */
                  *reg = (TD0_color_arg1_inv_enable |
                          TD0_color_arg2_diffuse |
                          TD0_color_sel_mul |
                          TD0_alpha_arg2_diffuse |
                          TD0_alpha_sel_mul);
                  /* Cv = C1 + Cs
                   * Av = A1
                   */
                  *(reg+1) = (TD0_color_arg2_prevstage |
                              TD0_color_add_add |
                              TD0_color_sel_add |
                              TD0_alpha_arg2_prevstage |
                              TD0_alpha_sel_arg2);
                  mmesa->dualtex_env = GL_TRUE;
            } else if (format == GL_INTENSITY) {
               if (mmesa->blend_flags & MGA_BLEND_ALPHA_ZERO) {
                  /* C1 = Cf ( 1 - Cs )
                   * A1 = Af ( 1 - As )
                   */
                  *reg = (TD0_color_arg1_inv_enable |
                          TD0_color_arg2_diffuse |
                          TD0_color_sel_mul |
                          TD0_alpha_arg1_inv_enable |
                          TD0_alpha_arg2_diffuse |
                          TD0_alpha_sel_mul);
                  /* Cv = C1 + Cs
                   * Av = A1
                   */
                  *(reg+1) = (TD0_color_arg2_prevstage |
                              TD0_color_add_add |
                              TD0_color_sel_add |
                              TD0_alpha_arg2_prevstage |
                              TD0_alpha_sel_arg2);
                  mmesa->dualtex_env = GL_TRUE;
               } else if (mmesa->blend_flags & MGA_BLEND_ALPHA_ONE) {
                  /* C1 = Cf ( 1 - Cs )
                   * A1 = Af ( 1 - As )
                   */
                  *reg = (TD0_color_arg1_inv_enable |
                          TD0_color_arg2_diffuse |
                          TD0_color_sel_mul |
                          TD0_alpha_arg1_inv_enable |
                          TD0_alpha_arg2_diffuse |
                          TD0_alpha_sel_mul);
                  /* Cv = C1 + Cs
                   * Av = A1 + As
                   */
                  *(reg+1) = (TD0_color_arg2_prevstage |
                              TD0_color_add_add |
                              TD0_color_sel_add |
                              TD0_alpha_arg2_prevstage |
                              TD0_alpha_add_enable |
                              TD0_alpha_sel_add);
                  mmesa->dualtex_env = GL_TRUE;
               } else {
                  t->texenv_fallback = GL_TRUE;
               }
            }
         } else {
            t->texenv_fallback = GL_TRUE;
         }
      }
      break;
   case GL_COMBINE_EXT:
      if (!mgaUpdateTextureEnvCombine(ctx, unit))
         t->texenv_fallback = GL_TRUE;
      break;
   default:
      break;
   }
}

static void disable_tex( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );

   /* Texture unit disabled */

   if ( mmesa->CurrentTexObj[unit] != NULL ) {
      /* The old texture is no longer bound to this texture unit.
       * Mark it as such.
       */

      mmesa->CurrentTexObj[unit]->base.bound &= ~(1UL << unit);
      mmesa->CurrentTexObj[unit] = NULL;
   }

   if ( unit != 0 && !mmesa->dualtex_env ) {
      mmesa->setup.tdualstage1 = mmesa->setup.tdualstage0;
   }

   if ( ctx->Texture._EnabledUnits == 0 ) {
      mmesa->setup.dwgctl &= DC_opcod_MASK;
      mmesa->setup.dwgctl |= DC_opcod_trap;
      mmesa->hw.alpha_sel = AC_alphasel_diffused;
   }

   mmesa->dirty |= MGA_UPLOAD_CONTEXT | (MGA_UPLOAD_TEX0 << unit);
}

static GLboolean enable_tex( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const int source = mmesa->tmu_source[unit];
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[source];
   const struct gl_texture_object *tObj = texUnit->_Current;
   mgaTextureObjectPtr t = (mgaTextureObjectPtr) tObj->DriverData;

   /* Upload teximages (not pipelined)
    */
   if (t->base.dirty_images[0]) {
      FLUSH_BATCH( mmesa );
      mgaSetTexImages( mmesa, tObj );
      if ( t->base.memBlock == NULL ) {
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}

static GLboolean update_tex_common( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const int source = mmesa->tmu_source[unit];
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[source];
   struct gl_texture_object	*tObj = texUnit->_Current;
   mgaTextureObjectPtr t = (mgaTextureObjectPtr) tObj->DriverData;

   /* Fallback if there's a texture border */
   if ( tObj->Image[tObj->BaseLevel]->Border > 0 ) {
      return GL_FALSE;
   }


   /* Update state if this is a different texture object to last
    * time.
    */
   if ( mmesa->CurrentTexObj[unit] != t ) {
      if ( mmesa->CurrentTexObj[unit] != NULL ) {
	 /* The old texture is no longer bound to this texture unit.
	  * Mark it as such.
	  */

	 mmesa->CurrentTexObj[unit]->base.bound &= ~(1UL << unit);
      }

      mmesa->CurrentTexObj[unit] = t;
      t->base.bound |= (1UL << unit);

      driUpdateTextureLRU( (driTextureObject *) t ); /* done too often */
   }

   /* register setup */
   if ( unit == 1 ) {
      mmesa->setup.tdualstage1 = mmesa->setup.tdualstage0;
   }

   t->setup.texctl2 &= TMC_dualtex_MASK;
   if (ctx->Texture._EnabledUnits == 0x03) {
      t->setup.texctl2 |= TMC_dualtex_enable;
   }

   t->texenv_fallback = GL_FALSE;

   /* Set this before mgaUpdateTextureEnvG400() since
    * GL_ARB_texture_env_crossbar may have to disable texturing.
    */
   mmesa->setup.dwgctl &= DC_opcod_MASK;
   mmesa->setup.dwgctl |= DC_opcod_texture_trap;

   /* FIXME: The Radeon has some cached state so that it can avoid calling
    * FIXME: UpdateTextureEnv in some cases.  Is that possible here?
    */
   if (MGA_IS_G400(mmesa)) {
      /* G400: Regardless of texture env mode, we use the alpha from the
       * texture unit (AC_alphasel_fromtex) since it will have already
       * been modulated by the incoming fragment color, if needed.
       * We don't want (AC_alphasel_modulate) since that'll effectively
       * do the modulation twice.
       */
      mmesa->hw.alpha_sel = AC_alphasel_fromtex;

      mgaUpdateTextureEnvG400( ctx, unit );
   } else {
      mmesa->hw.alpha_sel = 0;
      switch (ctx->Texture.Unit[0].EnvMode) {
      case GL_DECAL:
	 mmesa->hw.alpha_sel |= AC_alphasel_diffused;
      case GL_REPLACE:
	 mmesa->hw.alpha_sel |= AC_alphasel_fromtex;
	 break;
      case GL_BLEND:
      case GL_MODULATE:
	 mmesa->hw.alpha_sel |= AC_alphasel_modulated;
	 break;
      default:
	 break;
      }

      mgaUpdateTextureEnvG200( ctx, unit );
   }
   mmesa->dirty |= MGA_UPLOAD_CONTEXT | (MGA_UPLOAD_TEX0 << unit);

   FALLBACK( ctx, MGA_FALLBACK_BORDER_MODE, t->border_fallback );
   return !t->border_fallback && !t->texenv_fallback;
}


static GLboolean updateTextureUnit( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   const int source = mmesa->tmu_source[unit];
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[source];


   if ( texUnit->_ReallyEnabled == TEXTURE_2D_BIT ||
        texUnit->_ReallyEnabled == TEXTURE_RECT_BIT ) {
      return(enable_tex( ctx, unit ) &&
	     update_tex_common( ctx, unit ));
   }
   else if ( texUnit->_ReallyEnabled ) {
      return GL_FALSE;
   }
   else {
      disable_tex( ctx, unit );
      return GL_TRUE;
   }
}

/* The G400 is now programmed quite differently wrt texture environment.
 */
void mgaUpdateTextureState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLboolean ok;
   unsigned  i;

   mmesa->dualtex_env = GL_FALSE;

   /* This works around a quirk with the MGA hardware.  If only OpenGL 
    * TEXTURE1 is enabled, then the hardware TEXTURE0 must be used.  The
    * hardware TEXTURE1 can ONLY be used when hardware TEXTURE0 is also used.
    */

   mmesa->tmu_source[0] = 0;
   mmesa->tmu_source[1] = 1;

   if ((ctx->Texture._EnabledUnits & 0x03) == 0x02) {
      /* only texture 1 enabled */
      mmesa->tmu_source[0] = 1;
      mmesa->tmu_source[1] = 0;
   }

   for ( i = 0, ok = GL_TRUE 
	 ; (i < ctx->Const.MaxTextureUnits) && ok
	 ; i++ ) {
      ok = updateTextureUnit( ctx, i );
   }

   FALLBACK( ctx, MGA_FALLBACK_TEXTURE, !ok );
}
