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

/* Software TCL for NV10, NV20, NV30, NV40, G70 */

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
#include "nv20_swtcl.h"
#include "nouveau_context.h"
#include "nouveau_span.h"
#include "nouveau_ioctl.h"
#include "nouveau_reg.h"
#include "nouveau_tex.h"
#include "nouveau_fifo.h"

/* XXX hack for now */
#define channel 1

static void nv20RenderPrimitive( GLcontext *ctx, GLenum prim );
static void nv20RasterPrimitive( GLcontext *ctx, GLenum rprim, GLuint hwprim );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/
#define LINE_FALLBACK (0)
#define POINT_FALLBACK (0)
#define TRI_FALLBACK (0)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


/* the free room we want before we start a vertex batch. this is a performance-tunable */
#define NOUVEAU_MIN_PRIM_SIZE (32/4)
/* the size above which we fire the ring. this is a performance-tunable */
#define NOUVEAU_FIRE_SIZE (2048/4)

static inline void nv20StartPrimitive(struct nouveau_context* nmesa)
{
	if (nmesa->screen->card_type==NV_10)
		BEGIN_RING_SIZE(channel,NV10_PRIMITIVE,1);
	else if (nmesa->screen->card_type==NV_20)
		BEGIN_RING_SIZE(channel,NV20_PRIMITIVE,1);
	else
		BEGIN_RING_SIZE(channel,NV30_PRIMITIVE,1);
	OUT_RING(nmesa->current_primitive);

	if (nmesa->screen->card_type==NV_10)
		BEGIN_RING_PRIM(channel,NV10_BEGIN_VERTICES,NOUVEAU_MIN_PRIM_SIZE);
	else
		BEGIN_RING_PRIM(channel,NV20_BEGIN_VERTICES,NOUVEAU_MIN_PRIM_SIZE);
}

static inline void nv20FinishPrimitive(struct nouveau_context *nmesa)
{
	FINISH_RING_PRIM();
	if (nmesa->screen->card_type==NV_10)
		BEGIN_RING_SIZE(channel,NV10_PRIMITIVE,1);
	else if (nmesa->screen->card_type==NV_20)
		BEGIN_RING_SIZE(channel,NV20_PRIMITIVE,1);
	else
		BEGIN_RING_SIZE(channel,NV30_PRIMITIVE,1);
	OUT_RING(0x0);
	FIRE_RING();
}


static inline void nv20ExtendPrimitive(struct nouveau_context* nmesa, int size)
{
	/* when the fifo has enough stuff (2048 bytes) or there is not enough room, fire */
	if ((RING_AHEAD()>=NOUVEAU_FIRE_SIZE)||(RING_AVAILABLE()<size/4))
	{
		nv20FinishPrimitive(nmesa);
		nv20StartPrimitive(nmesa);
	}

	/* make sure there's enough room. if not, wait */
	if (RING_AVAILABLE()<size/4)
	{
		WAIT_RING(nmesa,size);
	}
}

static inline void nv20_draw_quad(nouveauContextPtr nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1,
		nouveauVertexPtr v2,
		nouveauVertexPtr v3)
{
	GLuint vertsize = nmesa->vertex_size;
	nv20ExtendPrimitive(nmesa, 4 * 4 * vertsize);

	OUT_RINGp(v0,vertsize);
	OUT_RINGp(v1,vertsize);
	OUT_RINGp(v2,vertsize);
	OUT_RINGp(v3,vertsize);
}

static inline void nv20_draw_triangle(nouveauContextPtr nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1,
		nouveauVertexPtr v2)
{
	GLuint vertsize = nmesa->vertex_size;
	nv20ExtendPrimitive(nmesa, 3 * 4 * vertsize);

	OUT_RINGp(v0,vertsize);
	OUT_RINGp(v1,vertsize);
	OUT_RINGp(v2,vertsize);
}

static inline void nv20_draw_line(nouveauContextPtr nmesa,
		nouveauVertexPtr v0,
		nouveauVertexPtr v1)
{
	GLuint vertsize = nmesa->vertex_size;
	nv20ExtendPrimitive(nmesa, 2 * 4 * vertsize);
	OUT_RINGp(v0,vertsize);
	OUT_RINGp(v1,vertsize);
}

static inline void nv20_draw_point(nouveauContextPtr nmesa,
		nouveauVertexPtr v0)
{
	GLuint vertsize = nmesa->vertex_size;
	nv20ExtendPrimitive(nmesa, 1 * 4 * vertsize);
	OUT_RINGp(v0,vertsize);
}



