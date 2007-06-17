/**************************************************************************

Copyright (C) 2007 Dave Airlie

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
THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Dave Airlie <airlied@linux.ie>
 */

/* derived from r200 swtcl path */



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

#include "r300_context.h"
#include "r300_swtcl.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "r300_emit.h"
#include "r300_mem.h"

static void flush_last_swtcl_prim( r300ContextPtr rmesa  );

void r300EmitVertexAOS(r300ContextPtr rmesa, GLuint vertex_size, GLuint offset);
void r300EmitVbufPrim(r300ContextPtr rmesa, GLuint primitive, GLuint vertex_nr);
#define EMIT_ATTR( ATTR, STYLE, F0 )					\
do {									\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].attrib = (ATTR);	\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].format = (STYLE);	\
   rmesa->swtcl.vertex_attr_count++;					\
   vap_fmt_0 |= F0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].attrib = 0;		\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].format = EMIT_PAD;	\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].offset = (N);		\
   rmesa->swtcl.vertex_attr_count++;					\
} while (0)

static void r300SetVertexFormat( GLcontext *ctx )
{
   r300ContextPtr rmesa = R300_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   int vap_fmt_0 = 0;
   int vap_fmt_1 = 0;
   int vap_vte_cntl = 0;
   int offset = 0;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   /* Important:
    */
   if ( VB->NdcPtr != NULL ) {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   }
   else {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->ClipPtr;
   }

   assert( VB->AttribPtr[VERT_ATTRIB_POS] != NULL );
   rmesa->swtcl.vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if ( !rmesa->swtcl.needproj ||
       RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) { /* need w coord for projected textures */
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F, R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT);
      vap_vte_cntl |= R300_VTX_XY_FMT | R300_VTX_Z_FMT | R300_VTX_W0_FMT;

      offset = 4;
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F, R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT );
      vap_vte_cntl |= R300_VTX_XY_FMT | R300_VTX_Z_FMT;
      offset = 3;
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_POINTSIZE )) {
      EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT);
      offset += 1;
   }

   rmesa->swtcl.coloroffset = offset;
#if MESA_LITTLE_ENDIAN 
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT );
#else
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT );
#endif
   offset += 1;

   rmesa->swtcl.specoffset = 0;
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {

#if MESA_LITTLE_ENDIAN 
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_RGB, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT );
      }
      else {
	 EMIT_PAD( 3 );
      }

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT );
      }
      else {
	 EMIT_PAD( 1 );
      }
#else
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT );
      }
      else {
	 EMIT_PAD( 1 );
      }

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT );
      }
      else {
	 EMIT_PAD( 3 );
      }
#endif
   }

   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      int i;

      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(i) )) {
	    GLuint sz = VB->TexCoordPtr[i]->size;

	    vap_fmt_1 |= sz << (3 * i);
	    EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_1F + sz - 1, 0 );
	 }
      }
   }

#if 0
   if ( (rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] & R200_FOG_USE_MASK)
      != R200_FOG_USE_SPEC_ALPHA ) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_USE_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= R200_FOG_USE_SPEC_ALPHA;
   }
#endif

   if (!RENDERINPUTS_EQUAL( rmesa->tnl_index_bitset, index_bitset ) ||
	(rmesa->hw.vof.cmd[R300_VOF_CNTL_0] != vap_fmt_0) ||
	(rmesa->hw.vof.cmd[R300_VOF_CNTL_1] != vap_fmt_1) ) {
//      R200_NEWPRIM(rmesa);
      R300_STATECHANGE(rmesa, vof);
      rmesa->hw.vof.cmd[R300_VOF_CNTL_0] =
	      vap_fmt_0;
      rmesa->hw.vof.cmd[R300_VOF_CNTL_1] =
	      vap_fmt_1;

      rmesa->swtcl.vertex_size =
	  _tnl_install_attrs( ctx,
			      rmesa->swtcl.vertex_attrs, 
			      rmesa->swtcl.vertex_attr_count,
			      NULL, 0 );
      rmesa->swtcl.vertex_size /= 4;
      RENDERINPUTS_COPY( rmesa->tnl_index_bitset, index_bitset );
   }
}


/* Flush vertices in the current dma region.
 */
