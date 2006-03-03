/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 * Copyright 2006 Stephane Marchesin. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Triangles for NV30, NV40, G70 */

#include <stdio.h>
#include <math.h>

#include "glheader.h"
#include "context.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "nouveau_tris.h"
#include "nv30_tris.h"
#include "nouveau_context.h"
#include "nouveau_state.h"
#include "nouveau_span.h"
#include "nouveau_ioctl.h"
#include "nouveau_3d_reg.h"
#include "nouveau_tex.h"

/* hack for now */
#define channel 1


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/
#define LINE_FALLBACK (0)
#define POINT_FALLBACK (0)
#define TRI_FALLBACK (0)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


#define COPY_DWORDS(vb, vertsize, v)		\
	do {					\
		int j;					\
		for (j = 0; j < vertsize; j++)		\
		vb[j] = ((GLuint *)v)[j];		\
		vb += vertsize;				\
	} while (0)
#endif

/* the free room we want before we start a vertex batch. this is a performance-tunable */
#define NV30_MIN_PRIM_SIZE (32/4)

static inline void nv30StartPrimitive(struct nouveau_context* nmesa)
{
	BEGIN_RING_SIZE(channel,0x1808,1);
	OUT_RING(nmesa->current_primitive);
	BEGIN_RING_PRIM(channel,0x1818,NV30_MIN_PRIM_SIZE);
}

static inline void nv30FinishPrimitive(struct nouveau_context *nmesa)
{
	FINISH_RING_PRIM();
	BEGIN_RING_SIZE(channel,0x1808,1);
	OUT_RING(0x0);
	FIRE_RING();
}


static inline void nv30ExtendPrimitive(struct nouveau_context* nmesa, int size)
{
	/* when the fifo has enough stuff (2048 bytes) or there is not enough room, fire */
	if ((RING_AHEAD()>=2048/4)||(RING_AVAILABLE()<size/4))
	{
		nv30FinishPrimitive(nmesa);
		nv30StartPrimitive(nmesa);
	}

	/* make sure there's enough room. if not, wait */
	if (RING_AVAILABLE()<size/4)
	{
		WAIT_RING(nmesa,size);
	}
}

static inline void nv30_draw_quad(struct nouveau_context *nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1,
		nouveauVertexPtr v2,
		nouveauVertexPtr v3)
{
	GLuint vertsize = nmesa->vertexSize;
	GLuint *vb = nv30ExtendPrimitive(nmesa, 4 * 4 * vertsize);

	COPY_DWORDS(vb, vertsize, v0);
	COPY_DWORDS(vb, vertsize, v1);
	COPY_DWORDS(vb, vertsize, v2);
	COPY_DWORDS(vb, vertsize, v3);
}

static inline void nv30_draw_triangle(struct nouveau_context *nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1,
		nouveauVertexPtr v2)
{
	GLuint vertsize = nmesa->vertexSize;
	GLuint *vb = nv30ExtendPrimitive(nmesa, 3 * 4 * vertsize);

	COPY_DWORDS(vb, vertsize, v0);
	COPY_DWORDS(vb, vertsize, v1);
	COPY_DWORDS(vb, vertsize, v2);
}

static inline void nouveau_draw_line(struct nouveau_context *nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1)
{
	GLuint vertsize = nmesa->vertexSize;
	GLuint *vb = nv30ExtendPrimitive(nmesa, 2 * 4 * vertsize);
	COPY_DWORDS(vb, vertsize, v0);
	COPY_DWORDS(vb, vertsize, v1);
}

static inline void nouveau_draw_point(struct nouveau_context *nmesa,
		nouveauVertexPtr v0)
{
	GLuint vertsize = nmesa->vertexSize;
	GLuint *vb = nv30ExtendPrimitive(nmesa, 4 * vertsize);
	COPY_DWORDS(vb, vertsize, v0);
}


/***********************************************************************
 *          Macros for nouveau_dd_tritmp.h to draw basic primitives        *
 ***********************************************************************/

#define TRI(a, b, c)                                \
	do {                                            \
		if (DO_FALLBACK)                            \
		nmesa->draw_tri(nmesa, a, b, c);         \
		else                                        \
		nouveau_draw_triangle(nmesa, a, b, c);      \
	} while (0)

