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
 *   Maciej Cencora <m.cencora@gmail.com>
 */

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "r300_state.h"
#include "r300_swtcl.h"
#include "r300_emit.h"
#include "r300_tex.h"
#include "r300_render.h"
#include "main/simple_list.h"

#define EMIT_ATTR( ATTR, STYLE )					\
do {									\
	rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].attrib = (ATTR);	\
	rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].format = (STYLE);	\
	rmesa->radeon.swtcl.vertex_attr_count++;					\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].attrib = 0;		\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].format = EMIT_PAD;	\
   rmesa->radeon.swtcl.vertex_attrs[rmesa->radeon.swtcl.vertex_attr_count].offset = (N);		\
   rmesa->radeon.swtcl.vertex_attr_count++;					\
} while (0)

#define ADD_ATTR(_attr, _format, _dst_loc, _swizzle, _write_mask, _normalize) \
do { \
	attrs[num_attrs].element = (_attr); \
	attrs[num_attrs].data_type = (_format); \
	attrs[num_attrs].dst_loc = (_dst_loc); \
	attrs[num_attrs].swizzle = (_swizzle); \
	attrs[num_attrs].write_mask = (_write_mask); \
	attrs[num_attrs]._signed = 0; \
	attrs[num_attrs].normalize = (_normalize); \
	++num_attrs; \
} while (0)

void r300ChooseSwtclVertexFormat(GLcontext *ctx, GLuint *_InputsRead,  GLuint *_OutputsWritten)
{
	r300ContextPtr rmesa = R300_CONTEXT( ctx );
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	int first_free_tex = 0;
	GLuint InputsRead = 0;
	GLuint OutputsWritten = 0;
	int num_attrs = 0;
	GLuint fp_reads = rmesa->selected_fp->InputsRead;
	struct vertex_attribute *attrs = rmesa->vbuf.attribs;

	radeon_print(RADEON_SWRENDER, RADEON_VERBOSE, "%s\n", __func__);
	rmesa->swtcl.coloroffset = rmesa->swtcl.specoffset = 0;
	rmesa->radeon.swtcl.vertex_attr_count = 0;

	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s\n", __func__);

	/* We always want non Ndc coords format */
	VB->AttribPtr[VERT_ATTRIB_POS] = VB->ClipPtr;

	/* Always write position vector */
	InputsRead |= 1 << VERT_ATTRIB_POS;
	OutputsWritten |= 1 << VERT_RESULT_HPOS;
	EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F );
	ADD_ATTR(VERT_ATTRIB_POS, R300_DATA_TYPE_FLOAT_4, SWTCL_OVM_POS, SWIZZLE_XYZW, MASK_XYZW, 0);
	rmesa->swtcl.coloroffset = 4;

	if (fp_reads & FRAG_BIT_COL0) {
		InputsRead |= 1 << VERT_ATTRIB_COLOR0;
		OutputsWritten |= 1 << VERT_RESULT_COL0;
#if MESA_LITTLE_ENDIAN
		EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA );
		ADD_ATTR(VERT_ATTRIB_COLOR0, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR0, SWIZZLE_XYZW, MASK_XYZW, 1);
#else
		EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR );
		ADD_ATTR(VERT_ATTRIB_COLOR0, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR0, SWIZZLE_XYZW, MASK_XYZW, 1);
#endif
	}

	if (fp_reads & FRAG_BIT_COL1) {
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
		InputsRead |= 1 << VERT_ATTRIB_COLOR1;
		OutputsWritten |= 1 << VERT_RESULT_COL1;
#if MESA_LITTLE_ENDIAN
		EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_4UB_4F_RGBA );
		ADD_ATTR(VERT_ATTRIB_COLOR1, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR1, swiz, MASK_XYZW, 1);
#else
		EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_4UB_4F_ABGR );
		ADD_ATTR(VERT_ATTRIB_COLOR1, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR1, swiz, MASK_XYZW, 1);
