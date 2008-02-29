/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                       Build 5   */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include "D3DHAL.h"
/*===========================================================================*/
/* Local function prototypes.                                                */
/*===========================================================================*/
static void	DestroyAllSurfaces( PMESAD3DHAL pHAL );
static void	DestroyDevice( PMESAD3DHAL pHAL );
static void	DestroyInterfaces( PMESAD3DHAL pHAL );

HRESULT WINAPI   EnumSurfacesHook( LPDIRECTDRAWSURFACE4 lpDDS, LPDDSURFACEDESC2 lpDDSDesc, LPVOID pVoid );
HRESULT CALLBACK EnumZBufferHook( DDPIXELFORMAT* pddpf, VOID *pVoid );
HRESULT CALLBACK EnumDeviceHook( GUID FAR* lpGuid, LPSTR lpDesc, LPSTR lpName, LPD3DDEVICEDESC lpD3DHWDesc, LPD3DDEVICEDESC lpD3DHELDesc,  void *pVoid );
/*===========================================================================*/
/* Globals.                                                                  */
/*===========================================================================*/
//char		*errorMsg;
/*===========================================================================*/
/*  This function is responable for allocating the actual MESAD3DHAL struct. */
/* Each Mesa context will have its own MESAD3DHAL struct so its like a mini  */
/* context to some extent. All one time allocations/operations get done here.*/
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
extern "C" PMESAD3DSHARED InitHAL( HWND hwnd )
{
  PMESAD3DHAL	pHAL;
  ULONG     	rc;

  DPF(( DBG_FUNC, "InitHAL();" ));
  DPF(( DBG_CNTX_INFO, "hwnd: %d", hwnd ));

  /* Allocate the structure and zero it out. */   
  pHAL = (PMESAD3DHAL)ALLOC( sizeof(MESAD3DHAL) );
  if ( pHAL == NULL )
  {   
    RIP( pHAL, "InitHAL->", "Memory Allocation" );
    return (PMESAD3DSHARED)NULL;
  }
  memset( pHAL, 0, sizeof(MESAD3DHAL) );

  /* Get the texture manager going. */
  rc = InitTMgrHAL( pHAL );
  if ( rc == FALSE )
  {   
    RIP( pHAL, "InitTMgrHAL->", "Failed" );
    return (PMESAD3DSHARED)NULL;
  }

  /* Fill in the window parameters if we can. */
  pHAL->shared.hwnd = hwnd;

  /* Parse the user's enviroment variables to generate a debug mask. */
  ReadDBGEnv();
  
  return (PMESAD3DSHARED)pHAL;
}
/*===========================================================================*/
/*  This function will unload all the resources that the MESAD3DHAL struct   */
/* has bound to it.  The actual structure itself will be freed.              */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void TermHAL( PMESAD3DSHARED pShared )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;

  DPF(( DBG_FUNC, "TermHAL();" ));

  /* Check for an empty wrapper structure. */
  if ( pHAL == NULL )
    return;

  /* Kill this texture manager. */
  TermTMgrHAL( pHAL );

  /* Kill any DDraw stuff if exists. */
  DestroyDevice( pHAL );
  DestroyAllSurfaces( pHAL );
  DestroyInterfaces( pHAL );

  FREE( pHAL );
}
/*===========================================================================*/
/*  This function is used to init and resize the rendering surface as the two*/
/* are almost the same.  First the device and all the surfaces are destoryed */
/* if they already exist.  Next we create a OffScreen rendering surface and  */
/* save some pixelformat info to do color convertions. Next we start to take */
/* care of getting the most out of the hardware. I use bHardware to determine*/
/* the state of the device we found in the device enumeration.  The enum proc*/
/* will try for hardware first.  I next use a bForceSW to make the enum proc */
/* choose a software device.  So I will try some combinations with HW first  */
/* until I feel I have to set the bForceSW and call this function again.  If */
/* this function is called with no width or height then use the internals.   */
/*   NOTE:  The worst case is that all will be in SW (RGBDevice) and really  */
/*         I should forget the whole thing and fall back to a DDraw span type*/
/*         rendering but what is the point.  This way I always know I have a */
/*         D3DDevice and that makes things easier.  I do impliment the span  */
/*         rendering function for stuff that I haven't done support for such */
/*         as points and lines.                                              */
/*===========================================================================*/
/* RETURN: TRUE, FALSE                                                       */
/*===========================================================================*/
extern "C" BOOL	CreateHAL( PMESAD3DSHARED pShared )
{
  PMESAD3DHAL 		pHAL = (PMESAD3DHAL)pShared;
  DDSURFACEDESC2	ddsd2;
  D3DDEVICEDESC  	D3DSWDevDesc;
  DDSCAPS2 	  	ddscaps;
  DWORD          	dwCoopFlags,
                    dwWidth,
                    dwHeight;
  ULONG          	rc;

  DPF(( DBG_FUNC, "CreateHAL();" ));

#define InitDDSD2(f)      memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) ); \
                          ddsd2.dwSize  = sizeof( DDSURFACEDESC2 ); \
                          ddsd2.dwFlags = f;

  if ( pHAL == NULL )
    return FALSE;

  /* Use the internal rectangle struct. */
  dwWidth  = pShared->rectW.right - pShared->rectW.left;
  dwHeight = pShared->rectW.bottom - pShared->rectW.top;

  DPF(( DBG_CNTX_INFO, "Width: %d Height: %d", dwWidth, dwHeight ));

  /* The dimensions might still be the same so just leave. */
  if ( (dwWidth == pShared->dwWidth) && (dwHeight == pShared->dwHeight) )
  {
    DPF(( DBG_CNTX_WARN, "Context size hasn't changed" ));
    return TRUE;
  }

  /* If one of the dimensions are zero then leave. WM_SIZE should get us back here. */
  if ( (dwWidth == 0) || (dwHeight == 0) )
    return TRUE;

  /* Save the renders dimensions. */
  pShared->dwWidth  = dwWidth;
  pShared->dwHeight = dwHeight;

  DPF(( DBG_CNTX_INFO, "Creating Context:\n cx:%d cy:%d", pShared->dwWidth, pShared->dwHeight ));

  /*=================================*/
  /* Create all required interfaces. */
  /*=================================*/

  /* Kill any DDraw stuff if exists. */
  DestroyDevice( pHAL );
  DestroyAllSurfaces( pHAL );
  DestroyInterfaces( pHAL );

  /* Create a instance of DDraw using the Primary display driver. */
  rc = DirectDrawCreate( NULL, &pHAL->lpDD, NULL );
  if( FAILED(rc) )
  {
    RIP( pHAL, "DirectDrawCreate->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Get the DDraw4 interface. */
  rc = pHAL->lpDD->QueryInterface( IID_IDirectDraw4, (void **)&pHAL->lpDD4 );
  if( FAILED(rc) )
  {
    RIP( pHAL, "QueryInterface (IID_IDirectDraw4) ->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Get the Direct3D3 interface. */
  rc = pHAL->lpDD4->QueryInterface( IID_IDirect3D3, (void **)&pHAL->lpD3D3 );
  if( FAILED(rc) )
  {
    RIP( pHAL, "QueryInterface (IID_IDirect3D3) ->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Set the Cooperative level. NOTE: we need to know if we are FS at this point.*/
  dwCoopFlags = (pShared->bWindow == TRUE) ? DDSCL_NORMAL : (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
  rc = pHAL->lpDD4->SetCooperativeLevel( pShared->hwnd, dwCoopFlags );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "SetCooperativeLevel->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /*==================================================================*/
  /* Get the best device we can and note whether its hardware or not. */
  /*==================================================================*/
  pShared->bForceSW = FALSE;
  pHAL->lpD3D3->EnumDevices( EnumDeviceHook, (void *)pHAL );
  pShared->bHardware = IsEqualIID( pHAL->guid, IID_IDirect3DHALDevice );
  DPF(( DBG_CNTX_INFO, "bHardware: %s", (pShared->bHardware) ? "TRUE" : "FALSE" ));
  DPF(( DBG_CNTX_INFO, "bWindowed: %s", (pShared->bWindow) ? "TRUE" : "FALSE" ));

  /*========================================================================*/
  /* HARDWARE was found.                                                    */
  /*========================================================================*/
  if ( pShared->bHardware == TRUE )
  {
    /*===================================*/
    /* HARDWARE -> Z-BUFFER.             */
    /*===================================*/

    /* Get a Z-Buffer pixelformat. */
    memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
    ddsd2.dwSize = sizeof( DDSURFACEDESC2 );
    rc = pHAL->lpD3D3->EnumZBufferFormats( pHAL->guid, EnumZBufferHook, (VOID*)&ddsd2.ddpfPixelFormat );
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "EnumZBufferFormatsl->", ErrorStringD3D(rc) );
	 return FALSE;
    }
        
    /* Setup our request structure for the Z-buffer surface. */
    ddsd2.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
    ddsd2.dwWidth        = dwWidth;
    ddsd2.dwHeight       = dwHeight;
    rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSZbuffer, NULL );
    if ( !FAILED(rc) )
    {
	 DPF(( DBG_CNTX_INFO, "HW ZBuffer" ));

	 /*===================================*/
	 /* HARDWARE -> Z-BUFFER -> FLIPABLE  */
	 /*===================================*/
	 if ( pShared->bWindow == FALSE )
	 {
	   InitDDSD2( DDSD_CAPS | DDSD_BACKBUFFERCOUNT );
	   ddsd2.dwBackBufferCount = 1;
	   ddsd2.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	   rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSPrimary, NULL );
	   if ( FAILED(rc) )
	   {	
		/* Make sure we try the next fall back. */
		DPF(( DBG_CNTX_WARN, "HW Flip/Complex not available" ));
		pHAL->lpDDSPrimary = NULL;
	   }
	   else
	   {
		/* Get the back buffer that was created. */
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		rc = pHAL->lpDDSPrimary->GetAttachedSurface( &ddscaps, &pHAL->lpDDSRender );
		if ( FAILED(rc) )
		{	
		  DPF(( DBG_CNTX_WARN, "GetAttachedSurface failed -> HW Flip/Complex" ));
		  
		  /* Make sure we try the next fall back. */
 		  pHAL->lpDDSPrimary->Release();
		  pHAL->lpDDSPrimary = NULL;
		}	
		else
		{
		  /*  I have had problems when a complex surface comes back  */
		  /* with the back buffer being created in SW.  Not sure why */
		  /* or how this is possable but I'm checking for it here.   */
		  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) ); 
		  ddsd2.dwSize = sizeof( DDSURFACEDESC2 );
		  DX_RESTORE( pHAL->lpDDSRender );
		  rc = pHAL->lpDDSRender->GetSurfaceDesc( &ddsd2 );
		  if ( FAILED(rc) )
		  {
		    RIP( pHAL, "GetSurfaceDesc (RENDER) ->", ErrorStringD3D(rc) );
		    return FALSE;
		  }

		  /* If the surface is in VID then we are happy with are Flipable. */
		  if ( ddsd2.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM )
		  {
		    pShared->bFlipable = TRUE;
		    DPF(( DBG_CNTX_INFO, "HW Flip/Complex!" ));
		  }
		  else
		  {
		    /* Kill this setup. */
		    pHAL->lpDDSPrimary->Release();
		    pHAL->lpDDSPrimary = NULL;
		  }
		}
	   }
	 }

	 /*===================================*/
	 /* HARDWARE -> Z-BUFFER -> BLT       */
	 /*===================================*/
	 if ( pHAL->lpDDSPrimary == NULL )
	 {
	   pShared->bFlipable = FALSE;

	   /* Create the Primary (front buffer). */
	   InitDDSD2( DDSD_CAPS );
	   ddsd2.ddsCaps.dwCaps  = DDSCAPS_PRIMARYSURFACE;
	   rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSPrimary, NULL );
	   if ( FAILED(rc) )
	   {
		/* This is an error as we should be able to do this at minimum. */
		RIP( pHAL, "CreateSurface (PRIMARY) ->", ErrorStringD3D(rc) );
		return FALSE;
	   }

	   /* Create the Render (back buffer). */
	   InitDDSD2( DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT );
	   ddsd2.dwWidth	    = dwWidth;
	   ddsd2.dwHeight	    = dwHeight;
	   ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	   rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSRender, NULL );
	   if ( FAILED(rc) )
	   {
		DPF(( DBG_CNTX_WARN, "Failed HW Offscreen surface" ));

		/* Make sure we try the next fall back. */
		pHAL->lpDDSPrimary->Release();
		pHAL->lpDDSPrimary = NULL;
	   }
	   else
	   {
		/*  Might as well check here too see if this surface is in */
		/* hardware.  If nothing else just to be consistant.       */
		memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) ); 
		ddsd2.dwSize = sizeof( DDSURFACEDESC2 );
		DX_RESTORE( pHAL->lpDDSRender );
		rc = pHAL->lpDDSRender->GetSurfaceDesc( &ddsd2 );
		if ( FAILED(rc) )
		{
		  RIP( pHAL, "GetSurfaceDesc (RENDER) ->", ErrorStringD3D(rc) );
		  return FALSE;
		}

		/* If the surface is in VID then we are happy. */
		if ( ddsd2.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM )
		{
		  /*  Create a clipper object so that DDraw will be able to blt windows that */
		  /* have been clipped by the screen or other windows.                       */
		  pHAL->lpDD4->CreateClipper( 0, &pHAL->lpClipper, NULL );
		  pHAL->lpClipper->SetHWnd( 0, pShared->hwnd );
		  pHAL->lpDDSPrimary->SetClipper( pHAL->lpClipper );
		  pHAL->lpClipper->Release();
		  DPF(( DBG_CNTX_INFO, "HW RENDER surface" ));
		}
		else
		{
		  /* Kill this setup. */
		  pHAL->lpDDSRender->Release();
		  pHAL->lpDDSRender = NULL;
		  pHAL->lpDDSPrimary->Release();
		  pHAL->lpDDSPrimary = NULL;
		}
	   }
	 }	

	 /*===================================*/
	 /* Create D3DDEVICE -> HARDWARE.     */
	 /*===================================*/
	 if ( pHAL->lpDDSZbuffer && pHAL->lpDDSPrimary && pHAL->lpDDSRender )
	 {
	   DX_RESTORE( pHAL->lpDDSRender );
	   DX_RESTORE( pHAL->lpDDSZbuffer );

	   rc = pHAL->lpDDSRender->AddAttachedSurface( pHAL->lpDDSZbuffer );
	   if ( FAILED(rc) )
	   {
		RIP( pHAL, "AddAttachedSurface (ZBUFFER) ->", ErrorStringD3D(rc) );
		return FALSE;
	   }

	   rc = pHAL->lpD3D3->CreateDevice( IID_IDirect3DHALDevice, pHAL->lpDDSRender, &pHAL->lpD3DDevice, NULL );
	   if ( rc != D3D_OK )
	   {
		DPF(( DBG_CNTX_WARN, "Failed HW Device" ));
		pHAL->lpD3DDevice = NULL;
	   }
	   else
	   {
		DPF(( DBG_CNTX_INFO, "HW Device" ));
	   }
	 }
    }	
  }
      
  /*========================================================================*/
  /* SOFTWARE fallback.                                                     */
  /*========================================================================*/
  if ( pHAL->lpD3DDevice == NULL )
  {
    DPF(( DBG_CNTX_INFO, "SW fallback :(" ));

    /* Make sure we have no surfaces allocated.  Just incase. */
    DestroyAllSurfaces( pHAL );

    /* Get a software device. */
    pShared->bFlipable = FALSE;
    pShared->bForceSW = TRUE;
    pHAL->lpD3D3->EnumDevices( EnumDeviceHook, (void *)pHAL );
    pShared->bHardware = IsEqualIID( pHAL->guid, IID_IDirect3DHALDevice );

     /*===================================*/
    /* SOFTWARE -> Z-BUFFER.             */
    /*===================================*/

    /*===================================*/
    /* SOFTWARE -> Z-BUFFER -> FLIPABLE  */
    /*===================================*/
    if ( pShared->bWindow == FALSE )
    {
	 InitDDSD2( DDSD_CAPS | DDSD_BACKBUFFERCOUNT );
	 ddsd2.dwBackBufferCount = 1;
	 ddsd2.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	 ddsd2.ddpfPixelFormat.dwSize  = sizeof( DDPIXELFORMAT );
	 ddsd2.ddpfPixelFormat.dwFlags = (DDPF_RGB | DDPF_ALPHAPIXELS);
	 rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSPrimary, NULL );
	 if ( FAILED(rc) )
	 {	
	   DPF(( DBG_CNTX_WARN, "Failed SW Flip/Complex" ));

	   /* Make sure we try the next fall back. */
	   pHAL->lpDDSPrimary = NULL;
	 }
	 else
	 {
	   ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	   rc = pHAL->lpDDSPrimary->GetAttachedSurface( &ddscaps, &pHAL->lpDDSRender );
	   if ( FAILED(rc) )
	   {	
		/* Make sure we try the next fall back. */
		DPF(( DBG_CNTX_WARN, "GetAttachedSurface failed -> SW Flip/Complex" ));
		pHAL->lpDDSPrimary->Release();
		pHAL->lpDDSPrimary = NULL;
	   }	
	   else
	   {
		DPF(( DBG_CNTX_INFO, "SW Flip/Complex" ));
		pShared->bFlipable = TRUE;
	   }
	 }
    }		

    /*===================================*/
    /* SOFTWARE -> Z-BUFFER -> BLT       */
    /*===================================*/
    if ( pHAL->lpDDSPrimary == NULL )
    {
	 /* Create the Primary (front buffer). */
	 InitDDSD2( DDSD_CAPS );
	 ddsd2.ddsCaps.dwCaps  = DDSCAPS_PRIMARYSURFACE;
	 rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSPrimary, NULL );
	 if ( FAILED(rc) )
	 {
	   /* This is an error as we should be able to do this at minimum. */
	   RIP( pHAL, "CreateSurface (PRIMARY) ->", ErrorStringD3D(rc) );
	   return FALSE;
	 }

	 /* Create the Render (back buffer). */
	 InitDDSD2( DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT );
	 ddsd2.dwWidth					= dwWidth;
	 ddsd2.dwHeight				= dwHeight;
	 ddsd2.ddsCaps.dwCaps 			= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	 ddsd2.ddpfPixelFormat.dwSize 	= sizeof( DDPIXELFORMAT );
	 ddsd2.ddpfPixelFormat.dwFlags	= (DDPF_RGB | DDPF_ALPHAPIXELS);
	 rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pHAL->lpDDSRender, NULL );
	 if ( FAILED(rc) )
	 {
	   /* That was our last hope. */
	   RIP( pHAL, "CreateSurface (RENDER) ->", ErrorStringD3D(rc) );
	   return FALSE;
	 }
	 else
	 {
	   DPF(( DBG_CNTX_INFO, "SW RENDER surface" ));

	   /*  Create a clipper object so that DDraw will be able to blt windows that */
	   /* have been clipped by the screen or other windows.                       */
	   pHAL->lpDD4->CreateClipper( 0, &pHAL->lpClipper, NULL );
	   pHAL->lpClipper->SetHWnd( 0, pShared->hwnd );
	   pHAL->lpDDSPrimary->SetClipper( pHAL->lpClipper );
	   pHAL->lpClipper->Release();
	 }
    }		
	
    /*===================================*/
    /* Create D3DDEVICE -> SOFTWARE.     */
    /*===================================*/
    if ( pHAL->lpDDSPrimary && pHAL->lpDDSRender )
    {
	 DX_RESTORE( pHAL->lpDDSRender );
	 rc = pHAL->lpD3D3->CreateDevice( IID_IDirect3DRGBDevice, pHAL->lpDDSRender, &pHAL->lpD3DDevice, NULL );
	 if ( rc != D3D_OK )
	 {
	   /* That was our last hope. */
	   RIP( pHAL, "CreateDevice (IID_IDirect3DRGBDevice) ->", ErrorStringD3D(rc) );
	   return FALSE;
	 }
	 
	 DPF(( DBG_CNTX_INFO, "SW Device" ));
    }	
  }

  /*==============================================================================*/
  /* Get a copy of the render pixelformat so that wgl.c can call GetPixelInfoD3D. */
  /*==============================================================================*/
  memset( &pHAL->ddpf, 0, sizeof(DDPIXELFORMAT) );
  pHAL->ddpf.dwSize = sizeof( DDPIXELFORMAT );
  rc = pHAL->lpDDSRender->GetPixelFormat( &pHAL->ddpf );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "GetPixelFormat ->", ErrorStringD3D(rc) );
    return FALSE;
  }
  DebugPixelFormat( "Using OFFSCREEN", &pHAL->ddpf );
  DebugPixelFormat( "Using ZBUFFER", &ddsd2.ddpfPixelFormat );

  /* Get a copy of what the D3DDevice supports for later use. */
  memset( &D3DSWDevDesc, 0, sizeof(D3DDEVICEDESC) );
  memset( &pHAL->D3DHWDevDesc, 0, sizeof(D3DDEVICEDESC) );
  D3DSWDevDesc.dwSize       = sizeof( D3DDEVICEDESC );
  pHAL->D3DHWDevDesc.dwSize = sizeof( D3DDEVICEDESC );
  rc = pHAL->lpD3DDevice->GetCaps( &pHAL->D3DHWDevDesc, &D3DSWDevDesc );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "GetCaps ->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Get a copy of the pixel convertion stuff for direct buffer access. */
  Solve8BitChannelPixelFormat( &pHAL->ddpf, &pShared->pixel );
  AlphaBlendTableHAL( pHAL );

  /* We must prime the Begin/End scene for SwapBuffers to work. */
  rc = pHAL->lpD3DDevice->BeginScene();   
  if ( FAILED(rc) )
  {
    RIP( pHAL, "BeginScene ->", ErrorStringD3D(rc) );
    return FALSE;
  }

