/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_swtcl.c,v 1.5 2003/05/06 23:52:08 daenzer Exp $ */
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
#include "mtypes.h"
#include "colormac.h"
#include "enums.h"
#include "image.h"
#include "imports.h"
#include "macros.h"

#include "swrast/s_context.h"
#include "swrast/s_fog.h"
#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vtx_api.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_swtcl.h"
#include "r200_tcl.h"

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/


#define R200_XYZW_BIT		0x01
#define R200_RGBA_BIT		0x02
#define R200_SPEC_BIT		0x04
#define R200_TEX0_BIT		0x08
#define R200_TEX1_BIT		0x10
#define R200_PTEX_BIT		0x20
#define R200_MAX_SETUP	0x40

static void flush_last_swtcl_prim( r200ContextPtr rmesa  );

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[R200_MAX_SETUP];


static int se_vtx_fmt_0[] = {
   0,

   (R200_VTX_XY |
    R200_VTX_Z0 |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT)),

   (R200_VTX_XY |
    R200_VTX_Z0 |
    R200_VTX_W0 |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT)),

   (R200_VTX_XY |
    R200_VTX_Z0 |
    R200_VTX_W0 |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT)),

   (R200_VTX_XY |
    R200_VTX_Z0 |
    R200_VTX_W0 |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT)),

   (R200_VTX_XY |
    R200_VTX_Z0 |
    R200_VTX_W0 |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) |
    (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT))
};

static int se_vtx_fmt_1[] = {
   0,
   0,
   0,
   ((2 << R200_VTX_TEX0_COMP_CNT_SHIFT)),
   ((2 << R200_VTX_TEX0_COMP_CNT_SHIFT) |
    (2 << R200_VTX_TEX1_COMP_CNT_SHIFT)),
   ((3 << R200_VTX_TEX0_COMP_CNT_SHIFT) |
    (3 << R200_VTX_TEX1_COMP_CNT_SHIFT)),
};

#define TINY_VERTEX_FORMAT	1
#define NOTEX_VERTEX_FORMAT	2
#define TEX0_VERTEX_FORMAT	3
#define TEX1_VERTEX_FORMAT	4
#define PROJ_TEX1_VERTEX_FORMAT	5
#define TEX2_VERTEX_FORMAT 0
#define TEX3_VERTEX_FORMAT 0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & R200_XYZW_BIT)
#define DO_RGBA (IND & R200_RGBA_BIT)
#define DO_SPEC (IND & R200_SPEC_BIT)
#define DO_FOG  (IND & R200_SPEC_BIT)
#define DO_TEX0 (IND & R200_TEX0_BIT)
#define DO_TEX1 (IND & R200_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & R200_PTEX_BIT)

#define VERTEX r200Vertex
#define VERTEX_COLOR r200_color_t
#define GET_VIEWPORT_MAT() 0
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() R200_CONTEXT(ctx)->swtcl.vertex_format
#define GET_VERTEX_STORE() R200_CONTEXT(ctx)->swtcl.verts
#define GET_VERTEX_SIZE() R200_CONTEXT(ctx)->swtcl.vertex_size * sizeof(GLuint)

#define HAVE_HW_VIEWPORT    1
#define HAVE_HW_DIVIDE      (IND & ~(R200_XYZW_BIT|R200_RGBA_BIT))
#define HAVE_TINY_VERTICES  1
#define HAVE_RGBA_COLOR     1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  1

#define CHECK_HW_DIVIDE    (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE| \
                                                    DD_TRI_UNFILLED)))

#define INTERP_VERTEX setup_tab[R200_CONTEXT(ctx)->swtcl.SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[R200_CONTEXT(ctx)->swtcl.SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) r200_##x
#define IND ~0
#include "tnl_dd/t_dd_vb.c"
#undef IND


