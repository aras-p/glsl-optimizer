/**************************************************************************

Copyright 2006 Nouveau
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"

#include "tnl/t_pipeline.h"

#include "mtypes.h"
#include "colormac.h"

static void nv30AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte ubRef;
	CLAMPED_FLOAT_TO_UBYTE(ubRef, ref);

	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC, 2);
	OUT_RING(func);     /* NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_FUNC */
	OUT_RING(ubRef);    /* NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_REF  */
}

static void nv30BlendColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx); 
	GLubyte cf[4];

	CLAMPED_FLOAT_TO_UBYTE(cf[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(cf[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(cf[2], color[2]);
	CLAMPED_FLOAT_TO_UBYTE(cf[3], color[3]);

	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_COLOR, 1);
	OUT_RING(PACK_COLOR_8888(cf[3], cf[1], cf[2], cf[0]));
}

static void nv30BlendEquationSeparate(GLcontext *ctx, GLenum modeRGB, GLenum modeA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_EQUATION, 1);
	OUT_RING((modeA<<16) | modeRGB);
}


static void nv30BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, GLenum dfactorRGB,
		GLenum sfactorA, GLenum dfactorA)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_FUNC_SRC, 2);
	OUT_RING((sfactorA<<16) | sfactorRGB);
	OUT_RING((dfactorA<<16) | dfactorRGB);
}

static void nv30ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte c[4];
	UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,color);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_ARGB, 1);
	OUT_RING(PACK_COLOR_8888(c[3],c[0],c[1],c[2]));
}

static void nv30ClearDepth(GLcontext *ctx, GLclampd d)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0x000000FF)|(((uint32_t)(d*0xFFFFFF))<<8));
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING(nmesa->clear_value);
}

/* we're don't support indexed buffers
   void (*ClearIndex)(GLcontext *ctx, GLuint index)
 */

static void nv30ClearStencil(GLcontext *ctx, GLint s)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nmesa->clear_value=((nmesa->clear_value&0xFFFFFF00)|(s&0x000000FF));
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 1);
	OUT_RING(nmesa->clear_value);
}

static void nv30ClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLIP_PLANE_A(plane), 4);
	OUT_RINGf(equation[0]);
	OUT_RINGf(equation[1]);
	OUT_RINGf(equation[2]);
	OUT_RINGf(equation[3]);
}

static void nv30ColorMask(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
		GLboolean bmask, GLboolean amask )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_COLOR_MASK, 1);
	OUT_RING(((amask && 0x01) << 24) | ((rmask && 0x01) << 16) | ((gmask && 0x01)<< 8) | ((bmask && 0x01) << 0));
}

static void nv30ColorMaterial(GLcontext *ctx, GLenum face, GLenum mode)
{
	// TODO I need love
}

static void nv30CullFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CULL_FACE, 1);
	OUT_RING(mode);
}

static void nv30FrontFace(GLcontext *ctx, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FRONT_FACE, 1);
	OUT_RING(mode);
}

static void nv30DepthFunc(GLcontext *ctx, GLenum func)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_FUNC, 1);
	OUT_RING(func);
}

static void nv30DepthMask(GLcontext *ctx, GLboolean flag)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING(flag);
}

static void nv30DepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RINGf(nearval);
	OUT_RINGf(farval);
}

/** Specify the current buffer for writing */
//void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
/** Specify the buffers for writing for fragment programs*/
//void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );

static void nv30Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	switch(cap)
	{
		case GL_ALPHA_TEST:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ALPHA_FUNC_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_AUTO_NORMAL:
		case GL_BLEND:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_BLEND_FUNC_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CLIP_PLANE_ENABLE(cap-GL_CLIP_PLANE0), 1);
			OUT_RING(state);
			break;
		case GL_COLOR_LOGIC_OP:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LOGIC_OP_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_COLOR_MATERIAL:
//		case GL_COLOR_SUM_EXT:
//		case GL_COLOR_TABLE:
//		case GL_CONVOLUTION_1D:
//		case GL_CONVOLUTION_2D:
		case GL_CULL_FACE:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_DEPTH_TEST:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DEPTH_TEST_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_DITHER:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DITHER_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_FOG:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_HISTOGRAM:
//		case GL_INDEX_LOGIC_OP:
		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			{
			uint32_t mask=0x11<<(2*(cap-GL_LIGHT0));
			nmesa->enabled_lights=((nmesa->enabled_lights&mask)|(mask*state));
			if (nmesa->lighting_enabled)
			{
				BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
				OUT_RING(nmesa->enabled_lights);
			}
			break;
			}
		case GL_LIGHTING:
			nmesa->lighting_enabled=state;
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_ENABLED_LIGHTS, 1);
			if (nmesa->lighting_enabled)
				OUT_RING(nmesa->enabled_lights);
			else
				OUT_RING(0x0);
			break;
//		case GL_LINE_SMOOTH:
//		case GL_LINE_STIPPLE:
//		case GL_MAP1_COLOR_4:
//		case GL_MAP1_INDEX:
//		case GL_MAP1_NORMAL:
//		case GL_MAP1_TEXTURE_COORD_1:
//		case GL_MAP1_TEXTURE_COORD_2:
//		case GL_MAP1_TEXTURE_COORD_3:
//		case GL_MAP1_TEXTURE_COORD_4:
//		case GL_MAP1_VERTEX_3:
//		case GL_MAP1_VERTEX_4:
//		case GL_MAP2_COLOR_4:
//		case GL_MAP2_INDEX:
//		case GL_MAP2_NORMAL:
//		case GL_MAP2_TEXTURE_COORD_1:
//		case GL_MAP2_TEXTURE_COORD_2:
//		case GL_MAP2_TEXTURE_COORD_3:
//		case GL_MAP2_TEXTURE_COORD_4:
//		case GL_MAP2_VERTEX_3:
//		case GL_MAP2_VERTEX_4:
//		case GL_MINMAX:
		case GL_NORMALIZE:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_NORMALIZE_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_POINT_SMOOTH:
		case GL_POLYGON_OFFSET_POINT:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_POINT_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_POLYGON_OFFSET_LINE:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_LINE_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_POLYGON_OFFSET_FILL:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_OFFSET_FILL_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_POLYGON_SMOOTH:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_SMOOTH_ENABLE, 1);
			OUT_RING(state);
			break;
		case GL_POLYGON_STIPPLE:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_STIPPLE_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_POST_COLOR_MATRIX_COLOR_TABLE:
//		case GL_POST_CONVOLUTION_COLOR_TABLE:
//		case GL_RESCALE_NORMAL:
//		case GL_SCISSOR_TEST:
//		case GL_SEPARABLE_2D:
		case GL_STENCIL_TEST:
			// TODO BACK and FRONT ?
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_ENABLE, 1);
			OUT_RING(state);
			break;
//		case GL_TEXTURE_GEN_Q:
//		case GL_TEXTURE_GEN_R:
//		case GL_TEXTURE_GEN_S:
//		case GL_TEXTURE_GEN_T:
//		case GL_TEXTURE_1D:
//		case GL_TEXTURE_2D:
//		case GL_TEXTURE_3D:
	}
}

static void nv30Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    switch(pname)
    {
        case GL_FOG_MODE:
            BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FOG_MODE, 1);
            //OUT_RING (params);
            break;
            /* TODO: unsure about the rest.*/
        default:
            break;
    }

}
   
static void nv30Hint(GLcontext *ctx, GLenum target, GLenum mode)
{
	// TODO I need love (fog and line_smooth hints)
}

// void (*IndexMask)(GLcontext *ctx, GLuint mask);