/***********************************************************************
 *          Macros for nouveau_dd_tritmp.h to draw basic primitives        *
 ***********************************************************************/

#define TRI(a, b, c)                                        \
	do {                                                \
		if (DO_FALLBACK)                            \
		nmesa->draw_tri(nmesa, a, b, c);            \
		else                                        \
		nv20_draw_triangle(nmesa, a, b, c);         \
	} while (0)

#define QUAD(a, b, c, d)                                    \
	do {                                                \
		if (DO_FALLBACK) {                          \
			nmesa->draw_tri(nmesa, a, b, d);    \
			nmesa->draw_tri(nmesa, b, c, d);    \
		}                                           \
		else                                        \
		nv20_draw_quad(nmesa, a, b, c, d);          \
	} while (0)

#define LINE(v0, v1)                                        \
	do {                                                \
		if (DO_FALLBACK)                            \
		nmesa->draw_line(nmesa, v0, v1);            \
		else                                        \
		nv20_draw_line(nmesa, v0, v1);              \
	} while (0)

#define POINT(v0)                                           \
	do {                                                \
		if (DO_FALLBACK)                            \
		nmesa->draw_point(nmesa, v0);               \
		else                                        \
		nv20_draw_point(nmesa, v0);                 \
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


#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW(a) (a > 0)
#define GET_VERTEX(e) (nmesa->verts + (e * nmesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
	do {								\
		nouveau_color_t *color = (nouveau_color_t *)&((v)->f[coloroffset]);	\
		color->red=(c)[0];					\
		color->green=(c)[1];					\
		color->blue=(c)[2];					\
		color->alpha=(c)[3];					\
	} while (0)

#define VERT_COPY_RGBA( v0, v1 )					\
	do {								\
		if (coloroffset) {					\
			v0->f[coloroffset][0] = v1->f[coloroffset][0];	\
			v0->f[coloroffset][1] = v1->f[coloroffset][1];	\
			v0->f[coloroffset][2] = v1->f[coloroffset][2];	\
			v0->f[coloroffset][3] = v1->f[coloroffset][3];	\
		}							\
	} while (0)

#define VERT_SET_SPEC( v, c )							\
	do {									\
		if (specoffset) {						\
			nouveau_color_t *color = (nouveau_color_t *)&((v)->f[specoffset]);	\
			UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
			UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
			UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
		}								\
	} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
	do {							\
		if (specoffset) {					\
			v0->f[specoffset][0] = v1->f[specoffset][0];	\
			v0->f[specoffset][1] = v1->f[specoffset][1];	\
			v0->f[specoffset][2] = v1->f[specoffset][2];	\
		}							\
	} while (0)


#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->f[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->f[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->f[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->f[specoffset] = spec[idx]


#define LOCAL_VARS(n)                                                          \
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);                  \
GLuint color[n], spec[n];                                                      \
GLuint coloroffset = nmesa->color_offset;                                      \
GLuint specoffset = nmesa->specular_offset;                                    \
(void)color; (void)spec; (void)coloroffset; (void)specoffset;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON+1] = {
	GL_POINTS+1,
	GL_LINES+1,
	GL_LINES+1,
	GL_LINES+1,
	GL_TRIANGLES+1,
	GL_TRIANGLES+1,
	GL_TRIANGLES+1,
	GL_QUADS+1,
	GL_QUADS+1,
	GL_TRIANGLES+1
};

#define RASTERIZE(x) nv20RasterPrimitive( ctx, x, hw_prim[x] )
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
#define INIT(x) nv20RasterPrimitive(ctx, x, hw_prim[x])
#undef LOCAL_VARS
#define LOCAL_VARS                                              \
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);                     \
GLubyte *vertptr = (GLubyte *)nmesa->verts;                 \
const GLuint vertsize = nmesa->vertex_size;          \
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
	GLuint vertsize = nmesa->vertex_size;
	nv20ExtendPrimitive(nmesa, (n - 2) * 3 * 4 * vertsize);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	const GLuint *start = (const GLuint *)V(elts[0]);
	int i;

	for (i = 2; i < n; i++) {
		OUT_RINGp(V(elts[i-1]),vertsize);
		OUT_RINGp(V(elts[i]),vertsize);
		OUT_RINGp(start,vertsize);
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

#define EMIT_ATTR( ATTR, STYLE )					\
do {									\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].attrib = (ATTR);	\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].format = (STYLE);	\
   nmesa->vertex_attr_count++;						\
} while (0)


