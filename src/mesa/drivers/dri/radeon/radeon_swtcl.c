/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_swtcl.c,v 1.6 2003/05/06 23:52:08 daenzer Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
#include "imports.h"
#include "macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vtx_api.h"	/* for _tnl_FlushVertices */

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/


#define RADEON_XYZW_BIT		0x01
#define RADEON_RGBA_BIT		0x02
#define RADEON_SPEC_BIT		0x04
#define RADEON_TEX0_BIT		0x08
#define RADEON_TEX1_BIT		0x10
#define RADEON_PTEX_BIT		0x20
#define RADEON_MAX_SETUP	0x40

static void flush_last_swtcl_prim( radeonContextPtr rmesa  );

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   tnl_interp_func		interp;
   tnl_copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_format;
} setup_tab[RADEON_MAX_SETUP];


#define TINY_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_PKCOLOR)

#define NOTEX_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC)

#define TEX0_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0)

#define TEX1_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0 |	\
					 RADEON_CP_VC_FRMT_ST1)

#define PROJ_TEX1_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0 |	\
					 RADEON_CP_VC_FRMT_Q0 |         \
					 RADEON_CP_VC_FRMT_ST1 |	\
					 RADEON_CP_VC_FRMT_Q1)

#define TEX2_VERTEX_FORMAT 0
#define TEX3_VERTEX_FORMAT 0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & RADEON_XYZW_BIT)
#define DO_RGBA (IND & RADEON_RGBA_BIT)
#define DO_SPEC (IND & RADEON_SPEC_BIT)
#define DO_FOG  (IND & RADEON_SPEC_BIT)
#define DO_TEX0 (IND & RADEON_TEX0_BIT)
#define DO_TEX1 (IND & RADEON_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & RADEON_PTEX_BIT)

#define VERTEX radeonVertex
#define VERTEX_COLOR radeon_color_t
#define GET_VIEWPORT_MAT() 0
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() RADEON_CONTEXT(ctx)->swtcl.vertex_format
#define GET_VERTEX_STORE() RADEON_CONTEXT(ctx)->swtcl.verts
#define GET_VERTEX_SIZE() RADEON_CONTEXT(ctx)->swtcl.vertex_size * sizeof(GLuint)

#define HAVE_HW_VIEWPORT    1
/* Tiny vertices don't seem to work atm - haven't looked into why.
 */
#define HAVE_HW_DIVIDE      (IND & ~(RADEON_XYZW_BIT|RADEON_RGBA_BIT))
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

#define INTERP_VERTEX setup_tab[RADEON_CONTEXT(ctx)->swtcl.SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[RADEON_CONTEXT(ctx)->swtcl.SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) radeon_##x
#define IND ~0
#include "tnl_dd/t_dd_vb.c"
#undef IND


/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_TEX1_BIT|\
             RADEON_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_TEX1_BIT|RADEON_PTEX_BIT)
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



void radeonPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & RADEON_XYZW_BIT)      ? " xyzw," : "",
	   (flags & RADEON_RGBA_BIT)     ? " rgba," : "",
	   (flags & RADEON_SPEC_BIT)     ? " spec/fog," : "",
	   (flags & RADEON_TEX0_BIT)     ? " tex-0," : "",
	   (flags & RADEON_TEX1_BIT)     ? " tex-1," : "",
	   (flags & RADEON_PTEX_BIT)     ? " proj-tex," : "");
}


