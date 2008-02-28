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
#ifndef _D3D_HAL_INC
#define _D3D_HAL_INC
   
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <stdlib.h>
#include <time.h>
#include "D3DShared.h"
#include "D3DTextureMgr.h"
#include "Debug.h"
/*===========================================================================*/
/* Defines.                                                                  */
/*===========================================================================*/
#define	DX_RESTORE(ps)			if ( (ps) && (ps)->IsLost() ) (ps)->Restore();
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
typedef struct _d3d_hal_struct
{
  MESAD3DSHARED		shared;

  GUID				guid;
  LPDIRECTDRAW  		lpDD;
  LPDIRECTDRAW4        	lpDD4;
  LPDIRECT3D3          	lpD3D3;
  LPDIRECT3DDEVICE3    	lpD3DDevice;
  D3DDEVICEDESC 	 	D3DHWDevDesc;
  LPDIRECTDRAWSURFACE4 	lpDDSPrimary,
                         lpDDSRender,
                         lpDDSZbuffer;
  LPDIRECT3DVIEWPORT3  	lpViewport;
  LPDIRECTDRAWCLIPPER	lpClipper;
  DDPIXELFORMAT        	ddpf,
                         ddpfZBuffer;
  PTM_OBJECT           	pTMList;

} MESAD3DHAL, *PMESAD3DHAL;
/*===========================================================================*/
/* External function prototypes.                                             */
/*===========================================================================*/
extern BOOL InitTMgrHAL( PMESAD3DHAL pHAL );
extern void TermTMgrHAL( PMESAD3DHAL pHAL );
extern void AlphaBlendTableHAL( PMESAD3DHAL pHAL );

extern void Solve8BitChannelPixelFormat( DDPIXELFORMAT *pddpf, PPIXELINFO pPixel );
extern char *ErrorStringD3D( HRESULT hr );
extern void  FatalShutDown( PMESAD3DHAL pHAL );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
extern char	*errorMsg;

#endif