static void flush_last_swtcl_prim( r300ContextPtr rmesa  )
{
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   rmesa->dma.flush = NULL;

   if (rmesa->dma.current.buf) {
     struct r300_dma_region *current = &rmesa->dma.current;
     //     GLuint current_offset = rmesa->state.swtcl_dma.aos_offset;

     GLuint current_offset = r300_mem_offset(rmesa,current->buf->id);
     //      assert (!(rmesa->swtcl.hw_primitive & R200_VF_PRIM_WALK_IND));

      assert (current->start + 
	      rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	      current->ptr);

      if (rmesa->dma.current.start != rmesa->dma.current.ptr) {
	//	 r200EnsureCmdBufSpace( rmesa, VERT_AOS_BUFSZ +
	//			        rmesa->hw.max_state_size + VBUF_BUFSZ );
	r300EmitVertexAOS( rmesa,
			   rmesa->swtcl.vertex_size,
			   current_offset);
	
	r300EmitVbufPrim( rmesa,
			  rmesa->swtcl.hw_primitive,
			  rmesa->swtcl.numverts);
      }

      rmesa->swtcl.numverts = 0;
      current->start = current->ptr;
   }
}


static void *
r300AllocDmaLowVerts( r300ContextPtr rmesa, int nverts, int vsize )
{
  GLuint bytes = vsize * nverts;

  r300AllocDmaRegion(rmesa, &rmesa->state.swtcl_dma, bytes, 0);

  if (!rmesa->dma.flush) {
    rmesa->dma.flush = flush_last_swtcl_prim;
    rmesa->radeon.glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
  }


  rmesa->swtcl.numverts += nverts;
  rmesa->dma.current.ptr += bytes;
  return (rmesa->dma.current.address + rmesa->dma.current.ptr);
}

static void r300RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void r300RenderPrimitive( GLcontext *ctx, GLenum prim );
//static void r300ResetLineStipple( GLcontext *ctx );

static void r300PrintVertex(r300Vertex *v)
{
  fprintf(stderr,"Vertex %p\n", v);

}

static const GLenum reduced_prim[GL_POLYGON+1] = {
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/


#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG r300ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->swtcl.vertex_size
#define ALLOC_VERTS( n, size ) r300AllocDmaLowVerts( rmesa, n, size * 4 )
#define LOCAL_VARS						\
   r300ContextPtr rmesa = R300_CONTEXT(ctx);		\
   const char *r300verts = (char *)rmesa->swtcl.verts;
#define VERT(x) (r300Vertex *)(r300verts + ((x) * vertsize * sizeof(int)))
#define VERTEX r300Vertex 
#define DO_DEBUG_VERTS (1 && (RADEON_DEBUG & DEBUG_VERTS))
#define PRINT_VERTEX(x) r300PrintVertex(x)
#undef TAG
#define TAG(x) r300_##x
#include "tnl_dd/t_dd_triemit.h"



/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) r300_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     r300_triangle( rmesa, a, b, c )
#define LINE( a, b )       r300_line( rmesa, a, b )
#define POINT( a )         r300_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R300_TWOSIDE_BIT	0x01
#define R300_UNFILLED_BIT	0x02
#define R300_MAX_TRIFUNC	0x04

static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R300_MAX_TRIFUNC];

#define DO_FALLBACK  0
#define DO_UNFILLED (IND & R300_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R300_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_OFFSET     0
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
#define GET_VERTEX(e) (rmesa->swtcl.verts + (e*rmesa->swtcl.vertex_size*sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   r300_color_t *color = (r300_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
do {								\
   if (specoffset) {						\
      r300_color_t *spec = (r300_color_t *)&((v)->ui[specoffset]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (specoffset) {					\
      r300_color_t *spec0 = (r300_color_t *)&((v0)->ui[specoffset]);	\
      r300_color_t *spec1 = (r300_color_t *)&((v1)->ui[specoffset]);	\
      spec0->red   = spec1->red;	\
      spec0->green = spec1->green;	\
      spec0->blue  = spec1->blue; 	\
   }							\
} while (0)

/* These don't need LE32_TO_CPU() as they used to save and restore
 * colors which are already in the correct format.
 */
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]

#undef LOCAL_VARS
#undef TAG
#undef INIT

#define LOCAL_VARS(n)							\
   r300ContextPtr rmesa = R300_CONTEXT(ctx);			\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = rmesa->swtcl.coloroffset;	\
   GLuint specoffset = rmesa->swtcl.specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) r300RasterPrimitive( ctx, reduced_prim[x] )
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

