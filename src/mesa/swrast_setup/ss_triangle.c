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
#include "types.h"

#include "ss_triangle.h"
#include "ss_context.h"

#define SS_FLAT_BIT	    0x1
#define SS_OFFSET_BIT	    0x2	
#define SS_TWOSIDE_BIT	    0x4	
#define SS_UNFILLED_BIT	    0x10	
#define SS_MAX_TRIFUNC      0x20

static triangle_func tri_tab[SS_MAX_TRIFUNC];
static line_func     line_tab[SS_MAX_TRIFUNC];
static points_func   points_tab[SS_MAX_TRIFUNC];
static quad_func     quad_tab[SS_MAX_TRIFUNC];


#define SS_COLOR(a,b) COPY_4UBV(a,b)
#define SS_SPEC(a,b) COPY_4UBV(a,b)
#define SS_IND(a,b) (a = b)

#define IND (0)
#define TAG(x) x
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT)
#define TAG(x) x##_flat
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT)
#define TAG(x) x##_offset
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_OFFSET_BIT)
#define TAG(x) x##_flat_offset
#include "ss_tritmp.h"

#define IND (SS_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_TWOSIDE_BIT)
#define TAG(x) x##_flat_twoside
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_TWOSIDE_BIT)
#define TAG(x) x##_offset_twoside
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_OFFSET_BIT|SS_TWOSIDE_BIT)
#define TAG(x) x##_flat_offset_twoside
#include "ss_tritmp.h"

#define IND (SS_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_flat_unfilled
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_OFFSET_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_flat_offset_unfilled
#include "ss_tritmp.h"

#define IND (SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_flat_twoside_unfilled
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_offset_twoside_unfilled
#include "ss_tritmp.h"

#define IND (SS_FLAT_BIT|SS_OFFSET_BIT|SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_flat_offset_twoside_unfilled
#include "ss_tritmp.h"


void _swsetup_trifuncs_init( GLcontext *ctx )
{
   (void) ctx;

   init();
   init_flat();
   init_offset();
   init_flat_offset();
   init_twoside();
   init_flat_twoside();
   init_offset_twoside();
   init_flat_offset_twoside();
   init_unfilled();
   init_flat_unfilled();
   init_offset_unfilled();
   init_flat_offset_unfilled();
   init_twoside_unfilled();
   init_flat_twoside_unfilled();
   init_offset_twoside_unfilled();
   init_flat_offset_twoside_unfilled();
}


void _swsetup_choose_trifuncs( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   GLuint ind = 0;

   if (ctx->Light.ShadeModel == GL_FLAT)
      ind |= SS_FLAT_BIT;

   if (ctx->Polygon._OffsetAny)
      ind |= SS_OFFSET_BIT;

   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
      ind |= SS_TWOSIDE_BIT;
   
   if (ctx->Polygon._Unfilled) 
      ind |= SS_UNFILLED_BIT;

   swsetup->Triangle = tri_tab[ind];
   swsetup->Line = line_tab[ind];
   swsetup->Points = points_tab[ind];
   swsetup->Quad = quad_tab[ind];
}

