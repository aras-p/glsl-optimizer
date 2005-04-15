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
* Description:  GLDirect Direct3D 8.x WGL (WindowsGL)
*
****************************************************************************/

#include "dglcontext.h"
#include "gld_driver.h"
#include "gld_dxerr9.h"
#include "gld_dx9.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"

// Copied from dglcontect.c
#define GLDERR_NONE     0
#define GLDERR_MEM      1
#define GLDERR_DDRAW    2
#define GLDERR_D3D      3
#define GLDERR_BPP      4
#define GLDERR_DDS      5
// This external var keeps track of any error
extern int nContextError;

#define DDLOG_CRITICAL_OR_WARN	DDLOG_CRITICAL

extern void _gld_mesa_warning(GLcontext *, char *);
extern void _gld_mesa_fatal(GLcontext *, char *);

//---------------------------------------------------------------------------

static char	szColorDepthWarning[] =
"GLDirect does not support the current desktop\n\
color depth.\n\n\
You may need to change the display resolution to\n\
16 bits per pixel or higher color depth using\n\
the Windows Display Settings control panel\n\
before running this OpenGL application.\n";

// The only depth-stencil formats currently supported by Direct3D
// Surface Format	Depth	Stencil		Total Bits
// D3DFMT_D32		32		-			32
// D3DFMT_D15S1		15		1			16
// D3DFMT_D24S8		24		8			32
// D3DFMT_D16		16		-			16
// D3DFMT_D24X8		24		-			32
// D3DFMT_D24X4S4	24		4			32

// This pixel format will be used as a template when compiling the list
// of pixel formats supported by the hardware. Many fields will be
// filled in at runtime.
// PFD flag defaults are upgraded to match ChoosePixelFormat() -- DaveM
static DGL_pixelFormat pfTemplateHW =
{
    {
	sizeof(PIXELFORMATDESCRIPTOR),	// Size of the data structure
		1,							// Structure version - should be 1
									// Flags:
		PFD_DRAW_TO_WINDOW |		// The buffer can draw to a window or device surface.
		PFD_DRAW_TO_BITMAP |		// The buffer can draw to a bitmap. (DaveM)
		PFD_SUPPORT_GDI |			// The buffer supports GDI drawing. (DaveM)
		PFD_SUPPORT_OPENGL |		// The buffer supports OpenGL drawing.
		PFD_DOUBLEBUFFER |			// The buffer is double-buffered.
		0,							// Placeholder for easy commenting of above flags
		PFD_TYPE_RGBA,				// Pixel type RGBA.
		16,							// Total colour bitplanes (excluding alpha bitplanes)
		5, 0,						// Red bits, shift
		5, 0,						// Green bits, shift
		5, 0,						// Blue bits, shift
		0, 0,						// Alpha bits, shift (destination alpha)
		0,							// Accumulator bits (total)
		0, 0, 0, 0,					// Accumulator bits: Red, Green, Blue, Alpha
		0,							// Depth bits
		0,							// Stencil bits
		0,							// Number of auxiliary buffers
		0,							// Layer type
		0,							// Specifies the number of overlay and underlay planes.
		0,							// Layer mask
		0,							// Specifies the transparent color or index of an underlay plane.
		0							// Damage mask
	},
	D3DFMT_UNKNOWN,	// No depth/stencil buffer
};