#endif
		rmesa->swtcl.specoffset = rmesa->swtcl.coloroffset + 1;
	}

	if (ctx->Light.Enabled && ctx->Light.Model.TwoSide) {
		VB->AttribPtr[VERT_ATTRIB_GENERIC0] = VB->BackfaceColorPtr;
		OutputsWritten |= 1 << VERT_RESULT_BFC0;
#if MESA_LITTLE_ENDIAN
		EMIT_ATTR( _TNL_ATTRIB_GENERIC0, EMIT_4UB_4F_RGBA );
		ADD_ATTR(VERT_ATTRIB_GENERIC0, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR2, SWIZZLE_XYZW, MASK_XYZW, 1);
#else
		EMIT_ATTR( _TNL_ATTRIB_GENERIC0, EMIT_4UB_4F_ABGR );
		ADD_ATTR(VERT_ATTRIB_GENERIC0, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR2, SWIZZLE_XYZW, MASK_XYZW, 1);
#endif
		if (fp_reads & FRAG_BIT_COL1) {
			VB->AttribPtr[VERT_ATTRIB_GENERIC1] = VB->BackfaceSecondaryColorPtr;
			GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
			OutputsWritten |= 1 << VERT_RESULT_BFC1;
#if MESA_LITTLE_ENDIAN
			EMIT_ATTR( _TNL_ATTRIB_GENERIC1, EMIT_4UB_4F_RGBA );
			ADD_ATTR(VERT_ATTRIB_GENERIC1, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR3, swiz, MASK_XYZW, 1);
#else
			EMIT_ATTR( _TNL_ATTRIB_GENERIC1, EMIT_4UB_4F_ABGR );
			ADD_ATTR(VERT_ATTRIB_GENERIC1, R300_DATA_TYPE_BYTE, SWTCL_OVM_COLOR3, swiz, MASK_XYZW, 1);
#endif
		}
	}

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_POINTSIZE )) {
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO);
		InputsRead |= 1 << VERT_ATTRIB_POINT_SIZE;
		OutputsWritten |= 1 << VERT_RESULT_PSIZ;
		EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F );
		ADD_ATTR(VERT_ATTRIB_POINT_SIZE, R300_DATA_TYPE_FLOAT_1, SWTCL_OVM_POINT_SIZE, swiz, MASK_X, 0);
	}

	if (rmesa->selected_fp->wpos_attr != FRAG_ATTRIB_MAX) {
		int tex_id = rmesa->selected_fp->wpos_attr - FRAG_ATTRIB_TEX0;

		VB->AttribPtr[VERT_ATTRIB_TEX0 + tex_id] = VB->AttribPtr[VERT_ATTRIB_POS];
		VB->AttribPtr[_TNL_ATTRIB_TEX0 + tex_id] = VB->AttribPtr[VERT_ATTRIB_POS];
		RENDERINPUTS_SET(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX0 + tex_id);
	}

	if (rmesa->selected_fp->fog_attr != FRAG_ATTRIB_MAX) {
		int tex_id = rmesa->selected_fp->fog_attr - FRAG_ATTRIB_TEX0;

		VB->AttribPtr[VERT_ATTRIB_TEX0 + tex_id] = VB->AttribPtr[VERT_ATTRIB_FOG];
		VB->AttribPtr[_TNL_ATTRIB_TEX0 + tex_id] = VB->AttribPtr[VERT_ATTRIB_FOG];
		RENDERINPUTS_SET(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX0 + tex_id);
	}

	/**
	 *  Sending only one texcoord component may lead to lock up,
	 *  so for all textures always output 4 texcoord components to RS.
	 */
	{
		int i;
		GLuint swiz, format, hw_format;
		for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
			if (fp_reads & FRAG_BIT_TEX(i)) {
				switch (VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]->size) {
					case 1:
						format = EMIT_1F;
						hw_format = R300_DATA_TYPE_FLOAT_1;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ONE);
						break;
					case 2:
						format = EMIT_2F;
						hw_format = R300_DATA_TYPE_FLOAT_2;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ONE);
						break;
					case 3:
						format = EMIT_3F;
						hw_format = R300_DATA_TYPE_FLOAT_3;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
						break;
					case 4:
						format = EMIT_4F;
						hw_format = R300_DATA_TYPE_FLOAT_4;
						swiz = SWIZZLE_XYZW;
						break;
					default:
						continue;
				}
				InputsRead |= 1 << (VERT_ATTRIB_TEX0 + i);
				OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
				EMIT_ATTR(_TNL_ATTRIB_TEX(i), format);
				ADD_ATTR(VERT_ATTRIB_TEX0 + i, hw_format, SWTCL_OVM_TEX(first_free_tex), swiz, MASK_XYZW, 0);
				++first_free_tex;
			}
		}
	}

	if (first_free_tex >= ctx->Const.MaxTextureUnits) {
		fprintf(stderr, "\tout of free texcoords to write fog coordinate\n");
		exit(-1);
	}

	R300_NEWPRIM(rmesa);
	rmesa->vbuf.num_attribs = num_attrs;
	*_InputsRead = InputsRead;
	*_OutputsWritten = OutputsWritten;

	RENDERINPUTS_COPY(rmesa->render_inputs_bitset, tnl->render_inputs_bitset);
}

