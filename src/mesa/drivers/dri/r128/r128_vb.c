/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_vb.c,v 1.16 2003/03/26 20:43:49 tsi Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "r128_context.h"
#include "r128_vb.h"
#include "r128_ioctl.h"
#include "r128_tris.h"
#include "r128_state.h"


#define R128_TEX1_BIT       0x1
#define R128_TEX0_BIT       0x2
#define R128_RGBA_BIT       0x4
#define R128_SPEC_BIT       0x8
#define R128_FOG_BIT	    0x10
#define R128_XYZW_BIT       0x20
#define R128_PTEX_BIT       0x40
#define R128_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[R128_MAX_SETUP];

#define TINY_VERTEX_FORMAT		(R128_CCE_VC_FRMT_DIFFUSE_ARGB)

#define NOTEX_VERTEX_FORMAT		(R128_CCE_VC_FRMT_RHW |		\
					 R128_CCE_VC_FRMT_DIFFUSE_ARGB |\
					 R128_CCE_VC_FRMT_SPEC_FRGB)

#define TEX0_VERTEX_FORMAT		(R128_CCE_VC_FRMT_RHW |		\
					 R128_CCE_VC_FRMT_DIFFUSE_ARGB |\
					 R128_CCE_VC_FRMT_SPEC_FRGB |	\
					 R128_CCE_VC_FRMT_S_T)

#define TEX1_VERTEX_FORMAT		(R128_CCE_VC_FRMT_RHW |		\
					 R128_CCE_VC_FRMT_DIFFUSE_ARGB |\
					 R128_CCE_VC_FRMT_SPEC_FRGB |	\
					 R128_CCE_VC_FRMT_S_T |		\
					 R128_CCE_VC_FRMT_S2_T2)


#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & R128_XYZW_BIT)
#define DO_RGBA (IND & R128_RGBA_BIT)
#define DO_SPEC (IND & R128_SPEC_BIT)
#define DO_FOG  (IND & R128_FOG_BIT)
#define DO_TEX0 (IND & R128_TEX0_BIT)
#define DO_TEX1 (IND & R128_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & R128_PTEX_BIT)

#define VERTEX r128Vertex
#define VERTEX_COLOR r128_color_t
#define LOCALVARS r128ContextPtr rmesa = R128_CONTEXT(ctx);
#define GET_VIEWPORT_MAT() rmesa->hw_viewport
#define GET_TEXSOURCE(n)  rmesa->tmu_source[n]
#define GET_VERTEX_FORMAT() rmesa->vertex_format
#define GET_VERTEX_STORE() rmesa->verts
#define GET_VERTEX_SIZE() rmesa->vertex_size * sizeof(GLuint)
#define INVALIDATE_STORED_VERTICES()

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0	/* r128 rhw2 not supported by template */

#define UNVIEWPORT_VARS  GLfloat h = R128_CONTEXT(ctx)->driDrawable->h
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z / rmesa->depth_scale

#define PTEX_FALLBACK() FALLBACK(R128_CONTEXT(ctx), R128_FALLBACK_TEXTURE, 1)

#define INTERP_VERTEX setup_tab[rmesa->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[rmesa->SetupIndex].copy_pv

/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) r128_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/


#define IND (R128_XYZW_BIT|R128_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_TEX0_BIT|R128_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_SPEC_BIT|R128_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_SPEC_BIT|R128_TEX0_BIT|\
             R128_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_SPEC_BIT|R128_TEX0_BIT|\
             R128_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_TEX0_BIT|\
             R128_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_TEX0_BIT|\
             R128_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT|\
             R128_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT|\
             R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_XYZW_BIT|R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT|\
             R128_TEX0_BIT|R128_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_FOG_BIT|R128_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_FOG_BIT|R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_SPEC_BIT|R128_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_SPEC_BIT|R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT|R128_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT|R128_TEX0_BIT|R128_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT|R128_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R128_RGBA_BIT|R128_FOG_BIT|R128_SPEC_BIT|R128_TEX0_BIT|\
             R128_TEX1_BIT)
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



