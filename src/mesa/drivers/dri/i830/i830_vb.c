/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Adapted for use on the I830M:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_vb.c,v 1.5 2002/12/10 01:26:54 dawes Exp $ */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_vb.h"
#include "i830_ioctl.h"
#include "i830_tris.h"
#include "i830_state.h"

#define I830_TEX1_BIT       0x1
#define I830_TEX0_BIT       0x2
#define I830_RGBA_BIT       0x4
#define I830_SPEC_BIT       0x8
#define I830_FOG_BIT	    0x10
#define I830_XYZW_BIT       0x20
#define I830_PTEX_BIT       0x40
#define I830_MAX_SETUP      0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[I830_MAX_SETUP];

#define TINY_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(0) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_XYZ)

#define NOTEX_VERTEX_FORMAT     (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(0) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

#define TEX0_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(1) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

#define TEX1_VERTEX_FORMAT      (STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(2) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)


/* I'm cheating here hardcore : if bit 31 is set I know to emit
 * a vf2 state == TEXCOORDFMT_3D.  We never mix 2d/3d texcoords,
 * so this solution works for now.
 */

#define PROJ_TEX1_VERTEX_FORMAT ((1<<31) | \
				 STATE3D_VERTEX_FORMAT_CMD | \
				 VRTX_TEX_COORD_COUNT(2) | \
				 VRTX_HAS_DIFFUSE | \
				 VRTX_HAS_SPEC | \
				 VRTX_HAS_XYZW)

/* Might want to do these later */
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & I830_XYZW_BIT)
#define DO_RGBA (IND & I830_RGBA_BIT)
#define DO_SPEC (IND & I830_SPEC_BIT)
#define DO_FOG  (IND & I830_FOG_BIT)
#define DO_TEX0 (IND & I830_TEX0_BIT)
#define DO_TEX1 (IND & I830_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & I830_PTEX_BIT)

#define VERTEX i830Vertex
#define VERTEX_COLOR i830_color_t
#define GET_VIEWPORT_MAT() I830_CONTEXT(ctx)->ViewportMatrix.m
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() I830_CONTEXT(ctx)->vertex_format
#define GET_VERTEX_STORE() ((GLubyte *)I830_CONTEXT(ctx)->verts)
#define GET_VERTEX_STRIDE_SHIFT() I830_CONTEXT(ctx)->vertex_stride_shift
#define GET_UBYTE_COLOR_STORE() &I830_CONTEXT(ctx)->UbyteColor
#define GET_UBYTE_SPEC_COLOR_STORE() &I830_CONTEXT(ctx)->UbyteSecondaryColor
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
#define HAVE_PTEX_VERTICES  1

#define UNVIEWPORT_VARS  GLfloat h = I830_CONTEXT(ctx)->driDrawable->h
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z * (float)I830_CONTEXT(ctx)->ClearDepth

#define PTEX_FALLBACK() FALLBACK(I830_CONTEXT(ctx), I830_FALLBACK_TEXTURE, 1)

#define IMPORT_FLOAT_COLORS i830_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS i830_import_float_spec_colors

#define INTERP_VERTEX setup_tab[I830_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[I830_CONTEXT(ctx)->SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) i830_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (I830_XYZW_BIT|I830_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_TEX0_BIT|I830_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT|\
             I830_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT|\
             I830_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT|\
             I830_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT|\
             I830_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|\
             I830_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|\
             I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|\
             I830_TEX0_BIT|I830_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_TEX0_BIT)
#define TAG(x) x##_t0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_FOG_BIT)
#define TAG(x) x##_f
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_FOG_BIT|I830_TEX0_BIT)
#define TAG(x) x##_ft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_FOG_BIT|I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_ft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT)
#define TAG(x) x##_g
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_SPEC_BIT)
#define TAG(x) x##_gs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT)
#define TAG(x) x##_gst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_gst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT)
#define TAG(x) x##_gf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT)
#define TAG(x) x##_gfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT)
#define TAG(x) x##_gft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT|I830_TEX1_BIT)
#define TAG(x) x##_gft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|I830_TEX0_BIT)
#define TAG(x) x##_gfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|I830_TEX0_BIT|\
             I830_TEX1_BIT)