static void r300PrepareVertices(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLuint InputsRead, OutputsWritten;
	radeon_print(RADEON_SWRENDER, RADEON_TRACE, "%s\n", __func__);

	r300ChooseSwtclVertexFormat(ctx, &InputsRead, &OutputsWritten);
	r300SetupVAP(ctx, InputsRead, OutputsWritten);

	rmesa->radeon.swtcl.vertex_size =
		_tnl_install_attrs( ctx,
				    rmesa->radeon.swtcl.vertex_attrs,
				    rmesa->radeon.swtcl.vertex_attr_count,
				    NULL, 0 );

	rmesa->radeon.swtcl.vertex_size /= 4;
}

static void r300_predict_emit_size( r300ContextPtr rmesa )
{
	if (!rmesa->radeon.swtcl.emit_prediction) {
		const int vertex_size = 7;
		const int prim_size = 3;
		const int cache_flush_size = 4;
		const int pre_emit_state = 4;
		const int scissor_size = 3;
		const int state_size = radeonCountStateEmitSize(&rmesa->radeon);

		if (rcommonEnsureCmdBufSpace(&rmesa->radeon,
					state_size + pre_emit_state + scissor_size
					+ vertex_size + prim_size + cache_flush_size * 2,
					__FUNCTION__))
			rmesa->radeon.swtcl.emit_prediction = radeonCountStateEmitSize(&rmesa->radeon);
		else
			rmesa->radeon.swtcl.emit_prediction = state_size;

		rmesa->radeon.swtcl.emit_prediction += rmesa->radeon.cmdbuf.cs->cdw
			+ vertex_size + scissor_size + prim_size + cache_flush_size * 2 + pre_emit_state;
		radeon_print(RADEON_SWRENDER, RADEON_VERBOSE,
				"%s, size %d\n",
				__func__, rmesa->radeon.cmdbuf.cs->cdw
				+ vertex_size + scissor_size + prim_size + cache_flush_size * 2 + pre_emit_state);
	}
}


static GLuint reduced_prim[] = {
	GL_POINTS,
	GL_LINES,
	GL_LINES,
	GL_LINES,
	GL_TRIANGLES,
	GL_TRIANGLES,
	GL_TRIANGLES,
	GL_TRIANGLES,
	GL_TRIANGLES,
	GL_TRIANGLES,
};

static void r300RasterPrimitive( GLcontext *ctx, GLuint prim );

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

static void* r300_alloc_verts(r300ContextPtr rmesa, GLuint n, GLuint size)
{
	void *rv;
	do {
		r300_predict_emit_size( rmesa );
		rv = rcommonAllocDmaLowVerts( &rmesa->radeon, n, size * 4 );
	} while (!rv);
	return rv;
}

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG r300ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->radeon.swtcl.vertex_size
#define ALLOC_VERTS( n, size ) r300_alloc_verts(rmesa, n, size);
#define LOCAL_VARS						\
   r300ContextPtr rmesa = R300_CONTEXT(ctx);		\
   const char *r300verts = (char *)rmesa->radeon.swtcl.verts;