static void nv30Lightfv(GLcontext *ctx, GLenum light, GLenum pname, const GLfloat *params )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	/* not sure where the fourth param value goes...*/
	switch(pname)
	{
		case GL_AMBIENT:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_AMBIENT_A(light), 3);
			OUT_RINGf(params[0]);
			OUT_RINGf(params[1]);
			OUT_RINGf(params[2]);
			break;
		case GL_DIFFUSE:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_DIFFUSE_A(light), 3);
			OUT_RINGf(params[0]);
			OUT_RINGf(params[1]);
			OUT_RINGf(params[2]);
			break;
		case GL_SPECULAR:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_FRONT_SIDE_PRODUCT_SPECULAR_A(light), 3);
			OUT_RINGf(params[0]);
			OUT_RINGf(params[1]);
			OUT_RINGf(params[2]);
			break;
		case GL_SPOT_DIRECTION:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_DIR_X(light), 3);
			OUT_RINGf(params[0]);
			OUT_RINGf(params[1]);
			OUT_RINGf(params[2]);
			break;
		case GL_POSITION:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_POSITION_X(light), 3);
			OUT_RINGf(params[0]);
			OUT_RINGf(params[1]);
			OUT_RINGf(params[2]);
			break;
		case GL_SPOT_EXPONENT:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_EXPONENT(light), 1);
			OUT_RINGf(*params);
			break;
		case GL_SPOT_CUTOFF:
			/* you can't factor these */
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_A(light), 1);
			OUT_RINGf(params[0]);
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_B(light), 1);
			OUT_RINGf(params[1]);
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_SPOT_CUTOFF_C(light), 1);
			OUT_RINGf(params[2]);
			break;
		case GL_CONSTANT_ATTENUATION:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_CONSTANT_ATTENUATION(light), 1);
			OUT_RINGf(*params);
			break;
		case GL_LINEAR_ATTENUATION:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_LINEAR_ATTENUATION(light), 1);
			OUT_RINGf(*params);
			break;
		case GL_QUADRATIC_ATTENUATION:
			BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LIGHT_QUADRATIC_ATTENUATION(light), 1);
			OUT_RINGf(*params);
			break;
		default:
			break;
	}
}

/** Set the lighting model parameters */
static void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);


static void nv30LineStipple(GLcontext *ctx, GLint factor, GLushort pattern )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LINE_STIPPLE_PATTERN, 1);
	OUT_RING((pattern << 16) | factor);
}

static void nv30LineWidth(GLcontext *ctx, GLfloat width)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LINE_WIDTH_SMOOTH, 1);
	OUT_RINGf(width);
}

static void nv30LogicOpcode(GLcontext *ctx, GLenum opcode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_LOGIC_OP_OP, 1);
	OUT_RING(opcode);
}

static void nv30PointParameterfv(GLcontext *ctx, GLenum pname, const GLfloat *params)
{
	/*TODO: not sure what goes here. */
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	
}

/** Specify the diameter of rasterized points */
static void nv30PointSize(GLcontext *ctx, GLfloat size)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POINT_SIZE, 1);
	OUT_RINGf(size);
}

/** Select a polygon rasterization mode */
static void nv30PolygonMode(GLcontext *ctx, GLenum face, GLenum mode)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_MODE_FRONT, 1);
		OUT_RING(mode);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_POLYGON_MODE_BACK, 1);
		OUT_RING(mode);
	}
}

/** Set the scale and units used to calculate depth values */
void (*PolygonOffset)(GLcontext *ctx, GLfloat factor, GLfloat units);
/** Set the polygon stippling pattern */
void (*PolygonStipple)(GLcontext *ctx, const GLubyte *mask );
/* Specifies the current buffer for reading */
void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
/** Set rasterization mode */
void (*RenderMode)(GLcontext *ctx, GLenum mode );
/** Define the scissor box */
void (*Scissor)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
/** Select flat or smooth shading */
void (*ShadeModel)(GLcontext *ctx, GLenum mode);

/** OpenGL 2.0 two-sided StencilFunc */
static void nv30StencilFuncSeparate(GLcontext *ctx, GLenum face, GLenum func,
		GLint ref, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_FUNC_FUNC, 3);
		OUT_RING(func);
		OUT_RING(ref);
		OUT_RING(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_FUNC_FUNC, 3);
		OUT_RING(func);
		OUT_RING(ref);
		OUT_RING(mask);
	}
}

