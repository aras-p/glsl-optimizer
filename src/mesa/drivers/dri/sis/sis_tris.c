/* $XFree86*/ /* -*- c-basic-offset: 3 -*- */
/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
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
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
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

#include "sis_tris.h"
#include "sis_state.h"
#include "sis_vb.h"
#include "sis_lock.h"

static const GLuint hw_prim[GL_POLYGON+1] = {
   OP_3D_POINT_DRAW,		/* GL_POINTS */
   OP_3D_LINE_DRAW,		/* GL_LINES */
   OP_3D_LINE_DRAW,		/* GL_LINE_LOOP */
   OP_3D_LINE_DRAW,		/* GL_LINE_STRIP */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLES */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLE_STRIP */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLE_FAN */
   OP_3D_TRIANGLE_DRAW,		/* GL_QUADS */
   OP_3D_TRIANGLE_DRAW,		/* GL_QUAD_STRIP */
   OP_3D_TRIANGLE_DRAW		/* GL_POLYGON */
};

static const GLuint hw_prim_mmio_fire[OP_3D_TRIANGLE_DRAW+1] = {
   OP_3D_FIRE_TSARGBa,
   OP_3D_FIRE_TSARGBb,
   OP_3D_FIRE_TSARGBc
};

static const GLuint hw_prim_mmio_shade[OP_3D_TRIANGLE_DRAW+1] = {
   SHADE_FLAT_VertexA,
   SHADE_FLAT_VertexB,
   SHADE_FLAT_VertexC
};

static const GLuint hw_prim_agp_type[OP_3D_TRIANGLE_DRAW+1] = {
   MASK_PsPointList,
   MASK_PsLineList,
   MASK_PsTriangleList
};

static const GLuint hw_prim_agp_shade[OP_3D_TRIANGLE_DRAW+1] = {
   MASK_PsShadingFlatA,
   MASK_PsShadingFlatB,
   MASK_PsShadingFlatC
};

static void sisRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void sisRenderPrimitive( GLcontext *ctx, GLenum prim );
static void sisMakeRoomAGP( sisContextPtr smesa, GLint num );
static void sisUpdateAGP( sisContextPtr smesa );
static void sisFireVertsAGP( sisContextPtr smesa );

static float *AGP_StartPtr;
static float *AGP_WritePtr;		/* Current write position */
static float *AGP_ReadPtr;		/* Last known engine readposition */
static long AGP_SpaceLeft;		/* Last known engine readposition */

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

/* Future optimizations:
 *
 * The previous code only emitted W when fog or textures were enabled.
 */

#define SIS_MMIO_WRITE_VERTEX(_v, i, lastvert)			\
do {								\
   MMIOBase[(REG_3D_TSXa+(i)*0x30)/4] = _v->v.x;		\
   MMIOBase[(REG_3D_TSYa+(i)*0x30)/4] = _v->v.y;	        \
   MMIOBase[(REG_3D_TSZa+(i)*0x30)/4] = _v->v.z;		\
   MMIOBase[(REG_3D_TSWGa+(i)*0x30)/4] = _v->v.w;		\
   /*((GLint *) MMIOBase)[(REG_3D_TSFSa+(i)*0x30)/4] = _v->ui[5];*/ \
   if (SIS_STATES & SIS_VERT_TEX0) {				\
      MMIOBase[(REG_3D_TSUAa+(i)*0x30)/4] = _v->v.u0;		\
      MMIOBase[(REG_3D_TSVAa+(i)*0x30)/4] = _v->v.v0;		\
   }								\
   if (SIS_STATES & SIS_VERT_TEX1) {				\
      MMIOBase[(REG_3D_TSUBa+(i)*0x30)/4] = _v->v.u1;		\
      MMIOBase[(REG_3D_TSVBa+(i)*0x30)/4] = _v->v.v1;		\
   }								\
   /*MMIOBase[(REG_3D_TSUCa+(i)*0x30)/4] = _v->v.u2;		\
   MMIOBase[(REG_3D_TSVCa+(i)*0x30)/4] = _v->v.v2;*/		\
   /* the ARGB write of the last vertex of the primitive fires the 3d engine*/ \
   if (lastvert || (SIS_STATES & SIS_VERT_SMOOTH))		\
      ((GLint *) MMIOBase)[(REG_3D_TSARGBa+(i)*0x30)/4] = _v->ui[4]; \
} while (0);

