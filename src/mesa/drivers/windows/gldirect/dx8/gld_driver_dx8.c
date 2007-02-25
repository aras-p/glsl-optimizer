/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  Driver interface code to Mesa
*
****************************************************************************/

//#include <windows.h>
#include "dglcontext.h"
#include "ddlog.h"
#include "gld_dx8.h"

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
// #include "mem.h"
//#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "vbo/vbo.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast_setup/ss_context.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

extern BOOL dglSwapBuffers(HDC hDC);

// HACK: Hack the _33 member of the OpenGL perspective projection matrix
const float _fPersp_33 = 1.6f;

//---------------------------------------------------------------------------
// Internal functions
//---------------------------------------------------------------------------

void _gld_mesa_warning(
	__GLcontext *gc,
	char *str)
{
	// Intercept Mesa's internal warning mechanism
	gldLogPrintf(GLDLOG_WARN, "Mesa warning: %s", str);
}

//---------------------------------------------------------------------------

void _gld_mesa_fatal(
	__GLcontext *gc,
	char *str)
{
	// Intercept Mesa's internal fatal-message mechanism
	gldLogPrintf(GLDLOG_CRITICAL, "Mesa FATAL: %s", str);

	// Mesa calls abort(0) here.
	ddlogClose();
	exit(0);
}

//---------------------------------------------------------------------------

D3DSTENCILOP _gldConvertStencilOp(
	GLenum StencilOp)
{
	// Used by Stencil: pass, fail and zfail

	switch (StencilOp) {
	case GL_KEEP:
		return D3DSTENCILOP_KEEP;
	case GL_ZERO:
		return D3DSTENCILOP_ZERO;
	case GL_REPLACE:
	    return D3DSTENCILOP_REPLACE;
	case GL_INCR:
		return D3DSTENCILOP_INCRSAT;
	case GL_DECR:
	    return D3DSTENCILOP_DECRSAT;
	case GL_INVERT:
		return D3DSTENCILOP_INVERT;
	case GL_INCR_WRAP_EXT:	// GL_EXT_stencil_wrap
		return D3DSTENCILOP_INCR;
	case GL_DECR_WRAP_EXT:	// GL_EXT_stencil_wrap
	    return D3DSTENCILOP_DECR;
	}

#ifdef _DEBUG
	gldLogMessage(GLDLOG_ERROR, "_gldConvertStencilOp: Unknown StencilOp\n");
#endif

	return D3DSTENCILOP_KEEP;
}

//---------------------------------------------------------------------------

D3DCMPFUNC _gldConvertCompareFunc(
	GLenum CmpFunc)
{
	// Used for Alpha func, depth func and stencil func.

	switch (CmpFunc) {
	case GL_NEVER:
		return D3DCMP_NEVER;
	case GL_LESS:
		return D3DCMP_LESS;
	case GL_EQUAL:
		return D3DCMP_EQUAL;
	case GL_LEQUAL:
		return D3DCMP_LESSEQUAL;
	case GL_GREATER:
		return D3DCMP_GREATER;
	case GL_NOTEQUAL:
		return D3DCMP_NOTEQUAL;
	case GL_GEQUAL:
		return D3DCMP_GREATEREQUAL;
	case GL_ALWAYS:
		return D3DCMP_ALWAYS;
	};

#ifdef _DEBUG
	gldLogMessage(GLDLOG_ERROR, "_gldConvertCompareFunc: Unknown CompareFunc\n");
#endif

	return D3DCMP_ALWAYS;
}

//---------------------------------------------------------------------------

D3DBLEND _gldConvertBlendFunc(
	GLenum blend,
	GLenum DefaultBlend)
{
	switch (blend) {
	case GL_ZERO:
		return D3DBLEND_ZERO;
	case GL_ONE:
		return D3DBLEND_ONE;
	case GL_DST_COLOR:
		return D3DBLEND_DESTCOLOR;
	case GL_SRC_COLOR:
		return D3DBLEND_SRCCOLOR;
	case GL_ONE_MINUS_DST_COLOR:
		return D3DBLEND_INVDESTCOLOR;
	case GL_ONE_MINUS_SRC_COLOR:
		return D3DBLEND_INVSRCCOLOR;
	case GL_SRC_ALPHA:
		return D3DBLEND_SRCALPHA;
	case GL_ONE_MINUS_SRC_ALPHA:
		return D3DBLEND_INVSRCALPHA;
	case GL_DST_ALPHA:
		return D3DBLEND_DESTALPHA;
	case GL_ONE_MINUS_DST_ALPHA:
		return D3DBLEND_INVDESTALPHA;
	case GL_SRC_ALPHA_SATURATE:
		return D3DBLEND_SRCALPHASAT;
	}

#ifdef _DEBUG
	gldLogMessage(GLDLOG_ERROR, "_gldConvertBlendFunc: Unknown BlendFunc\n");
#endif

	return DefaultBlend;
}

//---------------------------------------------------------------------------
// Misc. functions
//---------------------------------------------------------------------------

void gld_Noop_DX8(
	GLcontext *ctx)
{
#ifdef _DEBUG
	gldLogMessage(GLDLOG_ERROR, "gld_Noop called!\n");
#endif
}

