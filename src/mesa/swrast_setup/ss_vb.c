/* $Id: ss_vb.c,v 1.22 2002/10/29 20:29:00 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"

#include "swrast/swrast.h"
#include "tnl/t_context.h"
#include "math/m_vector.h"
#include "math/m_translate.h"

#include "ss_context.h"
#include "ss_vb.h"

static void do_import( struct vertex_buffer *VB,
		       struct gl_client_array *to,
		       struct gl_client_array *from )
{
   GLuint count = VB->Count;

   if (!to->Ptr) {
      to->Ptr = ALIGN_MALLOC( VB->Size * 4 * sizeof(GLchan), 32 );
      to->Type = CHAN_TYPE;
   }

   /* No need to transform the same value 3000 times.
    */
   if (!from->StrideB) {
      to->StrideB = 0;
      count = 1;
   }
   else
      to->StrideB = 4 * sizeof(GLchan);
   
   _math_trans_4chan( (GLchan (*)[4]) to->Ptr,
		      from->Ptr,
		      from->StrideB,
		      from->Type,
		      from->Size,
		      0,
		      count);
}

static void import_float_colors( GLcontext *ctx )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct gl_client_array *to = &SWSETUP_CONTEXT(ctx)->ChanColor;
   do_import( VB, to, VB->ColorPtr[0] );
   VB->ColorPtr[0] = to;
}

static void import_float_spec_colors( GLcontext *ctx )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct gl_client_array *to = &SWSETUP_CONTEXT(ctx)->ChanSecondaryColor;
   do_import( VB, to, VB->SecondaryColorPtr[0] );
   VB->SecondaryColorPtr[0] = to;
}


/* Provides a RasterSetup function which prebuilds vertices for the
 * software rasterizer.  This is required for the triangle functions
 * in this module, but not the rest of the swrast module.
 */


#define COLOR         0x1
#define INDEX         0x2
#define TEX0          0x4
#define MULTITEX      0x8
#define SPEC          0x10
#define FOG           0x20
#define POINT         0x40
#define MAX_SETUPFUNC 0x80

static setup_func setup_tab[MAX_SETUPFUNC];
static interp_func interp_tab[MAX_SETUPFUNC];
static copy_pv_func copy_pv_tab[MAX_SETUPFUNC];


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

#define IND (INDEX|FOG)
#define TAG(x) x##_index_fog
#include "ss_vbtmp.h"

#define IND (INDEX|POINT)
#define TAG(x) x##_index_point
#include "ss_vbtmp.h"

#define IND (INDEX|FOG|POINT)
#define TAG(x) x##_index_fog_point
#include "ss_vbtmp.h"


/***********************************************************************
 *      Additional setup and interp for back color and edgeflag. 
 ***********************************************************************/

#define GET_COLOR(ptr, idx) (((GLchan (*)[4])((ptr)->Ptr))[idx])

static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      INTERP_4CHAN( t,
		 GET_COLOR(VB->ColorPtr[1], dst),
		 GET_COLOR(VB->ColorPtr[1], out),
		 GET_COLOR(VB->ColorPtr[1], in) );

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3CHAN( t,
		    GET_COLOR(VB->SecondaryColorPtr[1], dst),
		    GET_COLOR(VB->SecondaryColorPtr[1], out),
		    GET_COLOR(VB->SecondaryColorPtr[1], in) );
      }
   }
   else if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst] = (GLuint) (GLint)
	 LINTERP( t,
		  (GLfloat) VB->IndexPtr[1]->data[out],
		  (GLfloat) VB->IndexPtr[1]->data[in] );
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   interp_tab[SWSETUP_CONTEXT(ctx)->SetupIndex](ctx, t, dst, out, in,
						force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
	 COPY_CHAN4( GET_COLOR(VB->ColorPtr[1], dst), 
		     GET_COLOR(VB->ColorPtr[1], src) );
	 
	 if (VB->SecondaryColorPtr[1]) {
	    COPY_3V( GET_COLOR(VB->SecondaryColorPtr[1], dst), 
		     GET_COLOR(VB->SecondaryColorPtr[1], src) );
	 }
   }
   else if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst] = VB->IndexPtr[1]->data[src];
   }

   copy_pv_tab[SWSETUP_CONTEXT(ctx)->SetupIndex](ctx, dst, src);
}




