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
* Description:  Primitive (points/lines/tris/quads) rendering
*
****************************************************************************/

//#include "../GLDirect.h"

//#include "gld_dx8.h"

#include "dglcontext.h"
#include "ddlog.h"
#include "gld_dx9.h"

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
#include "texstore.h"
#include "vbo/vbo.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast_setup/ss_context.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

// Disable compiler complaints about unreferenced local variables
#pragma warning (disable:4101)

//---------------------------------------------------------------------------
// Helper defines for primitives
//---------------------------------------------------------------------------

//static const float ooZ		= 1.0f / 65536.0f; // One over Z

#define GLD_COLOUR (D3DCOLOR_RGBA(swv->color[0], swv->color[1], swv->color[2], swv->color[3]))
#define GLD_SPECULAR (D3DCOLOR_RGBA(swv->specular[0], swv->specular[1], swv->specular[2], swv->specular[3]))
#define GLD_FLIP_Y(y) (gldCtx->dwHeight - (y))

//---------------------------------------------------------------------------
// 2D vertex setup
//---------------------------------------------------------------------------

#define GLD_SETUP_2D_VARS_POINTS							\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_2D_VERTEX	*pV		= (GLD_2D_VERTEX*)gld->PB2d.pPoints;	\
	SScontext		*ss		= SWSETUP_CONTEXT(ctx);			\
	SWvertex		*swv;									\
	DWORD			dwSpecularColour;						\
	DWORD			dwFlatColour

#define GLD_SETUP_2D_VARS_LINES								\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_2D_VERTEX	*pV		= (GLD_2D_VERTEX*)gld->PB2d.pLines;	\
	SScontext		*ss		= SWSETUP_CONTEXT(ctx);			\
	SWvertex		*swv;									\
	DWORD			dwSpecularColour;						\
	DWORD			dwFlatColour

#define GLD_SETUP_2D_VARS_TRIANGLES							\
	BOOL			bFog = ctx->Fog.Enabled;				\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_2D_VERTEX	*pV		= (GLD_2D_VERTEX*)gld->PB2d.pTriangles;	\
	SScontext		*ss		= SWSETUP_CONTEXT(ctx);			\
	SWvertex		*swv;									\
	DWORD			dwSpecularColour;						\
	DWORD			dwFlatColour;							\
	GLuint					facing = 0;						\
	struct vertex_buffer	*VB;							\
	GLchan					(*vbcolor)[4];					\
	GLchan					(*vbspec)[4]