//---------------------------------------------------------------------------

void gld_Error_DX8(
	GLcontext *ctx)
{
#ifdef _DEBUG
	// Quite useless.
//	gldLogMessage(GLDLOG_ERROR, "ctx->Driver.Error called!\n");
#endif
}

//---------------------------------------------------------------------------
// Required Mesa functions
//---------------------------------------------------------------------------

static GLboolean gld_set_draw_buffer_DX8(
	GLcontext *ctx,
	GLenum mode)
{
   (void) ctx;
   if ((mode==GL_FRONT_LEFT) || (mode == GL_BACK_LEFT)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

//---------------------------------------------------------------------------

static void gld_set_read_buffer_DX8(
	GLcontext *ctx,
	GLframebuffer *buffer,
	GLenum mode)
{
   /* separate read buffer not supported */
/*
   ASSERT(buffer == ctx->DrawBuffer);
   ASSERT(mode == GL_FRONT_LEFT);
*/
}

//---------------------------------------------------------------------------

void gld_Clear_DX8(
	GLcontext *ctx,
	GLbitfield mask,
	GLboolean all,
	GLint x,
	GLint y,
	GLint width,
	GLint height)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	DWORD		dwFlags = 0;
	D3DCOLOR	Color = 0;
	float		Z = 0.0f;
	DWORD		Stencil = 0;
	D3DRECT		d3dClearRect;

	// TODO: Colourmask
	const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;

	if (!gld->pDev)
		return;

	if (mask & (DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT)) {
		GLubyte col[4];
		CLAMPED_FLOAT_TO_UBYTE(col[0], ctx->Color.ClearColor[0]);
		CLAMPED_FLOAT_TO_UBYTE(col[1], ctx->Color.ClearColor[1]);
		CLAMPED_FLOAT_TO_UBYTE(col[2], ctx->Color.ClearColor[2]);
		CLAMPED_FLOAT_TO_UBYTE(col[3], ctx->Color.ClearColor[3]);
		dwFlags |= D3DCLEAR_TARGET;
		Color = D3DCOLOR_RGBA(col[0], col[1], col[2], col[3]);
//								ctx->Color.ClearColor[1], 
//								ctx->Color.ClearColor[2], 
//								ctx->Color.ClearColor[3]);
	}

	if (mask & DD_DEPTH_BIT) {
		// D3D8 will fail the Clear call if we try and clear a
		// depth buffer and we haven't created one.
		// Also, some apps try and clear a depth buffer,
		// when a depth buffer hasn't been requested by the app.
		if (ctx->Visual.depthBits == 0) {
			mask &= ~DD_DEPTH_BIT; // Remove depth bit from mask
		} else {
			dwFlags |= D3DCLEAR_ZBUFFER;
			Z = ctx->Depth.Clear;
		}
	}

	if (mask & DD_STENCIL_BIT) {
		if (ctx->Visual.stencilBits == 0) {
			// No stencil bits in depth buffer
			mask &= ~DD_STENCIL_BIT; // Remove stencil bit from mask
		} else {
			dwFlags |= D3DCLEAR_STENCIL;
			Stencil = ctx->Stencil.Clear;
		}
	}

	// Some apps do really weird things with the rect, such as Quake3.
	if ((x < 0) || (y < 0) || (width <= 0) || (height <= 0)) {
		all = GL_TRUE;
	}

	if (!all) {
		// Calculate clear subrect
		d3dClearRect.x1	= x;
		d3dClearRect.y1	= gldCtx->dwHeight - (y + height);
		d3dClearRect.x2	= x + width;
		d3dClearRect.y2	= d3dClearRect.y1 + height;
	}

	// dwFlags will be zero if there's nothing to clear
	if (dwFlags) {
		_GLD_DX8_DEV(Clear(
			gld->pDev,
			all ? 0 : 1,
			all ? NULL : &d3dClearRect,
			dwFlags,
			Color, Z, Stencil));
	}

	if (mask & DD_ACCUM_BIT) {
		// Clear accumulation buffer
	}
}

//---------------------------------------------------------------------------

// Mesa 5: Parameter change
static void gld_buffer_size_DX8(
//	GLcontext *ctx,
	GLframebuffer *fb,
	GLuint *width,
	GLuint *height)
{
//	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);

	*width = fb->Width; // gldCtx->dwWidth;
	*height = fb->Height; // gldCtx->dwHeight;
}

//---------------------------------------------------------------------------

static void gld_Finish_DX8(
	GLcontext *ctx)
{
}

//---------------------------------------------------------------------------

static void gld_Flush_DX8(
	GLcontext *ctx)
{
	GLD_context		*gld	= GLD_GET_CONTEXT(ctx);

	// TODO: Detect apps that glFlush() then SwapBuffers() ?

	if (gld->EmulateSingle) {
		// Emulating a single-buffered context.
		// [Direct3D doesn't allow rendering to front buffer]
		dglSwapBuffers(gld->hDC);
	}
}

//---------------------------------------------------------------------------

void gld_NEW_STENCIL(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	// Two-sided stencil. New for Mesa 5
	const GLuint		uiFace	= 0UL;

	struct gl_stencil_attrib *pStencil = &ctx->Stencil;

	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILENABLE, pStencil->Enabled ? TRUE : FALSE));
	if (pStencil->Enabled) {
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILFUNC, _gldConvertCompareFunc(pStencil->Function[uiFace])));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILREF, pStencil->Ref[uiFace]));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILMASK, pStencil->ValueMask[uiFace]));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILWRITEMASK, pStencil->WriteMask[uiFace]));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILFAIL, _gldConvertStencilOp(pStencil->FailFunc[uiFace])));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILZFAIL, _gldConvertStencilOp(pStencil->ZFailFunc[uiFace])));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_STENCILPASS, _gldConvertStencilOp(pStencil->ZPassFunc[uiFace])));
	}
}

