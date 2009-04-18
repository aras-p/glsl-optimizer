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

#define ADD_ATTR(_attr, _format, _dst_loc, _swizzle, _write_mask) \
do { \
	attrs[num_attrs].attr = (_attr); \
	attrs[num_attrs].format = (_format); \
	attrs[num_attrs].dst_loc = (_dst_loc); \
	attrs[num_attrs].swizzle = (_swizzle); \
	attrs[num_attrs].write_mask = (_write_mask); \
	++num_attrs; \
} while (0)

static void r300SwtclVAPSetup(GLcontext *ctx, GLuint InputsRead, GLuint OutputsWritten, GLuint vap_out_fmt_1)
{
	r300ContextPtr rmesa = R300_CONTEXT( ctx );
	struct vertex_attribute *attrs = rmesa->swtcl.vert_attrs;
	int i, j, reg_count;
	uint32_t *vir0 = &rmesa->hw.vir[0].cmd[1];
	uint32_t *vir1 = &rmesa->hw.vir[1].cmd[1];

	for (i = 0; i < R300_VIR_CMDSIZE-1; ++i)
		vir0[i] = vir1[i] = 0;

	for (i = 0, j = 0; i < rmesa->radeon.swtcl.vertex_attr_count; ++i) {
		int tmp, data_format;
		switch (attrs[i].format) {
			case EMIT_1F:
				data_format = R300_DATA_TYPE_FLOAT_1;
				break;
			case EMIT_2F:
				data_format = R300_DATA_TYPE_FLOAT_2;
				break;
			case EMIT_3F:
				data_format = R300_DATA_TYPE_FLOAT_3;
				break;
			case EMIT_4F:
				data_format = R300_DATA_TYPE_FLOAT_4;
				break;
			case EMIT_4UB_4F_RGBA:
			case EMIT_4UB_4F_ABGR:
				data_format = R300_DATA_TYPE_BYTE | R300_NORMALIZE;
				break;
			default:
				fprintf(stderr, "%s: Invalid data format type", __FUNCTION__);
				_mesa_exit(-1);
				break;
		}

		tmp = data_format | (attrs[i].dst_loc << R300_DST_VEC_LOC_SHIFT);
		if (i % 2 == 0) {
			vir0[j] = tmp << R300_DATA_TYPE_0_SHIFT;
			vir1[j] = attrs[i].swizzle | (attrs[i].write_mask << R300_WRITE_ENA_SHIFT);
		} else {
			vir0[j] |= tmp << R300_DATA_TYPE_1_SHIFT;
			vir1[j] |= (attrs[i].swizzle | (attrs[i].write_mask << R300_WRITE_ENA_SHIFT)) << R300_SWIZZLE1_SHIFT;
			++j;
		}
	}

	reg_count = (rmesa->radeon.swtcl.vertex_attr_count + 1) >> 1;
	if (rmesa->radeon.swtcl.vertex_attr_count % 2 != 0) {
		vir0[reg_count-1] |= R300_LAST_VEC << R300_DATA_TYPE_0_SHIFT;
	} else {
		vir0[reg_count-1] |= R300_LAST_VEC << R300_DATA_TYPE_1_SHIFT;
	}

	R300_STATECHANGE(rmesa, vir[0]);
	R300_STATECHANGE(rmesa, vir[1]);
	R300_STATECHANGE(rmesa, vof);
	R300_STATECHANGE(rmesa, vic);

	if (rmesa->radeon.radeonScreen->kernel_mm) {
		rmesa->hw.vir[0].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[1].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[0].cmd[0] |= (reg_count & 0x3FFF) << 16;
		rmesa->hw.vir[1].cmd[0] |= (reg_count & 0x3FFF) << 16;
	} else {
		((drm_r300_cmd_header_t *) rmesa->hw.vir[0].cmd)->packet0.count = reg_count;
		((drm_r300_cmd_header_t *) rmesa->hw.vir[1].cmd)->packet0.count = reg_count;
	}

	rmesa->hw.vic.cmd[R300_VIC_CNTL_0] = r300VAPInputCntl0(ctx, InputsRead);
	rmesa->hw.vic.cmd[R300_VIC_CNTL_1] = r300VAPInputCntl1(ctx, InputsRead);
	rmesa->hw.vof.cmd[R300_VOF_CNTL_0] = r300VAPOutputCntl0(ctx, OutputsWritten);
	/**
	  * Can't use r300VAPOutputCntl1 function because it assumes
	  * that all texture coords have 4 components and that's the case
	  * for HW TCL path, but not for SW TCL.
	  */
	rmesa->hw.vof.cmd[R300_VOF_CNTL_1] = vap_out_fmt_1;
}