static void radeonRenderStart( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );

   if (!setup_tab[rmesa->swtcl.SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = rmesa->swtcl.SetupIndex |= (RADEON_PTEX_BIT|RADEON_RGBA_BIT);

      /* Projective textures are handled nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != rmesa->swtcl.vertex_format) {
	 RADEON_NEWPRIM(rmesa);
	 rmesa->swtcl.vertex_format = setup_tab[ind].vertex_format;
	 rmesa->swtcl.vertex_size = setup_tab[ind].vertex_size;
      }

      if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[rmesa->swtcl.SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[rmesa->swtcl.SetupIndex].copy_pv;
      }
   }
   
   if (rmesa->dma.flush != 0 && 
       rmesa->dma.flush != flush_last_swtcl_prim)
      rmesa->dma.flush( rmesa );
}


void radeonBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			   GLuint newinputs )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   GLuint stride = rmesa->swtcl.vertex_size * sizeof(int);
   GLubyte *v = ((GLubyte *)rmesa->swtcl.verts + (start * stride));

   newinputs |= rmesa->swtcl.SetupNewInputs;
   rmesa->swtcl.SetupNewInputs = 0;

   if (!newinputs)
      return;

   setup_tab[rmesa->swtcl.SetupIndex].emit( ctx, start, count, v, stride );
}

void radeonChooseVertexState( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = (RADEON_XYZW_BIT | RADEON_RGBA_BIT);

   if (!rmesa->TclFallback || rmesa->Fallback)
      return;

   if (ctx->Fog.Enabled || (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR))
      ind |= RADEON_SPEC_BIT;

   if (ctx->Texture._EnabledUnits & 0x2)
      /* unit 1 enabled */
      ind |= RADEON_TEX0_BIT|RADEON_TEX1_BIT;
   else if (ctx->Texture._EnabledUnits & 0x1)
      /* unit 0 enabled */
      ind |= RADEON_TEX0_BIT;

   rmesa->swtcl.SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      tnl->Driver.Render.Interp = radeon_interp_extras;
      tnl->Driver.Render.CopyPV = radeon_copy_pv_extras;
   }
   else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }

   if (setup_tab[ind].vertex_format != rmesa->swtcl.vertex_format) {
      RADEON_NEWPRIM(rmesa);
      rmesa->swtcl.vertex_format = setup_tab[ind].vertex_format;
      rmesa->swtcl.vertex_size = setup_tab[ind].vertex_size;
   }

   {
      GLuint se_coord_fmt, needproj;

      /* HW perspective divide is a win, but tiny vertex formats are a
       * bigger one.
       */
      if (setup_tab[ind].vertex_format == TINY_VERTEX_FORMAT ||
	  (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 needproj = GL_TRUE;
	 se_coord_fmt = (RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
			 RADEON_VTX_Z_PRE_MULT_1_OVER_W0 |
			 RADEON_TEX1_W_ROUTING_USE_Q1);
      }
      else {
	 needproj = GL_FALSE;
	 se_coord_fmt = (RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
			 RADEON_TEX1_W_ROUTING_USE_Q1);
      }

      if ( se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT] ) {
	 RADEON_STATECHANGE( rmesa, set );
	 rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
      }
      _tnl_need_projected_coords( ctx, needproj );
   }
}


/* Flush vertices in the current dma region.
 */
static void flush_last_swtcl_prim( radeonContextPtr rmesa  )
{
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   rmesa->dma.flush = NULL;

   if (rmesa->dma.current.buf) {
      struct radeon_dma_region *current = &rmesa->dma.current;
      GLuint current_offset = (rmesa->radeonScreen->gart_buffer_offset +
			       current->buf->buf->idx * RADEON_BUFFER_SIZE + 
			       current->start);

      assert (!(rmesa->swtcl.hw_primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));

      assert (current->start + 
	      rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	      current->ptr);

      if (rmesa->dma.current.start != rmesa->dma.current.ptr) {
	 radeonEnsureCmdBufSpace( rmesa, VERT_AOS_BUFSZ +
			          rmesa->hw.max_state_size + VBUF_BUFSZ );
	 radeonEmitVertexAOS( rmesa,
			      rmesa->swtcl.vertex_size,
			      current_offset);

	 radeonEmitVbufPrim( rmesa,
			     rmesa->swtcl.vertex_format,
			     rmesa->swtcl.hw_primitive,
			     rmesa->swtcl.numverts);
      }

      rmesa->swtcl.numverts = 0;
      current->start = current->ptr;
   }
}


/* Alloc space in the current dma region.
 */
