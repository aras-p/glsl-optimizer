/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tris.c,v 1.8 2002/10/30 12:51:43 alanh Exp $ */ /* -*- c-basic-offset: 3 -*- */
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
#include "colormac.h"
#include "macros.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "r128_tris.h"
#include "r128_state.h"
#include "r128_tex.h"
#include "r128_vb.h"
#include "r128_ioctl.h"

static const GLuint hw_prim[GL_POLYGON+1] = {
   R128_CCE_VC_CNTL_PRIM_TYPE_POINT,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
};

static void r128RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void r128RenderPrimitive( GLcontext *ctx, GLenum prim );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#if defined(USE_X86_ASM)
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
			      : "0" (vertsize),				\
			        "D" ((long)vb),				\
			        "S" ((long)v) );			\
} while (0)
#else
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
   for ( j = 0 ; j < vertsize ; j++ )					\
      vb[j] = CPU_TO_LE32(((GLuint *)v)[j]);				\
   vb += vertsize;							\
} while (0)
#endif

static __inline void r128_draw_quad( r128ContextPtr rmesa,
				     r128VertexPtr v0,
				     r128VertexPtr v1,
				     r128VertexPtr v2,
				     r128VertexPtr v3 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)r128AllocDmaLow( rmesa, 6 * vertsize * 4 );
   GLuint j;

   rmesa->num_verts += 6;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v3 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}


static __inline void r128_draw_triangle( r128ContextPtr rmesa,
					 r128VertexPtr v0,
					 r128VertexPtr v1,
					 r128VertexPtr v2 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)r128AllocDmaLow( rmesa, 3 * vertsize * 4 );
   GLuint j;

   rmesa->num_verts += 3;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
}

static __inline void r128_draw_line( r128ContextPtr rmesa,
				     r128VertexPtr v0,
				     r128VertexPtr v1 )
{
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)r128AllocDmaLow( rmesa, 2 * vertsize * 4 );
   GLuint j;

   rmesa->num_verts += 2;
   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
}