static void r300SetVertexFormat( GLcontext *ctx )
{
	r300ContextPtr rmesa = R300_CONTEXT( ctx );
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	int first_free_tex = 0, vap_out_fmt_1 = 0;
	GLuint InputsRead = 0;
	GLuint OutputsWritten = 0;
	int num_attrs = 0;
	struct vertex_attribute *attrs = rmesa->swtcl.vert_attrs;

	rmesa->swtcl.coloroffset = rmesa->swtcl.specoffset = 0;
	rmesa->radeon.swtcl.vertex_attr_count = 0;

	/* We always want non Ndc coords format */
	VB->AttribPtr[VERT_ATTRIB_POS] = VB->ClipPtr;

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_POS)) {
		InputsRead |= 1 << VERT_ATTRIB_POS;
		OutputsWritten |= 1 << VERT_RESULT_HPOS;
		EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F );
		ADD_ATTR(VERT_ATTRIB_POS, EMIT_4F, SWTCL_OVM_POS, SWIZZLE_XYZW, MASK_XYZW);
		rmesa->swtcl.coloroffset = 4;
	}

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_COLOR0)) {
		InputsRead |= 1 << VERT_ATTRIB_COLOR0;
		OutputsWritten |= 1 << VERT_RESULT_COL0;
#if MESA_LITTLE_ENDIAN
		EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA );
		ADD_ATTR(VERT_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA, SWTCL_OVM_COLOR0, SWIZZLE_XYZW, MASK_XYZW);
#else
		EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR );
		ADD_ATTR(VERT_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR, SWTCL_OVM_COLOR0, SWIZZLE_XYZW, MASK_XYZW);
#endif
	}

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_COLOR1 )) {
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
		InputsRead |= 1 << VERT_ATTRIB_COLOR1;
		OutputsWritten |= 1 << VERT_RESULT_COL1;
#if MESA_LITTLE_ENDIAN
		EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_4UB_4F_RGBA );
		ADD_ATTR(VERT_ATTRIB_COLOR1, EMIT_4UB_4F_RGBA, SWTCL_OVM_COLOR1, swiz, MASK_XYZW);
#else
		EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_4UB_4F_ABGR );
		ADD_ATTR(VERT_ATTRIB_COLOR1, EMIT_4UB_4F_ABGR, SWTCL_OVM_COLOR1, swiz, MASK_XYZW);
#endif
		rmesa->swtcl.specoffset = rmesa->swtcl.coloroffset + 1;
	}

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_POINTSIZE )) {
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO);
		InputsRead |= 1 << VERT_ATTRIB_POINT_SIZE;
		OutputsWritten |= 1 << VERT_RESULT_PSIZ;
		EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F );
		ADD_ATTR(VERT_ATTRIB_POINT_SIZE, EMIT_1F, SWTCL_OVM_POINT_SIZE, swiz, MASK_X);
	}

	if (RENDERINPUTS_TEST_RANGE(tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
		int i, size;
		GLuint swiz, mask, format;
		for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
			if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX(i) )) {
				switch (VB->TexCoordPtr[i]->size) {
					case 1:
					case 2:
						format = EMIT_2F;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ZERO);
						mask = MASK_X | MASK_Y;
						size = 2;
						break;
					case 3:
						format = EMIT_3F;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);
						mask = MASK_X | MASK_Y | MASK_Z;
						size = 3;
						break;
					case 4:
						format = EMIT_4F;
						swiz = SWIZZLE_XYZW;
						mask = MASK_XYZW;
						size = 4;
						break;
					default:
						continue;
				}
				InputsRead |= 1 << (VERT_ATTRIB_TEX0 + i);
				OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
				EMIT_ATTR(_TNL_ATTRIB_TEX(i), format);
				ADD_ATTR(VERT_ATTRIB_TEX0 + i, format, SWTCL_OVM_TEX(i), swiz, mask);
				vap_out_fmt_1 |= size << (i * 3);
				++first_free_tex;
			}
		}
	}

	/* RS can't put fragment position on the pixel stack, so stuff it in texcoord if needed */
	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_POS) && (ctx->FragmentProgram._Current->Base.InputsRead & FRAG_BIT_WPOS)) {
		if (first_free_tex >= ctx->Const.MaxTextureUnits) {
			fprintf(stderr, "\tout of free texcoords to write w pos\n");
			_mesa_exit(-1);
		}

		InputsRead |= 1 << (VERT_ATTRIB_TEX0 + first_free_tex);
		OutputsWritten |= 1 << (VERT_RESULT_TEX0 + first_free_tex);
		EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F );
		ADD_ATTR(VERT_ATTRIB_POS, EMIT_4F, SWTCL_OVM_TEX(first_free_tex), SWIZZLE_XYZW, MASK_XYZW);
		vap_out_fmt_1 |= 4 << (first_free_tex * 3);
		++first_free_tex;
	}

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_FOG)) {
		if (first_free_tex >= ctx->Const.MaxTextureUnits) {
			fprintf(stderr, "\tout of free texcoords to write fog coordinate\n");
			_mesa_exit(-1);
		}

		InputsRead |= 1 << VERT_ATTRIB_FOG;
		OutputsWritten |= 1 << VERT_RESULT_FOGC;
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO);
		EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1F );
		ADD_ATTR(VERT_ATTRIB_FOG, EMIT_1F, SWTCL_OVM_TEX(first_free_tex), swiz, MASK_X);
		vap_out_fmt_1 |=  1 << (first_free_tex * 3);
	}

	R300_NEWPRIM(rmesa);
	r300SwtclVAPSetup(ctx, InputsRead, OutputsWritten, vap_out_fmt_1);

	rmesa->radeon.swtcl.vertex_size =
		_tnl_install_attrs( ctx,
				    rmesa->radeon.swtcl.vertex_attrs,
				    rmesa->radeon.swtcl.vertex_attr_count,
				    NULL, 0 );

	rmesa->radeon.swtcl.vertex_size /= 4;

	RENDERINPUTS_COPY(rmesa->render_inputs_bitset, tnl->render_inputs_bitset);
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
static void r300RenderPrimitive( GLcontext *ctx, GLenum prim );

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
#define GET_VERTEX_DWORDS() rmesa->radeon.swtcl.vertex_size
#define ALLOC_VERTS( n, size ) rcommonAllocDmaLowVerts( &rmesa->radeon, n, size * 4 )
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

	if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R300_TWOSIDE_BIT;
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


