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
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfxvb.c,v 1.7 2000/11/08 05:02:43 dawes Exp $ */
 
#include "glheader.h"
#include "mtypes.h"
#include "mem.h"
#include "macros.h"
#include "colormac.h"
#include "mmath.h"

#include "math/m_translate.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "fxdrv.h"


static void copy_pv( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *dst = fxMesa->verts + edst;
   GrVertex *src = fxMesa->verts + esrc;

   dst->r = src->r;
   dst->g = src->g;
   dst->b = src->b;
   dst->a = src->a;
}

typedef void (*emit_func)( GLcontext *, GLuint, GLuint, void * );

static struct {
   emit_func	        emit;
   interp_func		interp;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_format;
} setup_tab[MAX_SETUP];


static void import_float_colors( GLcontext *ctx )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct gl_client_array *from = VB->ColorPtr[0];
   struct gl_client_array *to = &FX_CONTEXT(ctx)->UbyteColor;
   GLuint count = VB->Count;

   if (!to->Ptr) {
      to->Ptr = ALIGN_MALLOC( VB->Size * 4 * sizeof(GLubyte), 32 );
      to->Type = GL_UNSIGNED_BYTE;
   }

   /* No need to transform the same value 3000 times.
    */
   if (!from->StrideB) {
      to->StrideB = 0;
      count = 1;
   }
   else
      to->StrideB = 4 * sizeof(GLubyte);
   
   _math_trans_4ub( (GLubyte (*)[4]) to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    from->Size,
		    0,
		    count);

   VB->ColorPtr[0] = to;
}


#define GET_COLOR(ptr, idx) (((GLfloat (*)[4])((ptr)->Ptr))[idx])


static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /*fprintf(stderr, "%s\n", __FUNCTION__);*/

   if (VB->ColorPtr[1]) {
      INTERP_4F( t,
		 GET_COLOR(VB->ColorPtr[1], dst),
		 GET_COLOR(VB->ColorPtr[1], out),
		 GET_COLOR(VB->ColorPtr[1], in) );

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3F( t,
		    GET_COLOR(VB->SecondaryColorPtr[1], dst),
		    GET_COLOR(VB->SecondaryColorPtr[1], out),
		    GET_COLOR(VB->SecondaryColorPtr[1], in) );
      }
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   setup_tab[FX_CONTEXT(ctx)->SetupIndex].interp(ctx, t, dst, out, in,
						   force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
	 COPY_4FV( GET_COLOR(VB->ColorPtr[1], dst), 
		   GET_COLOR(VB->ColorPtr[1], src) );

	 if (VB->SecondaryColorPtr[1]) {
	    COPY_4FV( GET_COLOR(VB->SecondaryColorPtr[1], dst), 
		      GET_COLOR(VB->SecondaryColorPtr[1], src) );
	 }
   }

   copy_pv(ctx, dst, src);
}


#define IND (SETUP_XYZW|SETUP_RGBA)
#define TAG(x) x##_wg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_wgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_wgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_PTEX)
#define TAG(x) x##_wgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_RGBA|SETUP_TMU0|SETUP_TMU1|\
             SETUP_PTEX)
#define TAG(x) x##_wgpt0t1
#include "fxvbtmp.h"


/* Snapping for voodoo-1
 */
#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA)
#define TAG(x) x##_wsg
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_wsgt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_TMU1)
#define TAG(x) x##_wsgt0t1
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
             SETUP_PTEX)
#define TAG(x) x##_wsgpt0
#include "fxvbtmp.h"

#define IND (SETUP_XYZW|SETUP_SNAP|SETUP_RGBA|SETUP_TMU0|\
	     SETUP_TMU1|SETUP_PTEX)
#define TAG(x) x##_wsgpt0t1
#include "fxvbtmp.h"


/* Vertex repair (multipass rendering)
 */
#define IND (SETUP_RGBA)
#define TAG(x) x##_g
#include "fxvbtmp.h"

#define IND (SETUP_TMU0)
#define TAG(x) x##_t0
#include "fxvbtmp.h"

#define IND (SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_t0t1
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_gt0
#include "fxvbtmp.h"

#define IND (SETUP_RGBA|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_gt0t1
#include "fxvbtmp.h"