static __inline void *radeonAllocDmaLowVerts( radeonContextPtr rmesa,
					      int nverts, int vsize )
{
   GLuint bytes = vsize * nverts;

   if ( rmesa->dma.current.ptr + bytes > rmesa->dma.current.end ) 
      radeonRefillCurrentDmaRegion( rmesa );

   if (!rmesa->dma.flush) {
      rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
      rmesa->dma.flush = flush_last_swtcl_prim;
   }

   assert( vsize == rmesa->swtcl.vertex_size * 4 );
   assert( rmesa->dma.flush == flush_last_swtcl_prim );
   assert (rmesa->dma.current.start + 
	   rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	   rmesa->dma.current.ptr);


   {
      GLubyte *head = (GLubyte *)(rmesa->dma.current.address + rmesa->dma.current.ptr);
      rmesa->dma.current.ptr += bytes;
      rmesa->swtcl.numverts += nverts;
      return head;
   }

}




static void *radeon_emit_contiguous_verts( GLcontext *ctx, 
					   GLuint start, 
					   GLuint count,
					   void *dest)
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint stride = rmesa->swtcl.vertex_size * 4;
   setup_tab[rmesa->swtcl.SetupIndex].emit( ctx, start, count, dest, stride );
   return (void *)((char *)dest + stride * (count - start));
}



void radeon_emit_indexed_verts( GLcontext *ctx, GLuint start, GLuint count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   radeonAllocDmaRegionVerts( rmesa, 
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
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    0
#define HAVE_ELTS        1

static const GLuint hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   0,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN,
   0,
   0,
   0
};

static __inline void radeonDmaPrimitive( radeonContextPtr rmesa, GLenum prim )
{
   RADEON_NEWPRIM( rmesa );
   rmesa->swtcl.hw_primitive = hw_prim[prim];
   assert(rmesa->dma.current.ptr == rmesa->dma.current.start);
}

static __inline void radeonEltPrimitive( radeonContextPtr rmesa, GLenum prim )
{
   RADEON_NEWPRIM( rmesa );
   rmesa->swtcl.hw_primitive = hw_prim[prim] | RADEON_CP_VC_CNTL_PRIM_WALK_IND;
}




#define LOCAL_VARS radeonContextPtr rmesa = RADEON_CONTEXT(ctx)
#define ELTS_VARS( buf )  GLushort *dest = buf; (void)rmesa;
#define INIT( prim ) radeonDmaPrimitive( rmesa, prim )
#define ELT_INIT(prim) radeonEltPrimitive( rmesa, prim )
#define FLUSH()  RADEON_NEWPRIM( rmesa )
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)rmesa->dma.current.end - (int)rmesa->dma.current.ptr) / (rmesa->swtcl.vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  ((RADEON_BUFFER_SIZE) / (rmesa->swtcl.vertex_size*4))

#if RADEON_OLD_PACKETS
# define GET_CURRENT_VB_MAX_ELTS() \
  ((RADEON_CMD_BUF_SZ - (rmesa->store.cmd_used + 24)) / 2)
#else
# define GET_CURRENT_VB_MAX_ELTS() \
  ((RADEON_CMD_BUF_SZ - (rmesa->store.cmd_used + 16)) / 2)
#endif
#define GET_SUBSEQUENT_VB_MAX_ELTS() \
  ((RADEON_CMD_BUF_SZ - 1024) / 2)


static void *radeon_alloc_elts( radeonContextPtr rmesa, int nr )
{
   if (rmesa->dma.flush == radeonFlushElts &&
       rmesa->store.cmd_used + nr*2 < RADEON_CMD_BUF_SZ) {

      rmesa->store.cmd_used += nr*2;

      return (void *)(rmesa->store.cmd_buf + rmesa->store.cmd_used);
   }
   else {
      if (rmesa->dma.flush) {
	 rmesa->dma.flush( rmesa );
      }

      radeonEmitVertexAOS( rmesa,
			   rmesa->swtcl.vertex_size,
			   (rmesa->radeonScreen->gart_buffer_offset +
			    rmesa->swtcl.indexed_verts.buf->buf->idx *
			    RADEON_BUFFER_SIZE +
			    rmesa->swtcl.indexed_verts.start));

      return (void *) radeonAllocEltsOpenEnded( rmesa,
						rmesa->swtcl.vertex_format,
						rmesa->swtcl.hw_primitive,
						nr );
   }
}

#define ALLOC_ELTS(nr) radeon_alloc_elts(rmesa, nr)

#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(offset, x) do {				\
	int off = offset + ( ( (GLuint)dest & 0x2 ) >> 1 );	\
	GLushort *des = (GLushort *)( (GLuint)dest & ~0x2 );	\
	(des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x); 	\
	(void)rmesa; } while (0)