//---------------------------------------------------------------------------
// Vertex Shaders
//---------------------------------------------------------------------------
/*
// Vertex Shader Declaration
static DWORD dwTwoSidedLightingDecl[] =
{
	D3DVSD_STREAM(0),
	D3DVSD_REG(0,  D3DVSDT_FLOAT3), 	 // XYZ position
	D3DVSD_REG(1,  D3DVSDT_FLOAT3), 	 // XYZ normal
	D3DVSD_REG(2,  D3DVSDT_D3DCOLOR),	 // Diffuse color
	D3DVSD_REG(3,  D3DVSDT_D3DCOLOR),	 // Specular color
	D3DVSD_REG(4,  D3DVSDT_FLOAT2), 	 // 2D texture unit 0
	D3DVSD_REG(5,  D3DVSDT_FLOAT2), 	 // 2D texture unit 1
	D3DVSD_END()
};

// Vertex Shader for two-sided lighting
static char *szTwoSidedLightingVS =
// This is a test shader!
"vs.1.0\n"
"m4x4 oPos,v0,c0\n"
"mov oD0,v2\n"
"mov oD1,v3\n"
"mov oT0,v4\n"
"mov oT1,v5\n"
;
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct {
	HINSTANCE			hD3D9DLL;			// Handle to d3d9.dll
	FNDIRECT3DCREATE9	fnDirect3DCreate9;	// Direct3DCreate9 function prototype
	BOOL				bDirect3D;			// Persistant Direct3D9 exists
	BOOL				bDirect3DDevice;	// Persistant Direct3DDevice9 exists
	IDirect3D9			*pD3D;				// Persistant Direct3D9
	IDirect3DDevice9	*pDev;				// Persistant Direct3DDevice9
} GLD_dx9_globals;

// These are "global" to all DX9 contexts. KeithH
static GLD_dx9_globals dx9Globals;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

BOOL gldGetDXErrorString_DX(
	HRESULT hr,
	char *buf,
	int nBufSize)
{
	//
	// Return a string describing the input HRESULT error code
	//

	const char *pStr = DXGetErrorString9(hr);

	if (pStr == NULL)
		return FALSE;

	if (strlen(pStr) > nBufSize)
		strncpy(buf, pStr, nBufSize);
	else
		strcpy(buf, pStr);

//	D3DXGetErrorString(hr, buf, nBufSize);

	return TRUE;
}

//---------------------------------------------------------------------------

static D3DMULTISAMPLE_TYPE _gldGetDeviceMultiSampleType(
	IDirect3D9 *pD3D9,
	D3DFORMAT SurfaceFormat,
	D3DDEVTYPE d3dDevType,
	BOOL Windowed)
{
	int			i;
	HRESULT		hr;

	if (glb.dwMultisample == GLDS_MULTISAMPLE_NONE)
		return D3DMULTISAMPLE_NONE;

	if (glb.dwMultisample == GLDS_MULTISAMPLE_FASTEST) {
		// Find fastest multisample
		for (i=2; i<17; i++) {
			hr = IDirect3D9_CheckDeviceMultiSampleType(
					pD3D9,
					glb.dwAdapter,
					d3dDevType,
					SurfaceFormat,
					Windowed,
					(D3DMULTISAMPLE_TYPE)i,
					NULL);
			if (SUCCEEDED(hr)) {
				return (D3DMULTISAMPLE_TYPE)i;
			}
		}
	} else {
		// Find nicest multisample
		for (i=16; i>1; i--) {
			hr = IDirect3D9_CheckDeviceMultiSampleType(
					pD3D9,
					glb.dwAdapter,
					d3dDevType,
					SurfaceFormat,
					Windowed,
					(D3DMULTISAMPLE_TYPE)i,
					NULL);
			if (SUCCEEDED(hr)) {
				return (D3DMULTISAMPLE_TYPE)i;
			}
		}
	}

	// Nothing found - return default
	return D3DMULTISAMPLE_NONE;
}

//---------------------------------------------------------------------------

void _gldDestroyPrimitiveBuffer(
	GLD_pb_dx9 *gldVB)
{
	SAFE_RELEASE(gldVB->pVB);

	// Sanity check...
	gldVB->nLines = gldVB->nPoints = gldVB->nTriangles = 0;
}

//---------------------------------------------------------------------------

HRESULT _gldCreatePrimitiveBuffer(
	GLcontext *ctx,
	GLD_driver_dx9 *lpCtx,
	GLD_pb_dx9 *gldVB)
{
	HRESULT		hResult;
	char		*szCreateVertexBufferFailed = "CreateVertexBuffer failed";
	DWORD		dwMaxVertices;	// Max number of vertices in vertex buffer
	DWORD		dwVBSize;		// Total size of vertex buffer

	// If CVA (Compiled Vertex Array) is used by an OpenGL app, then we
	// will need enough vertices to cater for Mesa::Const.MaxArrayLockSize.
	// We'll use IMM_SIZE if it's larger (which it should not be).
	dwMaxVertices = MAX_ARRAY_LOCK_SIZE;

	// Now calculate how many vertices to allow for in total
	// 1 per point, 2 per line, 6 per quad = 9
	dwVBSize = dwMaxVertices * 9 * gldVB->dwStride;

	hResult = IDirect3DDevice9_CreateVertexBuffer(
		lpCtx->pDev,
		dwVBSize,
		gldVB->dwUsage,
		gldVB->dwFVF,
		gldVB->dwPool,
		&gldVB->pVB,
		NULL);
	if (FAILED(hResult)) {
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, szCreateVertexBufferFailed);
		return hResult;
	}

	gldVB->nLines = gldVB->nPoints = gldVB->nTriangles = 0;
	gldVB->pPoints	= gldVB->pLines = gldVB->pTriangles = NULL;
	gldVB->iFirstLine = dwMaxVertices; // Index of first line in VB
	gldVB->iFirstTriangle = dwMaxVertices*3; // Index of first triangle in VB

	return S_OK;
}

//---------------------------------------------------------------------------
// Function: _gldCreateVertexShaders
// Create DX9 Vertex Shaders.
//---------------------------------------------------------------------------
/*
void _gldCreateVertexShaders(
	GLD_driver_dx9 *gld)
{
	DWORD			dwFlags;
	LPD3DXBUFFER	pVSOpcodeBuffer; // Vertex Shader opcode buffer
	HRESULT			hr;

#ifdef _DEBUG
	dwFlags = D3DXASM_DEBUG;
#else
	dwFlags = 0; // D3DXASM_SKIPVALIDATION;
#endif

	ddlogMessage(DDLOG_INFO, "Creating shaders...\n");

	// Init the shader handle
	gld->VStwosidelight.hShader = 0;

	if (gld->d3dCaps8.MaxStreams == 0) {
		// Lame DX8 driver doesn't support streams
		// Not fatal, as defaults will be used
		ddlogMessage(DDLOG_WARN, "Driver doesn't support Vertex Shaders (MaxStreams==0)\n");
		return;
	}

	// ** THIS DISABLES VERTEX SHADER SUPPORT **
//	return;
	// ** THIS DISABLES VERTEX SHADER SUPPORT **

	//
	// Two-sided lighting
	//

#if 0
	//
	// DEBUGGING: Load shader from a text file
	//
	{
	LPD3DXBUFFER	pVSErrorBuffer; // Vertex Shader error buffer
	hr = D3DXAssembleShaderFromFile(
			"twoside.vsh",
			dwFlags,
			NULL, // No constants
			&pVSOpcodeBuffer,
			&pVSErrorBuffer);
	if (pVSErrorBuffer && pVSErrorBuffer->lpVtbl->GetBufferPointer(pVSErrorBuffer))
		ddlogMessage(DDLOG_INFO, pVSErrorBuffer->lpVtbl->GetBufferPointer(pVSErrorBuffer));
	SAFE_RELEASE(pVSErrorBuffer);
	}
#else
	{
	LPD3DXBUFFER	pVSErrorBuffer; // Vertex Shader error buffer
	// Assemble ascii shader text into shader opcodes
	hr = D3DXAssembleShader(
			szTwoSidedLightingVS,
			strlen(szTwoSidedLightingVS),
			dwFlags,
			NULL, // No constants
			&pVSOpcodeBuffer,
			&pVSErrorBuffer);
	if (pVSErrorBuffer && pVSErrorBuffer->lpVtbl->GetBufferPointer(pVSErrorBuffer))
		ddlogMessage(DDLOG_INFO, pVSErrorBuffer->lpVtbl->GetBufferPointer(pVSErrorBuffer));
	SAFE_RELEASE(pVSErrorBuffer);
	}
#endif
	if (FAILED(hr)) {
		ddlogError(DDLOG_WARN, "AssembleShader failed", hr);
		SAFE_RELEASE(pVSOpcodeBuffer);
		return;
	}

// This is for debugging. Remove to enable vertex shaders in HW
#define _GLD_FORCE_SW_VS 0

	if (_GLD_FORCE_SW_VS) {
		// _GLD_FORCE_SW_VS should be disabled for Final Release
		ddlogMessage(DDLOG_SYSTEM, "[Forcing shaders in SW]\n");
	}

	// Try and create shader in hardware.
	// NOTE: The D3D Ref device appears to succeed when trying to
	//       create the device in hardware, but later complains
	//       when trying to set it with SetVertexShader(). Go figure.
	if (_GLD_FORCE_SW_VS || glb.dwDriver == GLDS_DRIVER_REF) {
		// Don't try and create a hardware shader with the Ref device
		hr = E_FAIL; // COM error/fail result
	} else {
		gld->VStwosidelight.bHardware = TRUE;
		hr = IDirect3DDevice8_CreateVertexShader(
			gld->pDev,
			dwTwoSidedLightingDecl,
			pVSOpcodeBuffer->lpVtbl->GetBufferPointer(pVSOpcodeBuffer),
			&gld->VStwosidelight.hShader,
			0);
	}
	if (FAILED(hr)) {
		ddlogMessage(DDLOG_INFO, "... HW failed, trying SW...\n");
		// Failed. Try and create shader for software processing
		hr = IDirect3DDevice8_CreateVertexShader(
			gld->pDev,
			dwTwoSidedLightingDecl,
			pVSOpcodeBuffer->lpVtbl->GetBufferPointer(pVSOpcodeBuffer),
			&gld->VStwosidelight.hShader,
			D3DUSAGE_SOFTWAREPROCESSING);
		if (FAILED(hr)) {
			gld->VStwosidelight.hShader = 0; // Sanity check
			ddlogError(DDLOG_WARN, "CreateVertexShader failed", hr);
			return;
		}
		// Succeeded, but for software processing
		gld->VStwosidelight.bHardware = FALSE;
	}

	SAFE_RELEASE(pVSOpcodeBuffer);

	ddlogMessage(DDLOG_INFO, "... OK\n");
}

//---------------------------------------------------------------------------

void _gldDestroyVertexShaders(
	GLD_driver_dx9 *gld)
{
	if (gld->VStwosidelight.hShader) {
		IDirect3DDevice8_DeleteVertexShader(gld->pDev, gld->VStwosidelight.hShader);
		gld->VStwosidelight.hShader = 0;
	}
}
*/
//---------------------------------------------------------------------------