static void nv20ChooseRenderState(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint index = 0;

	nmesa->draw_point = nv20_draw_point;
	nmesa->draw_line = nv20_draw_line;
	nmesa->draw_tri = nv20_draw_triangle;

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



static inline void nv20OutputVertexFormat(struct nouveau_context* nmesa, GLuint index)
{
	GLcontext* ctx=nmesa->glCtx;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	int attr_size[16];
	int default_attr_size[8]={3,3,3,4,3,1,4,4};
	int i;
	int slots=0;
	int total_size=0;

	/*
	 * Determine attribute sizes
	 */
	for(i=0;i<8;i++)
	{
		if (index&(1<<i))
			attr_size[i]=default_attr_size[i];
		else
			attr_size[i]=0;
	}
	for(i=8;i<16;i++)
	{
		if (index&(1<<i))
			attr_size[i]=VB->TexCoordPtr[i];
		else
			attr_size[i]=0;
	}

	/*
	 * Tell t_vertex about the vertex format
	 */
	for(i=0;i<16;i++)
	{
		if (index&(1<<i))
		{
			slots=i+1;
			if (i==0)
			{
				/* special-case POS */
				EMIT_ATTR(_TNL_ATTRIB_POS,EMIT_3F_VIEWPORT);
			}
			else
			{
				switch(attr_size[i])
				{
					case 1:
						EMIT_ATTR(i,EMIT_1F);
						break;
					case 2:
						EMIT_ATTR(i,EMIT_2F);
						break;
					case 3:
						EMIT_ATTR(i,EMIT_3F);
						break;
					case 4:
						EMIT_ATTR(i,EMIT_4F);
						break;
				}
			}
			if (i==_TNL_ATTRIB_COLOR0)
				nmesa->color_offset=total_size;
			if (i==_TNL_ATTRIB_COLOR1)
				nmesa->specular_offset=total_size;
			total_size+=attr_size[i];
		}
	}
	nmesa->vertex_size=total_size;

	/* 
	 * Tell the hardware about the vertex format
	 */
	if (nmesa->screen->card_type==NV_10) {
		// XXX needs some love
	} else if (nmesa->screen->card_type==NV_20) {
		for(i=0;i<16;i++)
		{
			int size=attr_size[i];
			BEGIN_RING_SIZE(channel,NV20_VERTEX_ATTRIBUTE(i),1);
			OUT_RING(NV20_VERTEX_ATTRIBUTE_TYPE_FLOAT|(size*0x10));
		}
	} else {
		BEGIN_RING_SIZE(channel,NV30_VERTEX_ATTRIBUTES,slots);
		for(i=0;i<slots;i++)
		{
			int size=attr_size[i];
			OUT_RING(NV20_VERTEX_ATTRIBUTE_TYPE_FLOAT|(size*0x10));
		}
		BEGIN_RING_SIZE(channel,NV30_UNKNOWN_0,1);
		OUT_RING(0);
		BEGIN_RING_SIZE(channel,NV30_UNKNOWN_0,1);
		OUT_RING(0);
		BEGIN_RING_SIZE(channel,NV30_UNKNOWN_0,1);
		OUT_RING(0);
	}
}


static void nv20ChooseVertexState( GLcontext *ctx )
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint index = tnl->render_inputs;

	if (index!=nmesa->render_inputs)
	{
		nmesa->render_inputs=index;
		nv20OutputVertexFormat(nmesa,index);
	}
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void nv20RenderStart(GLcontext *ctx)
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
		nv20ChooseVertexState(ctx);
		nv20ChooseRenderState(ctx);
		nmesa->newRenderState = 0;
	}
}

static void nv20RenderFinish(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	nv20FinishPrimitive(nmesa);
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void nv20RasterPrimitive(GLcontext *ctx,
		GLenum glprim,
		GLuint hwprim)
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
static void nv20RenderPrimitive( GLcontext *ctx, GLuint prim )
{
	nv20RasterPrimitive( ctx, prim, hw_prim[prim] );
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
	tnl->Driver.Render.Start = nv20RenderStart;
	tnl->Driver.Render.Finish = nv20RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = nv20RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = nouveauResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;

	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
			(6 + 2*ctx->Const.MaxTextureUnits) * sizeof(GLfloat) );

	nmesa->verts = (GLubyte *)tnl->clipspace.vertex_buf;

}