#else
#define EMIT_ELT(offset, x) do {				\
	(dest)[offset] = (GLushort) (x);			\
	(void)rmesa; } while (0)
#endif
#define EMIT_TWO_ELTS(offset, x, y)  *(GLuint *)(dest+offset) = ((y)<<16)|(x);
#define INCR_ELTS( nr ) dest += nr
#define ELTPTR dest
#define RELEASE_ELT_VERTS() \
  radeonReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, __FUNCTION__ )
#define EMIT_INDEXED_VERTS( ctx, start, count ) \
  radeon_emit_indexed_verts( ctx, start, count )


#define ALLOC_VERTS( nr ) \
  radeonAllocDmaLowVerts( rmesa, nr, rmesa->swtcl.vertex_size * 4 )
#define EMIT_VERTS( ctx, j, nr, buf ) \
  radeon_emit_contiguous_verts(ctx, j, (j)+(nr), buf)

#define TAG(x) radeon_dma_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean radeon_run_render( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   tnl_render_func *tab = TAG(render_tab_verts);
   GLuint i;

   if (rmesa->swtcl.indexed_verts.buf && (!VB->Elts || stage->changed_inputs)) 
      RELEASE_ELT_VERTS();
   	
   if (rmesa->swtcl.RenderIndex != 0 ||   
       !radeon_dma_validate_render( ctx, VB ))
      return GL_TRUE;		

   tnl->Driver.Render.Start( ctx );

   if (VB->Elts) {
      tab = TAG(render_tab_elts);
      if (!rmesa->swtcl.indexed_verts.buf) {
	 if (VB->Count > GET_SUBSEQUENT_VB_MAX_VERTS())
	    return GL_TRUE;
	 EMIT_INDEXED_VERTS(ctx, 0, VB->Count);
      }
   }

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (RADEON_DEBUG & DEBUG_PRIMS)
	 fprintf(stderr, "radeon_render.c: prim %s %d..%d\n", 
		 _mesa_lookup_enum_by_nr(prim & PRIM_MODE_MASK), 
		 start, start+length);

      if (length)
	 tab[prim & PRIM_MODE_MASK]( ctx, start, start + length, prim );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}



static void radeon_check_render( GLcontext *ctx,
				 struct tnl_pipeline_stage *stage )
{
   stage->inputs = TNL_CONTEXT(ctx)->render_inputs;
}


static void dtr( struct tnl_pipeline_stage *stage )
{
   (void)stage;
}


const struct tnl_pipeline_stage _radeon_render_stage =
{
   "radeon render",
   (_DD_NEW_SEPARATE_SPECULAR |
    _NEW_TEXTURE|
    _NEW_FOG|
    _NEW_RENDERMODE),		/* re-check (new inputs) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, NULL,			/* changed_inputs, private */
   dtr,				/* destructor */
   radeon_check_render,		/* check - initially set to alloc data */
   radeon_run_render		/* run */
};


/**************************************************************************/

/* Radeon texture rectangle expects coords in 0..1 range, not 0..dimension
 * as in the extension spec.  Need to translate here.
 *
 * Note that swrast expects 0..dimension, so if a fallback is active,
 * don't do anything.  (Maybe need to configure swrast to match hw)
 */
struct texrect_stage_data {
   GLvector4f texcoord[MAX_TEXTURE_UNITS];
};

#define TEXRECT_STAGE_DATA(stage) ((struct texrect_stage_data *)stage->privatePtr)