/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (R200_XYZW_BIT|R200_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_TEX0_BIT|R200_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_TEX0_BIT|R200_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_TEX0_BIT|R200_TEX1_BIT|\
             R200_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_SPEC_BIT|\
	     R200_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_SPEC_BIT|\
	     R200_TEX0_BIT|R200_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_SPEC_BIT|\
	     R200_TEX0_BIT|R200_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (R200_XYZW_BIT|R200_RGBA_BIT|R200_SPEC_BIT|\
	     R200_TEX0_BIT|R200_TEX1_BIT|R200_PTEX_BIT)
#define TAG(x) x##_wgfspt0t1
#include "tnl_dd/t_dd_vbtmp.h"


/***********************************************************************
 *                         Initialization 
 ***********************************************************************/

static void init_setup_tab( void )
{
   init_wg();
   init_wgt0();
   init_wgpt0();
   init_wgt0t1();
   init_wgpt0t1();
   init_wgfs();
   init_wgfst0();
   init_wgfspt0();
   init_wgfst0t1();
   init_wgfspt0t1();
}



void r200PrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & R200_XYZW_BIT)      ? " xyzw," : "",
	   (flags & R200_RGBA_BIT)     ? " rgba," : "",
	   (flags & R200_SPEC_BIT)     ? " spec/fog," : "",
	   (flags & R200_TEX0_BIT)     ? " tex-0," : "",
	   (flags & R200_TEX1_BIT)     ? " tex-1," : "",
	   (flags & R200_PTEX_BIT)     ? " proj-tex," : "");
}



static void r200SetVertexFormat( GLcontext *ctx, GLuint ind ) 
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   rmesa->swtcl.SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = r200_interp_extras;
      tnl->Driver.Render.CopyPV = r200_copy_pv_extras;
   }
   else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != rmesa->swtcl.vertex_format) {
      int i;
      R200_NEWPRIM(rmesa);
      i = rmesa->swtcl.vertex_format = setup_tab[ind].vertex_format;
      rmesa->swtcl.vertex_size = setup_tab[ind].vertex_size;

      R200_STATECHANGE( rmesa, vtx );
      rmesa->hw.vtx.cmd[VTX_VTXFMT_0] = se_vtx_fmt_0[i];
      rmesa->hw.vtx.cmd[VTX_VTXFMT_1] = se_vtx_fmt_1[i];
   }

   {
      GLuint vte = rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL];
      GLuint vap = rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL];
      GLuint needproj;

      /* HW perspective divide is a win, but tiny vertex formats are a
       * bigger one.
       */
      if (setup_tab[ind].vertex_format == TINY_VERTEX_FORMAT ||
	  (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 needproj = GL_TRUE;
	 vte |= R200_VTX_XY_FMT | R200_VTX_Z_FMT;
	 vte &= ~R200_VTX_W0_FMT;
	 vap |= R200_VAP_FORCE_W_TO_ONE;
      }
      else {
	 needproj = GL_FALSE;
	 vte &= ~(R200_VTX_XY_FMT | R200_VTX_Z_FMT);
	 vte |= R200_VTX_W0_FMT;
	 vap &= ~R200_VAP_FORCE_W_TO_ONE;
      }

      _tnl_need_projected_coords( ctx, needproj );
      if (vte != rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL]) {
	 R200_STATECHANGE( rmesa, vte );
	 rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = vte;
      }
      if (vap != rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL]) {
	 R200_STATECHANGE( rmesa, vap );
	 rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] = vap;
      }
   }
}

static void r200RenderStart( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   if (!setup_tab[rmesa->swtcl.SetupIndex].check_tex_sizes(ctx)) {
      r200SetVertexFormat( ctx, rmesa->swtcl.SetupIndex | R200_PTEX_BIT);
   }
   
   if (rmesa->dma.flush != 0 && 
       rmesa->dma.flush != flush_last_swtcl_prim)
      rmesa->dma.flush( rmesa );
}


void r200BuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			   GLuint newinputs )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   GLuint stride = rmesa->swtcl.vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)rmesa->swtcl.verts + (start * stride));

   newinputs |= rmesa->swtcl.SetupNewInputs;
   rmesa->swtcl.SetupNewInputs = 0;

   if (!newinputs)
      return;

   setup_tab[rmesa->swtcl.SetupIndex].emit( ctx, start, count, v, stride );
}


