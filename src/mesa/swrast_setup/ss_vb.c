/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "macros.h"
#include "stages.h"

#include "swrast/swrast.h"

#include "ss_context.h"
#include "ss_vb.h"



/* Provides a RasterSetup function which prebuilds vertices for the
 * software rasterizer.  This is required for the triangle functions
 * in this module, but not the rest of the swrast module.
 */

typedef void (*SetupFunc)( struct vertex_buffer *VB, 
				 GLuint start, GLuint end );

#define COLOR         0x1
#define INDEX         0x2
#define TEX0          0x4
#define MULTITEX      0x8
#define SPEC          0x10
#define FOG           0x20
#define EYE           0x40
#define MAX_SETUPFUNC 0x80

static SetupFunc setup_func[MAX_SETUPFUNC];


#define IND (COLOR)
#define TAG(x) x##_color
#include "ss_vbtmp.h"

#define IND (INDEX)
#define TAG(x) x##_index
#include "ss_vbtmp.h"

#define IND (TEX0|COLOR)
#define TAG(x) x##_tex0_color
#include "ss_vbtmp.h"

#define IND (TEX0|COLOR|SPEC)
#define TAG(x) x##_tex0_color_spec
#include "ss_vbtmp.h"

#define IND (TEX0|COLOR|SPEC|FOG)
#define TAG(x) x##_tex0_color_spec_fog
#include "ss_vbtmp.h"

#define IND (MULTITEX|COLOR)
#define TAG(x) x##_multitex_color
#include "ss_vbtmp.h"

#define IND (MULTITEX|COLOR|SPEC|FOG)
#define TAG(x) x##_multitex_color_spec_fog
#include "ss_vbtmp.h"

#define IND (TEX0|COLOR|EYE)
#define TAG(x) x##_tex0_color_eye
#include "ss_vbtmp.h"

#define IND (MULTITEX|COLOR|SPEC|INDEX|EYE|FOG)
#define TAG(x) x##_multitex_color_spec_index_eye_fog
#include "ss_vbtmp.h"

#define IND (COLOR|INDEX|TEX0)
#define TAG(x) x##_selection_feedback
#include "ss_vbtmp.h"



void 
_swsetup_vb_init( GLcontext *ctx )
{
   int i;
   (void) ctx;

   for (i = 0 ; i < Elements(setup_func) ; i++)
      setup_func[i] = rs_multitex_color_spec_index_eye_fog;

   /* Some specialized cases:
    */
   setup_func[0]     = rs_color;
   setup_func[COLOR] = rs_color;

   setup_func[INDEX] = rs_index;
   
   setup_func[TEX0]       = rs_tex0_color;
   setup_func[TEX0|COLOR] = rs_tex0_color;

   setup_func[SPEC]            = rs_tex0_color_spec;
   setup_func[COLOR|SPEC]      = rs_tex0_color_spec;
   setup_func[TEX0|SPEC]       = rs_tex0_color_spec;
   setup_func[TEX0|COLOR|SPEC] = rs_tex0_color_spec;

   setup_func[MULTITEX]       = rs_multitex_color;
   setup_func[MULTITEX|COLOR] = rs_multitex_color;

   setup_func[FOG]                      = rs_tex0_color_spec_fog;
   setup_func[COLOR|FOG]                = rs_tex0_color_spec_fog;
   setup_func[SPEC|FOG]                 = rs_tex0_color_spec_fog;
   setup_func[COLOR|SPEC|FOG]           = rs_tex0_color_spec_fog;
   setup_func[TEX0|FOG]                 = rs_tex0_color_spec_fog;
   setup_func[TEX0|COLOR|FOG]           = rs_tex0_color_spec_fog;
   setup_func[TEX0|SPEC|FOG]            = rs_tex0_color_spec_fog;
   setup_func[TEX0|COLOR|SPEC|FOG]      = rs_tex0_color_spec_fog;

   setup_func[MULTITEX|SPEC]            = rs_multitex_color_spec_fog;
   setup_func[MULTITEX|COLOR|SPEC]      = rs_multitex_color_spec_fog;
   setup_func[MULTITEX|FOG]             = rs_multitex_color_spec_fog;
   setup_func[MULTITEX|SPEC|FOG]        = rs_multitex_color_spec_fog;
   setup_func[MULTITEX|COLOR|SPEC|FOG]  = rs_multitex_color_spec_fog;

   setup_func[TEX0|EYE]       = rs_tex0_color_eye;
   setup_func[TEX0|COLOR|EYE] = rs_tex0_color_eye;

   setup_func[COLOR|INDEX|TEX0] = rs_selection_feedback;
}


void 
_swsetup_choose_rastersetup_func(GLcontext *ctx)
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   int funcindex;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Visual.RGBAflag) {
         funcindex = COLOR;

         if (ctx->Texture._ReallyEnabled & ~0xf) 
            funcindex |= MULTITEX;
         else if (ctx->Texture._ReallyEnabled & 0xf) 
            funcindex |= TEX0;

         if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ||
             ctx->Fog.ColorSumEnabled)
            funcindex |= SPEC;

         if (ctx->Point._Attenuated)
            funcindex |= EYE;
      }
      else {
         funcindex = INDEX;
      }
   }
   else {
      /* feedback or section */
      funcindex = (COLOR | INDEX | TEX0);
   }

   swsetup->RasterSetup = setup_func[funcindex];
}