//---------------------------------------------------------------------------

void gld_NEW_COLOR(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	DWORD		dwFlags = 0;
	D3DBLEND	src;
	D3DBLEND	dest;

	// Alpha func
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ALPHAFUNC, _gldConvertCompareFunc(ctx->Color.AlphaFunc)));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ALPHAREF, (DWORD)ctx->Color.AlphaRef));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ALPHATESTENABLE, ctx->Color.AlphaEnabled));

	// Blend func
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ALPHABLENDENABLE, ctx->Color.BlendEnabled));
	src		= _gldConvertBlendFunc(ctx->Color.BlendSrcRGB, GL_ONE);
	dest	= _gldConvertBlendFunc(ctx->Color.BlendDstRGB, GL_ZERO);
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SRCBLEND, src));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_DESTBLEND, dest));

	// Color mask
	if (ctx->Color.ColorMask[0]) dwFlags |= D3DCOLORWRITEENABLE_RED;
	if (ctx->Color.ColorMask[1]) dwFlags |= D3DCOLORWRITEENABLE_GREEN;
	if (ctx->Color.ColorMask[2]) dwFlags |= D3DCOLORWRITEENABLE_BLUE;
	if (ctx->Color.ColorMask[3]) dwFlags |= D3DCOLORWRITEENABLE_ALPHA;
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_COLORWRITEENABLE, dwFlags));
}

//---------------------------------------------------------------------------

void gld_NEW_DEPTH(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ZENABLE, ctx->Depth.Test ? D3DZB_TRUE : D3DZB_FALSE));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ZFUNC, _gldConvertCompareFunc(ctx->Depth.Func)));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ZWRITEENABLE, ctx->Depth.Mask ? TRUE : FALSE));
}

//---------------------------------------------------------------------------

void gld_NEW_POLYGON(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DFILLMODE	d3dFillMode = D3DFILL_SOLID;
	D3DCULL		d3dCullMode = D3DCULL_NONE;
	int			iOffset = 0;

	// Fillmode
	switch (ctx->Polygon.FrontMode) {
	case GL_POINT:
		d3dFillMode = D3DFILL_POINT;
		break;
	case GL_LINE:
		d3dFillMode = D3DFILL_WIREFRAME;
		break;
	case GL_FILL:
		d3dFillMode = D3DFILL_SOLID;
		break;
	}
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FILLMODE, d3dFillMode));

	if (ctx->Polygon.CullFlag) {
		switch (ctx->Polygon.CullFaceMode) {
		case GL_BACK:
			if (ctx->Polygon.FrontFace == GL_CCW)
				d3dCullMode = D3DCULL_CW;
			else
				d3dCullMode = D3DCULL_CCW;
			break;
		case GL_FRONT:
			if (ctx->Polygon.FrontFace == GL_CCW)
				d3dCullMode = D3DCULL_CCW;
			else
				d3dCullMode = D3DCULL_CW;
			break;
		case GL_FRONT_AND_BACK:
			d3dCullMode = D3DCULL_NONE;
			break;
		default:
			break;
		}
	} else {
		d3dCullMode = D3DCULL_NONE;
	}
//	d3dCullMode = D3DCULL_NONE; // TODO: DEBUGGING
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_CULLMODE, d3dCullMode));

	// Polygon offset
	// ZBIAS ranges from 0 to 16 and can only move towards the viewer
	// Mesa5: ctx->Polygon._OffsetAny removed
	if (ctx->Polygon.OffsetFill) {
		iOffset = (int)ctx->Polygon.OffsetUnits;
		if (iOffset < 0)
			iOffset = -iOffset;
		else
			iOffset = 0; // D3D can't push away
	}
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_ZBIAS, iOffset));
}

//---------------------------------------------------------------------------