void r128PrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & R128_XYZW_BIT)     ? " xyzw," : "",
	   (flags & R128_RGBA_BIT)     ? " rgba," : "",
	   (flags & R128_SPEC_BIT)     ? " spec," : "",
	   (flags & R128_FOG_BIT)      ? " fog," : "",
	   (flags & R128_TEX0_BIT)     ? " tex-0," : "",
	   (flags & R128_TEX1_BIT)     ? " tex-1," : "");
}



void r128CheckTexSizes( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT( ctx );

   if (!setup_tab[rmesa->SetupIndex].check_tex_sizes(ctx)) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);

      /* Invalidate stored verts
       */
      rmesa->SetupNewInputs = ~0;
      rmesa->SetupIndex |= R128_PTEX_BIT;

      if (!rmesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[rmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[rmesa->SetupIndex].copy_pv;
      }
      if (rmesa->Fallback) {
         tnl->Driver.Render.Start(ctx);
      }
   }
}

void r128BuildVertices( GLcontext *ctx,
			GLuint start,
			GLuint count,
			GLuint newinputs )
{
   r128ContextPtr rmesa = R128_CONTEXT( ctx );
   GLuint stride = rmesa->vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)rmesa->verts + (start * stride));

   newinputs |= rmesa->SetupNewInputs;
   rmesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[rmesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= R128_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= R128_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= R128_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= R128_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= R128_FOG_BIT;

      if (rmesa->SetupIndex & R128_PTEX_BIT)
	 ind = ~0;

      ind &= rmesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void r128ChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r128ContextPtr rmesa = R128_CONTEXT( ctx );
   GLuint ind = R128_XYZW_BIT|R128_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= R128_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= R128_FOG_BIT;

   if (ctx->Texture._EnabledUnits) {
      ind |= R128_TEX0_BIT;
      if (ctx->Texture.Unit[0]._ReallyEnabled &&
	  ctx->Texture.Unit[1]._ReallyEnabled)
	 ind |= R128_TEX1_BIT;
   }

   rmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = r128_interp_extras;
      tnl->Driver.Render.CopyPV = r128_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != rmesa->vertex_format) {
      FLUSH_BATCH(rmesa);
      rmesa->vertex_format = setup_tab[ind].vertex_format;
      rmesa->vertex_size = setup_tab[ind].vertex_size;
   }
}



void r128_emit_contiguous_verts( GLcontext *ctx,
				 GLuint start,
				 GLuint count )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint vertex_size = rmesa->vertex_size * 4;
   GLuint *dest = r128AllocDmaLow( rmesa, (count-start) * vertex_size);
   setup_tab[rmesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );
}


#if 0
void r128_emit_indexed_verts( GLcontext *ctx, GLuint start, GLuint count )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint vertex_size = rmesa->vertex_size * 4;
   GLuint bufsz = (count-start) * vertex_size;
   int32_t *dest;

   rmesa->vertex_low = (rmesa->vertex_low + 63) & ~63; /* alignment */
   rmesa->vertex_last_prim = rmesa->vertex_low;

   dest = r128AllocDmaLow( rmesa, bufsz, __FUNCTION__);
   setup_tab[rmesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );

   rmesa->retained_buffer = rmesa->vertex_buffer;
   rmesa->vb_offset = (rmesa->vertex_buffer->idx * R128_BUFFER_SIZE +
		       rmesa->vertex_low - bufsz);

   rmesa->vertex_low = (rmesa->vertex_low + 0x7) & ~0x7;  /* alignment */
   rmesa->vertex_last_prim = rmesa->vertex_low;
}
#endif


void r128InitVB( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;

   rmesa->verts = (GLubyte *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}


void r128FreeVB( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   if (rmesa->verts) {
      ALIGN_FREE(rmesa->verts);
      rmesa->verts = 0;
   }
}
