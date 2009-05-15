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

#include "r600_swtcl.h"
#include "r600_emit.h"

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

static void r600SwtclVAPSetup(GLcontext *ctx, GLuint InputsRead, GLuint OutputsWritten)
{
}


static void r600SetVertexFormat( GLcontext *ctx )
{
#if 0 /* to be enabled */
	r600ContextPtr rmesa = R600_CONTEXT( ctx );
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	int fog_id = -1;
	GLuint InputsRead = 0;
	GLuint OutputsWritten = 0;
	int num_attrs = 0;
	struct vertex_attribute *attrs = rmesa->swtcl.vert_attrs;

	rmesa->swtcl.coloroffset = rmesa->swtcl.specoffset = 0;
	rmesa->radeon.swtcl.vertex_attr_count = 0;

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

	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_FOG)) {
		/* find first free tex coord slot */
		if (RENDERINPUTS_TEST_RANGE(tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
			int i;
			for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
				if (!RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX(i) )) {
					fog_id = i;
					break;
				}
			}
		} else {
			fog_id = 0;
		}

		if (fog_id == -1) {
			fprintf(stderr, "\tout of free texcoords to do fog\n");
			_mesa_exit(-1);
		}

		InputsRead |= 1 << VERT_ATTRIB_FOG;
		OutputsWritten |= 1 << VERT_RESULT_FOGC;
		GLuint swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO);
		EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1F );
		ADD_ATTR(VERT_ATTRIB_FOG, EMIT_1F, SWTCL_OVM_TEX(fog_id), swiz, MASK_X);
	}

	if (RENDERINPUTS_TEST_RANGE(tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
		int i;
		GLuint swiz, mask, format;
		for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
			if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX(i) )) {
				switch (VB->TexCoordPtr[i]->size) {
					case 1:
					case 2:
						format = EMIT_2F;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ZERO);
						mask = MASK_X | MASK_Y;
						break;
					case 3:
						format = EMIT_3F;
						swiz = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);
						mask = MASK_X | MASK_Y | MASK_Z;
						break;
					case 4:
						format = EMIT_4F;
						swiz = SWIZZLE_XYZW;
						mask = MASK_XYZW;
						break;
					default:
						continue;
				}
				InputsRead |= 1 << (VERT_ATTRIB_TEX0 + i);
				OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
				EMIT_ATTR(_TNL_ATTRIB_TEX(i), format);
				ADD_ATTR(VERT_ATTRIB_TEX0 + i, format, SWTCL_OVM_TEX(i), swiz, mask);
			}
		}
	}

	/* RS can't put fragment position on the pixel stack, so stuff it in texcoord if needed */
	if (RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_POS) && (ctx->FragmentProgram._Current->Base.InputsRead & FRAG_BIT_WPOS)) {
		int first_free_tex = -1;
		if (fog_id >= 0) {
			first_free_tex = fog_id+1;
		} else {
			if (RENDERINPUTS_TEST_RANGE(tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
				int i;
				for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
					if (!RENDERINPUTS_TEST(tnl->render_inputs_bitset, _TNL_ATTRIB_TEX(i) )) {
						first_free_tex = i;
						break;
					}
				}
			} else {
				first_free_tex = 0;
			}
		}

		if (first_free_tex == -1) {
			fprintf(stderr, "\tout of free texcoords to write w pos\n");
			_mesa_exit(-1);
		}

		InputsRead |= 1 << (VERT_ATTRIB_TEX0 + first_free_tex);
		OutputsWritten |= 1 << (VERT_RESULT_TEX0 + first_free_tex);
		EMIT_ATTR( _TNL_ATTRIB_TEX(first_free_tex), EMIT_4F );
		ADD_ATTR(VERT_ATTRIB_TEX0 + first_free_tex, EMIT_4F, SWTCL_OVM_TEX(first_free_tex), SWIZZLE_XYZW, MASK_XYZW);
	}

	R600_NEWPRIM(rmesa);
	r600SwtclVAPSetup(ctx, InputsRead, OutputsWritten);

	rmesa->radeon.swtcl.vertex_size =
		_tnl_install_attrs( ctx,
				    rmesa->radeon.swtcl.vertex_attrs,
				    rmesa->radeon.swtcl.vertex_attr_count,
				    NULL, 0 );

	rmesa->radeon.swtcl.vertex_size /= 4;

	RENDERINPUTS_COPY(rmesa->state.render_inputs_bitset, tnl->render_inputs_bitset);