void gld_NEW_FOG(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DCOLOR	d3dFogColour;
	D3DFOGMODE	d3dFogMode = D3DFOG_LINEAR;

	// TODO: Fog is calculated seperately in the Mesa pipeline
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGENABLE, FALSE));
	return;

	// Fog enable
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGENABLE, ctx->Fog.Enabled));
	if (!ctx->Fog.Enabled) {
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGTABLEMODE, D3DFOG_NONE));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGVERTEXMODE, D3DFOG_NONE));
		return; // If disabled, don't bother setting any fog state
	}

	// Fog colour
	d3dFogColour = D3DCOLOR_COLORVALUE(	ctx->Fog.Color[0],
								ctx->Fog.Color[1],
								ctx->Fog.Color[2],
								ctx->Fog.Color[3]);
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGCOLOR, d3dFogColour));

	// Fog density
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGDENSITY, *((DWORD*) (&ctx->Fog.Density))));

	// Fog start
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGSTART, *((DWORD*) (&ctx->Fog.Start))));

	// Fog end
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGEND, *((DWORD*) (&ctx->Fog.End))));

	// Fog mode
	switch (ctx->Fog.Mode) {
	case GL_LINEAR:
		d3dFogMode = D3DFOG_LINEAR;
		break;
	case GL_EXP:
		d3dFogMode = D3DFOG_EXP;
		break;
	case GL_EXP2:
		d3dFogMode = D3DFOG_EXP2;
		break;
	}
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGTABLEMODE, d3dFogMode));
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_FOGVERTEXMODE, D3DFOG_NONE));
}

//---------------------------------------------------------------------------

void gld_NEW_LIGHT(
	GLcontext *ctx)
{
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);
	DWORD			dwSpecularEnable;

	// Shademode
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SHADEMODE, (ctx->Light.ShadeModel == GL_SMOOTH) ? D3DSHADE_GOURAUD : D3DSHADE_FLAT));

	// Separate specular colour
	if (ctx->Light.Enabled)
		dwSpecularEnable = (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) ? TRUE: FALSE;
	else
		dwSpecularEnable = FALSE;
	_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SPECULARENABLE, dwSpecularEnable));
}

//---------------------------------------------------------------------------

void gld_NEW_MODELVIEW(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DMATRIX	m;
	//GLfloat		*pM = ctx->ModelView.m;
	// Mesa5: Model-view is now a stack
	GLfloat		*pM = ctx->ModelviewMatrixStack.Top->m;
	m._11 = pM[0];
	m._12 = pM[1];
	m._13 = pM[2];
	m._14 = pM[3];
	m._21 = pM[4];
	m._22 = pM[5];
	m._23 = pM[6];
	m._24 = pM[7];
	m._31 = pM[8];
	m._32 = pM[9];
	m._33 = pM[10];
	m._34 = pM[11];
	m._41 = pM[12];
	m._42 = pM[13];
	m._43 = pM[14];
	m._44 = pM[15];

	gld->matModelView = m;
}

//---------------------------------------------------------------------------

void gld_NEW_PROJECTION(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DMATRIX	m;
	//GLfloat		*pM = ctx->ProjectionMatrix.m;
	// Mesa 5: Now a stack
	GLfloat		*pM = ctx->ProjectionMatrixStack.Top->m;
	m._11 = pM[0];
	m._12 = pM[1];
	m._13 = pM[2];
	m._14 = pM[3];

	m._21 = pM[4];
	m._22 = pM[5];
	m._23 = pM[6];
	m._24 = pM[7];

	m._31 = pM[8];
	m._32 = pM[9];
	m._33 = pM[10] / _fPersp_33; // / 1.6f;
	m._34 = pM[11];

	m._41 = pM[12];
	m._42 = pM[13];
	m._43 = pM[14] / 2.0f;
	m._44 = pM[15];

	gld->matProjection = m;
}

//---------------------------------------------------------------------------
/*
void gldFrustumHook_DX8(
	GLdouble left,
	GLdouble right,
	GLdouble bottom,
	GLdouble top,
	GLdouble nearval,
	GLdouble farval)
{
	GET_CURRENT_CONTEXT(ctx);
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	// Pass values on to Mesa first (in case we mess with them)
	_mesa_Frustum(left, right, bottom, top, nearval, farval);

	_fPersp_33 = farval / (nearval - farval);

//	ddlogPrintf(GLDLOG_SYSTEM, "Frustum: %f", farval/nearval);
}

//---------------------------------------------------------------------------

void gldOrthoHook_DX8(
	GLdouble left,
	GLdouble right,
	GLdouble bottom,
	GLdouble top,
	GLdouble nearval,
	GLdouble farval)
{
	GET_CURRENT_CONTEXT(ctx);
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	// Pass values on to Mesa first (in case we mess with them)
	_mesa_Ortho(left, right, bottom, top, nearval, farval);

	_fPersp_33 = 1.6f;

//	ddlogPrintf(GLDLOG_SYSTEM, "Ortho: %f", farval/nearval);
}
*/
//---------------------------------------------------------------------------

void gld_NEW_VIEWPORT(
	GLcontext *ctx)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8		*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DVIEWPORT8	d3dvp;
//	GLint			x, y;
//	GLsizei			w, h;

	// Set depth range
	_GLD_DX8_DEV(GetViewport(gld->pDev, &d3dvp));
	// D3D can't do Quake1/Quake2 z-trick
	if (ctx->Viewport.Near <= ctx->Viewport.Far) {
		d3dvp.MinZ		= ctx->Viewport.Near;
		d3dvp.MaxZ		= ctx->Viewport.Far;
	} else {
		d3dvp.MinZ		= ctx->Viewport.Far;
		d3dvp.MaxZ		= ctx->Viewport.Near;
	}