/** OpenGL 2.0 two-sided StencilMask */
static void nv30StencilMaskSeparate(GLcontext *ctx, GLenum face, GLuint mask)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_MASK, 1);
		OUT_RING(mask);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_MASK, 1);
		OUT_RING(mask);
	}
}

/** OpenGL 2.0 two-sided StencilOp */
static void nv30StencilOpSeparate(GLcontext *ctx, GLenum face, GLenum fail,
		GLenum zfail, GLenum zpass)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_FRONT_OP_FAIL, 1);
		OUT_RING(fail);
		OUT_RING(zfail);
		OUT_RING(zpass);
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STENCIL_BACK_OP_FAIL, 1);
		OUT_RING(fail);
		OUT_RING(zfail);
		OUT_RING(zpass);
	}
}

/** Control the generation of texture coordinates */
void (*TexGen)(GLcontext *ctx, GLenum coord, GLenum pname,
		const GLfloat *params);
/** Set texture environment parameters */
void (*TexEnv)(GLcontext *ctx, GLenum target, GLenum pname,
		const GLfloat *param);
/** Set texture parameters */
void (*TexParameter)(GLcontext *ctx, GLenum target,
		struct gl_texture_object *texObj,
		GLenum pname, const GLfloat *params);
void (*TextureMatrix)(GLcontext *ctx, GLuint unit, const GLmatrix *mat);

/** Set the viewport */
static void nv30Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    /* TODO: Where do the VIEWPORT_XFRM_* regs come in? */
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VIEWPORT_DIMS_0, 2);
    OUT_RING((w << 16) | x);
    OUT_RING((h << 16) | y);
}

void nv30InitStateFuncs(struct dd_function_table *func)
{
	func->AlphaFunc			= nv30AlphaFunc;
	func->BlendColor		= nv30BlendColor;
	func->BlendEquationSeparate	= nv30BlendEquationSeparate;
	func->BlendFuncSeparate		= nv30BlendFuncSeparate;
	func->ClearColor		= nv30ClearColor;
	func->ClearDepth		= nv30ClearDepth;
	func->ClearStencil		= nv30ClearStencil;
	func->ClipPlane			= nv30ClipPlane;
	func->ColorMask			= nv30ColorMask;
	func->ColorMaterial		= nv30ColorMaterial;
	func->CullFace			= nv30CullFace;
	func->FrontFace			= nv30FrontFace;
	func->DepthFunc			= nv30DepthFunc;
	func->DepthMask			= nv30DepthMask;
	func->DepthRange		= nv30DepthRange;
	func->Enable			= nv30Enable;
	func->Fogfv			= nv30Fogfv;
	func->Hint			= nv30Hint;
	func->Lightfv			= nv30Lightfv;
/*	func->LightModelfv		= nv30LightModelfv; */
	func->LineStipple		= nv30LineStipple;
	func->LineWidth			= nv30LineWidth;
	func->LogicOpcode		= nv30LogicOpcode;
	func->PointParameterfv		= nv30PointParameterfv;
	func->PointSize			= nv30PointSize;
	func->PolygonMode		= nv30PolygonMode;
#if 0
	func->PolygonOffset		= nv30PolygonOffset;
	func->PolygonStipple		= nv30PolygonStipple;
	func->ReadBuffer		= nv30ReadBuffer;
	func->RenderMode		= nv30RenderMode;
	func->Scissor			= nv30Scissor;
	func->ShadeModel		= nv30ShaderModel;
#endif
	func->StencilFuncSeparate	= nv30StencilFuncSeparate;
	func->StencilMaskSeparate	= nv30StencilMaskSeparate;
	func->StencilOpSeparate		= nv30StencilOpSeparate;
#if 0
	func->TexGen			= nv30TexGen;
	func->TexParameter		= nv30TexParameter;
	func->TextureMatrix		= nv30TextureMatrix;
#endif
	func->Viewport			= nv30Viewport;
}

