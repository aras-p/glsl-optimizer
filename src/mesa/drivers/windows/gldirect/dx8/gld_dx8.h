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
* Description:  GLDirect Direct3D 8.0 header file
*
****************************************************************************/

#ifndef _GLD_DX8_H
#define _GLD_DX8_H

//---------------------------------------------------------------------------
// Windows includes
//---------------------------------------------------------------------------

//#ifndef STRICT
//#define STRICT
//#endif

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <d3d8.h>
#include <d3dx8.h>

// MS screwed up with the DX8.1 SDK - there's no compile-time
// method of compiling for 8.0 via the 8.1 SDK unless you
// "make sure you don't use any 8.1 interfaces".
// We CAN use 8.1 D3DX static functions, though - just not new 8.1 interfaces.
//
// D3D_SDK_VERSION is 120 for 8.0 (supported by Windows 95).
// D3D_SDK_VERSION is 220 for 8.1 (NOT supported by Windows 95).
//
#define D3D_SDK_VERSION_DX8_SUPPORT_WIN95 120

// Typedef for obtaining function from d3d8.dll
typedef IDirect3D8* (WINAPI *FNDIRECT3DCREATE8) (UINT);


//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

#ifdef _DEBUG
#define _GLD_TEST_HRESULT(h)					\
{												\
	HRESULT _hr = (h);							\
	if (FAILED(_hr)) {							\
		gldLogError(GLDLOG_ERROR, #h, _hr);		\
	}											\
}
#define _GLD_DX8(func)		_GLD_TEST_HRESULT(IDirect3D8_##func##)
#define _GLD_DX8_DEV(func)	_GLD_TEST_HRESULT(IDirect3DDevice8_##func##)
#define _GLD_DX8_VB(func)	_GLD_TEST_HRESULT(IDirect3DVertexBuffer8_##func##)
#define _GLD_DX8_TEX(func)	_GLD_TEST_HRESULT(IDirect3DTexture8_##func##)
#else
#define _GLD_DX8(func)		IDirect3D8_##func
#define _GLD_DX8_DEV(func)	IDirect3DDevice8_##func
#define _GLD_DX8_VB(func)	IDirect3DVertexBuffer8_##func
#define _GLD_DX8_TEX(func)	IDirect3DTexture8_##func
#endif

#define SAFE_RELEASE(p)			\
{								\
	if (p) {					\
		(p)->lpVtbl->Release(p);	\
		(p) = NULL;				\
	}							\
}

#define SAFE_RELEASE_VB8(p)						\
{												\
	if (p) {									\
		IDirect3DVertexBuffer8_Release((p));	\
		(p) = NULL;								\
	}											\
}

#define SAFE_RELEASE_SURFACE8(p)		\
{										\
	if (p) {							\
		IDirect3DSurface8_Release((p));	\
		(p) = NULL;						\
	}									\
}

// Setup index.
enum {
	GLD_SI_FLAT				= 0,
	GLD_SI_SMOOTH			= 1,
	GLD_SI_FLAT_EXTRAS		= 2,
	GLD_SI_SMOOTH_EXTRAS	= 3,
};
/*
// Internal pipeline
typedef enum {
	GLD_PIPELINE_MESA			= 0,	// Mesa pipeline
	GLD_PIPELINE_D3D_FVF		= 1,	// Direct3D Fixed-function pipeline
	GLD_PIPELINE_D3D_VS_TWOSIDE	= 2		// Direct3D two-sided-lighting vertex shader
} GLD_tnl_pipeline;
*/
//---------------------------------------------------------------------------
// Vertex definitions for Fixed-Function pipeline
//---------------------------------------------------------------------------

//
// NOTE: If the number of texture units is altered then most of
//       the texture code will need to be revised.
//

#define GLD_MAX_TEXTURE_UNITS_DX8	2

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
// Vertex Shaders
//---------------------------------------------------------------------------
/*
// DX8 Vertex Shader
typedef struct {
	DWORD	hShader;	// If NULL, shader is invalid and cannot be used
	BOOL	bHardware;	// If TRUE then shader was created for hardware,
						// otherwise shader was created for software.
} GLD_vertexShader;
*/
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
//	ULARGE_INTEGER	dwD3D2SVS;	// Direct3D Two-Sided Vertex Shader pipeline
} GLD_pipeline_usage;

// GLDirect Primitive Buffer (points, lines, triangles and quads)
typedef struct {
	// Data for IDirect3DDevice8::CreateVertexBuffer()
	DWORD					dwStride;		// Stride of vertex
	DWORD					dwUsage;		// Usage flags
	DWORD					dwFVF;			// Direct3D Flexible Vertex Format
	DWORD					dwPool;			// Pool flags

	IDirect3DVertexBuffer8	*pVB;			// Holds points, lines, tris and quads.

	// Point list is assumed to be at start of buffer
	DWORD					iFirstLine;		// Index of start of line list
	DWORD					iFirstTriangle;	// Index of start of triangle list

	BYTE					*pPoints;		// Pointer to next free point
	BYTE					*pLines;		// Pointer to next free line
	BYTE					*pTriangles;	// Pointer to next free triangle

	DWORD					nPoints;		// Number of points ready to render
	DWORD					nLines;			// Number of lines ready to render
	DWORD					nTriangles;		// Number of triangles ready to render
} GLD_pb_dx8;

