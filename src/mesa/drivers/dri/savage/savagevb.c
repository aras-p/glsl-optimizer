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
 *    Keith Whitwell <keithw@valinux.com>
 *    Felix Kuehling <fxkuehl@gmx.de>
 */
/* $XFree86$ */

#include "savagecontext.h"
#include "savagevb.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "savagecontext.h"
#include "savage_bci.h"

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "tnl/t_context.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"

#include <stdio.h>
#include <stdlib.h>


#define SAVAGE_TEX1_BIT       0x1
#define SAVAGE_TEX0_BIT       0x2	
#define SAVAGE_RGBA_BIT       0x4
#define SAVAGE_SPEC_BIT       0x8
#define SAVAGE_FOG_BIT	      0x10
#define SAVAGE_XYZW_BIT       0x20
#define SAVAGE_PTEX_BIT       0x40
#define SAVAGE_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[SAVAGE_MAX_SETUP];

/* savage supports vertices without specular color, but this is not supported
 * by the vbtmp. have to check if/how tiny vertices work. */
#define TINY_VERTEX_FORMAT      0
#define NOTEX_VERTEX_FORMAT     (SAVAGE_HW_NO_UV0|SAVAGE_HW_NO_UV1)
#define TEX0_VERTEX_FORMAT      (SAVAGE_HW_NO_UV1)
#define TEX1_VERTEX_FORMAT      (0)
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & SAVAGE_XYZW_BIT)
#define DO_RGBA (IND & SAVAGE_RGBA_BIT)
#define DO_SPEC (IND & SAVAGE_SPEC_BIT)
#define DO_FOG  (IND & SAVAGE_FOG_BIT)
#define DO_TEX0 (IND & SAVAGE_TEX0_BIT)
#define DO_TEX1 (IND & SAVAGE_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & SAVAGE_PTEX_BIT)


#define VERTEX savageVertex
#define LOCALVARS savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() imesa->hw_viewport
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() (imesa->DrawPrimitiveCmd & \
                             (SAVAGE_HW_NO_UV0|SAVAGE_HW_NO_UV1))
#define GET_VERTEX_STORE() imesa->verts
#define GET_VERTEX_SIZE() (imesa->vertex_size * sizeof(int))

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  0
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0

#define UNVIEWPORT_VARS					\
   const GLfloat dx = - imesa->drawX - SUBPIXEL_X;	\
   const GLfloat dy = (imesa->driDrawable->h + 		\
		       imesa->drawY + SUBPIXEL_Y);	\
   const GLfloat sz = 1.0 / imesa->depth_scale

#define UNVIEWPORT_X(x)    x      + dx;
#define UNVIEWPORT_Y(y)  - y      + dy;
#define UNVIEWPORT_Z(z)    z * sz;

#define PTEX_FALLBACK() (void)imesa, FALLBACK(ctx, SAVAGE_FALLBACK_TEXTURE, 1)


#define INTERP_VERTEX (void)imesa, setup_tab[SAVAGE_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX (void)imesa, setup_tab[SAVAGE_CONTEXT(ctx)->SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) savage_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_TEX0_BIT|SAVAGE_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT|SAVAGE_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SAVAGE_RGBA_BIT|SAVAGE_FOG_BIT|SAVAGE_SPEC_BIT|SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT)
#define TAG(x) x##_gfst0t1
#include "tnl_dd/t_dd_vbtmp.h"


static void init_setup_tab( void )
{
   init_wg();
   init_wgs();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgst0();
   init_wgst0t1();
   init_wgspt0();
   init_wgf();
   init_wgfs();
   init_wgft0();
   init_wgft0t1();
   init_wgfpt0();
   init_wgfst0();
   init_wgfst0t1();
   init_wgfspt0();
   init_t0();
   init_t0t1();
   init_f();
   init_ft0();
   init_ft0t1();
   init_g();
   init_gs();
   init_gt0();
   init_gt0t1();
   init_gst0();
   init_gst0t1();
   init_gf();
   init_gfs();
   init_gft0();
   init_gft0t1();
   init_gfst0();
   init_gfst0t1();
}




void savagePrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s: %d %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & SAVAGE_XYZW_BIT)     ? " xyzw," : "", 
	   (flags & SAVAGE_RGBA_BIT)     ? " rgba," : "",
	   (flags & SAVAGE_SPEC_BIT)     ? " spec," : "",
	   (flags & SAVAGE_FOG_BIT)      ? " fog," : "",
	   (flags & SAVAGE_TEX0_BIT)     ? " tex-0," : "",
	   (flags & SAVAGE_TEX1_BIT)     ? " tex-1," : "");
}


void savageCheckTexSizes( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /*fprintf(stderr, "%s\n", __FUNCTION__);*/

   if (!setup_tab[imesa->SetupIndex].check_tex_sizes(ctx)) {
      imesa->SetupIndex |= SAVAGE_PTEX_BIT;
      imesa->SetupNewInputs = ~0;

      if (!imesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[imesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[imesa->SetupIndex].copy_pv;
      }
      if (imesa->Fallback)
	 tnl->Driver.Render.Start(ctx);
   }
}


void savageBuildVertices( GLcontext *ctx, 
			  GLuint start, 
			  GLuint count,
			  GLuint newinputs )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   GLuint stride = imesa->vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)imesa->verts + (start * stride));

   newinputs |= imesa->SetupNewInputs;
   imesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[imesa->SetupIndex].emit( ctx, start, count, v, stride );   
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= SAVAGE_RGBA_BIT;
      
      if (newinputs & VERT_BIT_COLOR1)
	 ind |= SAVAGE_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= SAVAGE_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= SAVAGE_TEX0_BIT|SAVAGE_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= SAVAGE_FOG_BIT;

      if (imesa->SetupIndex & SAVAGE_PTEX_BIT)
	 ind = ~0;

      ind &= imesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );   
      }
   }
}


void savageChooseVertexState( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = SAVAGE_XYZW_BIT|SAVAGE_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) 
      ind |= SAVAGE_SPEC_BIT;

   if (ctx->Fog.Enabled) 
      ind |= SAVAGE_FOG_BIT;
   
   if (ctx->Texture._EnabledUnits & 0x2) {
      if (ctx->Texture._EnabledUnits & 0x1) {
	 ind |= SAVAGE_TEX1_BIT|SAVAGE_TEX0_BIT;
      }
      else {
	 ind |= SAVAGE_TEX0_BIT;
      }
   }
   else if (ctx->Texture._EnabledUnits & 0x1) {
      ind |= SAVAGE_TEX0_BIT;
   }
   
   imesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = savage_interp_extras;
      tnl->Driver.Render.CopyPV = savage_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != GET_VERTEX_FORMAT()) {
      imesa->DrawPrimitiveCmd = setup_tab[ind].vertex_format;
      imesa->vertex_size = setup_tab[ind].vertex_size;
   }
}



void savageInitVB( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   imesa->verts = (GLubyte *)ALIGN_MALLOC(size * sizeof(savageVertex), 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }

   imesa->DrawPrimitiveCmd = setup_tab[0].vertex_format;
   imesa->vertex_size = setup_tab[0].vertex_size;

   if (imesa->savageScreen->chipset >= S3_SAVAGE4)
       imesa->DrawPrimitiveMask = ~0;
   else
       imesa->DrawPrimitiveMask = ~SAVAGE_HW_NO_UV1;
}


void savageFreeVB( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   if (imesa->verts) {
      ALIGN_FREE(imesa->verts);
      imesa->verts = 0;
   }
}