#define SIS_AGP_WRITE_VERTEX(_v)				\
do {								\
   AGP_WritePtr[0] = _v->v.x;					\
   AGP_WritePtr[1] = _v->v.y;					\
   AGP_WritePtr[2] = _v->v.z;					\
   AGP_WritePtr[3] = _v->v.w;					\
   ((GLint *)AGP_WritePtr)[4] = _v->ui[4];			\
   AGP_WritePtr += 5;						\
   if (SIS_STATES & SIS_VERT_TEX0) {				\
      AGP_WritePtr[0] = _v->v.u0;				\
      AGP_WritePtr[1] = _v->v.v0;				\
      AGP_WritePtr += 2;					\
   }								\
   if (SIS_STATES & SIS_VERT_TEX1) {				\
      AGP_WritePtr[0] = _v->v.u1;				\
      AGP_WritePtr[1] = _v->v.v1;				\
      AGP_WritePtr += 2;					\
   }								\
} while(0)

#define MMIO_VERT_REG_COUNT 10

#define SIS_VERT_SMOOTH	0x01
#define SIS_VERT_TEX0	0x02
#define SIS_VERT_TEX1	0x04

static sis_quad_func sis_quad_func_agp[8];
static sis_tri_func sis_tri_func_agp[8];
static sis_line_func sis_line_func_agp[8];
static sis_point_func sis_point_func_agp[8];
static sis_quad_func sis_quad_func_mmio[8];
static sis_tri_func sis_tri_func_mmio[8];
static sis_line_func sis_line_func_mmio[8];
static sis_point_func sis_point_func_mmio[8];

/* XXX: These definitions look questionable */
#define USE_XYZ		MASK_PsVertex_HAS_RHW
#define USE_W		MASK_PsVertex_HAS_NORMALXYZ
#define USE_RGB		MASK_PsVertex_HAS_SPECULAR
#define USE_UV1		MASK_PsVertex_HAS_UVSet2
#define USE_UV2		MASK_PsVertex_HAS_UVSet3

static GLint AGPParsingValues[8] = {
  (5 << 28) | USE_XYZ | USE_W | USE_RGB,
  (5 << 28) | USE_XYZ | USE_W | USE_RGB,
  (7 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV1,
  (7 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV1,
  (7 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV2,
  (7 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV2,
  (9 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV1 | USE_UV2,
  (9 << 28) | USE_XYZ | USE_W | USE_RGB | USE_UV1 | USE_UV2,
};

#define SIS_STATES (0)
#define TAG(x) x##_none
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_SMOOTH)
#define TAG(x) x##_s
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_TEX0)
#define TAG(x) x##_t0
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_SMOOTH | SIS_VERT_TEX0)
#define TAG(x) x##_st0
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_TEX1)
#define TAG(x) x##_t1
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_SMOOTH | SIS_VERT_TEX1)
#define TAG(x) x##_st1
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_TEX0 | SIS_VERT_TEX1)
#define TAG(x) x##_t0t1
#include "sis_tritmp.h"

#define SIS_STATES (SIS_VERT_SMOOTH | SIS_VERT_TEX0 | SIS_VERT_TEX1)
#define TAG(x) x##_st0t1
#include "sis_tritmp.h"

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define POINT( v0 ) smesa->draw_point( smesa, v0 )
#define LINE( v0, v1 ) smesa->draw_line( smesa, v0, v1 )
#define TRI( a, b, c ) smesa->draw_tri( smesa, a, b, c )
#define QUAD( a, b, c, d ) smesa->draw_quad( smesa, a, b, c, d )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define SIS_OFFSET_BIT		0x01
#define SIS_TWOSIDE_BIT		0x02
#define SIS_UNFILLED_BIT	0x04
#define SIS_FALLBACK_BIT	0x08
#define SIS_MAX_TRIFUNC		0x10