// GLDirect DX8 driver data
typedef struct {
	// GLDirect vars
	BOOL					bDoublebuffer;	// Doublebuffer (otherwise single-buffered)
	BOOL					bDepthStencil;	// Depth buffer needed (stencil optional)
	D3DFORMAT				RenderFormat;	// Format of back/front buffer
	D3DFORMAT				DepthFormat;	// Format of depth/stencil
//	float					fFlipWindowY;	// Value for flipping viewport Y coord

	// Direct3D vars
	D3DCAPS8				d3dCaps8;
	BOOL					bHasHWTnL;		// Device has Hardware Transform/Light?
	IDirect3D8				*pD3D;			// Base Direct3D8 interface
	IDirect3DDevice8		*pDev;			// Direct3D8 Device interface
	GLD_pb_dx8				PB2d;			// Vertices transformed by Mesa
	GLD_pb_dx8				PB3d;			// Vertices transformed by Direct3D
	D3DPRIMITIVETYPE		d3dpt;			// Current Direct3D primitive type
	D3DXMATRIX				matProjection;	// Projection matrix for D3D TnL
	D3DXMATRIX				matModelView;	// Model/View matrix for D3D TnL
	int						iSetupFunc;		// Which setup functions to use
	BOOL					bUseMesaTnL;	// Whether to use Mesa or D3D for TnL

	// Direct3D vars for two-sided lighting
//	GLD_vertexShader		VStwosidelight;	// Vertex Shader for two-sided lighting
//	D3DXMATRIX				matWorldViewProj;// World/View/Projection matrix for shaders


//	GLD_tnl_pipeline		TnLPipeline;	// Index of current internal pipeline
	GLD_pipeline_usage		PipelineUsage;
} GLD_driver_dx8;

#define GLD_GET_DX8_DRIVER(c) (GLD_driver_dx8*)(c)->glPriv

//---------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------

PROC	gldGetProcAddress_DX8(LPCSTR a);
void	gldEnableExtensions_DX8(GLcontext *ctx);
void	gldInstallPipeline_DX8(GLcontext *ctx);
void	gldSetupDriverPointers_DX8(GLcontext *ctx);
//void	gldResizeBuffers_DX8(GLcontext *ctx);
void	gldResizeBuffers_DX8(GLframebuffer *fb);


// Texture functions

void	gldCopyTexImage1D_DX8(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void	gldCopyTexImage2D_DX8(GLcontext *ctx, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void	gldCopyTexSubImage1D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
void	gldCopyTexSubImage2D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
void	gldCopyTexSubImage3D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );

void	gld_NEW_TEXTURE_DX8(GLcontext *ctx);
void	gld_DrawPixels_DX8(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, const GLvoid *pixels);
void	gld_ReadPixels_DX8(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const struct gl_pixelstore_attrib *unpack, GLvoid *dest);
void	gld_CopyPixels_DX8(GLcontext *ctx, GLint srcx, GLint srcy, GLsizei width, GLsizei height, GLint dstx, GLint dsty, GLenum type);
void	gld_Bitmap_DX8(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height, const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap);
const struct gl_texture_format* gld_ChooseTextureFormat_DX8(GLcontext *ctx, GLint internalFormat, GLenum srcFormat, GLenum srcType);
void	gld_TexImage2D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint height, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *tObj, struct gl_texture_image *texImage);
void	gld_TexImage1D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint internalFormat, GLint width, GLint border, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage );
void	gld_TexSubImage2D_DX8( GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage );
void	gld_TexSubImage1D_DX8(GLcontext *ctx, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels, const struct gl_pixelstore_attrib *packing, struct gl_texture_object *texObj, struct gl_texture_image *texImage);
void	gld_DeleteTexture_DX8(GLcontext *ctx, struct gl_texture_object *tObj);
void	gld_ResetLineStipple_DX8(GLcontext *ctx);

// 2D primitive functions

void	gld_Points2D_DX8(GLcontext *ctx, GLuint first, GLuint last);

void	gld_Line2DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Line2DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1);

void	gld_Triangle2DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DFlatExtras_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmoothExtras_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);

void	gld_Quad2DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DFlatExtras_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmoothExtras_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

// 3D primitive functions

void	gld_Points3D_DX8(GLcontext *ctx, GLuint first, GLuint last);
void	gld_Line3DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle3DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad3DFlat_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Line3DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle3DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad3DSmooth_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

// Primitive functions for Two-sided-lighting Vertex Shader

void	gld_Points2DTwoside_DX8(GLcontext *ctx, GLuint first, GLuint last);
void	gld_Line2DFlatTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Line2DSmoothTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1);
void	gld_Triangle2DFlatTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Triangle2DSmoothTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2);
void	gld_Quad2DFlatTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void	gld_Quad2DSmoothTwoside_DX8(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

#endif
