/* $Id: ss_vb.c,v 1.12 2001/03/29 21:16:26 keithw Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "colormac.h"
#include "macros.h"

#include "swrast/swrast.h"

#include "tnl/t_context.h"

#include "math/m_vector.h"
#include "ss_context.h"
#include "ss_vb.h"



/* Provides a RasterSetup function which prebuilds vertices for the
 * software rasterizer.  This is required for the triangle functions
 * in this module, but not the rest of the swrast module.
 */

typedef void (*SetupFunc)( GLcontext *ctx,
			   GLuint start, GLuint end, GLuint newinputs );

#define COLOR         0x1
#define INDEX         0x2
#define TEX0          0x4
#define MULTITEX      0x8
#define SPEC          0x10
#define FOG           0x20
#define POINT         0x40
#define MAX_SETUPFUNC 0x80

static SetupFunc setup_func[MAX_SETUPFUNC];


#define IND (0)
#define TAG(x) x##_none
#include "ss_vbtmp.h"

#define IND (COLOR)
#define TAG(x) x##_color
#include "ss_vbtmp.h"

#define IND (COLOR|SPEC)
#define TAG(x) x##_color_spec
#include "ss_vbtmp.h"

#define IND (COLOR|FOG)
#define TAG(x) x##_color_fog
#include "ss_vbtmp.h"

#define IND (COLOR|SPEC|FOG)
#define TAG(x) x##_color_spec_fog
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0)
#define TAG(x) x##_color_tex0
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|SPEC)
#define TAG(x) x##_color_tex0_spec
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|FOG)
#define TAG(x) x##_color_tex0_fog
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|SPEC|FOG)
#define TAG(x) x##_color_tex0_spec_fog
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX)
#define TAG(x) x##_color_multitex
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|SPEC)
#define TAG(x) x##_color_multitex_spec
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|FOG)
#define TAG(x) x##_color_multitex_fog
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|SPEC|FOG)
#define TAG(x) x##_color_multitex_spec_fog
#include "ss_vbtmp.h"

#define IND (COLOR|POINT)
#define TAG(x) x##_color_point
#include "ss_vbtmp.h"

#define IND (COLOR|SPEC|POINT)
#define TAG(x) x##_color_spec_point
#include "ss_vbtmp.h"

#define IND (COLOR|FOG|POINT)
#define TAG(x) x##_color_fog_point
#include "ss_vbtmp.h"

#define IND (COLOR|SPEC|FOG|POINT)
#define TAG(x) x##_color_spec_fog_point
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|POINT)
#define TAG(x) x##_color_tex0_point
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|SPEC|POINT)
#define TAG(x) x##_color_tex0_spec_point
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|FOG|POINT)
#define TAG(x) x##_color_tex0_fog_point
#include "ss_vbtmp.h"

#define IND (COLOR|TEX0|SPEC|FOG|POINT)
#define TAG(x) x##_color_tex0_spec_fog_point
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|POINT)
#define TAG(x) x##_color_multitex_point
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|SPEC|POINT)
#define TAG(x) x##_color_multitex_spec_point
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|FOG|POINT)
#define TAG(x) x##_color_multitex_fog_point
#include "ss_vbtmp.h"

#define IND (COLOR|MULTITEX|SPEC|FOG|POINT)
#define TAG(x) x##_color_multitex_spec_fog_point
#include "ss_vbtmp.h"

#define IND (INDEX)
#define TAG(x) x##_index
#include "ss_vbtmp.h"

#define IND (INDEX|TEX0)
#define TAG(x) x##_index_tex0
#include "ss_vbtmp.h"

#define IND (INDEX|FOG)
#define TAG(x) x##_index_fog
#include "ss_vbtmp.h"

#define IND (INDEX|TEX0|FOG)
#define TAG(x) x##_index_tex0_fog
#include "ss_vbtmp.h"

#define IND (INDEX|POINT)
#define TAG(x) x##_index_point
#include "ss_vbtmp.h"

#define IND (INDEX|TEX0|POINT)
#define TAG(x) x##_index_tex0_point
#include "ss_vbtmp.h"

#define IND (INDEX|FOG|POINT)
#define TAG(x) x##_index_fog_point
#include "ss_vbtmp.h"

#define IND (INDEX|TEX0|FOG|POINT)
#define TAG(x) x##_index_tex0_fog_point
#include "ss_vbtmp.h"


static void
rs_invalid( GLcontext *ctx, GLuint start, GLuint end, GLuint newinputs )
{
   fprintf(stderr, "swrast_setup: invalid setup function\n");
   (void) (ctx && start && end && newinputs);
}