#define QUAD(a, b, c, d)                            \
	do {                                            \
		if (DO_FALLBACK) {                          \
			nmesa->draw_tri(nmesa, a, b, d);         \
			nmesa->draw_tri(nmesa, b, c, d);         \
		}                                           \
		else                                        \
		nouveau_draw_quad(nmesa, a, b, c, d);       \
	} while (0)

#define LINE(v0, v1)                                \
	do {                                            \
		if (DO_FALLBACK)                            \
		nmesa->draw_line(nmesa, v0, v1);         \
		else                                        \
		nouveau_draw_line(nmesa, v0, v1);           \
	} while (0)

#define POINT(v0)                                    \
	do {                                             \
		if (DO_FALLBACK)                             \
		nmesa->draw_point(nmesa, v0);             \
		else                                         \
		nouveau_draw_point(nmesa, v0);               \
	} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define NOUVEAU_OFFSET_BIT         0x01
#define NOUVEAU_TWOSIDE_BIT        0x02
#define NOUVEAU_UNFILLED_BIT       0x04
#define NOUVEAU_FALLBACK_BIT       0x08
#define NOUVEAU_MAX_TRIFUNC        0x10


static struct {
	tnl_points_func          points;
	tnl_line_func            line;
	tnl_triangle_func        triangle;
	tnl_quad_func            quad;
} rast_tab[NOUVEAU_MAX_TRIFUNC + 1];


#define DO_FALLBACK (IND & NOUVEAU_FALLBACK_BIT)
#define DO_OFFSET   (IND & NOUVEAU_OFFSET_BIT)
#define DO_UNFILLED (IND & NOUVEAU_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & NOUVEAU_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_SPEC         1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            nouveauVertex
#define TAB               rast_tab

/* Only used to pull back colors into vertices (ie, we know color is
 * floating point).
 */
#define NOUVEAU_COLOR(dst, src)                     \
	do {                                        \
		dst[0] = src[2];                        \
		dst[1] = src[1];                        \
		dst[2] = src[0];                        \
		dst[3] = src[3];                        \
	} while (0)

#define NOUVEAU_SPEC(dst, src)                      \
	do {                                        \
		dst[0] = src[2];                        \
		dst[1] = src[1];                        \
		dst[2] = src[0];                        \
	} while (0)


#define DEPTH_SCALE nmesa->polygon_offset_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW(a) (a > 0)
#define GET_VERTEX(e) (nmesa->verts + (e * nmesa->vertexSize * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
	do {								\
		nouveau_color_t *color = (nouveau_color_t *)&((v)->ui[coloroffset]);	\
		UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
		UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
		UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
		UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
	} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
	do {								\
		if (specoffset) {						\
			nouveau_color_t *color = (nouveau_color_t *)&((v)->ui[specoffset]);	\
			UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
			UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
			UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
		}								\
	} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
	do {							\
		if (specoffset) {					\
			v0->ub4[specoffset][0] = v1->ub4[specoffset][0];	\
			v0->ub4[specoffset][1] = v1->ub4[specoffset][1];	\
			v0->ub4[specoffset][2] = v1->ub4[specoffset][2];	\
		}							\
	} while (0)


#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]