static struct {
   points_func	        points;
   line_func		line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[SIS_MAX_TRIFUNC];


#define DO_FALLBACK (IND & SIS_FALLBACK_BIT)
#define DO_OFFSET   (IND & SIS_OFFSET_BIT)
#define DO_UNFILLED (IND & SIS_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & SIS_TWOSIDE_BIT)
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
#define VERTEX sisVertex
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (smesa->verts + (e << smesa->vertex_stride_shift))

#define VERT_SET_RGBA( v, c )	\
	do {			\
		sis_color_t *vc = (sis_color_t *)&(v)->ui[coloroffset]; \
		vc->blue  = (c)[2];	\
		vc->green = (c)[1];	\
		vc->red   = (c)[0];	\
		vc->alpha = (c)[3];	\
	} while (0)
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v0, c )				\
	if (havespec) {					\
		(v0)->v.specular.red   = (c)[0];	\
		(v0)->v.specular.green = (c)[1];	\
		(v0)->v.specular.blue  = (c)[2];	\
	}
#define VERT_COPY_SPEC( v0, v1 )			\
	if (havespec) {					\
		(v0)->v.specular.red   = v1->v.specular.red;	\
		(v0)->v.specular.green = v1->v.specular.green;	\
		(v0)->v.specular.blue  = v1->v.specular.blue;	\
	}

#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)						\
   sisContextPtr smesa = SIS_CONTEXT(ctx);			\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = (smesa->vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (smesa->vertex_size == 4 ? 0 : 1);	\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (smesa->hw_primitive != hw_prim[x]) \
                        sisRasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE smesa->render_primitive
#define IND SIS_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT | SIS_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_OFFSET_BIT | SIS_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_OFFSET_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_UNFILLED_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT | SIS_UNFILLED_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_UNFILLED_BIT | SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT | SIS_OFFSET_BIT | SIS_UNFILLED_BIT |  \
	     SIS_FALLBACK_BIT)
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
sis_fallback_quad( sisContextPtr smesa,
		   sisVertex *v0,
		   sisVertex *v1,
		   sisVertex *v2,
		   sisVertex *v3 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[4];
   sis_translate_vertex( ctx, v0, &v[0] );
   sis_translate_vertex( ctx, v1, &v[1] );
   sis_translate_vertex( ctx, v2, &v[2] );
   sis_translate_vertex( ctx, v3, &v[3] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[3] );
   _swrast_Triangle( ctx, &v[1], &v[2], &v[3] );
}

static void
sis_fallback_tri( sisContextPtr smesa,
		  sisVertex *v0,
		  sisVertex *v1,
		  sisVertex *v2 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[3];
   sis_translate_vertex( ctx, v0, &v[0] );
   sis_translate_vertex( ctx, v1, &v[1] );
   sis_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
sis_fallback_line( sisContextPtr smesa,
		   sisVertex *v0,
		   sisVertex *v1 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[2];
   sis_translate_vertex( ctx, v0, &v[0] );
   sis_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
sis_fallback_point( sisContextPtr smesa,
		    sisVertex *v0 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[1];
   sis_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (sisVertex *)(sisverts + (x << shift))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      smesa->draw_point( smesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) smesa->draw_line( smesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 ) smesa->draw_tri( smesa, VERT(v0), VERT(v1), \
   VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) smesa->draw_quad( smesa, VERT(v0), \
   VERT(v1), VERT(v2), VERT(v3))
#define INIT(x) sisRenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS				\
    sisContextPtr smesa = SIS_CONTEXT(ctx);	\
    const GLuint shift = smesa->vertex_stride_shift;		\
    const char *sisverts = (char *)smesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) sis_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) sis_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Render clipped primitives                       */
/**********************************************************************/

static void sisRenderClippedPoly( GLcontext *ctx, const GLuint *elts, GLuint n )
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

static void sisRenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}