void r200ChooseVertexState( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   GLuint ind = (R200_XYZW_BIT | R200_RGBA_BIT);

   if (!rmesa->TclFallback || rmesa->Fallback)
      return;

   if (ctx->Fog.Enabled || (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR))
      ind |= R200_SPEC_BIT;

   if (ctx->Texture._EnabledUnits & 0x2)  /* unit 1 enabled */
      ind |= R200_TEX0_BIT|R200_TEX1_BIT;
   else if (ctx->Texture._EnabledUnits & 0x1)  /* unit 1 enabled */
      ind |= R200_TEX0_BIT;

   r200SetVertexFormat( ctx, ind );
}


/* Flush vertices in the current dma region.
 */
static void flush_last_swtcl_prim( r200ContextPtr rmesa  )
{
   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   rmesa->dma.flush = 0;

   if (rmesa->dma.current.buf) {
      struct r200_dma_region *current = &rmesa->dma.current;
      GLuint current_offset = (rmesa->r200Screen->gart_buffer_offset +
			       current->buf->buf->idx * RADEON_BUFFER_SIZE + 
			       current->start);

      assert (!(rmesa->swtcl.hw_primitive & R200_VF_PRIM_WALK_IND));

      assert (current->start + 
	      rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	      current->ptr);

      if (rmesa->dma.current.start != rmesa->dma.current.ptr) {
	 r200EmitVertexAOS( rmesa,
			      rmesa->swtcl.vertex_size,
			      current_offset);

	 r200EmitVbufPrim( rmesa,
			   rmesa->swtcl.hw_primitive,
			   rmesa->swtcl.numverts);
      }

      rmesa->swtcl.numverts = 0;
      current->start = current->ptr;
   }
}


/* Alloc space in the current dma region.
 */
static __inline void *r200AllocDmaLowVerts( r200ContextPtr rmesa,
					      int nverts, int vsize )
{
   GLuint bytes = vsize * nverts;

   if ( rmesa->dma.current.ptr + bytes > rmesa->dma.current.end ) 
      r200RefillCurrentDmaRegion( rmesa );

   if (!rmesa->dma.flush) {
      rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
      rmesa->dma.flush = flush_last_swtcl_prim;
   }

   ASSERT( vsize == rmesa->swtcl.vertex_size * 4 );
   ASSERT( rmesa->dma.flush == flush_last_swtcl_prim );
   ASSERT( rmesa->dma.current.start + 
	   rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	   rmesa->dma.current.ptr );


   {
      GLubyte *head = (GLubyte *) (rmesa->dma.current.address + rmesa->dma.current.ptr);
      rmesa->dma.current.ptr += bytes;
      rmesa->swtcl.numverts += nverts;
      return head;
   }

}




static void *r200_emit_contiguous_verts( GLcontext *ctx, 
					 GLuint start, 
					 GLuint count,
					 void *dest)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint stride = rmesa->swtcl.vertex_size * 4;
   setup_tab[rmesa->swtcl.SetupIndex].emit( ctx, start, count, dest, stride );
   return (void *)((char *)dest + stride * (count - start));
}



void r200_emit_indexed_verts( GLcontext *ctx, GLuint start, GLuint count )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   r200AllocDmaRegionVerts( rmesa, 
			      &rmesa->swtcl.indexed_verts, 
			      count - start,
			      rmesa->swtcl.vertex_size * 4, 
			      64);

   setup_tab[rmesa->swtcl.SetupIndex].emit( 
      ctx, start, count, 
      rmesa->swtcl.indexed_verts.address + rmesa->swtcl.indexed_verts.start, 
      rmesa->swtcl.vertex_size * 4 );
}


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       1
#define HAVE_QUAD_STRIPS 1
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1

static const GLuint hw_prim[GL_POLYGON+1] = {
   R200_VF_PRIM_POINTS,
   R200_VF_PRIM_LINES,
   0,
   R200_VF_PRIM_LINE_STRIP,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLE_STRIP,
   R200_VF_PRIM_TRIANGLE_FAN,
   R200_VF_PRIM_QUADS,
   R200_VF_PRIM_QUAD_STRIP,
   R200_VF_PRIM_POLYGON
};