/*	x = ctx->Viewport.X;
	y = ctx->Viewport.Y;
	w = ctx->Viewport.Width;
	h = ctx->Viewport.Height;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (w > gldCtx->dwWidth) 		w = gldCtx->dwWidth;
	if (h > gldCtx->dwHeight) 		h = gldCtx->dwHeight;
	// Ditto for D3D viewport dimensions
	if (w+x > gldCtx->dwWidth) 		w = gldCtx->dwWidth-x;
	if (h+y > gldCtx->dwHeight) 	h = gldCtx->dwHeight-y;
	d3dvp.X			= x;
	d3dvp.Y			= gldCtx->dwHeight - (y + h);
	d3dvp.Width		= w;
	d3dvp.Height	= h;*/
	_GLD_DX8_DEV(SetViewport(gld->pDev, &d3dvp));

//	gld->fFlipWindowY = (float)gldCtx->dwHeight;
}

//---------------------------------------------------------------------------

__inline BOOL _gldAnyEvalEnabled(
	GLcontext *ctx)
{
	struct gl_eval_attrib *eval = &ctx->Eval;

	if ((eval->AutoNormal) ||
		(eval->Map1Color4) ||
		(eval->Map1Index) ||
		(eval->Map1Normal) ||
		(eval->Map1TextureCoord1) ||
		(eval->Map1TextureCoord2) ||
		(eval->Map1TextureCoord3) ||
		(eval->Map1TextureCoord4) ||
		(eval->Map1Vertex3) ||
		(eval->Map1Vertex4) ||
		(eval->Map2Color4) ||
		(eval->Map2Index) ||
		(eval->Map2Normal) ||
		(eval->Map2TextureCoord1) ||
		(eval->Map2TextureCoord2) ||
		(eval->Map2TextureCoord3) ||
		(eval->Map2TextureCoord4) ||
		(eval->Map2Vertex3) ||
		(eval->Map2Vertex4)
		)
	return TRUE;

	return FALSE;
}

//---------------------------------------------------------------------------

BOOL _gldChooseInternalPipeline(
	GLcontext *ctx,
	GLD_driver_dx8 *gld)
{
//	return TRUE;	// DEBUGGING: ALWAYS USE MESA
//	return FALSE;	// DEBUGGING: ALWAYS USE D3D

	if ((glb.dwTnL == GLDS_TNL_MESA) || (gld->bHasHWTnL == FALSE))
	{
		gld->PipelineUsage.qwMesa.QuadPart++;
		return TRUE; // Force Mesa TnL
	}

	if ((ctx->Light.Enabled) ||
		(1) ||
		(ctx->Texture._TexGenEnabled) ||
		(ctx->Texture._TexMatEnabled) ||
//		(ctx->Transform._AnyClip) ||
		(ctx->Scissor.Enabled) ||
		_gldAnyEvalEnabled(ctx) // Put this last so we can early-out
		)
	{
		gld->PipelineUsage.qwMesa.QuadPart++;
		return TRUE;
	}

	gld->PipelineUsage.qwD3DFVF.QuadPart++;
	return FALSE;

/*	// Force Mesa pipeline?
	if (glb.dwTnL == GLDS_TNL_MESA) {
		gld->PipelineUsage.dwMesa.QuadPart++;
		return GLD_PIPELINE_MESA;
	}

	// Test for functionality not exposed in the D3D pathways
	if ((ctx->Texture._GenFlags)) {
		gld->PipelineUsage.dwMesa.QuadPart++;
		return GLD_PIPELINE_MESA;
	}

	// Now decide if vertex shader can be used.
	// If two sided lighting is enabled then we must either
	// use Mesa TnL or the vertex shader
	if (ctx->_TriangleCaps & DD_TRI_LIGHT_TWOSIDE) {
		if (gld->VStwosidelight.hShader && !ctx->Fog.Enabled) {
			// Use Vertex Shader
			gld->PipelineUsage.dwD3D2SVS.QuadPart++;
			return GLD_PIPELINE_D3D_VS_TWOSIDE;
		} else {
			// Use Mesa TnL
			gld->PipelineUsage.dwMesa.QuadPart++;
			return GLD_PIPELINE_MESA;
		}
	}

	// Must be D3D fixed-function pipeline
	gld->PipelineUsage.dwD3DFVF.QuadPart++;
	return GLD_PIPELINE_D3D_FVF;
*/
}

//---------------------------------------------------------------------------

void gld_update_state_DX8(
	GLcontext *ctx,
	GLuint new_state)
{
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);
	TNLcontext		*tnl = TNL_CONTEXT(ctx);
	GLD_pb_dx8		*gldPB;

	if (!gld || !gld->pDev)
		return;

	_swsetup_InvalidateState( ctx, new_state );
	_vbo_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );

	// SetupIndex will be used in the pipelines for choosing setup function
	if ((ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE | DD_SEPARATE_SPECULAR)) ||
		(ctx->Fog.Enabled))
	{
		if (ctx->_TriangleCaps & DD_FLATSHADE)
			gld->iSetupFunc = GLD_SI_FLAT_EXTRAS;
		else
			gld->iSetupFunc = GLD_SI_SMOOTH_EXTRAS;
	} else {
		if (ctx->_TriangleCaps & DD_FLATSHADE)
			gld->iSetupFunc = GLD_SI_FLAT;	// Setup flat shade + texture
		else
			gld->iSetupFunc = GLD_SI_SMOOTH; // Setup smooth shade + texture
	}

	gld->bUseMesaTnL = _gldChooseInternalPipeline(ctx, gld);
	if (gld->bUseMesaTnL) {
		gldPB = &gld->PB2d;
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SOFTWAREVERTEXPROCESSING, TRUE));
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_CLIPPING, FALSE));
		_GLD_DX8_DEV(SetVertexShader(gld->pDev, gldPB->dwFVF));
	} else {
		gldPB = &gld->PB3d;
		_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_CLIPPING, TRUE));