#define GLD_SETUP_GET_SWVERT(s)					\
	swv = &ss->verts[##s]

#define GLD_SETUP_2D_VERTEX						\
	pV->x			= swv->win[0];				\
	pV->y			= GLD_FLIP_Y(swv->win[1]);	\
	pV->rhw			= swv->win[3]

#define GLD_SETUP_SMOOTH_COLOUR					\
	pV->diffuse		= GLD_COLOUR

#define GLD_SETUP_GET_FLAT_COLOUR				\
	dwFlatColour	= GLD_COLOUR
#define GLD_SETUP_GET_FLAT_FOG_COLOUR			\
	dwFlatColour	= _gldComputeFog(ctx, swv)

#define GLD_SETUP_USE_FLAT_COLOUR				\
	pV->diffuse		= dwFlatColour

#define GLD_SETUP_GET_FLAT_SPECULAR				\
	dwSpecularColour= GLD_SPECULAR

#define GLD_SETUP_USE_FLAT_SPECULAR				\
	pV->specular	= dwSpecularColour

#define GLD_SETUP_DEPTH							\
	pV->sz			= swv->win[2] / ctx->DepthMaxF
//	pV->z			= swv->win[2] * ooZ;

#define GLD_SETUP_SPECULAR						\
	pV->specular	= GLD_SPECULAR

#define GLD_SETUP_FOG							\
	pV->diffuse		= _gldComputeFog(ctx, swv)

#define GLD_SETUP_TEX0							\
	pV->t0_u		= swv->texcoord[0][0];		\
	pV->t0_v		= swv->texcoord[0][1]

#define GLD_SETUP_TEX1							\
	pV->t1_u		= swv->texcoord[1][0];		\
	pV->t1_v		= swv->texcoord[1][1]

#define GLD_SETUP_LIGHTING(v)			\
	if (facing == 1) {					\
		pV->diffuse	= D3DCOLOR_RGBA(vbcolor[##v][0], vbcolor[##v][1], vbcolor[##v][2], vbcolor[##v][3]);	\
		if (vbspec) {																					\
			pV->specular = D3DCOLOR_RGBA(vbspec[##v][0], vbspec[##v][1], vbspec[##v][2], vbspec[##v][3]);	\
		}	\
	} else {	\
		if (bFog)						\
			GLD_SETUP_FOG;				\
		else							\
			GLD_SETUP_SMOOTH_COLOUR;	\
		GLD_SETUP_SPECULAR;				\
	}

#define GLD_SETUP_GET_FLAT_LIGHTING(v)	\
	if (facing == 1) {					\
		dwFlatColour = D3DCOLOR_RGBA(vbcolor[##v][0], vbcolor[##v][1], vbcolor[##v][2], vbcolor[##v][3]);	\
		if (vbspec) {																					\
			dwSpecularColour = D3DCOLOR_RGBA(vbspec[##v][0], vbspec[##v][1], vbspec[##v][2], vbspec[##v][3]);	\
		}	\
	}

#define GLD_SETUP_TWOSIDED_LIGHTING		\
	/* Two-sided lighting */				\
	if (ctx->_TriangleCaps & DD_TRI_LIGHT_TWOSIDE) {	\
		SWvertex	*verts = SWSETUP_CONTEXT(ctx)->verts;	\
		SWvertex	*v[3];									\
		GLfloat		ex,ey,fx,fy,cc;							\
		/* Get vars for later */							\
		VB		= &TNL_CONTEXT(ctx)->vb;					\
		vbcolor	= (GLchan (*)[4])VB->ColorPtr[1]->data;		\
		if (VB->SecondaryColorPtr[1]) {						\
			vbspec = (GLchan (*)[4])VB->SecondaryColorPtr[1]->data;	\
		} else {													\
			vbspec = NULL;											\
		}															\
		v[0] = &verts[v0];											\
		v[1] = &verts[v1];											\
		v[2] = &verts[v2];											\
		ex = v[0]->win[0] - v[2]->win[0];	\
		ey = v[0]->win[1] - v[2]->win[1];	\
		fx = v[1]->win[0] - v[2]->win[0];	\
		fy = v[1]->win[1] - v[2]->win[1];	\
		cc  = ex*fy - ey*fx;				\
		facing = (cc < 0.0) ^ ctx->Polygon._FrontBit;	\
	}

//---------------------------------------------------------------------------
// 3D vertex setup
//---------------------------------------------------------------------------

#define GLD_SETUP_3D_VARS_POINTS											\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_3D_VERTEX			*pV		= (GLD_3D_VERTEX*)gld->PB3d.pPoints;	\
	TNLcontext				*tnl	= TNL_CONTEXT(ctx);				\
	struct vertex_buffer	*VB		= &tnl->vb;						\
	GLfloat					(*p4f)[4];								\
	GLfloat					(*tc)[4];								\
	DWORD					dwColor;

#define GLD_SETUP_3D_VARS_LINES											\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_3D_VERTEX			*pV		= (GLD_3D_VERTEX*)gld->PB3d.pLines;	\
	TNLcontext				*tnl	= TNL_CONTEXT(ctx);				\
	struct vertex_buffer	*VB		= &tnl->vb;						\
	GLfloat					(*p4f)[4];								\
	GLfloat					(*tc)[4];								\
	DWORD					dwColor;

#define GLD_SETUP_3D_VARS_TRIANGLES											\
	GLD_context		*gldCtx	= GLD_GET_CONTEXT(ctx);			\
	GLD_driver_dx9	*gld	= GLD_GET_DX9_DRIVER(gldCtx);	\
	GLD_3D_VERTEX			*pV		= (GLD_3D_VERTEX*)gld->PB3d.pTriangles;	\
	TNLcontext				*tnl	= TNL_CONTEXT(ctx);				\
	struct vertex_buffer	*VB		= &tnl->vb;						\
	GLfloat					(*p4f)[4];								\
	GLfloat					(*tc)[4];								\
	DWORD					dwColor;

#define GLD_SETUP_3D_VERTEX(v)					\
	p4f				= VB->ObjPtr->data;			\
	pV->Position.x	= p4f[##v][0];				\
	pV->Position.y	= p4f[##v][1];				\
	pV->Position.z	= p4f[##v][2];

#define GLD_SETUP_SMOOTH_COLOUR_3D(v)															\
	p4f			= (GLfloat (*)[4])VB->ColorPtr[0]->data;										\
	pV->Diffuse	= D3DCOLOR_COLORVALUE(p4f[##v][0], p4f[##v][1], p4f[##v][2], p4f[##v][3]);


#define GLD_SETUP_GET_FLAT_COLOUR_3D(v)													\
	p4f		= (GLfloat (*)[4])VB->ColorPtr[0]->data;										\
	dwColor	= D3DCOLOR_COLORVALUE(p4f[##v][0], p4f[##v][1], p4f[##v][2], p4f[##v][3]);

#define GLD_SETUP_USE_FLAT_COLOUR_3D			\
	pV->Diffuse = dwColor;

#define GLD_SETUP_TEX0_3D(v)						\
	if (VB->TexCoordPtr[0]) {						\
		tc				= VB->TexCoordPtr[0]->data;	\
		pV->TexUnit0.x	= tc[##v][0];				\
		pV->TexUnit0.y	= tc[##v][1];				\
	}

#define GLD_SETUP_TEX1_3D(v)						\
	if (VB->TexCoordPtr[1]) {						\
		tc				= VB->TexCoordPtr[1]->data;	\
		pV->TexUnit1.x	= tc[##v][0];				\
		pV->TexUnit1.y	= tc[##v][1];				\
	}

//---------------------------------------------------------------------------
// Helper functions
//---------------------------------------------------------------------------

__inline DWORD _gldComputeFog(
	GLcontext *ctx,
	SWvertex *swv)
{
	// Full fog calculation.
	// Based on Mesa code.

	GLchan			rFog, gFog, bFog;
	GLchan			fR, fG, fB;
	const GLfloat	f = swv->fog;
	const GLfloat	g = 1.0 - f;
	
	UNCLAMPED_FLOAT_TO_CHAN(rFog, ctx->Fog.Color[RCOMP]);
	UNCLAMPED_FLOAT_TO_CHAN(gFog, ctx->Fog.Color[GCOMP]);
	UNCLAMPED_FLOAT_TO_CHAN(bFog, ctx->Fog.Color[BCOMP]);
	fR = f * swv->color[0] + g * rFog;
	fG = f * swv->color[1] + g * gFog;
	fB = f * swv->color[2] + g * bFog;
	return D3DCOLOR_RGBA(fR, fG, fB, swv->color[3]);
}

//---------------------------------------------------------------------------

void gld_ResetLineStipple_DX9(
	GLcontext *ctx)
{
	// TODO: Fake stipple with a 32x32 texture.
}

//---------------------------------------------------------------------------
// 2D (post-transformed) primitives
//---------------------------------------------------------------------------

void gld_Points2D_DX9(
	GLcontext *ctx,
	GLuint first,
	GLuint last)
{
	GLD_SETUP_2D_VARS_POINTS;

	unsigned				i;
	struct vertex_buffer	*VB = &TNL_CONTEXT(ctx)->vb;

	// _Size is already clamped to MaxPointSize and MinPointSize
	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_POINTSIZE, *((DWORD*)&ctx->Point._Size));

	if (VB->Elts) {
		for (i=first; i<last; i++, pV++) {
			if (VB->ClipMask[VB->Elts[i]] == 0) {
//				_swrast_Point( ctx, &verts[VB->Elts[i]] );
				GLD_SETUP_GET_SWVERT(VB->Elts[i]);
				GLD_SETUP_2D_VERTEX;
				GLD_SETUP_SMOOTH_COLOUR;
				GLD_SETUP_DEPTH;
				GLD_SETUP_SPECULAR;
				GLD_SETUP_TEX0;
				GLD_SETUP_TEX1;
			}
		}
	} else {
		GLD_SETUP_GET_SWVERT(first);
		for (i=first; i<last; i++, swv++, pV++) {
			if (VB->ClipMask[i] == 0) {
//				_swrast_Point( ctx, &verts[i] );
				GLD_SETUP_2D_VERTEX;
				GLD_SETUP_SMOOTH_COLOUR;
				GLD_SETUP_DEPTH;
				GLD_SETUP_SPECULAR;
				GLD_SETUP_TEX0;
				GLD_SETUP_TEX1;
			}
		}
	}

	gld->PB2d.pPoints = (BYTE*)pV;
	gld->PB2d.nPoints += (last-first);
}

//---------------------------------------------------------------------------

void gld_Line2DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1)
{
	GLD_SETUP_2D_VARS_LINES;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_GET_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_GET_FLAT_SPECULAR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	gld->PB2d.pLines = (BYTE*)pV;
	gld->PB2d.nLines++;
}

//---------------------------------------------------------------------------

void gld_Line2DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1)
{
	GLD_SETUP_2D_VARS_LINES;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_SPECULAR;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_SPECULAR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	gld->PB2d.pLines = (BYTE*)pV;
	gld->PB2d.nLines++;
}

//---------------------------------------------------------------------------

void gld_Triangle2DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_GET_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Triangle2DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{

	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Triangle2DFlatExtras_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_TWOSIDED_LIGHTING(v2);

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	if (bFog)
		GLD_SETUP_GET_FLAT_FOG_COLOUR;
	else
		GLD_SETUP_GET_FLAT_COLOUR;
	GLD_SETUP_GET_FLAT_SPECULAR;
	GLD_SETUP_GET_FLAT_LIGHTING(v2);
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Triangle2DSmoothExtras_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_TWOSIDED_LIGHTING(v0);

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v0);
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v1);
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v2);
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Quad2DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_GET_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles += 2;
}

//---------------------------------------------------------------------------

void gld_Quad2DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_SMOOTH_COLOUR;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles += 2;
}

//---------------------------------------------------------------------------

void gld_Quad2DFlatExtras_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_TWOSIDED_LIGHTING(v3);

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	if (bFog)
		GLD_SETUP_GET_FLAT_FOG_COLOUR;
	else
		GLD_SETUP_GET_FLAT_COLOUR;
	GLD_SETUP_GET_FLAT_SPECULAR;
	GLD_SETUP_GET_FLAT_LIGHTING(v3);
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_USE_FLAT_COLOUR;
	GLD_SETUP_USE_FLAT_SPECULAR;
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles += 2;
}

//---------------------------------------------------------------------------

void gld_Quad2DSmoothExtras_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_2D_VARS_TRIANGLES;

	GLD_SETUP_TWOSIDED_LIGHTING(v0);

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v0);
	pV++;

	GLD_SETUP_GET_SWVERT(v1);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v1);
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v2);
	pV++;

	GLD_SETUP_GET_SWVERT(v2);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v2);
	pV++;

	GLD_SETUP_GET_SWVERT(v3);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v3);
	pV++;

	GLD_SETUP_GET_SWVERT(v0);
	GLD_SETUP_2D_VERTEX;
	GLD_SETUP_DEPTH;
	GLD_SETUP_TEX0;
	GLD_SETUP_TEX1;
	GLD_SETUP_LIGHTING(v0);
	pV++;

	gld->PB2d.pTriangles = (BYTE*)pV;
	gld->PB2d.nTriangles += 2;
}

//---------------------------------------------------------------------------
// 3D (pre-transformed) primitives
//---------------------------------------------------------------------------

void gld_Points3D_DX9(
	GLcontext *ctx,
	GLuint first,
	GLuint last)
{
	GLD_SETUP_3D_VARS_POINTS

	unsigned				i;
//	struct vertex_buffer	*VB = &TNL_CONTEXT(ctx)->vb;

	// _Size is already clamped to MaxPointSize and MinPointSize
	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_POINTSIZE, *((DWORD*)&ctx->Point._Size));

	if (VB->Elts) {
		for (i=first; i<last; i++, pV++) {
			if (VB->ClipMask[VB->Elts[i]] == 0) {
//				_swrast_Point( ctx, &verts[VB->Elts[i]] );
//				GLD_SETUP_GET_SWVERT(VB->Elts[i]);
				GLD_SETUP_3D_VERTEX(VB->Elts[i])
				GLD_SETUP_SMOOTH_COLOUR_3D(i)
				GLD_SETUP_TEX0_3D(i)
				GLD_SETUP_TEX1_3D(i)
			}
		}
	} else {
//		GLD_SETUP_GET_SWVERT(first);
		for (i=first; i<last; i++, pV++) {
			if (VB->ClipMask[i] == 0) {
//				_swrast_Point( ctx, &verts[i] );
				GLD_SETUP_3D_VERTEX(i)
				GLD_SETUP_SMOOTH_COLOUR_3D(i)
				GLD_SETUP_TEX0_3D(i)
				GLD_SETUP_TEX1_3D(i)
			}
		}
	}
/*
	for (i=first; i<last; i++, pV++) {
		GLD_SETUP_3D_VERTEX(i)
		GLD_SETUP_SMOOTH_COLOUR_3D(i)
		GLD_SETUP_TEX0_3D(i)
		GLD_SETUP_TEX1_3D(i)
	}
*/
	gld->PB3d.pPoints = (BYTE*)pV;
	gld->PB3d.nPoints += (last-first);
}

//---------------------------------------------------------------------------
// Line functions
//---------------------------------------------------------------------------

void gld_Line3DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1)
{
	GLD_SETUP_3D_VARS_LINES

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_GET_FLAT_COLOUR_3D(v1)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	gld->PB3d.pLines = (BYTE*)pV;
	gld->PB3d.nLines++;
}

//---------------------------------------------------------------------------

void gld_Line3DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1)
{
	GLD_SETUP_3D_VARS_LINES

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_SMOOTH_COLOUR_3D(v1)
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_SMOOTH_COLOUR_3D(v0)
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	gld->PB3d.pLines = (BYTE*)pV;
	gld->PB3d.nLines++;
}

//---------------------------------------------------------------------------
// Triangle functions
//---------------------------------------------------------------------------

void gld_Triangle3DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{
	GLD_SETUP_3D_VARS_TRIANGLES

	GLD_SETUP_3D_VERTEX(v2)
	GLD_SETUP_TEX0_3D(v2)
	GLD_SETUP_TEX1_3D(v2)
	GLD_SETUP_GET_FLAT_COLOUR_3D(v2)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	pV++;

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	pV++;

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	pV++;

	gld->PB3d.pTriangles = (BYTE*)pV;
	gld->PB3d.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Triangle3DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2)
{
	GLD_SETUP_3D_VARS_TRIANGLES

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_SMOOTH_COLOUR_3D(v0)
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_SMOOTH_COLOUR_3D(v1)
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v2)
	GLD_SETUP_SMOOTH_COLOUR_3D(v2)
	GLD_SETUP_TEX0_3D(v2)
	GLD_SETUP_TEX1_3D(v2)
	pV++;

	gld->PB3d.pTriangles = (BYTE*)pV;
	gld->PB3d.nTriangles++;
}

//---------------------------------------------------------------------------
// Quad functions
//---------------------------------------------------------------------------

void gld_Quad3DFlat_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_3D_VARS_TRIANGLES

	GLD_SETUP_3D_VERTEX(v3)
	GLD_SETUP_GET_FLAT_COLOUR_3D(v3)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v3)
	GLD_SETUP_TEX1_3D(v3)
	pV++;

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v2)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v2)
	GLD_SETUP_TEX1_3D(v2)
	pV++;

	GLD_SETUP_3D_VERTEX(v3)
	GLD_SETUP_USE_FLAT_COLOUR_3D
	GLD_SETUP_TEX0_3D(v3)
	GLD_SETUP_TEX1_3D(v3)
	pV++;

	gld->PB3d.pTriangles = (BYTE*)pV;
	gld->PB3d.nTriangles += 2;
}