#define VERT(x) (r300Vertex *)(r300verts + ((x) * vertsize * sizeof(int)))
#define VERTEX r300Vertex
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

#define R300_UNFILLED_BIT	0x01
#define R300_MAX_TRIFUNC	0x02

static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R300_MAX_TRIFUNC];

#define DO_FALLBACK  0
#define DO_UNFILLED (IND & R300_UNFILLED_BIT)
#define DO_TWOSIDE   0
#define DO_FLAT      0
#define DO_OFFSET    0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

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
#define GET_VERTEX(e) (rmesa->radeon.swtcl.verts + (e*rmesa->radeon.swtcl.vertex_size*sizeof(int)))

#define VERT_SET_RGBA( v, c ) \
do { \
   r300_color_t *color = (r300_color_t *)&((v)->ui[coloroffset]); \
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]); \
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]); \
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]); \
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]); \
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v0, c ) \
do { \
   if (specoffset) { \
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.red, (c)[0]); \
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.green, (c)[1]); \
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.blue, (c)[2]); \
   } \
} while (0)

#define VERT_COPY_SPEC( v0, v1 ) \
do { \
   if (specoffset) { \
       v0->v.specular.red = v1->v.specular.red; \
       v0->v.specular.green = v1->v.specular.green; \
       v0->v.specular.blue = v1->v.specular.blue; \
   } \
} while (0)

#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]

#undef LOCAL_VARS
#undef TAG
#undef INIT

#define LOCAL_VARS(n)							\
   r300ContextPtr rmesa = R300_CONTEXT(ctx);			\
   GLuint color[n] = { 0, }, spec[n] = { 0, };				\
   GLuint coloroffset = rmesa->swtcl.coloroffset;	\
   GLuint specoffset = rmesa->swtcl.specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) r300RasterPrimitive( ctx, reduced_prim[x] )
#define RENDER_PRIMITIVE rmesa->radeon.swtcl.render_primitive
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

#define IND (R300_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_unfilled();
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
   const GLuint vertsize = rmesa->radeon.swtcl.vertex_size;		\
   const char *r300verts = (char *)rmesa->radeon.swtcl.verts;		\
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
static void r300ChooseRenderState( GLcontext *ctx )
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLuint index = 0;
	GLuint flags = ctx->_TriangleCaps;
	radeon_print(RADEON_SWRENDER, RADEON_VERBOSE, "%s\n", __func__);

	if (flags & DD_TRI_UNFILLED)      index |= R300_UNFILLED_BIT;

	if (index != rmesa->radeon.swtcl.RenderIndex) {
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

		rmesa->radeon.swtcl.RenderIndex = index;
	}
}

void r300RenderStart(GLcontext *ctx)
{
	radeon_print(RADEON_SWRENDER, RADEON_VERBOSE, "%s\n", __func__);
	r300ContextPtr rmesa = R300_CONTEXT( ctx );

	r300ChooseRenderState(ctx);

	r300UpdateShaders(rmesa);

	r300PrepareVertices(ctx);

	r300ValidateBuffers(ctx);

	r300UpdateShaderStates(rmesa);


	/* investigate if we can put back flush optimisation if needed */
	if (rmesa->radeon.dma.flush != NULL) {
		rmesa->radeon.dma.flush(ctx);
	}
}

void r300RenderFinish(GLcontext *ctx)
{
}

static void r300RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	radeon_print(RADEON_SWRENDER, RADEON_TRACE, "%s\n", __func__);

	if (rmesa->radeon.swtcl.hw_primitive != hwprim) {
		R300_NEWPRIM( rmesa );
		rmesa->radeon.swtcl.hw_primitive = hwprim;
	}
}

