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
* Description:  GLDirect Direct3D 7.0a header file
*
****************************************************************************/

#ifndef _GLD_DX7_H
#define _GLD_DX7_H

//---------------------------------------------------------------------------
// Windows includes
//---------------------------------------------------------------------------

#define DIRECTDRAW_VERSION	0x0700
#define DIRECT3D_VERSION	0x0700
#include <d3d.h>
#include <d3dx.h>

// Typedef for obtaining function from d3d7.dll
//typedef IDirect3D7* (WINAPI *FNDIRECT3DCREATE7) (UINT);


//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

#ifdef _DEBUG
// Debug build tests the return value of D3D calls
#define _GLD_TEST_HRESULT(h)					\
{												\
	HRESULT _hr = (h);							\
	if (FAILED(_hr)) {							\
		gldLogError(GLDLOG_ERROR, #h, _hr);		\
	}											\
}
#define _GLD_DX7(func)		_GLD_TEST_HRESULT(IDirect3D7_##func##)
#define _GLD_DX7_DEV(func)	_GLD_TEST_HRESULT(IDirect3DDevice7_##func##)
#define _GLD_DX7_VB(func)	_GLD_TEST_HRESULT(IDirect3DVertexBuffer7_##func##)
#define _GLD_DX7_TEX(func)	_GLD_TEST_HRESULT(IDirectDrawSurface7_##func##)
#else
#define _GLD_DX7(func)		IDirect3D7_##func
#define _GLD_DX7_DEV(func)	IDirect3DDevice7_##func
#define _GLD_DX7_VB(func)	IDirect3DVertexBuffer7_##func
#define _GLD_DX7_TEX(func)	IDirectDrawSurface7_##func
#endif

#define SAFE_RELEASE(p)			\
{								\
	if (p) {					\
		(p)->lpVtbl->Release(p);	\
		(p) = NULL;				\
	}							\
}

#define SAFE_RELEASE_VB7(p)						\
{												\
	if (p) {									\
		IDirect3DVertexBuffer7_Release((p));	\
		(p) = NULL;								\
	}											\
}

#define SAFE_RELEASE_SURFACE7(p)		\
{										\
	if (p) {							\
		IDirectDrawSurface7_Release((p));	\
		(p) = NULL;						\
	}									\
}

// Emulate some DX8 defines
#define D3DCOLOR_ARGB(a,r,g,b) ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_COLORVALUE(r,g,b,a) D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))


// Setup index.
enum {
	GLD_SI_FLAT				= 0,
	GLD_SI_SMOOTH			= 1,
	GLD_SI_FLAT_EXTRAS		= 2,
	GLD_SI_SMOOTH_EXTRAS	= 3,
};

//---------------------------------------------------------------------------
// Vertex definitions for Fixed-Function pipeline
//---------------------------------------------------------------------------

//
// NOTE: If the number of texture units is altered then most of
//       the texture code will need to be revised.
//

#define GLD_MAX_TEXTURE_UNITS_DX7	2

//
// 2D vertex transformed by Mesa
//
#define GLD_FVF_2D_VERTEX (	D3DFVF_XYZRHW |		\
							D3DFVF_DIFFUSE |	\
							D3DFVF_SPECULAR |	\
							D3DFVF_TEX2)
typedef struct {
	FLOAT	x, y;		// 2D raster coords
	FLOAT	sz;			// Screen Z (depth)
	FLOAT	rhw;		// Reciprocal homogenous W
	DWORD	diffuse;	// Diffuse colour
	DWORD	specular;	// For separate-specular support
	FLOAT	t0_u, t0_v;	// 1st set of texture coords
	FLOAT	t1_u, t1_v;	// 2nd set of texture coords
} GLD_2D_VERTEX;


//
// 3D vertex transformed by Direct3D
//
#define GLD_FVF_3D_VERTEX (	D3DFVF_XYZ |				\
							D3DFVF_DIFFUSE |			\
							D3DFVF_TEX2)

typedef struct {
	D3DXVECTOR3		Position;		// XYZ Vector in object space
	D3DCOLOR		Diffuse;		// Diffuse colour
	D3DXVECTOR2		TexUnit0;		// Texture unit 0
	D3DXVECTOR2		TexUnit1;		// Texture unit 1
} GLD_3D_VERTEX;

//---------------------------------------------------------------------------
// Structs
//---------------------------------------------------------------------------

// This keeps a count of how many times we choose each individual internal
// pathway. Useful for seeing if a certain pathway was ever used by an app, and
// how much each pathway is biased.
// Zero the members at context creation and dump stats at context deletion.
typedef struct {
	// Note: DWORD is probably too small
	ULARGE_INTEGER	qwMesa;		// Mesa TnL pipeline
	ULARGE_INTEGER	qwD3DFVF;	// Direct3D Fixed-Function pipeline
} GLD_pipeline_usage;

