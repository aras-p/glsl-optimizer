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
#ifndef _TEXTURE_MGR_INC
#define _TEXTURE_MGR_INC
   
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <stdlib.h>
#include <stdlib.h>
#include "GL/gl.h"
/*========================================================================*/
/* Defines.                                                               */
/*========================================================================*/
/*========================================================================*/
/* Type defines.                                                          */
/*========================================================================*/
typedef struct _local_texture_object
{
   DWORD                dwName,
                        dwPriority,
                        dwFlags,
                        dwSWidth,
                        dwSHeight,
                        dwVWidth,
                        dwVHeight;
   BOOL                 bLock,
	                   bDirty;		/*  I only update VID on SubImage calls so the system */
                                        /* texture can get invalid.                           */

   LPDIRECT3DDEVICE3    lpD3DDevice;	/* If the device changes we must get new handles... */
   LPDIRECTDRAWSURFACE4 lpDDS_System,
	                   lpDDS_Video;
   LPDIRECT3DTEXTURE2   lpD3DTexture2;

   PIXELINFO            pixel;

   struct _local_texture_object *next;
   struct _local_texture_object *prev;

} TM_OBJECT, *PTM_OBJECT;
/*========================================================================*/
/* Function prototypes.                                                   */
/*========================================================================*/
void APIENTRY  InitTMD3D( void *pVoid );
void APIENTRY  TermTMD3D( void *pVoid );
/*========================================================================*/
/* Global variables declaration.                                          */
/*========================================================================*/

#endif 