static void init_setup_tab( void )
{
   init_wg();
   init_wgt0();
   init_wgt0t1();
   init_wgpt0();
   init_wgpt0t1();

   init_wsg();
   init_wsgt0();
   init_wsgt0t1();
   init_wsgpt0();
   init_wsgpt0t1();

   init_g();
   init_t0();
   init_t0t1();
   init_gt0();
   init_gt0t1();
}


void fxPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & SETUP_XYZW)     ? " xyzw," : "", 
	   (flags & SETUP_SNAP)     ? " snap," : "", 
	   (flags & SETUP_RGBA)     ? " rgba," : "",
	   (flags & SETUP_TMU0)     ? " tex-0," : "",
	   (flags & SETUP_TMU1)     ? " tex-1," : "");
}



void fxCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   if (!setup_tab[fxMesa->SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = fxMesa->SetupIndex |= (SETUP_PTEX|SETUP_RGBA);

      /* Tdfx handles projective textures nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != fxMesa->stw_hint_state) {

	 fxMesa->stw_hint_state = setup_tab[ind].vertex_format;
	 FX_grHints(GR_HINT_STWHINT, fxMesa->stw_hint_state);

	 /* This is required as we have just changed the vertex
	  * format, so the interp routines must also change.
	  * In the unfilled and twosided cases we are using the
	  * Extras ones anyway, so leave them in place.
	  */
	 if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	    tnl->Driver.Render.Interp = setup_tab[fxMesa->SetupIndex].interp;
	 }
      }
   }
}


void fxBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			GLuint newinputs )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *v = (fxMesa->verts + start);

   if (!newinputs)
      return;

   if (newinputs & VERT_CLIP) {
      setup_tab[fxMesa->SetupIndex].emit( ctx, start, count, v );   
   } else {
      GLuint ind = 0;

      if (newinputs & VERT_RGBA)
	 ind |= SETUP_RGBA;
      
      if (newinputs & VERT_TEX0) 
	 ind |= SETUP_TMU0;

      if (newinputs & VERT_TEX1)
	 ind |= SETUP_TMU0|SETUP_TMU1;

      if (fxMesa->SetupIndex & SETUP_PTEX)
	 ind = ~0;

      ind &= fxMesa->SetupIndex;

      if (ind) {
	 setup_tab[ind].emit( ctx, start, count, v );   
      }
   }
}


void fxChooseVertexState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GLuint ind = SETUP_XYZW|SETUP_RGBA;

   if (fxMesa->snapVertices)
      ind |= SETUP_SNAP;

   fxMesa->tmu_source[0] = 0;
   fxMesa->tmu_source[1] = 1;

   if (ctx->Texture._ReallyEnabled & 0xf0) {
      if (ctx->Texture._ReallyEnabled & 0xf) {
	 ind |= SETUP_TMU1|SETUP_TMU0;
      }
      else {
	 fxMesa->tmu_source[0] = 1;
	 fxMesa->tmu_source[1] = 0;
	 ind |= SETUP_TMU0;
      }
   }
   else if (ctx->Texture._ReallyEnabled & 0xf) {
      ind |= SETUP_TMU0;
   }
   
   fxMesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = interp_extras;
      tnl->Driver.Render.CopyPV = copy_pv_extras;
   } else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = copy_pv;
   }

   if (setup_tab[ind].vertex_format != fxMesa->stw_hint_state) {
      fxMesa->stw_hint_state = setup_tab[ind].vertex_format;
      FX_grHints(GR_HINT_STWHINT, fxMesa->stw_hint_state);
   }
}



void fxAllocVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;
   if (firsttime) {
      init_setup_tab();
      firsttime = 0;
   }

   fxMesa->verts = (GrVertex *)ALIGN_MALLOC(size * sizeof(GrVertex), 32);
   fxMesa->SetupIndex = SETUP_XYZW|SETUP_RGBA;
}


void fxFreeVB( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   if (fxMesa->verts) {
      ALIGN_FREE(fxMesa->verts);
      fxMesa->verts = 0;
   }

   if (fxMesa->UbyteColor.Ptr) {
      ALIGN_FREE(fxMesa->UbyteColor.Ptr);
      fxMesa->UbyteColor.Ptr = 0;
   }
}