#if 0
static void sisFastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				      GLuint n )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   GLuint vertsize = smesa->vertex_size;
   GLuint *vb = r128AllocDmaLow( rmesa, (n-2) * 3 * 4 * vertsize );
   GLubyte *sisverts = (GLubyte *)smesa->verts;
   const GLuint shift = smesa->vertex_stride_shift;
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i,j;

   smesa->num_verts += (n-2) * 3;

   for (i = 2 ; i < n ; i++) {
      COPY_DWORDS( j, vb, vertsize, (sisVertexPtr) VERT(elts[i-1]) );
      COPY_DWORDS( j, vb, vertsize, (sisVertexPtr) VERT(elts[i]) );
      COPY_DWORDS( j, vb, vertsize, (sisVertexPtr) start );
   }
}
#endif




/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define _SIS_NEW_RENDER_STATE (_DD_NEW_LINE_STIPPLE |	\
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


static void sisChooseRenderState(GLcontext *ctx)
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;
   GLuint vertindex = 0;
   
   if (ctx->Texture.Unit[0]._ReallyEnabled)
      vertindex |= SIS_VERT_TEX0;
   if (ctx->Texture.Unit[1]._ReallyEnabled)
      vertindex |= SIS_VERT_TEX1;
   if (ctx->Light.ShadeModel == GL_SMOOTH)
      vertindex |= SIS_VERT_SMOOTH;

   if (smesa->AGPCmdModeEnabled) {
      smesa->draw_quad = sis_quad_func_agp[vertindex];
      smesa->draw_tri = sis_tri_func_agp[vertindex];
      smesa->draw_line = sis_line_func_agp[vertindex];
      smesa->draw_point = sis_point_func_agp[vertindex];
   } else {
      smesa->draw_quad = sis_quad_func_mmio[vertindex];
      smesa->draw_tri = sis_tri_func_mmio[vertindex];
      smesa->draw_line = sis_line_func_mmio[vertindex];
      smesa->draw_point = sis_point_func_mmio[vertindex];
   }
   smesa->AGPParseSet &= ~(MASK_VertexDWSize | MASK_VertexDataFormat);
   smesa->AGPParseSet |= AGPParsingValues[vertindex];

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= SIS_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= SIS_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= SIS_UNFILLED_BIT;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS) {
	 if (flags & POINT_FALLBACK)
            smesa->draw_point = sis_fallback_point;
	 if (flags & LINE_FALLBACK)
            smesa->draw_line = sis_fallback_line;
	 if (flags & TRI_FALLBACK) {
            smesa->draw_quad = sis_fallback_quad;
            smesa->draw_tri = sis_fallback_tri;
         }
	 index |= SIS_FALLBACK_BIT;
      }
   }

   if (index != smesa->RenderIndex) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = sis_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = sis_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = rast_tab[index].line;
         /*XXX: sisFastRenderClippedPoly*/
	 tnl->Driver.Render.ClippedPolygon = sisRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = sisRenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = sisRenderClippedPoly;
      }

      smesa->RenderIndex = index;
   }
}