static GLboolean run_texrect_stage( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   struct texrect_stage_data *store = TEXRECT_STAGE_DATA(stage);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   if (rmesa->Fallback)
      return GL_TRUE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
      if (!(ctx->Texture.Unit[i]._ReallyEnabled & TEXTURE_RECT_BIT))
	 continue;
   
      if (stage->changed_inputs & VERT_BIT_TEX(i)) {
	 struct gl_texture_object *texObj = ctx->Texture.Unit[i].CurrentRect;
	 struct gl_texture_image *texImage = texObj->Image[0][texObj->BaseLevel];
	 const GLfloat iw = 1.0/texImage->Width;
	 const GLfloat ih = 1.0/texImage->Height;
	 GLfloat *in = (GLfloat *)VB->TexCoordPtr[i]->data;
	 GLint instride = VB->TexCoordPtr[i]->stride;
	 GLfloat (*out)[4] = store->texcoord[i].data;
	 GLint j;
	 
	 for (j = 0 ; j < VB->Count ; j++) {
	    out[j][0] = in[0] * iw;
	    out[j][1] = in[1] * ih;
	    in = (GLfloat *)((GLubyte *)in + instride);
	 }
      }

      VB->TexCoordPtr[i] = &store->texcoord[i];
   }

   return GL_TRUE;
}


/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_texrect_data( GLcontext *ctx,
				     struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texrect_stage_data *store;
   GLuint i;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = TEXRECT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _mesa_vector4f_alloc( &store->texcoord[i], 0, VB->Size, 32 );

   /* Now run the stage.
    */
   stage->run = run_texrect_stage;
   return stage->run( ctx, stage );
}


static void check_texrect( GLcontext *ctx,
			   struct tnl_pipeline_stage *stage )
{
   GLuint flags = 0;

   if (ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_RECT_BIT)
      flags |= VERT_BIT_TEX0;

   if (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_RECT_BIT)
      flags |= VERT_BIT_TEX1;

   stage->inputs = flags;
   stage->outputs = flags;
   stage->active = (flags != 0);
}


static void free_texrect_data( struct tnl_pipeline_stage *stage )
{
   struct texrect_stage_data *store = TEXRECT_STAGE_DATA(stage);
   GLuint i;

   if (store) {
      for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
	 if (store->texcoord[i].data)
	    _mesa_vector4f_free( &store->texcoord[i] );
      FREE( store );
      stage->privatePtr = NULL;
   }
}


const struct tnl_pipeline_stage _radeon_texrect_stage =
{
   "radeon texrect stage",			/* name */
   _NEW_TEXTURE,	/* check_state */
   _NEW_TEXTURE,	/* run_state */
   GL_TRUE,				/* active? */
   0,					/* inputs */
   0,					/* outputs */
   0,					/* changed_inputs */
   NULL,				/* private data */
   free_texrect_data,			/* destructor */
   check_texrect,			/* check */
   alloc_texrect_data,			/* run -- initially set to init */
};


/**************************************************************************/


static const GLuint reduced_hw_prim[GL_POLYGON+1] = {
   RADEON_CP_VC_CNTL_PRIM_TYPE_POINT,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_LINE,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST,
   RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST
};

static void radeonRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void radeonRenderPrimitive( GLcontext *ctx, GLenum prim );
static void radeonResetLineStipple( GLcontext *ctx );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG radeonContextPtr rmesa
#define CTX_ARG2 rmesa
#define GET_VERTEX_DWORDS() rmesa->swtcl.vertex_size
#define ALLOC_VERTS( n, size ) radeonAllocDmaLowVerts( rmesa, n, size * 4 )
#undef LOCAL_VARS
#define LOCAL_VARS						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);		\
   const char *radeonverts = (char *)rmesa->swtcl.verts;
#define VERT(x) (radeonVertex *)(radeonverts + (x * vertsize * sizeof(int)))
#define VERTEX radeonVertex 
#undef TAG
#define TAG(x) radeon_##x
#include "tnl_dd/t_dd_triemit.h"


/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) radeon_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     radeon_triangle( rmesa, a, b, c )
#define LINE( a, b )       radeon_line( rmesa, a, b )
#define POINT( a )         radeon_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define RADEON_TWOSIDE_BIT	0x01
#define RADEON_UNFILLED_BIT	0x02
#define RADEON_MAX_TRIFUNC	0x08


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[RADEON_MAX_TRIFUNC];