void r300RenderPrimitive(GLcontext *ctx, GLenum prim)
{

	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	rmesa->radeon.swtcl.render_primitive = prim;
	radeon_print(RADEON_SWRENDER, RADEON_TRACE, "%s\n", __func__);

	if ((prim == GL_TRIANGLES) && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
		return;

	r300RasterPrimitive( ctx, reduced_prim[prim] );
}

void r300ResetLineStipple(GLcontext *ctx)
{
	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s\n", __func__);
}

void r300InitSwtcl(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	static int firsttime = 1;
	radeon_print(RADEON_SWRENDER, RADEON_NORMAL, "%s\n", __func__);

	if (firsttime) {
		init_rast_tab();
		firsttime = 0;
	}
	rmesa->radeon.swtcl.emit_prediction = 0;

	tnl->Driver.Render.Start = r300RenderStart;
	tnl->Driver.Render.Finish = r300RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = r300RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = r300ResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;

	/* FIXME: what are these numbers? */
	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12,
			    48 * sizeof(GLfloat) );

	rmesa->radeon.swtcl.verts = (GLubyte *)tnl->clipspace.vertex_buf;
	rmesa->radeon.swtcl.RenderIndex = ~0;
	rmesa->radeon.swtcl.render_primitive = GL_TRIANGLES;
	rmesa->radeon.swtcl.hw_primitive = 0;

	_tnl_invalidate_vertex_state( ctx, ~0 );
	_tnl_invalidate_vertices( ctx, ~0 );

	_tnl_need_projected_coords( ctx, GL_FALSE );
}

void r300DestroySwtcl(GLcontext *ctx)
{
}

static void r300EmitVertexAOS(r300ContextPtr rmesa, GLuint vertex_size, struct radeon_bo *bo, GLuint offset)
{
	BATCH_LOCALS(&rmesa->radeon);

	radeon_print(RADEON_SWRENDER, RADEON_TRACE,
		"%s:  vertex_size %d, offset 0x%x \n",
			__FUNCTION__, vertex_size, offset);

	BEGIN_BATCH(7);
	OUT_BATCH_PACKET3(R300_PACKET3_3D_LOAD_VBPNTR, 2);
	OUT_BATCH(1);
	OUT_BATCH(vertex_size | (vertex_size << 8));
	OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
	END_BATCH();
}

static void r300EmitVbufPrim(r300ContextPtr rmesa, GLuint primitive, GLuint vertex_nr)
{
	BATCH_LOCALS(&rmesa->radeon);
	int type, num_verts;
	if (RADEON_DEBUG & RADEON_VERTS)
		fprintf(stderr, "%s\n", __func__);

	type = r300PrimitiveType(rmesa, primitive);
	num_verts = r300NumVerts(rmesa, vertex_nr, primitive);

	BEGIN_BATCH(3);
	OUT_BATCH_PACKET3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
	OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (num_verts << 16) | type);
	END_BATCH();
}

void r300_swtcl_flush(GLcontext *ctx, uint32_t current_offset)
{
	radeon_print(RADEON_SWRENDER, RADEON_TRACE, "%s\n", __func__);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	r300EmitCacheFlush(rmesa);

	radeonEmitState(&rmesa->radeon);
	r300_emit_scissor(ctx);
	r300EmitVertexAOS(rmesa,
			  rmesa->radeon.swtcl.vertex_size,
			  rmesa->radeon.swtcl.bo,
			  current_offset);

	r300EmitVbufPrim(rmesa,
		   rmesa->radeon.swtcl.hw_primitive,
		   rmesa->radeon.swtcl.numverts);
	r300EmitCacheFlush(rmesa);
	if ( rmesa->radeon.swtcl.emit_prediction < rmesa->radeon.cmdbuf.cs->cdw )
		WARN_ONCE("Rendering was %d commands larger than predicted size."
			" We might overflow  command buffer.\n",
			rmesa->radeon.cmdbuf.cs->cdw - rmesa->radeon.swtcl.emit_prediction );
	rmesa->radeon.swtcl.emit_prediction = 0;
	COMMIT_BATCH();
}