//		if (gld->TnLPipeline == GLD_PIPELINE_D3D_VS_TWOSIDE) {
//			_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SOFTWAREVERTEXPROCESSING, !gld->VStwosidelight.bHardware));
//			_GLD_DX8_DEV(SetVertexShader(gld->pDev, gld->VStwosidelight.hShader));
//		} else {
			_GLD_DX8_DEV(SetRenderState(gld->pDev, D3DRS_SOFTWAREVERTEXPROCESSING, !gld->bHasHWTnL));
			_GLD_DX8_DEV(SetVertexShader(gld->pDev, gldPB->dwFVF));
//		}
	}

#define _GLD_TEST_STATE(a)		\
	if (new_state & (a)) {		\
		gld##a(ctx);			\
		new_state &= ~(a);		\
	}

#define _GLD_TEST_STATE_DX8(a)	\
	if (new_state & (a)) {		\
		gld##a##_DX8(ctx);		\
		new_state &= ~(a);		\
	}

#define _GLD_IGNORE_STATE(a) new_state &= ~(a);

//	if (!gld->bUseMesaTnL) {
		// Not required if Mesa is doing the TnL.
	// Problem: If gld->bUseMesaTnL is TRUE when these are signaled,
	// then we'll miss updating the D3D TnL pipeline.
	// Therefore, don't test for gld->bUseMesaTnL
	_GLD_TEST_STATE(_NEW_MODELVIEW);
	_GLD_TEST_STATE(_NEW_PROJECTION);
//	}

	_GLD_TEST_STATE_DX8(_NEW_TEXTURE); // extern, so guard with _DX8
	_GLD_TEST_STATE(_NEW_COLOR);
	_GLD_TEST_STATE(_NEW_DEPTH);
	_GLD_TEST_STATE(_NEW_POLYGON);
	_GLD_TEST_STATE(_NEW_STENCIL);
	_GLD_TEST_STATE(_NEW_FOG);
	_GLD_TEST_STATE(_NEW_LIGHT);
	_GLD_TEST_STATE(_NEW_VIEWPORT);

	_GLD_IGNORE_STATE(_NEW_TRANSFORM);


// Stubs for future use.
/*	_GLD_TEST_STATE(_NEW_TEXTURE_MATRIX);
	_GLD_TEST_STATE(_NEW_COLOR_MATRIX);
	_GLD_TEST_STATE(_NEW_ACCUM);
	_GLD_TEST_STATE(_NEW_EVAL);
	_GLD_TEST_STATE(_NEW_HINT);
	_GLD_TEST_STATE(_NEW_LINE);
	_GLD_TEST_STATE(_NEW_PIXEL);
	_GLD_TEST_STATE(_NEW_POINT);
	_GLD_TEST_STATE(_NEW_POLYGONSTIPPLE);
	_GLD_TEST_STATE(_NEW_SCISSOR);
	_GLD_TEST_STATE(_NEW_PACKUNPACK);
	_GLD_TEST_STATE(_NEW_ARRAY);
	_GLD_TEST_STATE(_NEW_RENDERMODE);
	_GLD_TEST_STATE(_NEW_BUFFERS);
	_GLD_TEST_STATE(_NEW_MULTISAMPLE);
*/

// For debugging.
#if 0
#define _GLD_TEST_UNHANDLED_STATE(a)									\
	if (new_state & (a)) {									\
		gldLogMessage(GLDLOG_ERROR, "Unhandled " #a "\n");	\
	}
	_GLD_TEST_UNHANDLED_STATE(_NEW_TEXTURE_MATRIX);
	_GLD_TEST_UNHANDLED_STATE(_NEW_COLOR_MATRIX);
	_GLD_TEST_UNHANDLED_STATE(_NEW_ACCUM);
	_GLD_TEST_UNHANDLED_STATE(_NEW_EVAL);
	_GLD_TEST_UNHANDLED_STATE(_NEW_HINT);
	_GLD_TEST_UNHANDLED_STATE(_NEW_LINE);
	_GLD_TEST_UNHANDLED_STATE(_NEW_PIXEL);
	_GLD_TEST_UNHANDLED_STATE(_NEW_POINT);
	_GLD_TEST_UNHANDLED_STATE(_NEW_POLYGONSTIPPLE);
	_GLD_TEST_UNHANDLED_STATE(_NEW_SCISSOR);
	_GLD_TEST_UNHANDLED_STATE(_NEW_PACKUNPACK);
	_GLD_TEST_UNHANDLED_STATE(_NEW_ARRAY);
	_GLD_TEST_UNHANDLED_STATE(_NEW_RENDERMODE);
	_GLD_TEST_UNHANDLED_STATE(_NEW_BUFFERS);
	_GLD_TEST_UNHANDLED_STATE(_NEW_MULTISAMPLE);
#undef _GLD_UNHANDLED_STATE
#endif

#undef _GLD_TEST_STATE
}