BOOL gldCreateDrawable_DX(
	DGL_ctx *ctx,
//	BOOL bDefaultDriver,
	BOOL bDirectDrawPersistant,
	BOOL bPersistantBuffers)
{
	//
	// bDirectDrawPersistant:	applies to IDirect3D9
	// bPersistantBuffers:		applies to IDirect3DDevice9
	//

	HRESULT					hResult;
	GLD_driver_dx9			*lpCtx = NULL;
	D3DDEVTYPE				d3dDevType;
	D3DPRESENT_PARAMETERS	d3dpp;
	D3DDISPLAYMODE			d3ddm;
	DWORD					dwBehaviourFlags;
	D3DADAPTER_IDENTIFIER9	d3dIdent;

	// Error if context is NULL.
	if (ctx == NULL)
		return FALSE;

	if (ctx->glPriv) {
		lpCtx = ctx->glPriv;
		// Release any existing interfaces
		SAFE_RELEASE(lpCtx->pDev);
		SAFE_RELEASE(lpCtx->pD3D);
	} else {
		lpCtx = (GLD_driver_dx9*)malloc(sizeof(GLD_driver_dx9));
		ZeroMemory(lpCtx, sizeof(lpCtx));
	}

	d3dDevType = (glb.dwDriver == GLDS_DRIVER_HAL) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;
	// TODO: Check this
//	if (bDefaultDriver)
//		d3dDevType = D3DDEVTYPE_REF;

	// Use persistant interface if needed
	if (bDirectDrawPersistant && dx9Globals.bDirect3D) {
		lpCtx->pD3D = dx9Globals.pD3D;
		IDirect3D9_AddRef(lpCtx->pD3D);
		goto SkipDirectDrawCreate;
	}

	// Create Direct3D9 object
	lpCtx->pD3D = dx9Globals.fnDirect3DCreate9(D3D_SDK_VERSION);
	if (lpCtx->pD3D == NULL) {
		MessageBox(NULL, "Unable to initialize Direct3D9", "GLDirect", MB_OK);
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, "Unable to create Direct3D9 interface");
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	// Cache Direct3D interface for subsequent GLRCs
	if (bDirectDrawPersistant && !dx9Globals.bDirect3D) {
		dx9Globals.pD3D = lpCtx->pD3D;
		IDirect3D9_AddRef(dx9Globals.pD3D);
		dx9Globals.bDirect3D = TRUE;
	}
SkipDirectDrawCreate:

	// Get the display mode so we can make a compatible backbuffer
	hResult = IDirect3D9_GetAdapterDisplayMode(lpCtx->pD3D, glb.dwAdapter, &d3ddm);
	if (FAILED(hResult)) {
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	// Get device caps
	hResult = IDirect3D9_GetDeviceCaps(lpCtx->pD3D, glb.dwAdapter, d3dDevType, &lpCtx->d3dCaps9);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "IDirect3D9_GetDeviceCaps failed", hResult);
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	// Check for hardware transform & lighting
	lpCtx->bHasHWTnL = lpCtx->d3dCaps9.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ? TRUE : FALSE;

/*
	//
	// GONE FOR DX9?
	//
	// If this flag is present then we can't default to Mesa
	// SW rendering between BeginScene() and EndScene().
	if (lpCtx->d3dCaps9.Caps2 & D3DCAPS2_NO2DDURING3DSCENE) {
		ddlogMessage(DDLOG_WARN,
			"Warning          : No 2D allowed during 3D scene.\n");
	}
*/

	//
	//	Create the Direct3D context
	//

	// Re-use original IDirect3DDevice if persistant buffers exist.
	// Note that we test for persistant IDirect3D9 as well
	// bDirectDrawPersistant == persistant IDirect3D9 (DirectDraw9 does not exist)
	if (bDirectDrawPersistant && bPersistantBuffers && dx9Globals.pD3D && dx9Globals.pDev) {
		lpCtx->pDev = dx9Globals.pDev;
		IDirect3DDevice9_AddRef(dx9Globals.pDev);
		goto skip_direct3ddevice_create;
	}

	// Clear the presentation parameters (sets all members to zero)
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	// Recommended by MS; needed for MultiSample.
	// Be careful if altering this for FullScreenBlit
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	d3dpp.BackBufferFormat	= d3ddm.Format;
	d3dpp.BackBufferCount	= 2; //1;
	d3dpp.MultiSampleType	= _gldGetDeviceMultiSampleType(lpCtx->pD3D, d3ddm.Format, d3dDevType, !ctx->bFullscreen);
	d3dpp.AutoDepthStencilFormat	= ctx->lpPF->dwDriverData;
	d3dpp.EnableAutoDepthStencil	= (d3dpp.AutoDepthStencilFormat == D3DFMT_UNKNOWN) ? FALSE : TRUE;

	if (ctx->bFullscreen) {
		ddlogWarnOption(FALSE); // Don't popup any messages in fullscreen
		d3dpp.Windowed							= FALSE;
		d3dpp.BackBufferWidth					= d3ddm.Width;
		d3dpp.BackBufferHeight					= d3ddm.Height;
		d3dpp.hDeviceWindow						= ctx->hWnd;
		d3dpp.FullScreen_RefreshRateInHz		= D3DPRESENT_RATE_DEFAULT;

		// Support for vertical retrace synchronisation.
		// Set default presentation interval in case caps bits are missing
		d3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_DEFAULT;
		if (glb.bWaitForRetrace) {
			if (lpCtx->d3dCaps9.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
				d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		} else {
			if (lpCtx->d3dCaps9.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
				d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}
	} else {
		ddlogWarnOption(glb.bMessageBoxWarnings); // OK to popup messages
		d3dpp.Windowed							= TRUE;
		d3dpp.BackBufferWidth					= ctx->dwWidth;
		d3dpp.BackBufferHeight					= ctx->dwHeight;
		d3dpp.hDeviceWindow						= ctx->hWnd;
		d3dpp.FullScreen_RefreshRateInHz		= 0;
		// PresentationInterval Windowed mode is optional now in DX9 (DaveM)
		d3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_DEFAULT;
		if (glb.bWaitForRetrace) {
				d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		} else {
				d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}
	}

	// Decide if we can use hardware TnL
	dwBehaviourFlags = (lpCtx->bHasHWTnL) ?
		D3DCREATE_MIXED_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	// Add flag to tell D3D to be thread-safe
	if (glb.bMultiThreaded)
		dwBehaviourFlags |= D3DCREATE_MULTITHREADED;
	// Add flag to tell D3D to be FPU-safe
	if (!glb.bFastFPU)
		dwBehaviourFlags |= D3DCREATE_FPU_PRESERVE;
	hResult = IDirect3D9_CreateDevice(lpCtx->pD3D,
								glb.dwAdapter,
								d3dDevType,
								ctx->hWnd,
								dwBehaviourFlags,
								&d3dpp,
								&lpCtx->pDev);
    if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "IDirect3D9_CreateDevice failed", hResult);
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	if (bDirectDrawPersistant && bPersistantBuffers && dx9Globals.pD3D) {
		dx9Globals.pDev = lpCtx->pDev;
		dx9Globals.bDirect3DDevice = TRUE;
	}

	// Dump some useful stats
	hResult = IDirect3D9_GetAdapterIdentifier(
		lpCtx->pD3D,
		glb.dwAdapter,
		0, // No WHQL detection (avoid few seconds delay)
		&d3dIdent);
	if (SUCCEEDED(hResult)) {
		ddlogPrintf(DDLOG_INFO, "[Driver Description: %s]", &d3dIdent.Description);
		ddlogPrintf(DDLOG_INFO, "[Driver file: %s %d.%d.%02d.%d]",
			d3dIdent.Driver,
			HIWORD(d3dIdent.DriverVersion.HighPart),
			LOWORD(d3dIdent.DriverVersion.HighPart),
			HIWORD(d3dIdent.DriverVersion.LowPart),
			LOWORD(d3dIdent.DriverVersion.LowPart));
		ddlogPrintf(DDLOG_INFO, "[VendorId: 0x%X, DeviceId: 0x%X, SubSysId: 0x%X, Revision: 0x%X]",
			d3dIdent.VendorId, d3dIdent.DeviceId, d3dIdent.SubSysId, d3dIdent.Revision);
	}

	// Test to see if IHV driver exposes Scissor Test (new for DX9)
	lpCtx->bCanScissor = lpCtx->d3dCaps9.RasterCaps & D3DPRASTERCAPS_SCISSORTEST;
	ddlogPrintf(DDLOG_INFO, "Can Scissor: %s", lpCtx->bCanScissor ? "Yes" : "No");

	// Init projection matrix for D3D TnL
	D3DXMatrixIdentity(&lpCtx->matProjection);
	lpCtx->matModelView = lpCtx->matProjection;
//		gld->bUseMesaProjection = TRUE;

skip_direct3ddevice_create:

	// Create buffers to hold primitives
	lpCtx->PB2d.dwFVF		= GLD_FVF_2D_VERTEX;
	lpCtx->PB2d.dwPool		= D3DPOOL_SYSTEMMEM;
	lpCtx->PB2d.dwStride	= sizeof(GLD_2D_VERTEX);
	lpCtx->PB2d.dwUsage		= D3DUSAGE_DONOTCLIP |
								D3DUSAGE_DYNAMIC |
								D3DUSAGE_SOFTWAREPROCESSING |
								D3DUSAGE_WRITEONLY;
	hResult = _gldCreatePrimitiveBuffer(ctx->glCtx, lpCtx, &lpCtx->PB2d);
	if (FAILED(hResult))
		goto return_with_error;

	lpCtx->PB3d.dwFVF		= GLD_FVF_3D_VERTEX;
	lpCtx->PB3d.dwPool		= D3DPOOL_DEFAULT;
	lpCtx->PB3d.dwStride	= sizeof(GLD_3D_VERTEX);
	lpCtx->PB3d.dwUsage		= D3DUSAGE_DYNAMIC |
//DaveM								D3DUSAGE_SOFTWAREPROCESSING |
								D3DUSAGE_WRITEONLY;
	hResult = _gldCreatePrimitiveBuffer(ctx->glCtx, lpCtx, &lpCtx->PB3d);
	if (FAILED(hResult))
		goto return_with_error;

/*	// NOTE: A FVF code of zero indicates a non-FVF vertex buffer (for vertex shaders)
	lpCtx->PBtwosidelight.dwFVF		= 0; //GLD_FVF_TWOSIDED_VERTEX;
	lpCtx->PBtwosidelight.dwPool	= D3DPOOL_DEFAULT;
	lpCtx->PBtwosidelight.dwStride	= sizeof(GLD_TWOSIDED_VERTEX);
	lpCtx->PBtwosidelight.dwUsage	= D3DUSAGE_DONOTCLIP |
								D3DUSAGE_DYNAMIC |
								D3DUSAGE_SOFTWAREPROCESSING |
								D3DUSAGE_WRITEONLY;
	hResult = _gldCreatePrimitiveBuffer(ctx->glCtx, lpCtx, &lpCtx->PBtwosidelight);
	if (FAILED(hResult))
		goto return_with_error;*/

	// Now try and create the DX9 Vertex Shaders
//	_gldCreateVertexShaders(lpCtx);

	// Zero the pipeline usage counters
	lpCtx->PipelineUsage.qwMesa.QuadPart = 
//	lpCtx->PipelineUsage.dwD3D2SVS.QuadPart =
	lpCtx->PipelineUsage.qwD3DFVF.QuadPart = 0;

	// Assign drawable to GL private
	ctx->glPriv = lpCtx;
	return TRUE;

return_with_error:
	// Clean up and bail

//	_gldDestroyVertexShaders(lpCtx);

//	_gldDestroyPrimitiveBuffer(&lpCtx->PBtwosidelight);
	_gldDestroyPrimitiveBuffer(&lpCtx->PB3d);
	_gldDestroyPrimitiveBuffer(&lpCtx->PB2d);

	SAFE_RELEASE(lpCtx->pDev);
	SAFE_RELEASE(lpCtx->pD3D);
	return FALSE;
}

//---------------------------------------------------------------------------

BOOL gldResizeDrawable_DX(
	DGL_ctx *ctx,
	BOOL bDefaultDriver,
	BOOL bPersistantInterface,
	BOOL bPersistantBuffers)
{
	GLD_driver_dx9			*gld = NULL;
	D3DDEVTYPE				d3dDevType;
	D3DPRESENT_PARAMETERS	d3dpp;
	D3DDISPLAYMODE			d3ddm;
	HRESULT					hResult;

	// Error if context is NULL.
	if (ctx == NULL)
		return FALSE;

	gld = ctx->glPriv;
	if (gld == NULL)
		return FALSE;

	if (ctx->bSceneStarted) {
		IDirect3DDevice9_EndScene(gld->pDev);
		ctx->bSceneStarted = FALSE;
	}

	d3dDevType = (glb.dwDriver == GLDS_DRIVER_HAL) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;
	if (!bDefaultDriver)
		d3dDevType = D3DDEVTYPE_REF; // Force Direct3D Reference Rasterise (software)

	// Get the display mode so we can make a compatible backbuffer
	hResult = IDirect3D9_GetAdapterDisplayMode(gld->pD3D, glb.dwAdapter, &d3ddm);
	if (FAILED(hResult)) {
        nContextError = GLDERR_D3D;
//		goto return_with_error;
		return FALSE;
	}

	// Destroy DX9 Vertex Shaders before Reset()
//	_gldDestroyVertexShaders(gld);

	// Release POOL_DEFAULT objects before Reset()
	if (gld->PB2d.dwPool == D3DPOOL_DEFAULT)
		_gldDestroyPrimitiveBuffer(&gld->PB2d);
	if (gld->PB3d.dwPool == D3DPOOL_DEFAULT)
		_gldDestroyPrimitiveBuffer(&gld->PB3d);
//	if (gld->PBtwosidelight.dwPool == D3DPOOL_DEFAULT)
//		_gldDestroyPrimitiveBuffer(&gld->PBtwosidelight);

	// Clear the presentation parameters (sets all members to zero)
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	// Recommended by MS; needed for MultiSample.
	// Be careful if altering this for FullScreenBlit
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	d3dpp.BackBufferFormat	= d3ddm.Format;
	d3dpp.BackBufferCount	= 1;
	d3dpp.MultiSampleType	= _gldGetDeviceMultiSampleType(gld->pD3D, d3ddm.Format, d3dDevType, !ctx->bFullscreen);
	d3dpp.AutoDepthStencilFormat	= ctx->lpPF->dwDriverData;
	d3dpp.EnableAutoDepthStencil	= (d3dpp.AutoDepthStencilFormat == D3DFMT_UNKNOWN) ? FALSE : TRUE;

	// TODO: Sync to refresh

	if (ctx->bFullscreen) {
		ddlogWarnOption(FALSE); // Don't popup any messages in fullscreen 
		d3dpp.Windowed						= FALSE;
		d3dpp.BackBufferWidth				= d3ddm.Width;
		d3dpp.BackBufferHeight				= d3ddm.Height;
		d3dpp.hDeviceWindow					= ctx->hWnd;
		d3dpp.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
		d3dpp.PresentationInterval			= D3DPRESENT_INTERVAL_DEFAULT;
		// Get better benchmark results? KeithH
//		d3dpp.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_UNLIMITED;
	} else {
		ddlogWarnOption(glb.bMessageBoxWarnings); // OK to popup messages
		d3dpp.Windowed						= TRUE;
		d3dpp.BackBufferWidth				= ctx->dwWidth;
		d3dpp.BackBufferHeight				= ctx->dwHeight;
		d3dpp.hDeviceWindow					= ctx->hWnd;
		d3dpp.FullScreen_RefreshRateInHz	= 0;
		d3dpp.PresentationInterval			= D3DPRESENT_INTERVAL_DEFAULT;
	}
	hResult = IDirect3DDevice9_Reset(gld->pDev, &d3dpp);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "dglResize: Reset failed", hResult);
		return FALSE;
		//goto cleanup_and_return_with_error;
	}

	//
	// Recreate POOL_DEFAULT objects
	//
	if (gld->PB2d.dwPool == D3DPOOL_DEFAULT) {
		_gldCreatePrimitiveBuffer(ctx->glCtx, gld, &gld->PB2d);
	}
	if (gld->PB3d.dwPool == D3DPOOL_DEFAULT) {
		_gldCreatePrimitiveBuffer(ctx->glCtx, gld, &gld->PB3d);
	}