#endif /* to be enabled */
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

static void r600RasterPrimitive( GLcontext *ctx, GLuint prim );
static void r600RenderPrimitive( GLcontext *ctx, GLenum prim );

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
#define CTX_ARG r600ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->radeon.swtcl.vertex_size
#define ALLOC_VERTS( n, size ) rcommonAllocDmaLowVerts( &rmesa->radeon, n, size * 4 )
#define LOCAL_VARS						\
   r600ContextPtr rmesa = R600_CONTEXT(ctx);		\
   const char *r600verts = (char *)rmesa->radeon.swtcl.verts;
#define VERT(x) (r600Vertex *)(r600verts + ((x) * vertsize * sizeof(int)))
#define VERTEX r600Vertex
#undef TAG
#define TAG(x) r600_##x
#include "tnl_dd/t_dd_triemit.h"



/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) r600_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     r600_triangle( rmesa, a, b, c )
#define LINE( a, b )       r600_line( rmesa, a, b )
#define POINT( a )         r600_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R600_TWOSIDE_BIT	0x01
#define R600_UNFILLED_BIT	0x02
#define R600_MAX_TRIFUNC	0x04

static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R600_MAX_TRIFUNC];

#define DO_FALLBACK  0
#define DO_UNFILLED (IND & R600_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R600_TWOSIDE_BIT)
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
   r600_color_t *color = (r600_color_t *)&((v)->ui[coloroffset]); \
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
   r600ContextPtr rmesa = R600_CONTEXT(ctx);			\
   GLuint color[n] = { 0, }, spec[n] = { 0, };						\
   GLuint coloroffset = rmesa->swtcl.coloroffset;	\
   GLuint specoffset = rmesa->swtcl.specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) r600RasterPrimitive( ctx, reduced_prim[x] )
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

#define IND (R600_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R600_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R600_TWOSIDE_BIT|R600_UNFILLED_BIT)
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
      r600_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r600_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r600_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r600_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   r600RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r600ContextPtr rmesa = R600_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->radeon.swtcl.vertex_size;		\
   const char *r600verts = (char *)rmesa->radeon.swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	//if ( stipple ) r200ResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r600_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r600_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"




/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/
static void r600ChooseRenderState( GLcontext *ctx )
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLuint index = 0;
	GLuint flags = ctx->_TriangleCaps;

	if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R600_TWOSIDE_BIT;
	if (flags & DD_TRI_UNFILLED)      index |= R600_UNFILLED_BIT;

	if (index != rmesa->radeon.swtcl.RenderIndex) {
		tnl->Driver.Render.Points = rast_tab[index].points;
		tnl->Driver.Render.Line = rast_tab[index].line;
		tnl->Driver.Render.ClippedLine = rast_tab[index].line;
		tnl->Driver.Render.Triangle = rast_tab[index].triangle;
		tnl->Driver.Render.Quad = rast_tab[index].quad;

		if (index == 0) {
			tnl->Driver.Render.PrimTabVerts = r600_render_tab_verts;
			tnl->Driver.Render.PrimTabElts = r600_render_tab_elts;
			tnl->Driver.Render.ClippedPolygon = r600_fast_clipped_poly;
		} else {
			tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
			tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
			tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
		}

		rmesa->radeon.swtcl.RenderIndex = index;
	}
}