#define TAG(x) x##_gfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

/* Add functions for proj texturing for t0 and t1 */
#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_TEX0_BIT|I830_TEX1_BIT|\
	     I830_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_SPEC_BIT|I830_TEX0_BIT|\
             I830_TEX1_BIT|I830_PTEX_BIT)
#define TAG(x) x##_wgspt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_TEX0_BIT|\
             I830_TEX1_BIT|I830_PTEX_BIT)
#define TAG(x) x##_wgfpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (I830_XYZW_BIT|I830_RGBA_BIT|I830_FOG_BIT|I830_SPEC_BIT|\
             I830_TEX1_BIT|I830_TEX0_BIT|I830_PTEX_BIT)
#define TAG(x) x##_wgfspt0t1
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
   /* Add proj texturing on t1 */
   init_wgpt0t1();
   init_wgspt0t1();
   init_wgfpt0t1();
   init_wgfspt0t1();
}



void i830PrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & I830_XYZW_BIT)      ? " xyzw," : "",
	   (flags & I830_RGBA_BIT)     ? " rgba," : "",
	   (flags & I830_SPEC_BIT)     ? " spec," : "",
	   (flags & I830_FOG_BIT)      ? " fog," : "",
	   (flags & I830_TEX0_BIT)     ? " tex-0," : "",
	   (flags & I830_TEX1_BIT)     ? " tex-1," : "",
	   (flags & I830_PTEX_BIT)     ? " ptex," : "");
}

void i830CheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i830ContextPtr imesa = I830_CONTEXT( ctx );

   if (!setup_tab[imesa->SetupIndex].check_tex_sizes(ctx)) {
      int ind = imesa->SetupIndex |= I830_PTEX_BIT;
	 
      if(setup_tab[ind].vertex_format != imesa->vertex_format) {
	 int vfmt = setup_tab[ind].vertex_format;

	 I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
	 imesa->Setup[I830_CTXREG_VF] = ~(1<<31) & vfmt;

	 if (vfmt & (1<<31)) {
	    /* Proj texturing */
	    imesa->Setup[I830_CTXREG_VF2] = (STATE3D_VERTEX_FORMAT_2_CMD |
					  VRTX_TEX_SET_0_FMT(TEXCOORDFMT_3D) |
					  VRTX_TEX_SET_1_FMT(TEXCOORDFMT_3D) | 
					  VRTX_TEX_SET_2_FMT(TEXCOORDFMT_3D) |
					  VRTX_TEX_SET_3_FMT(TEXCOORDFMT_3D));
	    i830UpdateTexUnitProj( ctx, 0, GL_TRUE );
	    i830UpdateTexUnitProj( ctx, 1, GL_TRUE );
	    
	 } else {
	    /* Normal texturing */
	    imesa->Setup[I830_CTXREG_VF2] = (STATE3D_VERTEX_FORMAT_2_CMD |
					  VRTX_TEX_SET_0_FMT(TEXCOORDFMT_2D) |
					  VRTX_TEX_SET_1_FMT(TEXCOORDFMT_2D) | 
					  VRTX_TEX_SET_2_FMT(TEXCOORDFMT_2D) |
					  VRTX_TEX_SET_3_FMT(TEXCOORDFMT_2D));
	    i830UpdateTexUnitProj( ctx, 0, GL_FALSE );
	    i830UpdateTexUnitProj( ctx, 1, GL_FALSE );
	 }
	 imesa->vertex_format = vfmt;
	 imesa->vertex_size = setup_tab[ind].vertex_size;
	 imesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
      }

      if (!imesa->Fallback &&
	  !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[imesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[imesa->SetupIndex].copy_pv;
      }
   }
}