#define IND (R300_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R300_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R300_TWOSIDE_BIT|R300_UNFILLED_BIT)
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
      r300_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r300_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r300_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r300_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   r300RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r300ContextPtr rmesa = R300_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->swtcl.vertex_size;		\
   const char *r300verts = (char *)rmesa->swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	//if ( stipple ) r200ResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r300_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r300_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"




/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

void r300ChooseRenderState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   GLuint index = 0;
   GLuint flags = ctx->_TriangleCaps;

   // if (!rmesa->TclFallback || rmesa->Fallback) 
//      return;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R300_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= R300_UNFILLED_BIT;

   if (index != rmesa->swtcl.RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	tnl->Driver.Render.PrimTabVerts = r300_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = r300_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = r300_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }

      rmesa->swtcl.RenderIndex = index;
   }
}


static void r300RenderStart(GLcontext *ctx)
{
  r300ContextPtr rmesa = R300_CONTEXT( ctx );
	fprintf(stderr, "%s\n", __FUNCTION__);

	r300SetVertexFormat(ctx);
	if (rmesa->dma.flush != 0 && 
	    rmesa->dma.flush != flush_last_swtcl_prim)
	  rmesa->dma.flush( rmesa );
}

static void r300RenderFinish(GLcontext *ctx)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
}

static void r300RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	
	if (rmesa->swtcl.hw_primitive != hwprim) {
//		R300_NEWPRIM( rmesa );
		rmesa->swtcl.hw_primitive = hwprim;
	}
}

static void r300RenderPrimitive(GLcontext *ctx, GLenum prim)
{

	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	rmesa->swtcl.render_primitive = prim;
	if (prim < GL_TRIANGLES || !(ctx->_TriangleCaps & DD_TRI_UNFILLED)) 
		r300RasterPrimitive( ctx, reduced_prim[prim] );
	fprintf(stderr, "%s\n", __FUNCTION__);
	
}

static void r300ResetLineStipple(GLcontext *ctx)
{


}

void r300InitSwtcl(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	static int firsttime = 1;
	
	if (firsttime) {
		init_rast_tab();
		firsttime = 0;
	}
	
	tnl->Driver.Render.Start = r300RenderStart;
	tnl->Driver.Render.Finish = r300RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = r300RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = r300ResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;
	
	/* FIXME: what are these numbers? */
	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
			    36 * sizeof(GLfloat) );
	
	rmesa->swtcl.verts = (GLubyte *)tnl->clipspace.vertex_buf;
	rmesa->swtcl.RenderIndex = ~0;
	rmesa->swtcl.render_primitive = GL_TRIANGLES;
	rmesa->swtcl.hw_primitive = 0;	

	_tnl_invalidate_vertex_state( ctx, ~0 );
	_tnl_invalidate_vertices( ctx, ~0 );
	RENDERINPUTS_ZERO( rmesa->tnl_index_bitset );

	r300ChooseRenderState(ctx);
}

void r300DestroySwtcl(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
//	if (rmesa->swtcl.indexed_verts.buf) 
//		r200ReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, __FUNCTION__ );

}

void r300EmitVertexAOS(r300ContextPtr rmesa, GLuint vertex_size, GLuint offset)
{
	int cmd_reserved = 0;
	int cmd_written = 0;

  drm_radeon_cmd_header_t *cmd = NULL;
  if (1)//RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s:  vertex_size %d, offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);

  start_packet3(CP_PACKET3(R300_PACKET3_3D_LOAD_VBPNTR, 2), 2);
  e32(1);
  e32(vertex_size | (vertex_size << 8));
  e32(offset);
}

void r300EmitVbufPrim(r300ContextPtr rmesa, GLuint primitive, GLuint vertex_nr)
{

	int cmd_reserved = 0;
	int cmd_written = 0;

  drm_radeon_cmd_header_t *cmd = NULL;
  r300EmitState(rmesa);
  
  start_packet3(CP_PACKET3(R300_PACKET3_3D_DRAW_VBUF_2, 0), 0);
  e32(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (vertex_nr << 16) | primitive);
}