//---------------------------------------------------------------------------
// Viewport
//---------------------------------------------------------------------------

void gld_Viewport_DX8(
	GLcontext *ctx,
	GLint x,
	GLint y,
	GLsizei w,
	GLsizei h)
{
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	D3DVIEWPORT8	d3dvp;

	if (!gld || !gld->pDev)
		return;

	// This is a hack. When the app is minimized, Mesa passes
	// w=1 and h=1 for viewport dimensions. Without this test
	// we get a GPF in gld_wgl_resize_buffers().
	if ((w==1) && (h==1))
		return;

	// Call ResizeBuffersMESA. This function will early-out
	// if no resize is needed.
	//ctx->Driver.ResizeBuffersMESA(ctx);
	// Mesa 5: Changed parameters
	ctx->Driver.ResizeBuffers(gldCtx->glBuffer);

#if 0
	ddlogPrintf(GLDLOG_SYSTEM, ">> Viewport x=%d y=%d w=%d h=%d", x,y,w,h);
#endif

	// ** D3D viewport must not be outside the render target surface **
	// Sanity check the GL viewport dimensions
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (w > gldCtx->dwWidth) 		w = gldCtx->dwWidth;
	if (h > gldCtx->dwHeight) 		h = gldCtx->dwHeight;
	// Ditto for D3D viewport dimensions
	if (w+x > gldCtx->dwWidth) 		w = gldCtx->dwWidth-x;
	if (h+y > gldCtx->dwHeight) 	h = gldCtx->dwHeight-y;

	d3dvp.X			= x;
	d3dvp.Y			= gldCtx->dwHeight - (y + h);
	d3dvp.Width		= w;
	d3dvp.Height	= h;
	if (ctx->Viewport.Near <= ctx->Viewport.Far) {
		d3dvp.MinZ		= ctx->Viewport.Near;
		d3dvp.MaxZ		= ctx->Viewport.Far;
	} else {
		d3dvp.MinZ		= ctx->Viewport.Far;
		d3dvp.MaxZ		= ctx->Viewport.Near;
	}

	// TODO: DEBUGGING
//	d3dvp.MinZ		= 0.0f;
//	d3dvp.MaxZ		= 1.0f;

	_GLD_DX8_DEV(SetViewport(gld->pDev, &d3dvp));

}

//---------------------------------------------------------------------------

extern BOOL dglWglResizeBuffers(GLcontext *ctx, BOOL bDefaultDriver);

// Mesa 5: Parameter change
void gldResizeBuffers_DX8(
//	GLcontext *ctx)
	GLframebuffer *fb)
{
	GET_CURRENT_CONTEXT(ctx);
	dglWglResizeBuffers(ctx, TRUE);
}

//---------------------------------------------------------------------------
#ifdef _DEBUG
// This is only for debugging.
// To use, plug into ctx->Driver.Enable pointer below.
void gld_Enable(
	GLcontext *ctx,
	GLenum e,
	GLboolean b)
{
	char buf[1024];
	sprintf(buf, "Enable: %s (%s)\n", _mesa_lookup_enum_by_nr(e), b?"TRUE":"FALSE");
	ddlogMessage(DDLOG_SYSTEM, buf);
}
#endif
//---------------------------------------------------------------------------
// Driver pointer setup
//---------------------------------------------------------------------------

extern const GLubyte* _gldGetStringGeneric(GLcontext*, GLenum);