static void r300RenderStart(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT( ctx );

	r300ChooseRenderState(ctx);
	r300SetVertexFormat(ctx);

	r300ValidateBuffers(ctx);

	r300UpdateShaders(rmesa);
	r300UpdateShaderStates(rmesa);

	r300EmitCacheFlush(rmesa);

	/* investigate if we can put back flush optimisation if needed */
	if (rmesa->radeon.dma.flush != NULL) {
		rmesa->radeon.dma.flush(ctx);
	}
}

static void r300RenderFinish(GLcontext *ctx)
{
}

static void r300RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	if (rmesa->radeon.swtcl.hw_primitive != hwprim) {
		R300_NEWPRIM( rmesa );
		rmesa->radeon.swtcl.hw_primitive = hwprim;
	}
}

static void r300RenderPrimitive(GLcontext *ctx, GLenum prim)
{

	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	rmesa->radeon.swtcl.render_primitive = prim;

	if ((prim == GL_TRIANGLES) && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
		return;

	r300RasterPrimitive( ctx, reduced_prim[prim] );
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
			    48 * sizeof(GLfloat) );

	rmesa->radeon.swtcl.verts = (GLubyte *)tnl->clipspace.vertex_buf;
	rmesa->radeon.swtcl.RenderIndex = ~0;
	rmesa->radeon.swtcl.render_primitive = GL_TRIANGLES;
	rmesa->radeon.swtcl.hw_primitive = 0;

	_tnl_invalidate_vertex_state( ctx, ~0 );
	_tnl_invalidate_vertices( ctx, ~0 );

	_tnl_need_projected_coords( ctx, GL_FALSE );
	r300ChooseRenderState(ctx);
}

void r300DestroySwtcl(GLcontext *ctx)
{
}

static void r300EmitVertexAOS(r300ContextPtr rmesa, GLuint vertex_size, struct radeon_bo *bo, GLuint offset)
{
	BATCH_LOCALS(&rmesa->radeon);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s:  vertex_size %d, offset 0x%x \n",
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

	type = r300PrimitiveType(rmesa, primitive);
	num_verts = r300NumVerts(rmesa, vertex_nr, primitive);

	BEGIN_BATCH(3);
	OUT_BATCH_PACKET3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
	OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (num_verts << 16) | type);
	END_BATCH();
}

void r300_swtcl_flush(GLcontext *ctx, uint32_t current_offset)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	rcommonEnsureCmdBufSpace(&rmesa->radeon,
			   rmesa->radeon.hw.max_state_size + (12*sizeof(int)),
			   __FUNCTION__);
	radeonEmitState(&rmesa->radeon);
	r300EmitVertexAOS(rmesa,
			rmesa->radeon.swtcl.vertex_size,
			rmesa->radeon.dma.current,
			current_offset);

	r300EmitVbufPrim(rmesa,
		   rmesa->radeon.swtcl.hw_primitive,
		   rmesa->radeon.swtcl.numverts);
	r300EmitCacheFlush(rmesa);
	COMMIT_BATCH();
}