static void r600RenderStart(GLcontext *ctx)
{
#if 0 /* to be enabled */
	r600ContextPtr rmesa = R600_CONTEXT( ctx );

	r600ChooseRenderState(ctx);
	r600SetVertexFormat(ctx);

	r600ValidateBuffers(ctx);

	r600UpdateShaders(rmesa);
	r600UpdateShaderStates(rmesa);

	r600EmitCacheFlush(rmesa);

	/* investigate if we can put back flush optimisation if needed */
	if (rmesa->radeon.dma.flush != NULL) {
		rmesa->radeon.dma.flush(ctx);
	}
#endif /* to be enabled */
}

static void r600RenderFinish(GLcontext *ctx)
{
}

static void r600RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
#if 0 /* to be enabled */
	r600ContextPtr rmesa = R600_CONTEXT(ctx);

	if (rmesa->radeon.swtcl.hw_primitive != hwprim) {
		R600_NEWPRIM( rmesa );
		rmesa->radeon.swtcl.hw_primitive = hwprim;
	}
#endif /* to be enabled */
}

static void r600RenderPrimitive(GLcontext *ctx, GLenum prim)
{

	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	rmesa->radeon.swtcl.render_primitive = prim;

	if ((prim == GL_TRIANGLES) && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
		return;

	r600RasterPrimitive( ctx, reduced_prim[prim] );
}

static void r600ResetLineStipple(GLcontext *ctx)
{
}

void r600InitSwtcl(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	static int firsttime = 1;

	if (firsttime) {
		init_rast_tab();
		firsttime = 0;
	}

	tnl->Driver.Render.Start = r600RenderStart;
	tnl->Driver.Render.Finish = r600RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = r600RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = r600ResetLineStipple;
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
	r600ChooseRenderState(ctx);
}

void r600DestroySwtcl(GLcontext *ctx)
{
}

static void r600EmitVertexAOS(r600ContextPtr rmesa, GLuint vertex_size, struct radeon_bo *bo, GLuint offset)
{
#if 0 /* to be enabled */
	BATCH_LOCALS(&rmesa->radeon);

	if (RADEON_DEBUG & DEBUG_VERTS)
		fprintf(stderr, "%s:  vertex_size %d, offset 0x%x \n",
			__FUNCTION__, vertex_size, offset);

	BEGIN_BATCH(7);
	OUT_BATCH_PACKET3(R600_PACKET3_3D_LOAD_VBPNTR, 2);
	R600_OUT_BATCH(1);
	R600_OUT_BATCH(vertex_size | (vertex_size << 8));
	OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
	END_BATCH();
#endif /* to be enabled */
}

static void r600EmitVbufPrim(r600ContextPtr rmesa, GLuint primitive, GLuint vertex_nr)
{
#if 0 /* to be enabled */
	BATCH_LOCALS(&rmesa->radeon);
	int type, num_verts;

	type = r600PrimitiveType(rmesa, primitive);
	num_verts = r600NumVerts(rmesa, vertex_nr, primitive);

	BEGIN_BATCH(3);
	OUT_BATCH_PACKET3(R600_PACKET3_3D_DRAW_VBUF_2, 0);
	R600_OUT_BATCH(R600_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (num_verts << 16) | type);
	END_BATCH();
#endif /* to be enabled */
}

void r600_swtcl_flush(GLcontext *ctx, uint32_t current_offset)
{
#if 0 /* to be enabled */
	r600ContextPtr rmesa = R600_CONTEXT(ctx);

	rcommonEnsureCmdBufSpace(&rmesa->radeon,
			   rmesa->radeon.hw.max_state_size + (12*sizeof(int)),
			   __FUNCTION__);
	radeonEmitState(&rmesa->radeon);
	r600EmitVertexAOS(rmesa,
			rmesa->radeon.swtcl.vertex_size,
			rmesa->radeon.dma.current,
			current_offset);

	r600EmitVbufPrim(rmesa,
		   rmesa->radeon.swtcl.hw_primitive,
		   rmesa->radeon.swtcl.numverts);
	r600EmitCacheFlush(rmesa);
	COMMIT_BATCH();
#endif /* to be enabled */
}