void gldSetupDriverPointers_DX8(
	GLcontext *ctx)
{
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx8	*gld	= GLD_GET_DX8_DRIVER(gldCtx);

	TNLcontext *tnl = TNL_CONTEXT(ctx);

	// Mandatory functions
	ctx->Driver.GetString				= _gldGetStringGeneric;
	ctx->Driver.UpdateState				= gld_update_state_DX8;
	ctx->Driver.Clear					= gld_Clear_DX8;
	ctx->Driver.DrawBuffer				= gld_set_draw_buffer_DX8;
	ctx->Driver.GetBufferSize			= gld_buffer_size_DX8;
	ctx->Driver.Finish					= gld_Finish_DX8;
	ctx->Driver.Flush					= gld_Flush_DX8;
	ctx->Driver.Error					= gld_Error_DX8;

	// Hardware accumulation buffer
	ctx->Driver.Accum					= NULL; // TODO: gld_Accum;

	// Bitmap functions
	ctx->Driver.CopyPixels				= gld_CopyPixels_DX8;
	ctx->Driver.DrawPixels				= gld_DrawPixels_DX8;
	ctx->Driver.ReadPixels				= gld_ReadPixels_DX8;
	ctx->Driver.Bitmap					= gld_Bitmap_DX8;

	// Buffer resize
	ctx->Driver.ResizeBuffers			= gldResizeBuffers_DX8;
	
	// Texture image functions
	ctx->Driver.ChooseTextureFormat		= gld_ChooseTextureFormat_DX8;
	ctx->Driver.TexImage1D				= gld_TexImage1D_DX8;
	ctx->Driver.TexImage2D				= gld_TexImage2D_DX8;
	ctx->Driver.TexImage3D				= _mesa_store_teximage3d;
	ctx->Driver.TexSubImage1D			= gld_TexSubImage1D_DX8;
	ctx->Driver.TexSubImage2D			= gld_TexSubImage2D_DX8;
	ctx->Driver.TexSubImage3D			= _mesa_store_texsubimage3d;
	
	ctx->Driver.CopyTexImage1D			= gldCopyTexImage1D_DX8; //NULL;
	ctx->Driver.CopyTexImage2D			= gldCopyTexImage2D_DX8; //NULL;
	ctx->Driver.CopyTexSubImage1D		= gldCopyTexSubImage1D_DX8; //NULL;
	ctx->Driver.CopyTexSubImage2D		= gldCopyTexSubImage2D_DX8; //NULL;
	ctx->Driver.CopyTexSubImage3D		= gldCopyTexSubImage3D_DX8;
	ctx->Driver.TestProxyTexImage		= _mesa_test_proxy_teximage;

	// Texture object functions
	ctx->Driver.BindTexture				= NULL;
	ctx->Driver.NewTextureObject		= NULL; // Not yet implemented by Mesa!;
	ctx->Driver.DeleteTexture			= gld_DeleteTexture_DX8;
	ctx->Driver.PrioritizeTexture		= NULL;

	// Imaging functionality
	ctx->Driver.CopyColorTable			= NULL;
	ctx->Driver.CopyColorSubTable		= NULL;
	ctx->Driver.CopyConvolutionFilter1D = NULL;
	ctx->Driver.CopyConvolutionFilter2D = NULL;

	// State changing functions
	ctx->Driver.AlphaFunc				= NULL; //gld_AlphaFunc;
	ctx->Driver.BlendFuncSeparate		= NULL; //gld_BlendFunc;
	ctx->Driver.ClearColor				= NULL; //gld_ClearColor;
	ctx->Driver.ClearDepth				= NULL; //gld_ClearDepth;
	ctx->Driver.ClearStencil			= NULL; //gld_ClearStencil;
	ctx->Driver.ColorMask				= NULL; //gld_ColorMask;
	ctx->Driver.CullFace				= NULL; //gld_CullFace;
	ctx->Driver.ClipPlane				= NULL; //gld_ClipPlane;
	ctx->Driver.FrontFace				= NULL; //gld_FrontFace;
	ctx->Driver.DepthFunc				= NULL; //gld_DepthFunc;
	ctx->Driver.DepthMask				= NULL; //gld_DepthMask;
	ctx->Driver.DepthRange				= NULL;
	ctx->Driver.Enable					= NULL; //gld_Enable;
	ctx->Driver.Fogfv					= NULL; //gld_Fogfv;
	ctx->Driver.Hint					= NULL; //gld_Hint;
	ctx->Driver.Lightfv					= NULL; //gld_Lightfv;
	ctx->Driver.LightModelfv			= NULL; //gld_LightModelfv;
	ctx->Driver.LineStipple				= NULL; //gld_LineStipple;
	ctx->Driver.LineWidth				= NULL; //gld_LineWidth;
	ctx->Driver.LogicOpcode				= NULL; //gld_LogicOpcode;
	ctx->Driver.PointParameterfv		= NULL; //gld_PointParameterfv;
	ctx->Driver.PointSize				= NULL; //gld_PointSize;
	ctx->Driver.PolygonMode				= NULL; //gld_PolygonMode;
	ctx->Driver.PolygonOffset			= NULL; //gld_PolygonOffset;
	ctx->Driver.PolygonStipple			= NULL; //gld_PolygonStipple;
	ctx->Driver.RenderMode				= NULL; //gld_RenderMode;
	ctx->Driver.Scissor					= NULL; //gld_Scissor;
	ctx->Driver.ShadeModel				= NULL; //gld_ShadeModel;
	ctx->Driver.StencilFunc				= NULL; //gld_StencilFunc;
	ctx->Driver.StencilMask				= NULL; //gld_StencilMask;
	ctx->Driver.StencilOp				= NULL; //gld_StencilOp;
	ctx->Driver.TexGen					= NULL; //gld_TexGen;
	ctx->Driver.TexEnv					= NULL;
	ctx->Driver.TexParameter			= NULL;
	ctx->Driver.TextureMatrix			= NULL; //gld_TextureMatrix;
	ctx->Driver.Viewport				= gld_Viewport_DX8;

	_swsetup_Wakeup(ctx);

	tnl->Driver.RunPipeline				= _tnl_run_pipeline;
	tnl->Driver.Render.ResetLineStipple	= gld_ResetLineStipple_DX8;
	tnl->Driver.Render.ClippedPolygon	= _tnl_RenderClippedPolygon;
	tnl->Driver.Render.ClippedLine		= _tnl_RenderClippedLine;

	// Hook into glFrustum() and glOrtho()
//	ctx->Exec->Frustum					= gldFrustumHook_DX8;
//	ctx->Exec->Ortho					= gldOrthoHook_DX8;

}

//---------------------------------------------------------------------------