/***********************************************************************
 *                         Initialization 
 ***********************************************************************/

static void
emit_invalid( GLcontext *ctx, GLuint start, GLuint end, GLuint newinputs )
{
   _mesa_debug(ctx, "swrast_setup: invalid setup function\n");
   (void) (ctx && start && end && newinputs);
}


static void 
interp_invalid( GLcontext *ctx, GLfloat t,
		GLuint edst, GLuint eout, GLuint ein,
		GLboolean force_boundary )
{
   _mesa_debug(ctx, "swrast_setup: invalid interp function\n");
   (void) (ctx && t && edst && eout && ein && force_boundary);
}


static void 
copy_pv_invalid( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   _mesa_debug(ctx, "swrast_setup: invalid copy_pv function\n");
   (void) (ctx && edst && esrc );
}


static void init_standard( void )
{
   GLuint i;

   for (i = 0 ; i < Elements(setup_tab) ; i++) {
      setup_tab[i] = emit_invalid;
      interp_tab[i] = interp_invalid;
      copy_pv_tab[i] = copy_pv_invalid;
   }

   init_none();
   init_color();
   init_color_spec();
   init_color_fog();
   init_color_spec_fog();
   init_color_tex0();
   init_color_tex0_spec();
   init_color_tex0_fog();
   init_color_tex0_spec_fog();
   init_color_multitex();
   init_color_multitex_spec();
   init_color_multitex_fog();
   init_color_multitex_spec_fog();
   init_color_point();
   init_color_spec_point();
   init_color_fog_point();
   init_color_spec_fog_point();
   init_color_tex0_point();
   init_color_tex0_spec_point();
   init_color_tex0_fog_point();
   init_color_tex0_spec_fog_point();
   init_color_multitex_point();
   init_color_multitex_spec_point();
   init_color_multitex_fog_point();
   init_color_multitex_spec_fog_point();
   init_index();
   init_index_fog();
   init_index_point();
   init_index_fog_point();
}


/* debug only */
#if 0
static void
printSetupFlags(const GLcontext *ctx, char *msg, GLuint flags )
{
   _mesa_debug(ctx, "%s(%x): %s%s%s%s%s%s%s\n",
               msg,
               (int) flags,
               (flags & COLOR) ? "color, " : "",
               (flags & INDEX) ? "index, " : "",
               (flags & TEX0) ? "tex0, " : "",
               (flags & MULTITEX) ? "multitex, " : "",
               (flags & SPEC) ? "spec, " : "",
               (flags & FOG) ? "fog, " : "",
               (flags & POINT) ? "point, " : "");
}
#endif

void
_swsetup_choose_rastersetup_func(GLcontext *ctx)
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   int funcindex = 0;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Visual.rgbMode) {
         funcindex = COLOR;

         if (ctx->Texture._EnabledUnits > 1)
            funcindex |= MULTITEX; /* a unit above unit[0] is enabled */
         else if (ctx->Texture._EnabledUnits == 1)
            funcindex |= TEX0;  /* only unit 0 is enabled */

         if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
            funcindex |= SPEC;
      }
      else {
         funcindex = INDEX;
      }

      if (ctx->Point._Attenuated ||
          (ctx->VertexProgram.Enabled && ctx->VertexProgram.PointSizeEnabled))
         funcindex |= POINT;

      if (ctx->Fog.Enabled)
	 funcindex |= FOG;
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      if (ctx->Visual.rgbMode)
	 funcindex = (COLOR | TEX0); /* is feedback color subject to fogging? */
      else
	 funcindex = INDEX;
   }
   else
      funcindex = 0;
   
   swsetup->SetupIndex = funcindex;
   tnl->Driver.Render.BuildVertices = setup_tab[funcindex];

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = interp_extras;
      tnl->Driver.Render.CopyPV = copy_pv_extras;
   }
   else {
      tnl->Driver.Render.Interp = interp_tab[funcindex];
      tnl->Driver.Render.CopyPV = copy_pv_tab[funcindex];
   }

   ASSERT(tnl->Driver.Render.BuildVertices);
   ASSERT(tnl->Driver.Render.BuildVertices != emit_invalid);
}


void
_swsetup_vb_init( GLcontext *ctx )
{
   (void) ctx;
   init_standard();
   /*
   printSetupFlags(ctx);
   */
}
   