#undef InitDDSD2

  return TRUE;
}
/*===========================================================================*/
/*  This function will make sure a viewport is created and set for the device*/
/* in the supplied structure.  If a rect is supplied then it will be used for*/
/* the viewport otherwise the current setting in the strucute will be used.  */
/* Note that the rect is relative to the window.  So left/top must be 0,0 to */
/* use the whole window else there is scissoring going down.                 */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
extern "C" BOOL SetViewportHAL( PMESAD3DSHARED pShared, RECT *pRect, float minZ, float maxZ  )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;
  D3DVIEWPORT2 vdData;
  ULONG		rc;
  POINT		pt;

  DPF(( DBG_FUNC, "SetViewportHAL();" ));

  /* Make sure we have enough info. */
  if ( !pHAL || !pHAL->lpDDSPrimary || !pHAL->lpD3DDevice )
  {
    DPF(( DBG_CNTX_WARN, "SetViewport() -> NULL Pointer" ));
    return FALSE;
  }

  /* TODO: this is just a temp fix to stop redundant changes. */
  if ( pRect &&
	  (pShared->rectV.left   == pRect->left)    &&
	  (pShared->rectV.right  == pRect->right)   &&
	  (pShared->rectV.top    == pRect->top)     &&
	  (pShared->rectV.bottom == pRect->bottom) )
  {
    DPF(( DBG_CNTX_WARN, "Redundant viewport" ));
    return TRUE;
  }

  DPF(( DBG_CNTX_INFO, "Current Viewport:" ));
  DPF(( DBG_CNTX_INFO, "x: %d y: %d", pShared->rectV.left, pShared->rectV.top ));
  DPF(( DBG_CNTX_INFO, "cx: %d cy: %d", (pShared->rectV.right-pShared->rectV.left), (pShared->rectV.bottom-pShared->rectV.top) ));
  DPF(( DBG_CNTX_INFO, "New Viewport:" ));
  DPF(( DBG_CNTX_INFO, "x: %d y: %d", pRect->left, pRect->top ));
  DPF(( DBG_CNTX_INFO, "cx: %d cy: %d", (pRect->right-pRect->left), (pRect->bottom-pRect->top) ));

  /* Update the current viewport rect if one is supplied. */
  if ( pRect )      
    memcpy( &pShared->rectV, pRect, sizeof(RECT) );
	 
  /* Build the request structure. */
  memset( &vdData, 0, sizeof(D3DVIEWPORT2) );
  vdData.dwSize   = sizeof(D3DVIEWPORT2);  
  vdData.dwX      = pShared->rectV.left;
  vdData.dwY      = pShared->rectV.top;
  vdData.dwWidth  = (pShared->rectV.right - pShared->rectV.left);
  vdData.dwHeight = (pShared->rectV.bottom - pShared->rectV.top);

  if ( !vdData.dwWidth || !vdData.dwHeight )
  {
    GetClientRect( pShared->hwnd, &pShared->rectW );
    pt.x = pt.y = 0;
    ClientToScreen( pShared->hwnd, &pt );
    OffsetRect( &pShared->rectW, pt.x, pt.y);
    vdData.dwX      = pShared->rectW.left;
    vdData.dwY      = pShared->rectW.top;
    vdData.dwWidth  = (pShared->rectW.right - pShared->rectW.left);
    vdData.dwHeight = (pShared->rectW.bottom - pShared->rectW.top);
    memcpy( &pShared->rectV, &pShared->rectW, sizeof(RECT) );
  }

  // The dvClipX, dvClipY, dvClipWidth, dvClipHeight, dvMinZ, 
  // and dvMaxZ members define the non-normalized post-perspective 
  // 3-D view volume which is visible to the viewer. In most cases, 
  // dvClipX is set to -1.0 and dvClipY is set to the inverse of 
  // the viewport's aspect ratio on the target surface, which can be 
  // calculated by dividing the dwHeight member by dwWidth. Similarly, 
  // the dvClipWidth member is typically 2.0 and dvClipHeight is set 
  // to twice the aspect ratio set in dwClipY. The dvMinZ and dvMaxZ 
  // are usually set to 0.0 and 1.0.
  vdData.dvClipX      = -1.0f;
  vdData.dvClipWidth  = 2.0f;
  vdData.dvClipY      = 1.0f;
  vdData.dvClipHeight = 2.0f;
  vdData.dvMaxZ       = maxZ;
  vdData.dvMinZ       = minZ;

  DPF(( DBG_CNTX_INFO, "zMin: %f zMax: %f", minZ, maxZ ));

  /*  I'm going to destroy the viewport everytime as when we size we will */
  /* have a new D3DDevice.  As this area doesn't need to be fast...       */
  if ( pHAL->lpViewport )
  {
    DPF(( DBG_CNTX_INFO, "DeleteViewport" ));

    pHAL->lpD3DDevice->DeleteViewport( pHAL->lpViewport );
    rc = pHAL->lpViewport->Release();
    pHAL->lpViewport = NULL;
  }

  rc = pHAL->lpD3D3->CreateViewport( &pHAL->lpViewport, NULL );
  if ( rc != D3D_OK )
  {
    DPF(( DBG_CNTX_ERROR, "CreateViewport Failed" ));
    return FALSE;
  }

  /* Update the device with the new viewport. */
  pHAL->lpD3DDevice->AddViewport( pHAL->lpViewport );
  pHAL->lpViewport->SetViewport2( &vdData );
  pHAL->lpD3DDevice->SetCurrentViewport( pHAL->lpViewport );

  return TRUE; 
}
/*===========================================================================*/
/*                                                                           */
/*                                                                           */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
HRESULT WINAPI EnumSurfacesHook( LPDIRECTDRAWSURFACE4 lpDDS, LPDDSURFACEDESC2 lpDDSDesc, LPVOID pVoid )
{
  DDSURFACEDESC2 *pddsd2 = (DDSURFACEDESC2 *)pVoid;

  DPF(( DBG_FUNC, "EnumSurfacesHook();" ));

  if ( (lpDDSDesc->ddpfPixelFormat.dwFlags == pddsd2->ddpfPixelFormat.dwFlags) && (lpDDSDesc->ddsCaps.dwCaps == pddsd2->ddsCaps.dwCaps) )
  {
    /* Save the pixelformat now so that we know we have one. */
    memcpy( pddsd2, lpDDSDesc, sizeof(DDSURFACEDESC2) );

    return D3DENUMRET_CANCEL;
  }

  return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This is the callback proc to get a Z-Buffer.  Thats it.                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
HRESULT CALLBACK  EnumZBufferHook( DDPIXELFORMAT* pddpf, VOID *pVoid )
{
  DDPIXELFORMAT  *pddpfChoice = (DDPIXELFORMAT *)pVoid;

  DPF(( DBG_FUNC, "EnumZBufferHook();" ));

  /* If this is ANY type of depth-buffer, stop. */
  if( pddpf->dwFlags == DDPF_ZBUFFER )
  {
    /* Save the pixelformat now so that we know we have one. */
    memcpy( pddpfChoice, pddpf, sizeof(DDPIXELFORMAT) );

    /* I feel if the hardware supports this low then lets use it.  Could get ugly. */
    if( pddpf->dwZBufferBitDepth >= 8 )
    {
	 return D3DENUMRET_CANCEL;
    }
  }
 
   return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This function handles the callback for the D3DDevice enumeration.  Good  */
/* god who's idea was this?  The D3D wrapper has two variable related to what*/
/* kind of device we want and have.  First we have a Bool that is set if we  */
/* have allocated a HW device.  We always look for the HW device first.  The */
/* other variable is used to force SW.  If we have run into a case that we   */
/* want to fallback to SW then we set this.  We will fallback if we cannot   */
/* texture in video memory (among others).                                   */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
HRESULT CALLBACK  EnumDeviceHook( GUID FAR* lpGuid, LPSTR lpDesc, LPSTR lpName, LPD3DDEVICEDESC lpD3DHWDesc, LPD3DDEVICEDESC lpD3DHELDesc,  void *pVoid )
{
  PMESAD3DHAL		pHAL = (PMESAD3DHAL)pVoid;
  LPD3DDEVICEDESC   pChoice = lpD3DHWDesc;

  DPF(( DBG_FUNC, "EnumDeviceHook();" ));

  /* Determine if which device description is valid. */
  if ( pChoice->dcmColorModel == 0 )
    pChoice = lpD3DHELDesc;

  /* Make sure we always have a GUID. */
  memcpy( &pHAL->guid, lpGuid, sizeof(GUID) );

  /* This controls whether we will except HW or not. */
  if ( pHAL->shared.bForceSW == TRUE )
  {
    return (pChoice == lpD3DHELDesc) ? D3DENUMRET_CANCEL : D3DENUMRET_OK;
  }

  /* Always try for hardware. */
  if ( pChoice == lpD3DHWDesc )
  {
    return D3DENUMRET_CANCEL;
  }

  return D3DENUMRET_OK;
}
/*===========================================================================*/
/*  This function will destroy any and all surfaces that this context has    */
/* allocated.  If there is a clipper object then it will also be destoryed as*/
/* it is part of the Primary Surface.                                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void DestroyAllSurfaces( PMESAD3DHAL pHAL )
{
  LONG	refCount;

  DPF(( DBG_FUNC, "DestroyAllSurfaces();" ));

  DX_RESTORE( pHAL->lpDDSPrimary );
  DX_RESTORE( pHAL->lpDDSRender );
  DX_RESTORE( pHAL->lpDDSZbuffer);

  if ( pHAL->lpDDSRender )
  {
    pHAL->lpDDSRender->Unlock( NULL );

    /* If this isn't a Flipable surface then we must clean up the render. */
    if ( pHAL->shared.bFlipable == FALSE)
    {
	 if ( pHAL->lpDDSZbuffer )
	 {
	   DPF(( DBG_CNTX_INFO, "Remove attached surfaces from RENDER" ));
	   pHAL->lpDDSRender->DeleteAttachedSurface( 0, NULL );
	 }

	 DPF(( DBG_CNTX_INFO, "Release RENDER" ));
	 refCount = pHAL->lpDDSRender->Release();
	 pHAL->lpDDSRender = NULL;
    }
  }

  if ( pHAL->lpDDSZbuffer )
  {
    DPF(( DBG_CNTX_INFO, "Release ZBuffer" ));
    pHAL->lpDDSZbuffer->Unlock( NULL );
    refCount = pHAL->lpDDSZbuffer->Release();
    pHAL->lpDDSZbuffer = NULL;
  }

  if ( pHAL->lpClipper )
  {
    DPF(( DBG_CNTX_INFO, "Release Clipper" ));
    refCount = pHAL->lpClipper->Release();
    pHAL->lpClipper = NULL;
  }

  if ( pHAL->lpDDSPrimary )
  {
    pHAL->lpDDSPrimary->Unlock( NULL );

    DPF(( DBG_CNTX_INFO, "Release PRIMARY" ));
    refCount = pHAL->lpDDSPrimary->Release();
    pHAL->lpDDSPrimary = NULL;
  }
}
/*===========================================================================*/
/*  This function will destroy the current D3DDevice and any resources that  */
/* belong to it.                                                             */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void DestroyDevice( PMESAD3DHAL pHAL )
{
  LONG	refCount;

  DPF(( DBG_FUNC, "DestroyDevice();" ));

  /* Kill the D3D stuff if exists. */
  if ( pHAL->lpViewport )
  {
    DPF(( DBG_CNTX_INFO, "Delete Viewport" ));
    pHAL->lpD3DDevice->DeleteViewport( pHAL->lpViewport );

    DPF(( DBG_CNTX_INFO, "Release Viewport" ));
    refCount = pHAL->lpViewport->Release();
    pHAL->lpViewport = NULL;
  }

  if ( pHAL->lpD3DDevice != NULL )
  {
    DPF(( DBG_CNTX_INFO, "Release D3DDevice" ));
    refCount = pHAL->lpD3DDevice->EndScene();  
    refCount = pHAL->lpD3DDevice->Release();
    pHAL->lpD3DDevice = NULL;
  }
}
/*===========================================================================*/
/*  This function will destroy the current D3DDevice and any resources that  */
/* belong to it.                                                             */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void DestroyInterfaces( PMESAD3DHAL pHAL )
{
  LONG	refCount;

  DPF(( DBG_FUNC, "DestroyInterfaces();" ));

  if ( pHAL->lpD3D3 != NULL )
  {
    DPF(( DBG_CNTX_INFO, "Release Direct3D3" ));
    refCount = pHAL->lpD3D3->Release();
    pHAL->lpD3D3 = NULL;
  }

  if ( pHAL->lpDD4 != NULL )
  {
    DPF(( DBG_CNTX_INFO, "Release DDraw4" ));
    refCount = pHAL->lpDD4->Release();
    pHAL->lpDD4 = NULL;
  }

  if ( pHAL->lpDD != NULL )
  {
    DPF(( DBG_CNTX_INFO, "Release DDraw" ));
    refCount = pHAL->lpDD->Release();
    pHAL->lpDD = NULL;
  }
}
/*===========================================================================*/
/*  This function will first send (not post) a message to the client window  */
/* that this context is using.  The client will respond by unbinding itself  */
/* and binding the 'default' context.  This allows the API to be supported   */
/* until the window can be destroyed.  Finally we post the quit message to   */
/* the client in hopes to end the application.                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void  FatalShutDown( PMESAD3DHAL pHAL )
{
  /* Whip this baby in too try and support the API until we die... */
  if ( pHAL )
    SendMessage( pHAL->shared.hwnd, UM_FATALSHUTDOWN, 0L, 0L );

  /* Close the client application down. */
  PostQuitMessage( 0 );
}