// GLDirect Primitive Buffer (points, lines, triangles and quads)
typedef struct {
	// Data for IDirect3D7::CreateVertexBuffer()
	DWORD					dwStride;		// Stride of vertex
	DWORD					dwCreateFlags;	// Create flags
	DWORD					dwFVF;			// Direct3D Flexible Vertex Format

	IDirect3DVertexBuffer7	*pVB;			// Holds points, lines, tris and quads.

	// Point list is assumed to be at start of buffer
	DWORD					iFirstLine;		// Index of start of line list
	DWORD					iFirstTriangle;	// Index of start of triangle list

	BYTE					*pPoints;		// Pointer to next free point
	BYTE					*pLines;		// Pointer to next free line
	BYTE					*pTriangles;	// Pointer to next free triangle

	DWORD					nPoints;		// Number of points ready to render
	DWORD					nLines;			// Number of lines ready to render
	DWORD					nTriangles;		// Number of triangles ready to render
} GLD_pb_dx7;

// GLDirect DX7 driver data
typedef struct {
	// GLDirect vars
	BOOL					bDoublebuffer;	// Doublebuffer (otherwise single-buffered)
	BOOL					bDepthStencil;	// Depth buffer needed (stencil optional)
	D3DX_SURFACEFORMAT		RenderFormat;	// Format of back/front buffer
	D3DX_SURFACEFORMAT		DepthFormat;	// Format of depth/stencil

	// Direct3D vars
	DDCAPS					ddCaps;
	D3DDEVICEDESC7			d3dCaps;
	BOOL					bHasHWTnL;		// Device has Hardware Transform/Light?
	ID3DXContext			*pD3DXContext;	// Base D3DX context
	IDirectDraw7			*pDD;			// DirectDraw7 interface
	IDirect3D7				*pD3D;			// Base Direct3D7 interface
	IDirect3DDevice7		*pDev;			// Direct3D7 Device interface
	GLD_pb_dx7				PB2d;			// Vertices transformed by Mesa
	GLD_pb_dx7				PB3d;			// Vertices transformed by Direct3D
	D3DPRIMITIVETYPE		d3dpt;			// Current Direct3D primitive type
	D3DMATRIX				matProjection;	// Projection matrix for D3D TnL
	D3DMATRIX				matModelView;	// Model/View matrix for D3D TnL
	int						iSetupFunc;		// Which setup functions to use
	BOOL					bUseMesaTnL;	// Whether to use Mesa or D3D for TnL

	GLD_pipeline_usage		PipelineUsage;
} GLD_driver_dx7;

#define GLD_GET_DX7_DRIVER(c) (GLD_driver_dx7*)(c)->glPriv

//---------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------

PROC	gldGetProcAddress_DX7(LPCSTR a);
void	gldEnableExtensions_DX7(GLcontext *ctx);
void	gldInstallPipeline_DX7(GLcontext *ctx);
void	gldSetupDriverPointers_DX7(GLcontext *ctx);
void	gldResizeBuffers_DX7(GLframebuffer *fb);


// Texture functions

void	gldCopyTexImage1D_DX7(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void	gldCopyTexImage2D_DX7(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void	gldCopyTexSubImage1D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
void	gldCopyTexSubImage2D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
void	gldCopyTexSubImage3D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );

void	gld_NEW_TEXTURE_DX7(GLcontext *ctx);
void	gld_DrawPixels_DX7(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, const GLvoid *pixels);
void	gld_ReadPixels_DX7(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, GLvoid *dest);
void	gld_CopyPixels_DX7(GLcontext *ctx, GLint srcx, GLint srcy, GLsizei width, GLsizei height, GLint dstx, GLint dsty, GLenum type);
void	gld_Bitmap_DX7(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap);
const struct gl_texture_format* gld_ChooseTextureFormat_DX7(GLcontext *ctx, GLint internalFormat, GLenum srcFormat, GLenum srcType);
void	gld_TexImage2D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint height, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *tObj, struct gl_texture_image *texImage);
void	gld_TexImage1D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage );
void	gld_TexSubImage2D_DX7( GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage );
void	gld_TexSubImage1D_DX7(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage);
void	gld_DeleteTexture_DX7(GLcontext *ctx, struct gl_texture_object *tObj);
void	gld_ResetLineStipple_DX7(GLcontext *ctx);

// 2D primitive functions

void	gld_Points2D_DX7(GLcontext *ctx, GLuint first, GLuint last);

void	gld_Line2DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Line2DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1);

void	gld_Triangle2DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DFlatExtras_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmoothExtras_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);

void	gld_Quad2DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DFlatExtras_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmoothExtras_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

// 3D primitive functions

void	gld_Points3D_DX7(GLcontext *ctx, GLuint first, GLuint last);
void	gld_Line3DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle3DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad3DFlat_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Line3DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle3DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad3DSmooth_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

// Primitive functions for Two-sided-lighting Vertex Shader

void	gld_Points2DTwoside_DX7(GLcontext *ctx, GLuint first, GLuint last);
void	gld_Line2DFlatTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Line2DSmoothTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle2DFlatTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmoothTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad2DFlatTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmoothTwoside_DX7(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

#endif