static __inline void r200DmaPrimitive( r200ContextPtr rmesa, GLenum prim )
{
   R200_NEWPRIM( rmesa );
   rmesa->swtcl.hw_primitive = hw_prim[prim];
   assert(rmesa->dma.current.ptr == rmesa->dma.current.start);
}

static __inline void r200EltPrimitive( r200ContextPtr rmesa, GLenum prim )
{
   R200_NEWPRIM( rmesa );
   rmesa->swtcl.hw_primitive = hw_prim[prim] | R200_VF_PRIM_WALK_IND;
}




#define LOCAL_VARS r200ContextPtr rmesa = R200_CONTEXT(ctx)
#define ELTS_VARS(buf)  GLushort *dest = buf
#define INIT( prim ) r200DmaPrimitive( rmesa, prim )
#define ELT_INIT(prim) r200EltPrimitive( rmesa, prim )
#define FLUSH()  R200_NEWPRIM( rmesa )
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)rmesa->dma.current.end - (int)rmesa->dma.current.ptr) / (rmesa->swtcl.vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  ((RADEON_BUFFER_SIZE) / (rmesa->swtcl.vertex_size*4))

#define GET_CURRENT_VB_MAX_ELTS() \
  ((R200_CMD_BUF_SZ - (rmesa->store.cmd_used + 16)) / 2)
#define GET_SUBSEQUENT_VB_MAX_ELTS() \
  ((R200_CMD_BUF_SZ - 1024) / 2)

static void *r200_alloc_elts( r200ContextPtr rmesa, int nr )
{
   if (rmesa->dma.flush == r200FlushElts &&
       rmesa->store.cmd_used + nr*2 < R200_CMD_BUF_SZ) {

      rmesa->store.cmd_used += nr*2;

      return (void *)(rmesa->store.cmd_buf + rmesa->store.cmd_used);
   }
   else {
      if (rmesa->dma.flush) {
	 rmesa->dma.flush( rmesa );
      }

      r200EmitVertexAOS( rmesa,
			   rmesa->swtcl.vertex_size,
			   (rmesa->r200Screen->gart_buffer_offset +
			    rmesa->swtcl.indexed_verts.buf->buf->idx *
			    RADEON_BUFFER_SIZE +
			    rmesa->swtcl.indexed_verts.start));

      return (void *) r200AllocEltsOpenEnded( rmesa,
					      rmesa->swtcl.hw_primitive,
					      nr );
   }
}

#define ALLOC_ELTS(nr) r200_alloc_elts(rmesa, nr)



#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(offset, x) do {                                \
        int off = offset + ( ( (GLuint)dest & 0x2 ) >> 1 );     \
        GLushort *des = (GLushort *)( (GLuint)dest & ~0x2 );    \
        (des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x); } while (0)
#else
#define EMIT_ELT(offset, x) (dest)[offset] = (GLushort) (x)
#endif
#define EMIT_TWO_ELTS(offset, x, y)  *(GLuint *)(dest+offset) = ((y)<<16)|(x);
#define INCR_ELTS( nr ) dest += nr
#define ELTPTR dest
#define RELEASE_ELT_VERTS() \
  r200ReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, __FUNCTION__ )

#define EMIT_INDEXED_VERTS( ctx, start, count ) \
  r200_emit_indexed_verts( ctx, start, count )


#define ALLOC_VERTS( nr ) \
  r200AllocDmaLowVerts( rmesa, nr, rmesa->swtcl.vertex_size * 4 )
#define EMIT_VERTS( ctx, j, nr, buf ) \
  r200_emit_contiguous_verts(ctx, j, (j)+(nr), buf)



#define TAG(x) r200_dma_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/



