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
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_vb.c,v 1.3 2002/10/30 12:52:01 alanh Exp $ */
 
#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "math/m_translate.h"
#include "swrast_setup/swrast_setup.h"

#include "tdfx_context.h"
#include "tdfx_vb.h"
#include "tdfx_tris.h"
#include "tdfx_state.h"
#include "tdfx_render.h"

static void copy_pv_rgba4( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GLubyte *tdfxverts = (GLubyte *)fxMesa->verts;
   GLuint shift = fxMesa->vertex_stride_shift;
   tdfxVertex *dst = (tdfxVertex *)(tdfxverts + (edst << shift));
   tdfxVertex *src = (tdfxVertex *)(tdfxverts + (esrc << shift));
   dst->ui[4] = src->ui[4];
}

static void copy_pv_rgba3( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GLubyte *tdfxverts = (GLubyte *)fxMesa->verts;
   GLuint shift = fxMesa->vertex_stride_shift;
   tdfxVertex *dst = (tdfxVertex *)(tdfxverts + (edst << shift));
   tdfxVertex *src = (tdfxVertex *)(tdfxverts + (esrc << shift));
   dst->ui[3] = src->ui[3];
}

typedef void (*emit_func)( GLcontext *, GLuint, GLuint, void *, GLuint );

static struct {
   emit_func	        emit;
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[TDFX_MAX_SETUP];




#define GET_COLOR(ptr, idx) ((ptr)->data[idx])


static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /*fprintf(stderr, "%s\n", __FUNCTION__);*/

   if (VB->ColorPtr[1]) {
      INTERP_4CHAN( t,
		    GET_COLOR(VB->ColorPtr[1], dst),
		    GET_COLOR(VB->ColorPtr[1], out),
		    GET_COLOR(VB->ColorPtr[1], in) );
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   setup_tab[TDFX_CONTEXT(ctx)->SetupIndex].interp(ctx, t, dst, out, in,
						   force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
	 COPY_CHAN4( GET_COLOR(VB->ColorPtr[1], dst), 
		     GET_COLOR(VB->ColorPtr[1], src) );
   }

   setup_tab[TDFX_CONTEXT(ctx)->SetupIndex].copy_pv(ctx, dst, src);
}



#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT)
#define TAG(x) x##_wg
#include "tdfx_vbtmp.h"

/* Special for tdfx: fog requires w
 */
#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT)
#define TAG(x) x##_wg_fog
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_XYZ_BIT|TDFX_RGBA_BIT|TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT|\
             TDFX_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT)
#define TAG(x) x##_g
#include "tdfx_vbtmp.h"

#define IND (TDFX_TEX0_BIT)
#define TAG(x) x##_t0
#include "tdfx_vbtmp.h"

#define IND (TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_t0t1
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT|TDFX_TEX0_BIT)
#define TAG(x) x##_gt0
#include "tdfx_vbtmp.h"

#define IND (TDFX_RGBA_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT)
#define TAG(x) x##_gt0t1
#include "tdfx_vbtmp.h"



static void init_setup_tab( void )
{
   init_wg();
   init_wg_fog();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgpt0t1();

   init_g();
   init_t0();
   init_t0t1();
   init_gt0();
   init_gt0t1();
}


void tdfxPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & TDFX_XYZ_BIT)     ? " xyz," : "", 
	   (flags & TDFX_W_BIT)     ? " w," : "", 
	   (flags & TDFX_RGBA_BIT)     ? " rgba," : "",
	   (flags & TDFX_TEX0_BIT)     ? " tex-0," : "",
	   (flags & TDFX_TEX1_BIT)     ? " tex-1," : "");
}



void tdfxCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   if (!setup_tab[fxMesa->SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = fxMesa->SetupIndex |= (TDFX_PTEX_BIT|TDFX_RGBA_BIT);

      /* Tdfx handles projective textures nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != fxMesa->vertexFormat) {
	 FLUSH_BATCH(fxMesa);
	 fxMesa->dirty |= TDFX_UPLOAD_VERTEX_LAYOUT;      
	 fxMesa->vertexFormat = setup_tab[ind].vertex_format;
	 fxMesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;

	 /* This is required as we have just changed the vertex
	  * format, so the interp and copy routines must also change.
	  * In the unfilled and twosided cases we are using the
	  * swrast_setup ones anyway, so leave them in place.
	  */
	 if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	    tnl->Driver.Render.Interp = setup_tab[fxMesa->SetupIndex].interp;
	    tnl->Driver.Render.CopyPV = setup_tab[fxMesa->SetupIndex].copy_pv;
	 }
      }
   }
}


void tdfxBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			GLuint newinputs )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GLubyte *v = (fxMesa->verts + (start<<fxMesa->vertex_stride_shift));
   GLuint stride = 1<<fxMesa->vertex_stride_shift;

   newinputs |= fxMesa->SetupNewInputs;
   fxMesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   if (newinputs & VERT_BIT_POS) {
      setup_tab[fxMesa->SetupIndex].emit( ctx, start, count, v, stride );   
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_BIT_COLOR0)
	 ind |= TDFX_RGBA_BIT;
      
      if (newinputs & VERT_BIT_TEX0)
	 ind |= TDFX_TEX0_BIT;

      if (newinputs & VERT_BIT_TEX1)
	 ind |= TDFX_TEX0_BIT|TDFX_TEX1_BIT;

      if (fxMesa->SetupIndex & TDFX_PTEX_BIT)
	 ind = ~0;

      ind &= fxMesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v, stride );   
      }
   }
}


void tdfxChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GLuint ind = TDFX_XYZ_BIT|TDFX_RGBA_BIT;

   if (ctx->Texture._EnabledUnits & 0x2)
      /* unit 1 enabled */
      ind |= TDFX_W_BIT|TDFX_TEX1_BIT|TDFX_TEX0_BIT;
   else if (ctx->Texture._EnabledUnits & 0x1) 
      /* unit 0 enabled */
      ind |= TDFX_W_BIT|TDFX_TEX0_BIT;
   else if (ctx->Fog.Enabled)
      ind |= TDFX_W_BIT;
   
   fxMesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = interp_extras;
      tnl->Driver.Render.CopyPV = copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != fxMesa->vertexFormat) {
      FLUSH_BATCH(fxMesa);
      fxMesa->dirty |= TDFX_UPLOAD_VERTEX_LAYOUT;      
      fxMesa->vertexFormat = setup_tab[ind].vertex_format;
      fxMesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
   }
}



void tdfxInitVB( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;
   if (firsttime) {
      init_setup_tab();
      firsttime = 0;
   }

   fxMesa->verts = (GLubyte *)ALIGN_MALLOC(size * sizeof(tdfxVertex), 32);
   fxMesa->vertexFormat = setup_tab[TDFX_XYZ_BIT|TDFX_RGBA_BIT].vertex_format;
   fxMesa->vertex_stride_shift = setup_tab[(TDFX_XYZ_BIT|
					    TDFX_RGBA_BIT)].vertex_stride_shift;
   fxMesa->SetupIndex = TDFX_XYZ_BIT|TDFX_RGBA_BIT;
}


void tdfxFreeVB( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   if (fxMesa->verts) {
      ALIGN_FREE(fxMesa->verts);
      fxMesa->verts = 0;
   }
}
