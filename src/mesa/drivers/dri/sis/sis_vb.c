/* $XFree86$ */
/**************************************************************************

Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "sis_context.h"
#include "sis_vb.h"
#include "sis_tris.h"
#include "sis_state.h"


#define SIS_TEX1_BIT       0x1
#define SIS_TEX0_BIT       0x2
#define SIS_RGBA_BIT       0x4
#define SIS_SPEC_BIT       0x8
#define SIS_FOG_BIT	   0x10
#define SIS_XYZW_BIT       0x20
#define SIS_PTEX_BIT       0x40
#define SIS_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[SIS_MAX_SETUP];

#define TEX0_VERTEX_FORMAT	1
#define TEX1_VERTEX_FORMAT	2

#define TINY_VERTEX_FORMAT	0
#define NOTEX_VERTEX_FORMAT	0
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & SIS_XYZW_BIT)
#define DO_RGBA (IND & SIS_RGBA_BIT)
#define DO_SPEC (IND & SIS_SPEC_BIT)
#define DO_FOG  (IND & SIS_FOG_BIT)
#define DO_TEX0 (IND & SIS_TEX0_BIT)
#define DO_TEX1 (IND & SIS_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & SIS_PTEX_BIT)

#define VERTEX sisVertex
#define VERTEX_COLOR sis_color_t
#define LOCALVARS sisContextPtr smesa = SIS_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() smesa->hw_viewport
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() smesa->vertex_format
#define GET_VERTEX_STORE() smesa->verts
#define GET_VERTEX_STRIDE_SHIFT() smesa->vertex_stride_shift
#define GET_UBYTE_COLOR_STORE() &smesa->UbyteColor
#define GET_UBYTE_SPEC_COLOR_STORE() &smesa->UbyteSecondaryColor

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  0
#define HAVE_NOTEX_VERTICES 0
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0

#define UNVIEWPORT_VARS  GLfloat h = SIS_CONTEXT(ctx)->driDrawable->h
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z / smesa->depth_scale

#define PTEX_FALLBACK() FALLBACK(smesa, SIS_FALLBACK_TEXTURE, 1)

#define IMPORT_FLOAT_COLORS sis_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS sis_import_float_spec_colors

#define INTERP_VERTEX setup_tab[smesa->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[smesa->SetupIndex].copy_pv

/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) sis_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_TEX0_BIT|SIS_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT|\
             SIS_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT|\
             SIS_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_TEX0_BIT|\
             SIS_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_TEX0_BIT|\
             SIS_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT|\
             SIS_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT|\
             SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_XYZW_BIT|SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT|\
             SIS_TEX0_BIT|SIS_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_FOG_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_FOG_BIT|SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT|SIS_TEX0_BIT|SIS_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (SIS_RGBA_BIT|SIS_FOG_BIT|SIS_SPEC_BIT|SIS_TEX0_BIT|\
             SIS_TEX1_BIT)
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



void sisPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & SIS_XYZW_BIT)     ? " xyzw," : "",
	   (flags & SIS_RGBA_BIT)     ? " rgba," : "",
	   (flags & SIS_SPEC_BIT)     ? " spec," : "",
	   (flags & SIS_FOG_BIT)      ? " fog," : "",
	   (flags & SIS_TEX0_BIT)     ? " tex-0," : "",
	   (flags & SIS_TEX1_BIT)     ? " tex-1," : "");
}



void sisCheckTexSizes( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );

   if (!setup_tab[smesa->SetupIndex].check_tex_sizes(ctx)) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);

      /* Invalidate stored verts
       */
      smesa->SetupNewInputs = ~0;
      smesa->SetupIndex |= SIS_PTEX_BIT;

      if (!smesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[smesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[smesa->SetupIndex].copy_pv;
      }
   }
}

void sisBuildVertices( GLcontext *ctx,
		       GLuint start,
		       GLuint count,
		       GLuint newinputs )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   GLubyte *v = ((GLubyte *)smesa->verts + (start<<smesa->vertex_stride_shift));
   GLuint stride = 1 << smesa->vertex_stride_shift;

   newinputs |= smesa->SetupNewInputs;
   smesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[smesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= SIS_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= SIS_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= SIS_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= SIS_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= SIS_FOG_BIT;

      if (smesa->SetupIndex & SIS_PTEX_BIT)
	 ind = ~0;

      ind &= smesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void sisChooseVertexState( GLcontext *ctx )
{
  TNLcontext *tnl = TNL_CONTEXT(ctx);
  sisContextPtr smesa = SIS_CONTEXT( ctx );
  GLuint ind = SIS_XYZW_BIT | SIS_RGBA_BIT;

  if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
    ind |= SIS_SPEC_BIT;

  if (ctx->Fog.Enabled)
    ind |= SIS_FOG_BIT;

  if (ctx->Texture._EnabledUnits) {
    ind |= SIS_TEX0_BIT;
    if (ctx->Texture.Unit[0]._ReallyEnabled &&
        ctx->Texture.Unit[1]._ReallyEnabled)
    ind |= SIS_TEX1_BIT;
  }

  smesa->SetupIndex = ind;

  if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE | DD_TRI_UNFILLED)) {
    tnl->Driver.Render.Interp = sis_interp_extras;
    tnl->Driver.Render.CopyPV = sis_copy_pv_extras;
  } else {
    tnl->Driver.Render.Interp = setup_tab[ind].interp;
    tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
  }

  if (setup_tab[ind].vertex_format != smesa->vertex_format) {
    smesa->vertex_format = setup_tab[ind].vertex_format;
    smesa->vertex_size = setup_tab[ind].vertex_size;
    smesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
  }
}


void sisInitVB( GLcontext *ctx )
{
  sisContextPtr smesa = SIS_CONTEXT( ctx );
  GLuint size = TNL_CONTEXT(ctx)->vb.Size;
  static int firsttime = 1;

  smesa->verts = (GLubyte *)ALIGN_MALLOC(size * 4 * 16, 32);

  if (firsttime) {
    init_setup_tab();
    firsttime = 0;
  }
}

void sisFreeVB( GLcontext *ctx )
{
  sisContextPtr smesa = SIS_CONTEXT( ctx );
  if (smesa->verts) {
    ALIGN_FREE(smesa->verts);
    smesa->verts = NULL;
  }


  if (smesa->UbyteSecondaryColor.Ptr) {
    ALIGN_FREE(smesa->UbyteSecondaryColor.Ptr);
    smesa->UbyteSecondaryColor.Ptr = NULL;
  }

  if (smesa->UbyteColor.Ptr) {
    ALIGN_FREE(smesa->UbyteColor.Ptr);
    smesa->UbyteColor.Ptr = NULL;
  }
}