static GLboolean r200_run_render( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   render_func *tab = TAG(render_tab_verts);

   if (rmesa->swtcl.indexed_verts.buf && (!VB->Elts || stage->changed_inputs)) 
      RELEASE_ELT_VERTS();
   	
   

   if ((R200_DEBUG & DEBUG_VERTS) ||
       rmesa->swtcl.RenderIndex != 0 ||
       !r200_dma_validate_render( ctx, VB ))
      return GL_TRUE;		

   if (VB->Elts) {
      tab = TAG(render_tab_elts);
      if (!rmesa->swtcl.indexed_verts.buf) {
	 if (VB->Count > GET_SUBSEQUENT_VB_MAX_VERTS())
	    return GL_TRUE;
	 EMIT_INDEXED_VERTS(ctx, 0, VB->Count);
      }
   }

   tnl->Driver.Render.Start( ctx );

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (R200_DEBUG & DEBUG_PRIMS)
	 fprintf(stderr, "r200_render.c: prim %s %d..%d\n", 
		 _mesa_lookup_enum_by_nr(prim & PRIM_MODE_MASK), 
		 start, start+length);

      tab[prim & PRIM_MODE_MASK]( ctx, start, start + length, prim );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}



static void r200_check_render( GLcontext *ctx,
				 struct tnl_pipeline_stage *stage )
{
   GLuint inputs = _TNL_BIT_POS | _TNL_BIT_COLOR0;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
	 inputs |= _TNL_BIT_COLOR1;

      if (ctx->Texture.Unit[0]._ReallyEnabled)
	 inputs |= _TNL_BIT_TEX0;

      if (ctx->Texture.Unit[1]._ReallyEnabled)
	 inputs |= _TNL_BIT_TEX1;

      if (ctx->Fog.Enabled)
	 inputs |= _TNL_BIT_FOG;
   }

   stage->inputs = inputs;
}


static void dtr( struct tnl_pipeline_stage *stage )
{
   (void)stage;
}


const struct tnl_pipeline_stage _r200_render_stage =
{
   "r200 render",
   (_DD_NEW_SEPARATE_SPECULAR |
    _NEW_TEXTURE|
    _NEW_FOG|
    _NEW_RENDERMODE),		/* re-check (new inputs) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, 0,			/* changed_inputs, private */
   dtr,				/* destructor */
   r200_check_render,		/* check - initially set to alloc data */
   r200_run_render		/* run */
};



/**************************************************************************/


static const GLuint reduced_hw_prim[GL_POLYGON+1] = {
   R200_VF_PRIM_POINTS,
   R200_VF_PRIM_LINES,
   R200_VF_PRIM_LINES,
   R200_VF_PRIM_LINES,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLES,
   R200_VF_PRIM_TRIANGLES
};

static void r200RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void r200RenderPrimitive( GLcontext *ctx, GLenum prim );
static void r200ResetLineStipple( GLcontext *ctx );

#undef HAVE_QUADS
#define HAVE_QUADS 0

#undef HAVE_QUAD_STRIPS
#define HAVE_QUAD_STRIPS 0

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG r200ContextPtr rmesa
#define CTX_ARG2 rmesa
#define GET_VERTEX_DWORDS() rmesa->swtcl.vertex_size
#define ALLOC_VERTS( n, size ) r200AllocDmaLowVerts( rmesa, n, size * 4 )
#define LOCAL_VARS						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   const char *r200verts = (char *)rmesa->swtcl.verts;
#define VERT(x) (r200Vertex *)(r200verts + ((x) * vertsize * sizeof(int)))
#define VERTEX r200Vertex 
#define DO_DEBUG_VERTS (1 && (R200_DEBUG & DEBUG_VERTS))
#define PRINT_VERTEX(v) r200_print_vertex(rmesa->glCtx, v)
#undef TAG
#define TAG(x) r200_##x
#include "tnl_dd/t_dd_triemit.h"


/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) r200_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     r200_triangle( rmesa, a, b, c )
#define LINE( a, b )       r200_line( rmesa, a, b )
#define POINT( a )         r200_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R200_TWOSIDE_BIT	0x01
#define R200_UNFILLED_BIT	0x02
#define R200_MAX_TRIFUNC	0x04