void
_swsetup_vb_init( GLcontext *ctx )
{
   GLuint i;
   (void) ctx;

   for (i = 0 ; i < Elements(setup_func) ; i++)
      setup_func[i] = rs_invalid;

   setup_func[0] = rs_none;
   setup_func[COLOR] = rs_color;
   setup_func[COLOR|SPEC] = rs_color_spec;
   setup_func[COLOR|FOG] = rs_color_fog;
   setup_func[COLOR|SPEC|FOG] = rs_color_spec_fog;
   setup_func[COLOR|TEX0] = rs_color_tex0;
   setup_func[COLOR|TEX0|SPEC] = rs_color_tex0_spec;
   setup_func[COLOR|TEX0|FOG] = rs_color_tex0_fog;
   setup_func[COLOR|TEX0|SPEC|FOG] = rs_color_tex0_spec_fog;
   setup_func[COLOR|MULTITEX] = rs_color_multitex;
   setup_func[COLOR|MULTITEX|SPEC] = rs_color_multitex_spec;
   setup_func[COLOR|MULTITEX|FOG] = rs_color_multitex_fog;
   setup_func[COLOR|MULTITEX|SPEC|FOG] = rs_color_multitex_spec_fog;
   setup_func[COLOR|POINT] = rs_color_point;
   setup_func[COLOR|SPEC|POINT] = rs_color_spec_point;
   setup_func[COLOR|FOG|POINT] = rs_color_fog_point;
   setup_func[COLOR|SPEC|FOG|POINT] = rs_color_spec_fog_point;
   setup_func[COLOR|TEX0|POINT] = rs_color_tex0_point;
   setup_func[COLOR|TEX0|SPEC|POINT] = rs_color_tex0_spec_point;
   setup_func[COLOR|TEX0|FOG|POINT] = rs_color_tex0_fog_point;
   setup_func[COLOR|TEX0|SPEC|FOG|POINT] = rs_color_tex0_spec_fog_point;
   setup_func[COLOR|MULTITEX|POINT] = rs_color_multitex_point;
   setup_func[COLOR|MULTITEX|SPEC|POINT] = rs_color_multitex_spec_point;
   setup_func[COLOR|MULTITEX|FOG|POINT] = rs_color_multitex_fog_point;
   setup_func[COLOR|MULTITEX|SPEC|FOG|POINT] = rs_color_multitex_spec_fog_point;
   setup_func[INDEX] = rs_index;
   setup_func[INDEX|TEX0] = rs_index_tex0;
   setup_func[INDEX|FOG] = rs_index_fog;
   setup_func[INDEX|TEX0|FOG] = rs_index_tex0_fog;
   setup_func[INDEX|POINT] = rs_index_point;
   setup_func[INDEX|TEX0|POINT] = rs_index_tex0_point;
   setup_func[INDEX|FOG|POINT] = rs_index_fog_point;
   setup_func[INDEX|TEX0|FOG|POINT] = rs_index_tex0_fog_point;
}

static void printSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & COLOR) ? "color, " : "",
	   (flags & INDEX) ? "index, " : "",
	   (flags & TEX0) ? "tex0, " : "",
	   (flags & MULTITEX) ? "multitex, " : "",
	   (flags & SPEC) ? "spec, " : "",
	   (flags & FOG) ? "fog, " : "",
	   (flags & POINT) ? "point, " : "");
}


void
_swsetup_choose_rastersetup_func(GLcontext *ctx)
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   int funcindex = 0;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Visual.rgbMode) {
         funcindex = COLOR;

         if (ctx->Texture._ReallyEnabled & ~0xf)
            funcindex |= MULTITEX;
         else if (ctx->Texture._ReallyEnabled & 0xf)
            funcindex |= TEX0;

         if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ||
             ctx->Fog.ColorSumEnabled)
            funcindex |= SPEC;
      }
      else {
         funcindex = INDEX;
      }

      if (ctx->Point._Attenuated)
         funcindex |= POINT;

      if (ctx->Fog.Enabled)
	 funcindex |= FOG;
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      if (ctx->Visual.rgbMode)
	 funcindex = (COLOR | TEX0); /* is feedback color subject to fogging? */
      else
	 funcindex = (INDEX | TEX0);
   }
   else
      funcindex = 0;

   if (0) printSetupFlags("software setup func", funcindex); 
   swsetup->BuildProjVerts = setup_func[funcindex];
   ASSERT(setup_func[funcindex] != rs_invalid);
}
