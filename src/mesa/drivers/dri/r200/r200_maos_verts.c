/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_maos_verts.c,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "mmath.h"
#include "mtypes.h"
#include "enums.h"
#include "colormac.h"
#include "light.h"

#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_imm_debug.h"

#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_tex.h"
#include "r200_tcl.h"
#include "r200_swtcl.h"
#include "r200_maos.h"


#define R200_TCL_MAX_SETUP 13

union emit_union { float f; GLuint ui; GLubyte ub[4]; };

static struct {
   void   (*emit)( GLcontext *, GLuint, GLuint, void * );
   GLuint vertex_size;
   GLuint vertex_format;
} setup_tab[R200_TCL_MAX_SETUP];

#define DO_W    (IND & R200_CP_VC_FRMT_W0)
#define DO_RGBA (IND & R200_CP_VC_FRMT_PKCOLOR)
#define DO_SPEC (IND & R200_CP_VC_FRMT_PKSPEC)
#define DO_FOG  (IND & R200_CP_VC_FRMT_PKSPEC)
#define DO_TEX0 (IND & R200_CP_VC_FRMT_ST0)
#define DO_TEX1 (IND & R200_CP_VC_FRMT_ST1)
#define DO_PTEX (IND & R200_CP_VC_FRMT_Q0)
#define DO_NORM (IND & R200_CP_VC_FRMT_N0)

#define DO_TEX2 0
#define DO_TEX3 0

#define GET_TEXSOURCE(n)  n

/***********************************************************************
 *             Generate vertex emit functions               *
 ***********************************************************************/


/* Defined in order of increasing vertex size:
 */
#define IDX 0
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR)
#define TAG(x) x##_rgba
#include "r200_maos_vbtmp.h"

#define IDX 1
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_n
#include "r200_maos_vbtmp.h"

#define IDX 2
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_ST0)
#define TAG(x) x##_rgba_st
#include "r200_maos_vbtmp.h"

#define IDX 3
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_n
#include "r200_maos_vbtmp.h"

#define IDX 4
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_st_n
#include "r200_maos_vbtmp.h"

#define IDX 5
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_ST1)
#define TAG(x) x##_rgba_st_st
#include "r200_maos_vbtmp.h"

#define IDX 6
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_st_n
#include "r200_maos_vbtmp.h"

#define IDX 7
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_PKSPEC|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_ST1)
#define TAG(x) x##_rgba_spec_st_st
#include "r200_maos_vbtmp.h"

#define IDX 8
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_ST1|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_st_st_n
#include "r200_maos_vbtmp.h"

#define IDX 9
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_PKSPEC|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_ST1|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_rgba_spec_st_st_n
#include "r200_maos_vbtmp.h"

#define IDX 10
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_Q0)
#define TAG(x) x##_rgba_stq
#include "r200_maos_vbtmp.h"

#define IDX 11
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_ST1|		\
	     R200_CP_VC_FRMT_Q1|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_Q0)
#define TAG(x) x##_rgba_stq_stq
#include "r200_maos_vbtmp.h"

#define IDX 12
#define IND (R200_CP_VC_FRMT_XY|		\
	     R200_CP_VC_FRMT_Z|		\
	     R200_CP_VC_FRMT_W0|		\
	     R200_CP_VC_FRMT_PKCOLOR|		\
	     R200_CP_VC_FRMT_PKSPEC|		\
	     R200_CP_VC_FRMT_ST0|		\
	     R200_CP_VC_FRMT_Q0|		\
	     R200_CP_VC_FRMT_ST1|		\
	     R200_CP_VC_FRMT_Q1|		\
	     R200_CP_VC_FRMT_N0)
#define TAG(x) x##_w_rgba_spec_stq_stq_n
#include "r200_maos_vbtmp.h"





/***********************************************************************
 *                         Initialization 
 ***********************************************************************/


static void init_tcl_verts( void )
{
   init_rgba();
   init_n();
   init_rgba_n();
   init_rgba_st();
   init_st_n();
   init_rgba_st_st();
   init_rgba_st_n();
   init_rgba_spec_st_st();
   init_st_st_n();
   init_rgba_spec_st_st_n();
   init_rgba_stq();
   init_rgba_stq_stq();
   init_w_rgba_spec_stq_stq_n();
}


void r200EmitArrays( GLcontext *ctx, GLuint inputs )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint req = 0;
   GLuint vtx = (rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &
		 ~(R200_TCL_VTX_Q0|R200_TCL_VTX_Q1));
   int i;
   static int firsttime = 1;

   if (firsttime) {
      init_tcl_verts();
      firsttime = 0;
   }
		     
   if (1) {
      req |= R200_CP_VC_FRMT_Z;
      if (VB->ObjPtr->size == 4) {
	 req |= R200_CP_VC_FRMT_W0;
      }
   }

   if (inputs & VERT_BIT_NORMAL) {
      req |= R200_CP_VC_FRMT_N0;
   }
   
   if (inputs & VERT_BIT_COLOR0) {
      req |= R200_CP_VC_FRMT_PKCOLOR;
   }

   if (inputs & VERT_BIT_COLOR1) {
      req |= R200_CP_VC_FRMT_PKSPEC;
   }

   if (inputs & VERT_BIT_TEX0) {
      req |= R200_CP_VC_FRMT_ST0;

      if (VB->TexCoordPtr[0]->size == 4) {
	 req |= R200_CP_VC_FRMT_Q0;
	 vtx |= R200_TCL_VTX_Q0;
      }
   }

   if (inputs & VERT_BIT_TEX1) {
      req |= R200_CP_VC_FRMT_ST1;

      if (VB->TexCoordPtr[1]->size == 4) {
	 req |= R200_CP_VC_FRMT_Q1;
	 vtx |= R200_TCL_VTX_Q1;
      }
   }

   if (vtx != rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT]) {
      R200_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = vtx;
   }

   for (i = 0 ; i < R200_TCL_MAX_SETUP ; i++) 
      if ((setup_tab[i].vertex_format & req) == req) 
	 break;

   if (rmesa->tcl.vertex_format == setup_tab[i].vertex_format &&
       rmesa->tcl.indexed_verts.buf)
      return;

   if (rmesa->tcl.indexed_verts.buf)
      r200ReleaseArrays( ctx, ~0 );

   r200AllocDmaRegionVerts( rmesa, 
			      &rmesa->tcl.indexed_verts, 
			      VB->Count,
			      setup_tab[i].vertex_size * 4, 
			      4);

   setup_tab[i].emit( ctx, 0, VB->Count, 
		      rmesa->tcl.indexed_verts.address + 
		      rmesa->tcl.indexed_verts.start );

   rmesa->tcl.vertex_format = setup_tab[i].vertex_format;
   rmesa->tcl.indexed_verts.aos_start = GET_START( &rmesa->tcl.indexed_verts );
   rmesa->tcl.indexed_verts.aos_size = setup_tab[i].vertex_size;
   rmesa->tcl.indexed_verts.aos_stride = setup_tab[i].vertex_size;

   rmesa->tcl.aos_components[0] = &rmesa->tcl.indexed_verts;
   rmesa->tcl.nr_aos_components = 1;
}



void r200ReleaseArrays( GLcontext *ctx, GLuint newinputs )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   if (R200_DEBUG & DEBUG_VERTS) 
      _tnl_print_vert_flags( __FUNCTION__, newinputs );

   if (newinputs) 
     r200ReleaseDmaRegion( rmesa, &rmesa->tcl.indexed_verts, __FUNCTION__ );
}