//	if (gld->PBtwosidelight.dwPool == D3DPOOL_DEFAULT) {
//		_gldCreatePrimitiveBuffer(ctx->glCtx, gld, &gld->PB2d);
//	}

	// Recreate DX9 Vertex Shaders
//	_gldCreateVertexShaders(gld);

	// Signal a complete state update
	ctx->glCtx->Driver.UpdateState(ctx->glCtx, _NEW_ALL);

	// Begin a new scene
	IDirect3DDevice9_BeginScene(gld->pDev);
	ctx->bSceneStarted = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldDestroyDrawable_DX(
	DGL_ctx *ctx)
{
	GLD_driver_dx9			*lpCtx = NULL;

	// Error if context is NULL.
	if (!ctx)
		return FALSE;

	// Error if the drawable does not exist.
	if (!ctx->glPriv)
		return FALSE;

	lpCtx = ctx->glPriv;

#ifdef _DEBUG
	// Dump out stats
	ddlogPrintf(DDLOG_SYSTEM, "Usage: M:0x%X%X, D:0x%X%X",
		lpCtx->PipelineUsage.qwMesa.HighPart,
		lpCtx->PipelineUsage.qwMesa.LowPart,
		lpCtx->PipelineUsage.qwD3DFVF.HighPart,
		lpCtx->PipelineUsage.qwD3DFVF.LowPart);
#endif

//	_gldDestroyVertexShaders(lpCtx);
	
//	_gldDestroyPrimitiveBuffer(&lpCtx->PBtwosidelight);
	_gldDestroyPrimitiveBuffer(&lpCtx->PB3d);
	_gldDestroyPrimitiveBuffer(&lpCtx->PB2d);

	SAFE_RELEASE(lpCtx->pDev);
	SAFE_RELEASE(lpCtx->pD3D);

	// Free the private drawable data
	free(ctx->glPriv);
	ctx->glPriv = NULL;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldCreatePrivateGlobals_DX(void)
{
	ZeroMemory(&dx9Globals, sizeof(dx9Globals));

	// Load d3d9.dll
	dx9Globals.hD3D9DLL = LoadLibrary("D3D9.DLL");
	if (dx9Globals.hD3D9DLL == NULL)
		return FALSE;

	// Now try and obtain Direct3DCreate9
	dx9Globals.fnDirect3DCreate9 = (FNDIRECT3DCREATE9)GetProcAddress(dx9Globals.hD3D9DLL, "Direct3DCreate9");
	if (dx9Globals.fnDirect3DCreate9 == NULL) {
		FreeLibrary(dx9Globals.hD3D9DLL);
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldDestroyPrivateGlobals_DX(void)
{
	if (dx9Globals.bDirect3DDevice) {
		SAFE_RELEASE(dx9Globals.pDev);
		dx9Globals.bDirect3DDevice = FALSE;
	}
	if (dx9Globals.bDirect3D) {
		SAFE_RELEASE(dx9Globals.pD3D);
		dx9Globals.bDirect3D = FALSE;
	}

	FreeLibrary(dx9Globals.hD3D9DLL);
	dx9Globals.hD3D9DLL = NULL;
	dx9Globals.fnDirect3DCreate9 = NULL;

	return TRUE;
}

//---------------------------------------------------------------------------

static void _BitsFromDisplayFormat(
	D3DFORMAT fmt,
	BYTE *cColorBits,
	BYTE *cRedBits,
	BYTE *cGreenBits,
	BYTE *cBlueBits,
	BYTE *cAlphaBits)
{
	switch (fmt) {
	case D3DFMT_X1R5G5B5:
		*cColorBits = 16;
		*cRedBits = 5;
		*cGreenBits = 5;
		*cBlueBits = 5;
		*cAlphaBits = 0;
		return;
	case D3DFMT_R5G6B5:
		*cColorBits = 16;
		*cRedBits = 5;
		*cGreenBits = 6;
		*cBlueBits = 5;
		*cAlphaBits = 0;
		return;
	case D3DFMT_X8R8G8B8:
		*cColorBits = 32;
		*cRedBits = 8;
		*cGreenBits = 8;
		*cBlueBits = 8;
		*cAlphaBits = 0;
		return;
	case D3DFMT_A8R8G8B8:
		*cColorBits = 32;
		*cRedBits = 8;
		*cGreenBits = 8;
		*cBlueBits = 8;
		*cAlphaBits = 8;
		return;
	}

	// Should not get here!
	*cColorBits = 32;
	*cRedBits = 8;
	*cGreenBits = 8;
	*cBlueBits = 8;
	*cAlphaBits = 0;
}

//---------------------------------------------------------------------------

static void _BitsFromDepthStencilFormat(
	D3DFORMAT fmt,
	BYTE *cDepthBits,
	BYTE *cStencilBits)
{
	// NOTE: GL expects either 32 or 16 as depth bits.
	switch (fmt) {
	case D3DFMT_D32:
		*cDepthBits = 32;
		*cStencilBits = 0;
		return;
	case D3DFMT_D15S1:
		*cDepthBits = 16;
		*cStencilBits = 1;
		return;
	case D3DFMT_D24S8:
		*cDepthBits = 32;
		*cStencilBits = 8;
		return;
	case D3DFMT_D16:
		*cDepthBits = 16;
		*cStencilBits = 0;
		return;
	case D3DFMT_D24X8:
		*cDepthBits = 32;
		*cStencilBits = 0;
		return;
	case D3DFMT_D24X4S4:
		*cDepthBits = 32;
		*cStencilBits = 4;
		return;
	}
}

//---------------------------------------------------------------------------

BOOL gldBuildPixelformatList_DX(void)
{
	D3DDISPLAYMODE		d3ddm;
	D3DFORMAT			fmt[6];
	IDirect3D9			*pD3D = NULL;
	HRESULT				hr;
	int					nSupportedFormats = 0;
	int					i;
	DGL_pixelFormat		*pPF;
	BYTE				cColorBits, cRedBits, cGreenBits, cBlueBits, cAlphaBits;
//	char				buf[128];
//	char				cat[8];

	// Direct3D (SW or HW)
	// These are arranged so that 'best' pixelformat
	// is higher in the list (for ChoosePixelFormat).
	const D3DFORMAT DepthStencil[6] = {
// New order: increaing Z, then increasing stencil
		D3DFMT_D15S1,
		D3DFMT_D16,
		D3DFMT_D24X4S4,
		D3DFMT_D24X8,
		D3DFMT_D24S8,
		D3DFMT_D32,
	};

	// Dump DX version
	ddlogMessage(GLDLOG_SYSTEM, "DirectX Version  : 9.0\n");

	// Release any existing pixelformat list
	if (glb.lpPF) {
		free(glb.lpPF);
	}

	glb.nPixelFormatCount	= 0;
	glb.lpPF				= NULL;

	//
	// Pixelformats for Direct3D (SW or HW) rendering
	//

	// Get a Direct3D 9.0 interface
	pD3D = dx9Globals.fnDirect3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) {
		return FALSE;
	}

	// We will use the display mode format when finding compliant
	// rendertarget/depth-stencil surfaces.
	hr = IDirect3D9_GetAdapterDisplayMode(pD3D, glb.dwAdapter, &d3ddm);
	if (FAILED(hr)) {
		IDirect3D9_Release(pD3D);
		return FALSE;
	}
	
	// Run through the possible formats and detect supported formats
	for (i=0; i<6; i++) {
		hr = IDirect3D9_CheckDeviceFormat(
			pD3D,
			glb.dwAdapter,
			glb.dwDriver==GLDS_DRIVER_HAL ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF,
            d3ddm.Format,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE,
			DepthStencil[i]);
		if (FAILED(hr))
			// A failure here is not fatal.
			continue;

	    // Verify that the depth format is compatible.
	    hr = IDirect3D9_CheckDepthStencilMatch(
				pD3D,
				glb.dwAdapter,
                glb.dwDriver==GLDS_DRIVER_HAL ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF,
                d3ddm.Format,
                d3ddm.Format,
                DepthStencil[i]);
		if (FAILED(hr))
			// A failure here is not fatal, just means depth-stencil
			// format is not compatible with this display mode.
			continue;

		fmt[nSupportedFormats++] = DepthStencil[i];
	}

	IDirect3D9_Release(pD3D);

	if (nSupportedFormats == 0)
		return FALSE; // Bail: no compliant pixelformats

	// Total count of pixelformats is:
	// (nSupportedFormats+1)*2
	// UPDATED: nSupportedFormats*2
	glb.lpPF = (DGL_pixelFormat *)calloc(nSupportedFormats*2, sizeof(DGL_pixelFormat));
	glb.nPixelFormatCount = nSupportedFormats*2;
	if (glb.lpPF == NULL) {
		glb.nPixelFormatCount = 0;
		return FALSE;
	}

	// Get a copy of pointer that we can alter
	pPF = glb.lpPF;

	// Cache colour bits from display format
	_BitsFromDisplayFormat(d3ddm.Format, &cColorBits, &cRedBits, &cGreenBits, &cBlueBits, &cAlphaBits);

	//
	// Add single-buffer formats
	//
/*
	// NOTE: No longer returning pixelformats that don't contain depth
	// Single-buffer, no depth-stencil buffer
	memcpy(pPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
	pPF->pfd.dwFlags &= ~PFD_DOUBLEBUFFER; // Remove doublebuffer flag
	pPF->pfd.cColorBits		= cColorBits;
	pPF->pfd.cRedBits		= cRedBits;
	pPF->pfd.cGreenBits		= cGreenBits;
	pPF->pfd.cBlueBits		= cBlueBits;
	pPF->pfd.cAlphaBits		= cAlphaBits;
	pPF->pfd.cDepthBits		= 0;
	pPF->pfd.cStencilBits	= 0;
	pPF->dwDriverData		= D3DFMT_UNKNOWN;
	pPF++;
*/
	for (i=0; i<nSupportedFormats; i++, pPF++) {
		memcpy(pPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
		pPF->pfd.dwFlags &= ~PFD_DOUBLEBUFFER; // Remove doublebuffer flag
		pPF->pfd.cColorBits		= cColorBits;
		pPF->pfd.cRedBits		= cRedBits;
		pPF->pfd.cGreenBits		= cGreenBits;
		pPF->pfd.cBlueBits		= cBlueBits;
		pPF->pfd.cAlphaBits		= cAlphaBits;
		_BitsFromDepthStencilFormat(fmt[i], &pPF->pfd.cDepthBits, &pPF->pfd.cStencilBits);
		pPF->dwDriverData		= fmt[i];
	}

	//
	// Add double-buffer formats
	//

	// NOTE: No longer returning pixelformats that don't contain depth
/*
	memcpy(pPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
	pPF->pfd.cColorBits		= cColorBits;
	pPF->pfd.cRedBits		= cRedBits;
	pPF->pfd.cGreenBits		= cGreenBits;
	pPF->pfd.cBlueBits		= cBlueBits;
	pPF->pfd.cAlphaBits		= cAlphaBits;
	pPF->pfd.cDepthBits		= 0;
	pPF->pfd.cStencilBits	= 0;
	pPF->dwDriverData		= D3DFMT_UNKNOWN;
	pPF++;
*/
	for (i=0; i<nSupportedFormats; i++, pPF++) {
		memcpy(pPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
		pPF->pfd.cColorBits		= cColorBits;
		pPF->pfd.cRedBits		= cRedBits;
		pPF->pfd.cGreenBits		= cGreenBits;
		pPF->pfd.cBlueBits		= cBlueBits;
		pPF->pfd.cAlphaBits		= cAlphaBits;
		_BitsFromDepthStencilFormat(fmt[i], &pPF->pfd.cDepthBits, &pPF->pfd.cStencilBits);
		pPF->dwDriverData		= fmt[i];
	}

	// Popup warning message if non RGB color mode
	{
		// This is a hack. KeithH
		HDC hdcDesktop = GetDC(NULL);
		DWORD dwDisplayBitDepth = GetDeviceCaps(hdcDesktop, BITSPIXEL);
		ReleaseDC(0, hdcDesktop);
		if (dwDisplayBitDepth <= 8) {
			ddlogPrintf(DDLOG_WARN, "Current Color Depth %d bpp is not supported", dwDisplayBitDepth);
			MessageBox(NULL, szColorDepthWarning, "GLDirect", MB_OK | MB_ICONWARNING);
		}
	}

	// Mark list as 'current'
	glb.bPixelformatsDirty = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldInitialiseMesa_DX(
	DGL_ctx *lpCtx)
{
	GLD_driver_dx9	*gld = NULL;
	int				MaxTextureSize, TextureLevels;
	BOOL			bSoftwareTnL;

	if (lpCtx == NULL)
		return FALSE;

	gld = lpCtx->glPriv;
	if (gld == NULL)
		return FALSE;

	if (glb.bMultitexture) {
		lpCtx->glCtx->Const.MaxTextureUnits = gld->d3dCaps9.MaxSimultaneousTextures;
		// Only support MAX_TEXTURE_UNITS texture units.
		// ** If this is altered then the FVF formats must be reviewed **.
		if (lpCtx->glCtx->Const.MaxTextureUnits > GLD_MAX_TEXTURE_UNITS_DX9)
			lpCtx->glCtx->Const.MaxTextureUnits = GLD_MAX_TEXTURE_UNITS_DX9;
	} else {
		// Multitexture override
		lpCtx->glCtx->Const.MaxTextureUnits = 1;
	}

	// max texture size
	MaxTextureSize = min(gld->d3dCaps9.MaxTextureHeight, gld->d3dCaps9.MaxTextureWidth);
	if (MaxTextureSize == 0)
		MaxTextureSize = 256; // Sanity check

	//
	// HACK!!
	if (MaxTextureSize > 1024)
		MaxTextureSize = 1024; // HACK - CLAMP TO 1024
	// HACK!!
	//

	// Got to set MAX_TEXTURE_SIZE as max levels.
	// Who thought this stupid idea up? ;)
	TextureLevels = 0;
	// Calculate power-of-two.
	while (MaxTextureSize) {
		TextureLevels++;
		MaxTextureSize >>= 1;
	}
	lpCtx->glCtx->Const.MaxTextureLevels = (TextureLevels) ? TextureLevels : 8;

	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_LIGHTING, FALSE);
	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_DITHERENABLE, TRUE);
	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_ZENABLE,
		(lpCtx->lpPF->dwDriverData!=D3DFMT_UNKNOWN) ? D3DZB_TRUE : D3DZB_FALSE);

	// Set the view matrix
	{
		D3DXMATRIX	vm;
#if 1
		D3DXMatrixIdentity(&vm);
#else
		D3DXVECTOR3 Eye(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 At(0.0f, 0.0f, -1.0f);
		D3DXVECTOR3 Up(0.0f, 1.0f, 0.0f);
		D3DXMatrixLookAtRH(&vm, &Eye, &At, &Up);
		vm._31 = -vm._31;
		vm._32 = -vm._32;
		vm._33 = -vm._33;
		vm._34 = -vm._34;
#endif
		IDirect3DDevice9_SetTransform(gld->pDev, D3DTS_VIEW, &vm);
	}

	if (gld->bHasHWTnL) {
		if (glb.dwTnL == GLDS_TNL_DEFAULT)
			bSoftwareTnL = FALSE; // HW TnL
		else {
			bSoftwareTnL = ((glb.dwTnL == GLDS_TNL_MESA) || (glb.dwTnL == GLDS_TNL_D3DSW)) ? TRUE : FALSE;
		}
	} else {
		// No HW TnL, so no choice possible
		bSoftwareTnL = TRUE;
	}
//	IDirect3DDevice9_SetRenderState(gld->pDev, D3DRS_SOFTWAREVERTEXPROCESSING, bSoftwareTnL);
	IDirect3DDevice9_SetSoftwareVertexProcessing(gld->pDev, bSoftwareTnL);

// Dump this in a Release build as well, now.
//#ifdef _DEBUG
	ddlogPrintf(DDLOG_INFO, "HW TnL: %s",
		gld->bHasHWTnL ? (bSoftwareTnL ? "Disabled" : "Enabled") : "Unavailable");
//#endif

	gldEnableExtensions_DX9(lpCtx->glCtx);
	gldInstallPipeline_DX9(lpCtx->glCtx);
	gldSetupDriverPointers_DX9(lpCtx->glCtx);

	// Signal a complete state update
	lpCtx->glCtx->Driver.UpdateState(lpCtx->glCtx, _NEW_ALL);

	// Start a scene
	IDirect3DDevice9_BeginScene(gld->pDev);
	lpCtx->bSceneStarted = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------

BOOL gldSwapBuffers_DX(
	DGL_ctx *ctx,
	HDC hDC,
	HWND hWnd)
{
	HRESULT			hr;
	GLD_driver_dx9	*gld = NULL;

	if (ctx == NULL)
		return FALSE;

	gld = ctx->glPriv;
	if (gld == NULL)
		return FALSE;

	if (ctx->bSceneStarted) {
		IDirect3DDevice9_EndScene(gld->pDev);
		ctx->bSceneStarted = FALSE;
	}

	// Swap the buffers. hWnd may override the hWnd used for CreateDevice()
	hr = IDirect3DDevice9_Present(gld->pDev, NULL, NULL, hWnd, NULL);

exit_swap:

	IDirect3DDevice9_BeginScene(gld->pDev);
	ctx->bSceneStarted = TRUE;

// Debugging code
#ifdef _DEBUG
//	ddlogMessage(GLDLOG_WARN, "SwapBuffers\n");
#endif

	return (FAILED(hr)) ? FALSE : TRUE;
}

//---------------------------------------------------------------------------

BOOL gldGetDisplayMode_DX(
	DGL_ctx *ctx,
	GLD_displayMode *glddm)
{
	D3DDISPLAYMODE	d3ddm;
	HRESULT			hr;
	GLD_driver_dx9	*lpCtx = NULL;
	BYTE cColorBits, cRedBits, cGreenBits, cBlueBits, cAlphaBits;

	if ((glddm == NULL) || (ctx == NULL))
		return FALSE;

	lpCtx = ctx->glPriv;
	if (lpCtx == NULL)
		return FALSE;

	if (lpCtx->pD3D == NULL)
		return FALSE;

	hr = IDirect3D9_GetAdapterDisplayMode(lpCtx->pD3D, glb.dwAdapter, &d3ddm);
	if (FAILED(hr))
		return FALSE;

	// Get info from the display format
	_BitsFromDisplayFormat(d3ddm.Format,
		&cColorBits, &cRedBits, &cGreenBits, &cBlueBits, &cAlphaBits);

	glddm->Width	= d3ddm.Width;
	glddm->Height	= d3ddm.Height;
	glddm->BPP		= cColorBits;
	glddm->Refresh	= d3ddm.RefreshRate;

	return TRUE;
}

//---------------------------------------------------------------------------

