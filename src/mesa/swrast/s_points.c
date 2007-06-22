/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "texstate.h"
#include "s_context.h"
#include "s_feedback.h"
#include "s_points.h"
#include "s_span.h"


#define RGBA       0x1
#define INDEX      0x2
#define SMOOTH     0x4
#define ATTRIBS    0x8
#define SPECULAR  0x10
#define LARGE     0x20
#define ATTENUATE 0x40
#define SPRITE    0x80


/*
 * CI points with size == 1.0
 */
#define FLAGS (INDEX)
#define NAME size1_ci_point
#include "s_pointtemp.h"


/*
 * General CI points.
 */
#define FLAGS (INDEX | LARGE)
#define NAME general_ci_point
#include "s_pointtemp.h"


/*
 * Antialiased CI points.
 */
#define FLAGS (INDEX | SMOOTH)
#define NAME antialiased_ci_point
#include "s_pointtemp.h"


/*
 * Distance attenuated, general CI points.
 */
#define FLAGS (INDEX | ATTENUATE)
#define NAME atten_general_ci_point
#include "s_pointtemp.h"


/*
 * RGBA points with size == 1.0
 */
#define FLAGS (RGBA)
#define NAME size1_rgba_point
#include "s_pointtemp.h"


/*
 * General RGBA points.
 */
#define FLAGS (RGBA | LARGE)
#define NAME general_rgba_point
#include "s_pointtemp.h"


/*
 * Antialiased RGBA points.
 */
#define FLAGS (RGBA | SMOOTH)
#define NAME antialiased_rgba_point
#include "s_pointtemp.h"


/*
 * Textured RGBA points.
 */
#define FLAGS (RGBA | LARGE | ATTRIBS | SPECULAR)
#define NAME textured_rgba_point
#include "s_pointtemp.h"


/*
 * Antialiased points with texture mapping.
 */
#define FLAGS (RGBA | SMOOTH | ATTRIBS | SPECULAR)
#define NAME antialiased_tex_rgba_point
#include "s_pointtemp.h"


/*
 * Distance attenuated, general RGBA points.
 */
#define FLAGS (RGBA | ATTENUATE)
#define NAME atten_general_rgba_point
#include "s_pointtemp.h"


/*
 * Distance attenuated, textured RGBA points.
 */
#define FLAGS (RGBA | ATTENUATE | ATTRIBS | SPECULAR)
#define NAME atten_textured_rgba_point
#include "s_pointtemp.h"


/*
 * Distance attenuated, antialiased points with or without texture mapping.
 */
#define FLAGS (RGBA | ATTENUATE | ATTRIBS | SMOOTH)
#define NAME atten_antialiased_rgba_point
#include "s_pointtemp.h"


/*
 * Sprite (textured point)
 */
#define FLAGS (RGBA | SPRITE | SPECULAR)
#define NAME sprite_point
#include "s_pointtemp.h"


#define FLAGS (RGBA | SPRITE | SPECULAR | ATTENUATE)
#define NAME atten_sprite_point
#include "s_pointtemp.h"


void
_swrast_add_spec_terms_point(GLcontext *ctx, const SWvertex *v0)
{
   SWvertex *ncv0 = (SWvertex *) v0;
   GLfloat rSum, gSum, bSum;
   GLchan cSave[4];

   /* save */
   COPY_CHAN4( cSave, ncv0->color );
   /* sum */
   rSum = CHAN_TO_FLOAT(ncv0->color[0]) + ncv0->attrib[FRAG_ATTRIB_COL1][0];
   gSum = CHAN_TO_FLOAT(ncv0->color[1]) + ncv0->attrib[FRAG_ATTRIB_COL1][1];
   bSum = CHAN_TO_FLOAT(ncv0->color[2]) + ncv0->attrib[FRAG_ATTRIB_COL1][2];
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[0], rSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[1], gSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[2], bSum);
   /* draw */
   SWRAST_CONTEXT(ctx)->SpecPoint(ctx, ncv0);
   /* restore */
   COPY_CHAN4(ncv0->color, cSave);
}



/* record the current point function name */
#ifdef DEBUG

static const char *pntFuncName = NULL;

#define USE(pntFunc)                   \
do {                                   \
    pntFuncName = #pntFunc;            \
    /*printf("%s\n", pntFuncName);*/   \
    swrast->Point = pntFunc;           \
} while (0)

#else

#define USE(pntFunc)  swrast->Point = pntFunc

#endif


/*
 * Examine the current context to determine which point drawing function
 * should be used.
 */
void
_swrast_choose_point( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLboolean rgbMode = ctx->Visual.rgbMode;
   GLboolean specular = (ctx->Fog.ColorSumEnabled ||
                         (ctx->Light.Enabled &&
                          ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR));
   GLboolean attribs = (ctx->FragmentProgram._Current ||
                        ctx->Texture._EnabledCoordUnits ||
                        swrast->_FogEnabled ||
                        specular);

   /*
    * XXX this is a mess that should be cleaned up someday
    */

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->Point.PointSprite) {
         /* GL_ARB_point_sprite / GL_NV_point_sprite */
         /* XXX this might not be good enough */
         if (ctx->Point._Attenuated)
            USE(atten_sprite_point);
         else
            USE(sprite_point);
      }
      else if (ctx->Point.SmoothFlag && !attribs) {
         /* Smooth points */
         if (rgbMode) {
            if (ctx->Point._Attenuated || ctx->VertexProgram.PointSizeEnabled) {
               USE(atten_antialiased_rgba_point);
            }
            else if (ctx->Texture._EnabledCoordUnits) {
               USE(antialiased_tex_rgba_point);
            }
            else {
               USE(antialiased_rgba_point);
            }
         }
         else {
            USE(antialiased_ci_point);
         }
      }
      else if (ctx->Point._Attenuated || ctx->VertexProgram.PointSizeEnabled) {
         if (rgbMode) {
            if (attribs) {
               if (ctx->Point.SmoothFlag) {
                  USE(atten_antialiased_rgba_point);
               }
               else {
                  USE(atten_textured_rgba_point);
               }
            }
            else {
               USE(atten_general_rgba_point);
            }
         }
         else {
            /* ci, atten */
            USE(atten_general_ci_point);
         }
      }
      else if (attribs && rgbMode) {
         /* textured, fogged */
         USE(textured_rgba_point);
      }
      else if (ctx->Point._Size != 1.0) {
         /* large points */
         if (rgbMode) {
            USE(general_rgba_point);
         }
         else {
            USE(general_ci_point);
         }
      }
      else {
         /* single pixel points */
         if (rgbMode) {
            assert((swrast->_ActiveAttribMask & FRAG_BIT_COL1) == 0);
            USE(size1_rgba_point);
         }
         else {
            USE(size1_ci_point);
         }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      USE(_swrast_feedback_point);
   }
   else {
      /* GL_SELECT mode */
      USE(_swrast_select_point);
   }
}