static struct {
   points_func	        points;
   line_func		line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[R200_MAX_TRIFUNC];


#define DO_FALLBACK  0
#define DO_UNFILLED (IND & R200_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R200_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_OFFSET     0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_INDEX  0
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a < 0)
#define GET_VERTEX(e) (rmesa->swtcl.verts + (e*rmesa->swtcl.vertex_size*sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   r200_color_t *color = (r200_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v0, c )					\
do {								\
   if (havespec) {						\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (havespec) {					\
      v0->v.specular.red   = v1->v.specular.red;	\
      v0->v.specular.green = v1->v.specular.green;	\
      v0->v.specular.blue  = v1->v.specular.blue; 	\
   }							\
} while (0)

/* These don't need LE32_TO_CPU() as they used to save and restore
 * colors which are already in the correct format.
 */
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#undef LOCAL_VARS
#undef TAG
#undef INIT

#define LOCAL_VARS(n)							\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = (rmesa->swtcl.vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (rmesa->swtcl.vertex_size > 4);			\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) r200RasterPrimitive( ctx, reduced_hw_prim[x] )
#define RENDER_PRIMITIVE rmesa->swtcl.render_primitive
#undef TAG
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_TWOSIDE_BIT|R200_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_twoside();
   init_unfilled();
   init_twoside_unfilled();
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      r200_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r200_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r200_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r200_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   r200RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->swtcl.vertex_size;		\
   const char *r200verts = (char *)rmesa->swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) r200ResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r200_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r200_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"



/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

void r200ChooseRenderState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint index = 0;
   GLuint flags = ctx->_TriangleCaps;

   if (!rmesa->TclFallback || rmesa->Fallback) 
      return;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R200_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= R200_UNFILLED_BIT;

   if (index != rmesa->swtcl.RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = r200_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = r200_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = r200_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }

      rmesa->swtcl.RenderIndex = index;
   }
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void r200RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->swtcl.hw_primitive != hwprim) {
      R200_NEWPRIM( rmesa );
      rmesa->swtcl.hw_primitive = hwprim;
   }
}

static void r200RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   rmesa->swtcl.render_primitive = prim;
   if (prim < GL_TRIANGLES || !(ctx->_TriangleCaps & DD_TRI_UNFILLED)) 
      r200RasterPrimitive( ctx, reduced_hw_prim[prim] );
}

static void r200RenderFinish( GLcontext *ctx )
{
}

static void r200ResetLineStipple( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, lin );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "glBlendEquation",
   "glBlendFunc(mode != ADD)",
   "R200_NO_RAST",
   "Mixing GL_CLAMP_TO_BORDER and GL_CLAMP (or GL_MIRROR_CLAMP_ATI)"
};


static const char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}