//---------------------------------------------------------------------------

void gld_Quad3DSmooth_DX9(
	GLcontext *ctx,
	GLuint v0,
	GLuint v1,
	GLuint v2,
	GLuint v3)
{
	GLD_SETUP_3D_VARS_TRIANGLES

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_SMOOTH_COLOUR_3D(v0)
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	GLD_SETUP_3D_VERTEX(v1)
	GLD_SETUP_SMOOTH_COLOUR_3D(v1)
	GLD_SETUP_TEX0_3D(v1)
	GLD_SETUP_TEX1_3D(v1)
	pV++;

	GLD_SETUP_3D_VERTEX(v2)
	GLD_SETUP_SMOOTH_COLOUR_3D(v2)
	GLD_SETUP_TEX0_3D(v2)
	GLD_SETUP_TEX1_3D(v2)
	pV++;

	GLD_SETUP_3D_VERTEX(v2)
	GLD_SETUP_SMOOTH_COLOUR_3D(v2)
	GLD_SETUP_TEX0_3D(v2)
	GLD_SETUP_TEX1_3D(v2)
	pV++;

	GLD_SETUP_3D_VERTEX(v3)
	GLD_SETUP_SMOOTH_COLOUR_3D(v3)
	GLD_SETUP_TEX0_3D(v3)
	GLD_SETUP_TEX1_3D(v3)
	pV++;

	GLD_SETUP_3D_VERTEX(v0)
	GLD_SETUP_SMOOTH_COLOUR_3D(v0)
	GLD_SETUP_TEX0_3D(v0)
	GLD_SETUP_TEX1_3D(v0)
	pV++;

	gld->PB3d.pTriangles = (BYTE*)pV;
	gld->PB3d.nTriangles += 2;
}