#define DO_FALLBACK  0
#define DO_OFFSET    0
#define DO_UNFILLED (IND & RADEON_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & RADEON_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
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
#define GET_VERTEX(e) (rmesa->swtcl.verts + (e * rmesa->swtcl.vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   radeon_color_t *color = (radeon_color_t *)&((v)->ui[coloroffset]);	\
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
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);			\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = (rmesa->swtcl.vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (rmesa->swtcl.vertex_size > 4);			\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) radeonRasterPrimitive( ctx, reduced_hw_prim[x] )
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

#define IND (RADEON_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (RADEON_TWOSIDE_BIT|RADEON_UNFILLED_BIT)
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

#define VERT(x) (radeonVertex *)(radeonverts + (x * vertsize * sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      radeon_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   radeon_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   radeon_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   radeon_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#undef INIT
#define INIT(x) do {					\
   radeonRenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->swtcl.vertex_size;		\
   const char *radeonverts = (char *)rmesa->swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) radeonResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) radeon_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) radeon_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"



/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

void radeonChooseRenderState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint index = 0;
   GLuint flags = ctx->_TriangleCaps;

   if (!rmesa->TclFallback || rmesa->Fallback) 
      return;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= RADEON_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= RADEON_UNFILLED_BIT;

   if (index != rmesa->swtcl.RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = radeon_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = radeon_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = radeon_fast_clipped_poly;
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


static void radeonRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (rmesa->swtcl.hw_primitive != hwprim) {
      RADEON_NEWPRIM( rmesa );
      rmesa->swtcl.hw_primitive = hwprim;
   }
}

static void radeonRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   rmesa->swtcl.render_primitive = prim;
   if (prim < GL_TRIANGLES || !(ctx->_TriangleCaps & DD_TRI_UNFILLED)) 
      radeonRasterPrimitive( ctx, reduced_hw_prim[prim] );
}

static void radeonRenderFinish( GLcontext *ctx )
{
}

static void radeonResetLineStipple( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   RADEON_STATECHANGE( rmesa, lin );
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
   "glBlendFunc",
   "RADEON_NO_RAST",
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


void radeonFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 RADEON_FIREVERTICES( rmesa );
	 TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_RASTER, GL_TRUE );
	 _swsetup_Wakeup( ctx );
	 _tnl_need_projected_coords( ctx, GL_TRUE );
	 rmesa->swtcl.RenderIndex = ~0;
         if (RADEON_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "Radeon begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = radeonRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
	 tnl->Driver.Render.Finish = radeonRenderFinish;
	 tnl->Driver.Render.BuildVertices = radeonBuildVertices;
	 tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
	 TCL_FALLBACK( ctx, RADEON_TCL_FALLBACK_RASTER, GL_FALSE );
	 if (rmesa->TclFallback) {
	    /* These are already done if rmesa->TclFallback goes to
	     * zero above. But not if it doesn't (RADEON_NO_TCL for
	     * example?)
	     */
	    radeonChooseVertexState( ctx );
	    radeonChooseRenderState( ctx );
	 }
         if (RADEON_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "Radeon end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}


void radeonFlushVertices( GLcontext *ctx, GLuint flags )
{
   _tnl_FlushVertices( ctx, flags );

   if (flags & FLUSH_STORED_VERTICES)
      RADEON_NEWPRIM( RADEON_CONTEXT( ctx ) );
}

/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void radeonInitSwtcl( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      init_setup_tab();
      firsttime = 0;
   }

   tnl->Driver.Render.Start = radeonRenderStart;
   tnl->Driver.Render.Finish = radeonRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = radeonRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = radeonResetLineStipple;
   tnl->Driver.Render.BuildVertices = radeonBuildVertices;

   rmesa->swtcl.verts = (GLubyte *)ALIGN_MALLOC( size * 16 * 4, 32 );
   rmesa->swtcl.RenderIndex = ~0;
   rmesa->swtcl.render_primitive = GL_TRIANGLES;
   rmesa->swtcl.hw_primitive = 0;
}


void radeonDestroySwtcl( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (rmesa->swtcl.indexed_verts.buf) 
      radeonReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, 
			      __FUNCTION__ );

   if (rmesa->swtcl.verts) {
      ALIGN_FREE(rmesa->swtcl.verts);
      rmesa->swtcl.verts = NULL;
   }

}