/**********************************************************************/
/*                Multipass rendering for front buffering             */
/**********************************************************************/
static GLboolean multipass_cliprect( GLcontext *ctx, GLuint pass )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );

   if (pass >= smesa->driDrawable->numClipRects) {
      return GL_FALSE;
   } else {
      GLint x1, y1, x2, y2;

      x1 = smesa->driDrawable->pClipRects[pass].x1 - smesa->driDrawable->x;
      y1 = smesa->driDrawable->pClipRects[pass].y1 - smesa->driDrawable->y;
      x2 = smesa->driDrawable->pClipRects[pass].x2 - smesa->driDrawable->x;
      y2 = smesa->driDrawable->pClipRects[pass].y2 - smesa->driDrawable->y;

      if (ctx->Scissor.Enabled) {
         GLint scisy1 = Y_FLIP(ctx->Scissor.Y + ctx->Scissor.Height - 1);
         GLint scisy2 = Y_FLIP(ctx->Scissor.Y);

         if (ctx->Scissor.X > x1)
            x1 = ctx->Scissor.X;
         if (scisy1 > y1)
            y1 = scisy1;
         if (ctx->Scissor.X + ctx->Scissor.Width - 1 < x2)
            x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
         if (scisy2 < y2)
            y2 = scisy2;
      }

      MMIO(REG_3D_ClipTopBottom, y1 << 13 | y2);
      MMIO(REG_3D_ClipLeftRight, x1 << 13 | x2);
      /* Mark that we clobbered these registers */
      smesa->GlobalFlag |= GFLAG_CLIPPING;
      return GL_TRUE;
   }
}



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void sisRunPipeline( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );

   LOCK_HARDWARE();
   sisUpdateHWState( ctx );

   if (smesa->AGPCmdModeEnabled) {
      AGP_WritePtr = (GLfloat *)smesa->AGPCmdBufBase + *smesa->pAGPCmdBufNext;
      AGP_StartPtr = AGP_WritePtr;
      AGP_ReadPtr = (GLfloat *)((long)MMIO_READ(REG_3D_AGPCmBase) -
         (long)smesa->AGPCmdBufAddr + (long)smesa->AGPCmdBufBase);
      sisUpdateAGP( smesa );
   }

   if (!smesa->Fallback && smesa->NewGLState) {
      if (smesa->NewGLState & _SIS_NEW_VERTEX_STATE)
	 sisChooseVertexState( ctx );

      if (smesa->NewGLState & (_SIS_NEW_RENDER_STATE | _NEW_TEXTURE))
	 sisChooseRenderState( ctx );

      smesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );

   if (smesa->AGPCmdModeEnabled)
      sisFireVertsAGP( smesa );
   else
      mEndPrimitive();
   UNLOCK_HARDWARE();
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* This is called when Mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 */

static void sisRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   if (smesa->hw_primitive != hwprim) {
      if (smesa->AGPCmdModeEnabled) {
         sisFireVertsAGP( smesa );
         smesa->AGPParseSet &= ~(MASK_PsDataType | MASK_PsShadingMode);
         smesa->AGPParseSet |= hw_prim_agp_type[hwprim];
         if (ctx->Light.ShadeModel == GL_FLAT)
            smesa->AGPParseSet |= hw_prim_agp_shade[hwprim];
         else
            smesa->AGPParseSet |= MASK_PsShadingSmooth;
      } else {
         mEndPrimitive();
         smesa->dwPrimitiveSet &= ~(MASK_DrawPrimitiveCommand | 
            MASK_SetFirePosition | MASK_ShadingMode);
         smesa->dwPrimitiveSet |= hwprim | hw_prim_mmio_fire[hwprim];
         if (ctx->Light.ShadeModel == GL_FLAT)
            smesa->dwPrimitiveSet |= hw_prim_mmio_shade[hwprim];
         else
            smesa->dwPrimitiveSet |= SHADE_GOURAUD;
      }
   }
   smesa->hw_primitive = hwprim;
}

static void sisRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   smesa->render_primitive = prim;
   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
   sisRasterPrimitive( ctx, hw_prim[prim] );
}


static void sisRenderStart( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   /* Check for projective texturing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)
    */
   sisCheckTexSizes( ctx );

   if (ctx->Color._DrawDestMask == FRONT_LEFT_BIT && 
      smesa->driDrawable->numClipRects != 0)
   {
      multipass_cliprect(ctx, 0);
      if (smesa->driDrawable->numClipRects > 1)
         tnl->Driver.Render.Multipass = multipass_cliprect;
      else
         tnl->Driver.Render.Multipass = NULL;
   } else {
      tnl->Driver.Render.Multipass = NULL;
   }
}

static void sisRenderFinish( GLcontext *ctx )
{
}

/* Update SpaceLeft after an engine or current write pointer update */
static void sisUpdateAGP( sisContextPtr smesa )
{
   /* ReadPtr == WritePtr is the empty case */
   if (AGP_ReadPtr <= AGP_WritePtr)
      AGP_SpaceLeft = (long)smesa->AGPCmdBufBase + (long)smesa->AGPCmdBufSize - 
         (long)AGP_WritePtr;
   else
      AGP_SpaceLeft = AGP_ReadPtr - AGP_WritePtr - 4;
}

/* Fires a set of vertices that have been written from AGP_StartPtr to
 * AGP_WritePtr, using the smesa->AGPParseSet format.
 */