//---------------------------------------------------------------------------
// Vertex setup for two-sided-lighting vertex shader
//---------------------------------------------------------------------------

/*

void gld_Points2DTwoside_DX9(GLcontext *ctx, GLuint first, GLuint last)
{
	// NOTE: Two-sided lighting does not apply to Points
}

//---------------------------------------------------------------------------

void gld_Line2DFlatTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1)
{
	// NOTE: Two-sided lighting does not apply to Lines
}

//---------------------------------------------------------------------------

void gld_Line2DSmoothTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1)
{
	// NOTE: Two-sided lighting does not apply to Lines
}

//---------------------------------------------------------------------------

void gld_Triangle2DFlatTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2)
{
}

//---------------------------------------------------------------------------

void gld_Triangle2DSmoothTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx9		*gld	= GLD_GET_DX9_DRIVER(gldCtx);
	GLD_TWOSIDED_VERTEX	*pV		= (GLD_TWOSIDED_VERTEX*)gld->PBtwosidelight.pTriangles;
	SScontext			*ss		= SWSETUP_CONTEXT(ctx);
	SWvertex			*swv;
	DWORD				dwSpecularColour;
	DWORD				dwFlatColour;
	GLuint					facing = 0;
	struct vertex_buffer	*VB;
	GLchan					(*vbcolor)[4];
	GLchan					(*vbspec)[4];

	// Reciprocal of DepthMax
	const float ooDepthMax = 1.0f / ctx->DepthMaxF; 

	// 1st vert
	swv = &ss->verts[v0];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 2nd vert
	swv = &ss->verts[v1];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 3rd vert
	swv = &ss->verts[v2];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	gld->PBtwosidelight.pTriangles = (BYTE*)pV;
	gld->PBtwosidelight.nTriangles++;
}

//---------------------------------------------------------------------------

void gld_Quad2DFlatTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx9		*gld	= GLD_GET_DX9_DRIVER(gldCtx);
	GLD_TWOSIDED_VERTEX	*pV		= (GLD_TWOSIDED_VERTEX*)gld->PBtwosidelight.pTriangles;
	SScontext			*ss		= SWSETUP_CONTEXT(ctx);
	SWvertex			*swv;
	DWORD				dwSpecularColour;
	DWORD				dwFlatColour;
	GLuint					facing = 0;
	struct vertex_buffer	*VB;
	GLchan					(*vbcolor)[4];
	GLchan					(*vbspec)[4];

	// Reciprocal of DepthMax
	const float ooDepthMax = 1.0f / ctx->DepthMaxF; 

	// 1st vert
	swv = &ss->verts[v0];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 2nd vert
	swv = &ss->verts[v1];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 3rd vert
	swv = &ss->verts[v2];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 4th vert
	swv = &ss->verts[v2];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 5th vert
	swv = &ss->verts[v3];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 6th vert
	swv = &ss->verts[v0];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	gld->PBtwosidelight.pTriangles = (BYTE*)pV;
	gld->PBtwosidelight.nTriangles += 2;
}

//---------------------------------------------------------------------------

void gld_Quad2DSmoothTwoside_DX9(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	GLD_context			*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx9		*gld	= GLD_GET_DX9_DRIVER(gldCtx);
	GLD_TWOSIDED_VERTEX	*pV		= (GLD_TWOSIDED_VERTEX*)gld->PBtwosidelight.pTriangles;
	SScontext			*ss		= SWSETUP_CONTEXT(ctx);
	SWvertex			*swv;
	DWORD				dwSpecularColour;
	DWORD				dwFlatColour;
	GLuint					facing = 0;
	struct vertex_buffer	*VB;
	GLchan					(*vbcolor)[4];
	GLchan					(*vbspec)[4];

	// Reciprocal of DepthMax
	const float ooDepthMax = 1.0f / ctx->DepthMaxF; 

	// 1st vert
	swv = &ss->verts[v0];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 2nd vert
	swv = &ss->verts[v1];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 3rd vert
	swv = &ss->verts[v2];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 4th vert
	swv = &ss->verts[v2];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 5th vert
	swv = &ss->verts[v3];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	// 6th vert
	swv = &ss->verts[v0];
	pV->Position.x = swv->win[0];
	pV->Position.y = GLD_FLIP_Y(swv->win[1]);
	pV->Position.z = swv->win[2] * ooDepthMax;
	pV->Position.w = swv->win[3];
	pV->TexUnit0.x = swv->texcoord[0][0];
	pV->TexUnit0.y = swv->texcoord[0][1];
	pV->TexUnit1.x = swv->texcoord[1][0];
	pV->TexUnit1.y = swv->texcoord[1][1];
	pV->FrontDiffuse = GLD_COLOUR;
	pV->FrontSpecular = GLD_SPECULAR;
	pV++;

	gld->PBtwosidelight.pTriangles = (BYTE*)pV;
	gld->PBtwosidelight.nTriangles += 2;
}

//---------------------------------------------------------------------------

*/