static __inline void r128_draw_point( r128ContextPtr rmesa,
				      r128VertexPtr v0 )
{
   int vertsize = rmesa->vertex_size;
   GLuint *vb = (GLuint *)r128AllocDmaLow( rmesa, vertsize * 4 );
   int j;

   rmesa->num_verts += 1;
   COPY_DWORDS( j, vb, vertsize, v0 );
}

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_tri( rmesa, a, b, c );	\
   else						\
      r128_draw_triangle( rmesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      rmesa->draw_tri( rmesa, a, b, d );	\
      rmesa->draw_tri( rmesa, b, c, d );	\
   } else 					\
      r128_draw_quad( rmesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_line( rmesa, v0, v1 );	\
   else 					\
      r128_draw_line( rmesa, v0, v1 );	\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_point( rmesa, v0 );		\
   else 					\
      r128_draw_point( rmesa, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R128_OFFSET_BIT	0x01
#define R128_TWOSIDE_BIT	0x02
#define R128_UNFILLED_BIT	0x04
#define R128_FALLBACK_BIT	0x08
#define R128_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R128_MAX_TRIFUNC];


#define DO_FALLBACK (IND & R128_FALLBACK_BIT)
#define DO_OFFSET   (IND & R128_OFFSET_BIT)
#define DO_UNFILLED (IND & R128_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R128_TWOSIDE_BIT)
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
#define VERTEX r128Vertex
#define TAB rast_tab

#define DEPTH_SCALE rmesa->depth_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (rmesa->verts + (e * rmesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   r128_color_t *color = (r128_color_t *)&((v)->ui[coloroffset]);	\
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


#define LOCAL_VARS(n)						\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);			\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = (rmesa->vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (rmesa->vertex_size == 4 ? 0 : 1);	\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (rmesa->hw_primitive != hw_prim[x]) \
                        r128RasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE rmesa->render_primitive
#define IND R128_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_UNFILLED_BIT| \
	     R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
   init_fallback();
   init_offset_fallback();
   init_twoside_fallback();
   init_twoside_offset_fallback();
   init_unfilled_fallback();
   init_offset_unfilled_fallback();
   init_twoside_unfilled_fallback();
   init_twoside_offset_unfilled_fallback();
}



/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
r128_fallback_tri( r128ContextPtr rmesa,
		     r128Vertex *v0,
		     r128Vertex *v1,
		     r128Vertex *v2 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[3];
   r128_translate_vertex( ctx, v0, &v[0] );
   r128_translate_vertex( ctx, v1, &v[1] );
   r128_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
r128_fallback_line( r128ContextPtr rmesa,
		    r128Vertex *v0,
		    r128Vertex *v1 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[2];
   r128_translate_vertex( ctx, v0, &v[0] );
   r128_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
r128_fallback_point( r128ContextPtr rmesa,
		     r128Vertex *v0 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[1];
   r128_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (r128Vertex *)(r128verts + (x * vertsize * sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      r128_draw_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r128_draw_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r128_draw_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r128_draw_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);	\
   r128RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
    r128ContextPtr rmesa = R128_CONTEXT(ctx);		\
    const GLuint vertsize = rmesa->vertex_size;		\
    const char *r128verts = (char *)rmesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r128_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r128_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Render clipped primitives                       */
/**********************************************************************/

static void r128RenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				     GLuint n )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /* Render the new vertices as an unclipped polygon.
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }
}

static void r128RenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}

static void r128FastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
					 GLuint n )
{
   r128ContextPtr rmesa = R128_CONTEXT( ctx );
   GLuint vertsize = rmesa->vertex_size;
   GLuint *vb = r128AllocDmaLow( rmesa, (n-2) * 3 * 4 * vertsize );
   GLubyte *r128verts = (GLubyte *)rmesa->verts;
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i,j;

   rmesa->num_verts += (n-2) * 3;

   for (i = 2 ; i < n ; i++) {
      COPY_DWORDS( j, vb, vertsize, (r128VertexPtr) VERT(elts[i-1]) );
      COPY_DWORDS( j, vb, vertsize, (r128VertexPtr) VERT(elts[i]) );
      COPY_DWORDS( j, vb, vertsize, (r128VertexPtr) start );
   }
}




/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define _R128_NEW_RENDER_STATE (_DD_NEW_LINE_STIPPLE |	\
			          _DD_NEW_LINE_SMOOTH |		\
			          _DD_NEW_POINT_SMOOTH |	\
			          _DD_NEW_TRI_SMOOTH |		\
			          _DD_NEW_TRI_UNFILLED |	\
			          _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			          _DD_NEW_TRI_OFFSET)		\


#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_STIPPLE|DD_LINE_SMOOTH)
#define TRI_FALLBACK (DD_TRI_SMOOTH)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


static void r128ChooseRenderState(GLcontext *ctx)
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {
      rmesa->draw_point = r128_draw_point;
      rmesa->draw_line = r128_draw_line;
      rmesa->draw_tri = r128_draw_triangle;

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R128_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= R128_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= R128_UNFILLED_BIT;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)) {
	 if (flags & POINT_FALLBACK) rmesa->draw_point = r128_fallback_point;
	 if (flags & LINE_FALLBACK)  rmesa->draw_line = r128_fallback_line;
	 if (flags & TRI_FALLBACK)   rmesa->draw_tri = r128_fallback_tri;
	 index |= R128_FALLBACK_BIT;
      }
   }

   if (index != rmesa->RenderIndex) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = r128_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = r128_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = rast_tab[index].line;
	 tnl->Driver.Render.ClippedPolygon = r128FastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = r128RenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = r128RenderClippedPoly;
      }

      rmesa->RenderIndex = index;
   }
}

/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void r128RunPipeline( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if (rmesa->new_state || rmesa->NewGLState & _NEW_TEXTURE)
      r128DDUpdateHWState( ctx );

   if (!rmesa->Fallback && rmesa->NewGLState) {
      if (rmesa->NewGLState & _R128_NEW_VERTEX_STATE)
	 r128ChooseVertexState( ctx );

      if (rmesa->NewGLState & _R128_NEW_RENDER_STATE)
	 r128ChooseRenderState( ctx );

      rmesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* This is called when Mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 *
 * As the r128 uses triangles to render lines and points, it is
 * necessary to turn off hardware culling when rendering these
 * primitives.
 */

static void r128RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   rmesa->setup.dp_gui_master_cntl_c &= ~R128_GMC_BRUSH_NONE;

   if ( ctx->Polygon.StippleFlag && hwprim == GL_TRIANGLES ) {
      rmesa->setup.dp_gui_master_cntl_c |= R128_GMC_BRUSH_32x32_MONO_FG_LA;
   }
   else {
      rmesa->setup.dp_gui_master_cntl_c |= R128_GMC_BRUSH_SOLID_COLOR;
   }

   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= R128_UPLOAD_CONTEXT;

   if (rmesa->hw_primitive != hwprim) {
      FLUSH_BATCH( rmesa );
      rmesa->hw_primitive = hwprim;
   }
}

static void r128RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint hw = hw_prim[prim];
   rmesa->render_primitive = prim;
   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
   r128RasterPrimitive( ctx, hw );
}


static void r128RenderStart( GLcontext *ctx )
{
   /* Check for projective texturing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)
    */
   r128CheckTexSizes( ctx );
}

static void r128RenderFinish( GLcontext *ctx )
{
   if (R128_CONTEXT(ctx)->RenderIndex & R128_FALLBACK_BIT)
      _swrast_flush( ctx );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

void r128Fallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 FLUSH_BATCH( rmesa );
	 _swsetup_Wakeup( ctx );
	 rmesa->RenderIndex = ~0;
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = r128RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = r128RenderPrimitive;
	 tnl->Driver.Render.Finish = r128RenderFinish;
	 tnl->Driver.Render.BuildVertices = r128BuildVertices;
	 rmesa->NewGLState |= (_R128_NEW_RENDER_STATE|
			       _R128_NEW_VERTEX_STATE);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void r128InitTriFuncs( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = r128RunPipeline;
   tnl->Driver.Render.Start = r128RenderStart;
   tnl->Driver.Render.Finish = r128RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = r128RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = r128BuildVertices;
   rmesa->NewGLState |= (_R128_NEW_RENDER_STATE|
			 _R128_NEW_VERTEX_STATE);

/*     r128Fallback( ctx, 0x100000, 1 ); */
}