void i830BuildVertices( GLcontext *ctx,
			GLuint start,
			GLuint count,
			GLuint newinputs )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   GLubyte *v = ((GLubyte *)
		 imesa->verts + (start<<imesa->vertex_stride_shift));
   GLuint stride = 1<<imesa->vertex_stride_shift;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   newinputs |= imesa->SetupNewInputs;
   imesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[imesa->SetupIndex].emit( ctx, start, count, v, stride );
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= I830_RGBA_BIT;

      if (newinputs & VERT_BIT_COLOR1)
	 ind |= I830_SPEC_BIT;

      if (newinputs & VERT_BIT_TEX0)
	 ind |= I830_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= I830_TEX1_BIT;

      if (newinputs & VERT_BIT_FOG)
	 ind |= I830_FOG_BIT;

#if 0
      if (imesa->SetupIndex & I830_PTEX_BIT)
	 ind = ~0;
#endif

      ind &= imesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );
      }
   }
}

void i830ChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   GLuint ind = I830_XYZW_BIT|I830_RGBA_BIT;

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= I830_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= I830_FOG_BIT;

   if (ctx->Texture._EnabledUnits & 0x2)
      /* unit 1 enabled */
      ind |= I830_TEX1_BIT|I830_TEX0_BIT;
   else if (ctx->Texture._EnabledUnits & 0x1)
      /* unit 0 enabled */
      ind |= I830_TEX0_BIT;

   imesa->SetupIndex = ind;

   if (I830_DEBUG & (DEBUG_VERTS|DEBUG_STATE))
      i830PrintSetupFlags( __FUNCTION__, ind );

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = i830_interp_extras;
      tnl->Driver.Render.CopyPV = i830_copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != imesa->vertex_format) {
      int vfmt = setup_tab[ind].vertex_format;

      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_VF] = ~(1<<31) & vfmt;

      if (vfmt & (1<<31)) {
	 /* Proj texturing */
	 imesa->Setup[I830_CTXREG_VF2] = (STATE3D_VERTEX_FORMAT_2_CMD |
					  VRTX_TEX_SET_0_FMT(TEXCOORDFMT_3D) |
					  VRTX_TEX_SET_1_FMT(TEXCOORDFMT_3D) | 
					  VRTX_TEX_SET_2_FMT(TEXCOORDFMT_3D) |
					  VRTX_TEX_SET_3_FMT(TEXCOORDFMT_3D));
	 i830UpdateTexUnitProj( ctx, 0, GL_TRUE );
	 i830UpdateTexUnitProj( ctx, 1, GL_TRUE );
      } else {
	 /* Normal texturing */
	 imesa->Setup[I830_CTXREG_VF2] = (STATE3D_VERTEX_FORMAT_2_CMD |
					  VRTX_TEX_SET_0_FMT(TEXCOORDFMT_2D) |
					  VRTX_TEX_SET_1_FMT(TEXCOORDFMT_2D) | 
					  VRTX_TEX_SET_2_FMT(TEXCOORDFMT_2D) |
					  VRTX_TEX_SET_3_FMT(TEXCOORDFMT_2D));
	 i830UpdateTexUnitProj( ctx, 0, GL_FALSE );
	 i830UpdateTexUnitProj( ctx, 1, GL_FALSE );
      }
      imesa->vertex_format = vfmt;
      imesa->vertex_size = setup_tab[ind].vertex_size;
      imesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
   }
}



void i830_emit_contiguous_verts( GLcontext *ctx,
				 GLuint start,
				 GLuint count )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint vertex_size = imesa->vertex_size * 4;
   GLuint *dest = i830AllocDmaLow( imesa, (count-start) * vertex_size);
   setup_tab[imesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );
}



void i830InitVB( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   
   imesa->verts = (char *)ALIGN_MALLOC(size * 4 * 16, 32);

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}


void i830FreeVB( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   if (imesa->verts) {
      ALIGN_FREE(imesa->verts);
      imesa->verts = 0;
   }

   if (imesa->UbyteSecondaryColor.Ptr) {
      ALIGN_FREE(imesa->UbyteSecondaryColor.Ptr);
      imesa->UbyteSecondaryColor.Ptr = 0;
   }

   if (imesa->UbyteColor.Ptr) {
      ALIGN_FREE(imesa->UbyteColor.Ptr);
      imesa->UbyteColor.Ptr = 0;
   }
}