#define LOCAL_VARS(n)                                                   \
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);                             \
GLuint color[n], spec[n];                                           \
GLuint coloroffset = nmesa->coloroffset;              \
GLuint specoffset = nmesa->specoffset;                       \
(void)color; (void)spec; (void)coloroffset; (void)specoffset;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLenum hwPrim[GL_POLYGON+1] = {
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

#define RASTERIZE(x) nv30RasterPrimitive( ctx, x, hwPrim[x] )
#define RENDER_PRIMITIVE nmesa->renderPrimitive
#define TAG(x) x
#define IND NOUVEAU_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND
#undef RASTERIZE

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/
#define RASTERIZE(x)

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_OFFSET_BIT|NOUVEAU_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_OFFSET_BIT|NOUVEAU_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_OFFSET_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_OFFSET_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_UNFILLED_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_OFFSET_BIT|NOUVEAU_UNFILLED_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_UNFILLED_BIT|NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (NOUVEAU_TWOSIDE_BIT|NOUVEAU_OFFSET_BIT|NOUVEAU_UNFILLED_BIT| \
		NOUVEAU_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


/* Catchall case for flat, separate specular triangles */
#undef  DO_FALLBACK
#undef  DO_OFFSET
#undef  DO_UNFILLED
#undef  DO_TWOSIDE
#undef  DO_FLAT
#define DO_FALLBACK (0)
#define DO_OFFSET   (ctx->_TriangleCaps & DD_TRI_OFFSET)
#define DO_UNFILLED (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define DO_TWOSIDE  (ctx->_TriangleCaps & DD_TRI_LIGHT_TWOSIDE)
#define DO_FLAT     1
#define TAG(x) x##_flat_specular
#define IND NOUVEAU_MAX_TRIFUNC
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab(void)
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

	init_flat_specular();	/* special! */
}


/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/
#define IND 0
#define V(x) (nouveauVertex *)(vertptr + ((x) * vertsize * sizeof(int)))
#define RENDER_POINTS(start, count)   \
	for (; start < count; start++) POINT(V(ELT(start)));
#define RENDER_LINE(v0, v1)         LINE(V(v0), V(v1))
#define RENDER_TRI( v0, v1, v2)     TRI( V(v0), V(v1), V(v2))
#define RENDER_QUAD(v0, v1, v2, v3) QUAD(V(v0), V(v1), V(v2), V(v3))
#define INIT(x) nv30RasterPrimitive(ctx, x, hwPrim[x])
#undef LOCAL_VARS
#define LOCAL_VARS                                              \
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);                     \
GLubyte *vertptr = (GLubyte *)nmesa->verts;                 \
const GLuint vertsize = nmesa->vertexSize;          \
const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;       \
const GLboolean stipple = ctx->Line.StippleFlag;		\
(void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) nouveauResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) nouveau_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) nouveau_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#undef NEED_EDGEFLAG_SETUP
#undef EDGEFLAG_GET
#undef EDGEFLAG_SET
#undef RESET_OCCLUSION


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void nouveauRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
		GLuint n)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
	GLuint prim = NOUVEAU_CONTEXT(ctx)->renderPrimitive;

	/* Render the new vertices as an unclipped polygon.
	 */
	{
		GLuint *tmp = VB->Elts;
		VB->Elts = (GLuint *)elts;
		tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n,
				PRIM_BEGIN|PRIM_END);
		VB->Elts = tmp;
	}

	/* Restore the render primitive
	 */
	if (prim != GL_POLYGON &&
			prim != GL_POLYGON + 1)
		tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void nouveauRenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	tnl->Driver.Render.Line(ctx, ii, jj);
}

static void nouveauFastRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
		GLuint n)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint vertsize = nmesa->vertexSize;
	GLuint *vb = nouveauExtendPrimitive(nmesa, (n - 2) * 3 * 4 * vertsize);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	const GLuint *start = (const GLuint *)V(elts[0]);
	int i;

	for (i = 2; i < n; i++) {
		COPY_DWORDS(vb, vertsize, V(elts[i - 1]));
		COPY_DWORDS(vb, vertsize, V(elts[i]));
		COPY_DWORDS(vb, vertsize, start);	
	}
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/




#define _NOUVEAU_NEW_VERTEX (_NEW_TEXTURE |                         \
		_DD_NEW_SEPARATE_SPECULAR |            \
		_DD_NEW_TRI_UNFILLED |                 \
		_DD_NEW_TRI_LIGHT_TWOSIDE |            \
		_NEW_FOG)

#define _NOUVEAU_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |            \
		_DD_NEW_TRI_UNFILLED |            \
		_DD_NEW_TRI_LIGHT_TWOSIDE |       \
		_DD_NEW_TRI_OFFSET |              \
		_DD_NEW_TRI_STIPPLE |             \
		_NEW_POLYGONSTIPPLE)