void
sisFireVertsAGP( sisContextPtr smesa )
{
   if (AGP_WritePtr == AGP_StartPtr)
      return;

   mWait3DCmdQueue(5);
   mEndPrimitive();
   MMIO(REG_3D_AGPCmBase, (long)AGP_StartPtr - (long)smesa->AGPCmdBufBase +
      (long)smesa->AGPCmdBufAddr);
   MMIO(REG_3D_AGPTtDwNum, (((long)AGP_WritePtr - (long)AGP_StartPtr) >> 2) |
      0x50000000);
   MMIO(REG_3D_ParsingSet, smesa->AGPParseSet);
   
   MMIO(REG_3D_AGPCmFire, (GLint)(-1));
   mEndPrimitive();

   *(smesa->pAGPCmdBufNext) = (((long)AGP_WritePtr -
      (long)smesa->AGPCmdBufBase) + 0xf) & ~0xf;
   AGP_StartPtr = AGP_WritePtr;
   sisUpdateAGP( smesa );
}

/* Make sure there are more than num dwords left in the AGP queue. */
static void
sisMakeRoomAGP( sisContextPtr smesa, GLint num )
{
   int size = num * 4;
   
   if (size <= AGP_SpaceLeft) {
      AGP_SpaceLeft -= size;
      return;
   }
   /* Wrapping */
   if (AGP_WritePtr + num > (GLfloat *)(smesa->AGPCmdBufBase +
      smesa->AGPCmdBufSize))
   {
      sisFireVertsAGP( smesa );
      AGP_WritePtr = (GLfloat *)smesa->AGPCmdBufBase;
      AGP_StartPtr = AGP_WritePtr;
      sisUpdateAGP( smesa );
      WaitEngIdle( smesa ); /* XXX Why is this necessary? */
   }

   if (size > AGP_SpaceLeft) {
      /* Update the cached engine read pointer */
      AGP_ReadPtr = (GLfloat *)((long)MMIO_READ(REG_3D_AGPCmBase) -
         (long)smesa->AGPCmdBufAddr + (long)smesa->AGPCmdBufBase);
      sisUpdateAGP( smesa );
      while (size > AGP_SpaceLeft) {
         /* Spin until space is available. */
         usleep(1);
         AGP_ReadPtr = (GLfloat *)((long)MMIO_READ(REG_3D_AGPCmBase) -
            (long)smesa->AGPCmdBufAddr + (long)smesa->AGPCmdBufBase);
         sisUpdateAGP( smesa );
      }
   }
   AGP_SpaceLeft -= size;
}

/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

void sisFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLuint oldfallback = smesa->Fallback;

   if (mode) {
      smesa->Fallback |= bit;
      if (oldfallback == 0) {
	 _swsetup_Wakeup( ctx );
	 smesa->RenderIndex = ~0;
      }
   }
   else {
      smesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = sisRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = sisRenderPrimitive;
	 tnl->Driver.Render.Finish = sisRenderFinish;
	 tnl->Driver.Render.BuildVertices = sisBuildVertices;
	 smesa->NewGLState |= (_SIS_NEW_RENDER_STATE|
			       _SIS_NEW_VERTEX_STATE);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void sisInitTriFuncs( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;

      sis_vert_init_none();
      sis_vert_init_s();
      sis_vert_init_t0();
      sis_vert_init_st0();
      sis_vert_init_t1();
      sis_vert_init_st1();
      sis_vert_init_t0t1();
      sis_vert_init_st0t1();
   }

   tnl->Driver.RunPipeline = sisRunPipeline;
   tnl->Driver.Render.Start = sisRenderStart;
   tnl->Driver.Render.Finish = sisRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = sisRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = sisBuildVertices;
   tnl->Driver.Render.Multipass		= NULL;

   if (getenv("SIS_FORCE_FALLBACK") != NULL)
      sisFallback(ctx, SIS_FALLBACK_FORCE, 1);
   else
      sisFallback(ctx, SIS_FALLBACK_FORCE, 0);

   smesa->RenderIndex = ~0;
   smesa->NewGLState |= (_SIS_NEW_RENDER_STATE|
			 _SIS_NEW_VERTEX_STATE);
}
