/* $Id: s_points.c,v 1.15 2001/03/12 00:48:42 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "texstate.h"
#include "s_context.h"
#include "s_feedback.h"
#include "s_pb.h"
#include "s_points.h"
#include "s_span.h"



#define INDEX      0x0
#define RGBA       0x1
#define SMOOTH     0x2
#define TEXTURE    0x4
#define SPECULAR   0x8
#define LARGE     0x10
#define ATTENUATE 0x20
#define SPRITE    0x40


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
#define FLAGS (RGBA | LARGE | TEXTURE | SPECULAR)
#define NAME textured_rgba_point
#include "s_pointtemp.h"


/*
 * Antialiased points with texture mapping.
 */
#define FLAGS (RGBA | SMOOTH | TEXTURE | SPECULAR)
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
#define FLAGS (RGBA | ATTENUATE | TEXTURE | SPECULAR)
#define NAME atten_textured_rgba_point
#include "s_pointtemp.h"


/*
 * Distance attenuated, antialiased points with or without texture mapping.
 */
#define FLAGS (RGBA | ATTENUATE | TEXTURE | SMOOTH)
#define NAME atten_antialiased_rgba_point
#include "s_pointtemp.h"


/*
 * Sprite (textured point)
 */
#define FLAGS (RGBA | TEXTURE | SPRITE)
#define NAME sprite_point
#include "s_pointtemp.h"


#define FLAGS (RGBA | ATTENUATE | TEXTURE | SPRITE)
#define NAME atten_sprite_point
#include "s_pointtemp.h"



void _swrast_add_spec_terms_point( GLcontext *ctx,
				   const SWvertex *v0 )
{
   SWvertex *ncv0 = (SWvertex *)v0;
   GLchan c[1][4];
   COPY_CHAN4( c[0], ncv0->color );
   ACC_3V( ncv0->color, ncv0->specular );
   SWRAST_CONTEXT(ctx)->SpecPoint( ctx, ncv0 );
   COPY_CHAN4( ncv0->color, c[0] );
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

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->Point.SpriteMode) {
         /* XXX this might not be good enough */
         if (ctx->Point._Attenuated)
            USE(atten_sprite_point);
         else
            USE(sprite_point);
      }
      else if (ctx->Point.SmoothFlag) {
         /* Smooth points */
         if (rgbMode) {
            if (ctx->Point._Attenuated) {
               USE(atten_antialiased_rgba_point);
            }
            else if (ctx->Texture._ReallyEnabled) {
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
      else if (ctx->Point._Attenuated) {
         if (rgbMode) {
            if (ctx->Texture._ReallyEnabled) {
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
      else if (ctx->Texture._ReallyEnabled && rgbMode) {
         /* textured */
         USE(textured_rgba_point);
      }
      else if (ctx->Point.Size != 1.0) {
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
            USE(size1_rgba_point);
         }
         else {
            USE(size1_ci_point);
         }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      USE(_mesa_feedback_point);
   }
   else {
      /* GL_SELECT mode */
      USE(_mesa_select_point);
   }
}