void r200Fallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 R200_FIREVERTICES( rmesa );
	 TCL_FALLBACK( ctx, R200_TCL_FALLBACK_RASTER, GL_TRUE );
	 _swsetup_Wakeup( ctx );
	 _tnl_need_projected_coords( ctx, GL_TRUE );
	 rmesa->swtcl.RenderIndex = ~0;
         if (R200_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "R200 begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = r200RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = r200RenderPrimitive;
	 tnl->Driver.Render.Finish = r200RenderFinish;
	 tnl->Driver.Render.BuildVertices = r200BuildVertices;
	 tnl->Driver.Render.ResetLineStipple = r200ResetLineStipple;
	 TCL_FALLBACK( ctx, R200_TCL_FALLBACK_RASTER, GL_FALSE );
	 if (rmesa->TclFallback) {
	    /* These are already done if rmesa->TclFallback goes to
	     * zero above. But not if it doesn't (R200_NO_TCL for
	     * example?)
	     */
	    r200ChooseVertexState( ctx );
	    r200ChooseRenderState( ctx );
	 }
         if (R200_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "R200 end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}




/* Cope with depth operations by drawing individual pixels as points??? 
 */
void
r200PointsBitmap( GLcontext *ctx, GLint px, GLint py,
		  GLsizei width, GLsizei height,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLubyte *bitmap )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const GLfloat *rc = ctx->Current.RasterColor; 
   GLint row, col;
   r200Vertex vert;
   GLuint orig_vte;
   GLuint h;


   /* Turn off tcl.  
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_BITMAP, 1 );

   /* Choose tiny vertex format
    */
   r200SetVertexFormat( ctx, R200_XYZW_BIT | R200_RGBA_BIT );

   /* Ready for point primitives:
    */
   r200RenderPrimitive( ctx, GL_POINTS );

   /* Turn off the hw viewport transformation:
    */
   R200_STATECHANGE( rmesa, vte );
   orig_vte = rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL];
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] &= ~(R200_VPORT_X_SCALE_ENA |
					   R200_VPORT_Y_SCALE_ENA |
					   R200_VPORT_Z_SCALE_ENA |
					   R200_VPORT_X_OFFSET_ENA |
					   R200_VPORT_Y_OFFSET_ENA |
					   R200_VPORT_Z_OFFSET_ENA); 

   /* Turn off other stuff:  Stipple?, texture?, blending?, etc.
    */


   /* Populate the vertex
    *
    * Incorporate FOG into RGBA
    */
   if (ctx->Fog.Enabled) {
      const GLfloat *fc = ctx->Fog.Color;
      GLfloat color[4];
      GLfloat f;

      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         f = _swrast_z_to_fogfactor(ctx, ctx->Current.Attrib[VERT_ATTRIB_FOG][0]);
      else
         f = _swrast_z_to_fogfactor(ctx, ctx->Current.RasterDistance);

      color[0] = f * rc[0] + (1.F - f) * fc[0];
      color[1] = f * rc[1] + (1.F - f) * fc[1];
      color[2] = f * rc[2] + (1.F - f) * fc[2];
      color[3] = rc[3];

      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.red,   color[0]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.green, color[1]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.blue,  color[2]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.alpha, color[3]);
   }
   else {
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.red,   rc[0]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.green, rc[1]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.blue,  rc[2]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.alpha, rc[3]);
   }


   vert.tv.z = ctx->Current.RasterPos[2];


   /* Update window height
    */
   LOCK_HARDWARE( rmesa );
   UNLOCK_HARDWARE( rmesa );
   h = rmesa->dri.drawable->h + rmesa->dri.drawable->y;
   px += rmesa->dri.drawable->x;

   /* Clipping handled by existing mechansims in r200_ioctl.c?
    */
   for (row=0; row<height; row++) {
      const GLubyte *src = (const GLubyte *) 
	 _mesa_image_address( unpack, bitmap, width, height, 
			      GL_COLOR_INDEX, GL_BITMAP, 0, row, 0 );

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
	       vert.tv.x = px+col;
	       vert.tv.y = h - (py+row) - 1;
	       r200_point( rmesa, &vert );
            }
	    src += (mask >> 7);
	    mask = ((mask << 1) & 0xff) | (mask >> 7);
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
	       vert.tv.x = px+col;
	       vert.tv.y = h - (py+row) - 1;
	       r200_point( rmesa, &vert );
            }
	    src += mask & 1;
	    mask = ((mask << 7) & 0xff) | (mask >> 1);
         }
         /* get ready for next row */
         if (mask != 128)
            src++;
      }
   }

   /* Fire outstanding vertices, restore state
    */
   R200_STATECHANGE( rmesa, vte );
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = orig_vte;

   /* Unfallback
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_BITMAP, 0 );

   /* Need to restore vertexformat?
    */
   if (rmesa->TclFallback)
      r200ChooseVertexState( ctx );
}


void r200FlushVertices( GLcontext *ctx, GLuint flags )
{
   _tnl_FlushVertices( ctx, flags );

   if (flags & FLUSH_STORED_VERTICES)
      R200_NEWPRIM( R200_CONTEXT( ctx ) );
}

/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void r200InitSwtcl( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      init_setup_tab();
      firsttime = 0;
   }

   tnl->Driver.Render.Start = r200RenderStart;
   tnl->Driver.Render.Finish = r200RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = r200RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = r200ResetLineStipple;
   tnl->Driver.Render.BuildVertices = r200BuildVertices;

   rmesa->swtcl.verts = (GLubyte *)ALIGN_MALLOC( size * 16 * 4, 32 );
   rmesa->swtcl.RenderIndex = ~0;
   rmesa->swtcl.render_primitive = GL_TRIANGLES;
   rmesa->swtcl.hw_primitive = 0;
}


void r200DestroySwtcl( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->swtcl.indexed_verts.buf) 
      r200ReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, __FUNCTION__ );

   if (rmesa->swtcl.verts) {
      ALIGN_FREE(rmesa->swtcl.verts);
      rmesa->swtcl.verts = 0;
   }
}
