/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                                 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#ifndef D3D_MESA_ALL_H
#define D3D_MESA_ALL_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <stdio.h>
#include <string.h>
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
#define	TM_ACTION_LOAD      0x01
#define 	TM_ACTION_BIND      0x02
#define 	TM_ACTION_UPDATE  	0x04

#define	UM_FATALSHUTDOWN	(WM_USER+42)
/*===========================================================================*/
/* Macros defines.                                                           */
/*===========================================================================*/
#define  ALLOC(cb)            malloc( (cb) )
#define  FREE(p)              { free( (p) ); (p) = NULL; }
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
typedef struct _pixel_convert
{
  int	cb,			/* Count in bytes of one pixel. */
          rShift,		/* Shift count that postions each componet. */
	     gShift,		
	     bShift,		
          aShift;		
  float   rScale,		/* Value that scales a color that ranges 0.0 -> 1.0 */
          gScale,		/* to this pixel format.                            */
          bScale,
          aScale;
  DWORD   dwRMask,		/* Color mask per component. */
	     dwGMask,
          dwBMask,
          dwAMask;

} PIXELINFO, *PPIXELINFO;


typedef struct _d3d_shared_info
{
  HWND		hwnd;
  BOOL    	bWindow,
               bFlipable,
               bForceSW,
               bHardware;
  RECT    	rectW,			/* Window size and postion in screen space. */
               rectV;			/* Viewport size and postion. */
  DWORD		dwWidth,			/* Current render size for quick checks. */
               dwHeight;

  PIXELINFO	pixel;			
  DWORD		dwSrcBlendCaps[14],	/* See D3DCAPS.CPP */
               dwDestBlendCaps[14],
               dwTexFunc[4];

} MESAD3DSHARED, *PMESAD3DSHARED;

typedef struct _render_options
{
  BOOL	bForceSoftware,		/* TODO: Add user switches. */
	     bStretchtoPrimary;

} USER_CTRL, *PUSER_CRTL;

enum { s_zero = 0,				
	  s_one, 
	  s_dst_color, 
	  s_one_minus_dst_color, 
	  s_src_alpha, 
	  s_one_minus_src_alpha, 
	  s_dst_alpha,
	  s_one_minus_dst_alpha,
	  s_src_alpha_saturate, 
	  s_constant_color, 
	  s_one_minus_constant_color, 
	  s_constant_alpha, 
	  s_one_minus_constant_alpha };

enum { d_zero = 0, 
	  d_one, 
	  d_src_color, 
	  d_one_minus_src_color, 
	  d_src_alpha, 
	  d_one_minus_src_alpha, 
	  d_dst_alpha,
	  d_one_minus_dst_alpha,
	  d_constant_color,
	  d_one_minus_constant_color,
	  d_constant_alpha,
	  d_one_minus_constant_alpha };

enum { d3dtblend_decal = 0,
	  d3dtblend_decalalpha,
	  d3dtblend_modulate,
	  d3dtblend_modulatealpha };

/*===========================================================================*/
/* Function prototypes.                                                      */
/*===========================================================================*/
PMESAD3DSHARED	InitHAL( HWND hwnd );
void 	   	TermHAL( PMESAD3DSHARED pShared );
BOOL 	   	CreateHAL( PMESAD3DSHARED pShared );
BOOL 	   	SetViewportHAL( PMESAD3DSHARED pShared, RECT *pRect, float minZ, float maxZ  );

void ClearHAL( PMESAD3DSHARED pShared, DWORD dwFlags, BOOL bAll, int x, int y, int cx, int cy, DWORD dwColor, float zv, DWORD dwStencil );
void SetStateHAL( PMESAD3DSHARED pShared, DWORD dwType, DWORD dwState );
void DrawPrimitiveHAL( PMESAD3DSHARED pShared, D3DPRIMITIVETYPE dptPrimitiveType, D3DTLVERTEX *pVertices, DWORD dwCount );

void 		SwapBuffersHAL( PMESAD3DSHARED pShared );
DDSURFACEDESC2 *LockHAL( PMESAD3DSHARED pShared, BOOL bBack );
void 		UnlockHAL( PMESAD3DSHARED pShared, BOOL bBack );
void 		UpdateScreenPosHAL( PMESAD3DSHARED pShared );
void			GetPixelInfoHAL( PMESAD3DSHARED pShared, PPIXELINFO pPixel );
BOOL CreateTMgrHAL( PMESAD3DSHARED pShared, DWORD dwName, int level, DWORD dwRequestFlags, RECT *rectDirty, DWORD dwWidth, DWORD dwHeight, DWORD dwAction, void *pPixels );
void DisableTMgrHAL( PMESAD3DSHARED pShared );


int  SaveDIBitmap( char *filename, BITMAPINFO *info, void *bits );
int	ARGB_SaveBitmap( char *filename, int width, int height, unsigned char *pARGB );
int  BGRA_SaveBitmap( char *filename, int width, int height, unsigned char *pBGRA );
int  BGR_SaveBitmap( char *filename, int width, int height, unsigned char *pBGR );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
extern float	g_DepthScale,	/* Mesa needs to scale Z in SW.  The HAL */
               g_MaxDepth;    /* doesn't but I wanted SW still to work.*/

#ifdef __cplusplus
}
#endif

#endif