static void nv30ChooseRenderState(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint index = 0;

	nmesa->draw_point = nouveau_draw_point;
	nmesa->draw_line = nouveau_draw_line;
	nmesa->draw_tri = nouveau_draw_triangle;

	if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS)) {
		if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= NOUVEAU_TWOSIDE_BIT;
		if (flags & DD_TRI_OFFSET)           index |= NOUVEAU_OFFSET_BIT;
		if (flags & DD_TRI_UNFILLED)         index |= NOUVEAU_UNFILLED_BIT;
		if (flags & ANY_FALLBACK_FLAGS)      index |= NOUVEAU_FALLBACK_BIT;

		/* Hook in fallbacks for specific primitives.
		 */
		if (flags & POINT_FALLBACK)
			nmesa->draw_point = nouveau_fallback_point;

		if (flags & LINE_FALLBACK)
			nmesa->draw_line = nouveau_fallback_line;

		if (flags & TRI_FALLBACK)
			nmesa->draw_tri = nouveau_fallback_tri;
	}


	if ((flags & DD_SEPARATE_SPECULAR) &&
			ctx->Light.ShadeModel == GL_FLAT) {
		index = NOUVEAU_MAX_TRIFUNC;	/* flat specular */
	}

	if (nmesa->renderIndex != index) {
		nmesa->renderIndex = index;

		tnl->Driver.Render.Points = rast_tab[index].points;
		tnl->Driver.Render.Line = rast_tab[index].line;
		tnl->Driver.Render.Triangle = rast_tab[index].triangle;
		tnl->Driver.Render.Quad = rast_tab[index].quad;

		if (index == 0) {
			tnl->Driver.Render.PrimTabVerts = nouveau_render_tab_verts;
			tnl->Driver.Render.PrimTabElts = nouveau_render_tab_elts;
			tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
			tnl->Driver.Render.ClippedPolygon = nouveauFastRenderClippedPoly;
		}
		else {
			tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
			tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
			tnl->Driver.Render.ClippedLine = nouveauRenderClippedLine;
			tnl->Driver.Render.ClippedPolygon = nouveauRenderClippedPoly;
		}
	}
}



static inline void nv30OutputVertexFormat(struct nouveau_context* mesa, GLuint index)
{
	/*
	 * Determine how many inputs we need in the vertex format.
	 * We need to find & setup the right input "slots"
	 * 
	 * The hw attribute order matches nv_vertex_program, and _TNL_BIT_* 
	 * also matches this order, so we can take shortcuts...
	 */
	int i;
	int slots=0;
	for(i=0;i<16;i++)
		if (index&(1<<i))
			slots=i+1;

	BEGIN_RING_SIZE(channel,0x1740,slots);
	for(i=0;i<slots;i++)
		if (index&(1<<i))
		{
			/* XXX for now we only emit 3-sized attributes 
			 * Where can we get the attribute size ? */
			int size=3;
			OUR_RING(0x00000002|(size*0x10));
		}
		else
		{
			OUR_RING(0x00000002);
		}
	BEGIN_RING_SIZE(channel,0x1718,1);
	OUT_RING(0);
	BEGIN_RING_SIZE(channel,0x1718,1);
	OUT_RING(0);
	BEGIN_RING_SIZE(channel,0x1718,1);
	OUT_RING(0);
}


static void nv30ChooseVertexState( GLcontext *ctx )
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint index = tnl->render_inputs;

	if (index!=nmesa->render_inputs)
	{
		nmesa->render_inputs=index;
		nv30OutputVertexFormat(nmesa,index);
	}
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void nv30RenderStart(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->newState) {
		nmesa->newRenderState |= nmesa->newState;
		nouveauValidateState( ctx );
	}

	if (nmesa->Fallback) {
		tnl->Driver.Render.Start(ctx);
		return;
	}

	if (nmesa->newRenderState) {
		nv30ChooseVertexState(ctx);
		nv30ChooseRenderState(ctx);
		nmesa->newRenderState = 0;
	}
}

static void nv30RenderFinish(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	nv30FinishPrimitive(nmesa);
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void nv30RasterPrimitive(GLcontext *ctx,
		GLenum glprim,
		GLenum hwprim)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	assert (!nmesa->newState);
	
	if (hwprim != nmesa->current_primitive)
	{
		nmesa->current_primitive=hwprim;
		
	}
}

/* Callback for mesa:
 */
static void nv30RenderPrimitive( GLcontext *ctx, GLuint prim )
{
	nv30RasterPrimitive( ctx, prim, hwPrim[prim] );
}



/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void nouveauInitTriFuncs(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	static int firsttime = 1;

	if (firsttime) {
		init_rast_tab();
		firsttime = 0;
	}

	tnl->Driver.RunPipeline = nouveauRunPipeline;
	tnl->Driver.Render.Start = nv30RenderStart;
	tnl->Driver.Render.Finish = nv30RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = nv30RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = nouveauResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;

	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
			(6 + 2*ctx->Const.MaxTextureUnits) * sizeof(GLfloat) );

	nmesa->verts = (GLubyte *)tnl->clipspace.vertex_buf;

}

