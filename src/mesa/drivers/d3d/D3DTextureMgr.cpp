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
#include "D3DHAL.h"
/*===========================================================================*/
/* Local function prototypes.                                                */
/*===========================================================================*/
static void UpdateTexture( PTM_OBJECT pTMObj, BOOL bVideo, RECT *pRect, UCHAR *pixels );
static BOOL LoadTextureInVideo( PMESAD3DHAL pHAL, PTM_OBJECT pTMObj );
static BOOL FreeTextureMemory( PMESAD3DHAL pHAL, PTM_OBJECT pTMObject );
static BOOL DestroyTextureObject( PMESAD3DHAL pHAL, PTM_OBJECT pTMObject );
HRESULT CALLBACK EnumPFHook( LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext );
/*===========================================================================*/
/*  This function will simply set the top of stack to NULL.  I only used it  */
/* just incase I want to add something later.                                */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL InitTMgrHAL( PMESAD3DHAL pHAL )
{
  DPF(( DBG_FUNC, "InitTMgrHAL();" ));

  /* Be clean my friend. */
  pHAL->pTMList = NULL;

  return TRUE;
}
/*===========================================================================*/
/*  This function will walk the Texture Managers linked list and destroy all */
/* surfaces (SYSTEM/VIDEO).  The texture objects themselves also will be     */
/* freed.                                                                    */
/*  NOTE: this is per/context.                                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void TermTMgrHAL( PMESAD3DHAL pHAL )
{
  DPF(( DBG_FUNC, "TermTMgrHAL();" ));

  if ( pHAL && pHAL->pTMList )
  {
    /* Destroy the surface and remove the TMO from the stack. */
    while( DestroyTextureObject(pHAL,NULL) );

    /* Be clean my friend. */
    pHAL->pTMList = NULL;
  }
}
/*===========================================================================*/
/*  This function is a HACK as I don't know how I can disable a texture with-*/
/* out booting it out.  Is there know state change?                          */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void DisableTMgrHAL( PMESAD3DSHARED pShared )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;

  DPF(( DBG_FUNC, "DisableTMgrHAL();" ));

  /* Check too see that we have a valid context. */
  if ( (pHAL == NULL) && (pHAL->lpD3DDevice != NULL)  ) 
  {
    DPF(( DBG_TXT_WARN, "Null HAL/Direct3D Device!" ));
    return;
  }

  // TODO: This is a hack to shut off textures.
  pHAL->lpD3DDevice->SetTexture( 0, NULL );
}
/*===========================================================================*/
/*  This function is the only entry into the TextureManager that Mesa/wgl    */
/* will see.  It uses a dwAction to specify what we are doing.  I did this as*/
/* depending on the cards resources the action taken can change.             */
/*  When this function is called we will always search the Texture Managers  */
/* linked list (per context remember) and try and find a structure that has  */
/* the same dwName.  If we have a match we pull it out of the list and put it*/
/* at the top of the list (TOL).  If we don't find one then we create a struc*/
/* and put it a TOL.  This TOL idea makes for some caching as we will always */
/* destroy Texture Surfaces from the bottom up...                            */
/*  All texture objects at this point will create a texture surface in System*/
/* memory (SMEM).  Then we will copy the Mesa texture into the surface using */
/* the 'pixel' struc to get the translation info.  So now this means that all*/
/* textures that Mesa gives me I will have a Surface with a copy.  If Mesa   */
/* changes the texture the I update the surface in (SMEM).                   */
/*  Now we have a texture struc and a Texture Surface in SMEM.  At this point*/
/* we create another surface on the card (VMEM).  Finally we blt from the    */
/* SMEM to the VMEM and set the texture as current.  Why do I need two? First*/
/* this solves square textures.  If the cards CAPS is square textures only   */
/* then I change the dimensions of the VMEM surface and the blt solves it for*/
/* me.  Second it saves me from filling D3D textures over and over if the    */
/* card needs to be creating and destroying surfaces because of low memory.  */
/*  The surface in SMEM is expected to work always.  When a surface has to be*/
/* created in VMEM then we put it in a loop that tries to create the surface.*/
/* If we create the surface ok then we brake from the loop.  If we fail then */
/* we will call 'FreeTextureMemory' that will return TRUE/FALSE as to whether*/
/* memory was freed.  If memory was freed then we can try again. If no memory*/
/* was freed then it just can't fit.                                         */
/*  'FreeTextureMemory' will find the end of the list and start freeing VMEM */
/* (never SMEM) surfaces that are not locked.                                */
/*  BIND - when we bind and there is a texture struct with a texture surface */
/* in VMEM then we just make it current.  If we have a struct and a surface  */
/* in SMEM but no VMEM surface then we create the surface in VMEM and blt    */
/* from the SMEM surface.  If we have nothing its just like a creation...    */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
extern "C" BOOL CreateTMgrHAL( PMESAD3DSHARED pShared, DWORD dwName, int level, DWORD dwRequestFlags, 
						 RECT *rectDirty, DWORD dwWidth, DWORD dwHeight, DWORD dwAction, void *pPixels )
{
  PMESAD3DHAL		pHAL = (PMESAD3DHAL)pShared;
  PTM_OBJECT     	pTMObj,
                    pTemp;
  DDSURFACEDESC2 	ddsd2;
  HRESULT        	rc;


  DPF(( DBG_FUNC, "CreateTMgrHAL();" ));

  DPF(( DBG_TXT_INFO, "Texture:" ));
  DPF(( DBG_TXT_INFO, "cx: %d cy: %d", dwWidth, dwHeight ));
  DPF(( DBG_TXT_INFO, "Rect:" ));
  if ( rectDirty )
  {
    DPF(( DBG_TXT_INFO, "x0: %d y0: %d", rectDirty->left, rectDirty->top ));
    DPF(( DBG_TXT_INFO, "x1: %d y1: %d", rectDirty->right, rectDirty->bottom ));
  }

  /* Check too see that we have a valid context. */
  if ( (pHAL == NULL) && (pHAL->lpD3DDevice != NULL)  ) 
  {
    DPF(( DBG_TXT_WARN, "Null HAL/Direct3D Device!" ));
    return FALSE;
  }

  /*=================================================*/
  /* See if we can find this texture object by name. */
  /*=================================================*/
  for( pTMObj = pHAL->pTMList; pTMObj && (pTMObj->dwName != dwName); pTMObj = pTMObj->next );

  /*=========================================================*/
  /* Allocate a new object if we didn't get a matching name. */
  /*=========================================================*/
  if ( pTMObj == NULL )
  {
    pTMObj = (PTM_OBJECT)ALLOC( sizeof(TM_OBJECT) );
    if ( pTMObj == NULL )
	 return FALSE;
    memset( pTMObj, 0, sizeof(TM_OBJECT) );

    /* Put the object at the beginning of the list. */
    pTMObj->next = pHAL->pTMList;
    if ( pTMObj->next )
    {
	 pTemp = pTMObj->next;
	 pTemp->prev = pTMObj;
    }
    pHAL->pTMList = pTMObj;
  }
  else
  {
    /*===============================================================*/
    /* Make some caching happen by pulling this object to the front. */ 
    /*===============================================================*/
    if ( pHAL->pTMList != pTMObj )
    {
	 /* Pull the object out of the list. */
	 if ( pTMObj->prev )
	 {
	   pTemp = pTMObj->prev;
	   pTemp->next = pTMObj->next;
	 }
	 if ( pTMObj->next )
      {
	   pTemp = pTMObj->next;
	   pTemp->prev = pTMObj->prev;
	 }

	 pTMObj->prev = NULL;
	 pTMObj->next = NULL;

	 /* Put the object at the front of the list. */
	 pTMObj->next = pHAL->pTMList;
	 if ( pTMObj->next )
      {
	   pTemp = pTMObj->next;
	   pTemp->prev = pTMObj;
	 }
	 pHAL->pTMList = pTMObj;
    }
  }

  /*========================================================*/
  /* If we are doing BIND and the texture is in VID memory. */
  /*========================================================*/
  if ( (dwAction == TM_ACTION_BIND) && pTMObj->lpDDS_Video  )
  {
    DPF(( DBG_TXT_PROFILE, "Cache HIT (%d)", dwName ));
	   
    /* Make this the current texture. */
    rc = pHAL->lpD3DDevice->SetTexture( 0, pTMObj->lpD3DTexture2 );
    if ( FAILED(rc) )
    {
	 DPF(( DBG_TXT_WARN, "Failed SetTexture() (%s)", ErrorStringD3D(rc) ));
	 pHAL->lpD3DDevice->SetTexture( 0, NULL );
	 return FALSE;
    }	

    return TRUE;
  }	

  /*=================================================================*/
  /* If we are doing BIND and the texture is at least in SYS memory. */
  /*=================================================================*/
  if ( (dwAction == TM_ACTION_BIND) && pTMObj->lpDDS_System  )
  {
    DPF(( DBG_TXT_PROFILE, "Cache MISS (%d)", dwName ));

    /* Create the texture on the card. */
    rc = LoadTextureInVideo( pHAL, pTMObj );
    if ( rc == FALSE )
	 return FALSE;
	   
    /* Make this the current texture. */
    rc = pHAL->lpD3DDevice->SetTexture( 0, pTMObj->lpD3DTexture2 );
    if ( FAILED(rc) )
    {
	 DPF(( DBG_TXT_WARN, "Failed SetTexture() (%s)", ErrorStringD3D(rc) ));
	 pHAL->lpD3DDevice->SetTexture( 0, NULL );
	 return FALSE;
    }	

    return TRUE;
  }	

  /*=========================================================*/
  /* If we are doing UPDATE then try in VID first for speed. */
  /*=========================================================*/
  if ( (dwAction == TM_ACTION_UPDATE) && pTMObj->lpDDS_Video &&
	  !(pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) )
  {
    DPF(( DBG_TXT_INFO, "Fix the SubTexture update Leigh!" ));

    /* Update the texture on the card. */
    UpdateTexture( pTMObj, TRUE, rectDirty, (UCHAR *)pPixels );

    /* We updated the texture in VID so kill the SYS so we know its dirty. */
    if ( pTMObj->lpDDS_System )
    {
	 DPF(( DBG_TXT_INFO, "Release texture (SYS)" ));
	 DX_RESTORE( pTMObj->lpDDS_System );
	 pTMObj->lpDDS_System->Release();
	 pTMObj->lpDDS_System = NULL;
    }

    /* Make this the current texture. */
    rc = pHAL->lpD3DDevice->SetTexture( 0, pTMObj->lpD3DTexture2 );
    if ( FAILED(rc) )
    {
	 DPF(( DBG_TXT_WARN, "Failed SetTexture() (%s)", ErrorStringD3D(rc) ));
	 pHAL->lpD3DDevice->SetTexture( 0, NULL );
	 return FALSE;
    }	

    return TRUE;
  }

  /*===========================================================*/
  /* If we are doing UPDATE then try in SYS still gives speed. */
  /*===========================================================*/
  if ( (dwAction == TM_ACTION_UPDATE) && pTMObj->lpDDS_System )
  {
    DPF(( DBG_TXT_INFO, "Fix the SubTexture update Leigh!" ));

    /* Update the texture in SYS. */
    UpdateTexture( pTMObj, FALSE, NULL, (UCHAR *)pPixels );

    /* We updated the SYS texture only so now blt to the VID. */
    rc = LoadTextureInVideo( pHAL, pTMObj );
    if ( rc == FALSE )
	 return FALSE;

    /* Make this the current texture. */
    rc = pHAL->lpD3DDevice->SetTexture( 0, pTMObj->lpD3DTexture2 );
    if ( FAILED(rc) )
    {
	 DPF(( DBG_TXT_WARN, "Failed SetTexture() (%s)", ErrorStringD3D(rc) ));
	 pHAL->lpD3DDevice->SetTexture( 0, NULL );
	 return FALSE;
    }	

    return TRUE;
  }

  /*  At this point we have a valid Texture Manager Object with updated */
  /* links.  We now need to create or update a texture surface that is  */
  /* in system memory.  Every texture has a copy in system so we can use*/
  /* blt to solve problems with textures allocated on the card (square  */
  /* only textures, pixelformats...).                                   */
  
  // TODO: make support for update also.  Dirty rectangle basicly...

  /* Kill the interface if we have one no matter what. */
  if ( pTMObj->lpD3DTexture2 )
  {
    DPF(( DBG_TXT_INFO, "Release Texture2" ));
    pTMObj->lpD3DTexture2->Release();
    pTMObj->lpD3DTexture2 = NULL;
  }	

  /* Kill the system surface. TODO: should try to get the SubIMage going again */
  if ( pTMObj->lpDDS_System )
  {
    DPF(( DBG_TXT_INFO, "Release texture (SYS)" ));
    DX_RESTORE( pTMObj->lpDDS_System );
    pTMObj->lpDDS_System->Release();
    pTMObj->lpDDS_System = NULL;
  }

  /* Kill the Video surface. TODO: need some reuse system... */
  if ( pTMObj->lpDDS_Video )
  {
    DPF(( DBG_TXT_INFO, "Release texture (VID)" ));
    DX_RESTORE( pTMObj->lpDDS_Video );
    pTMObj->lpDDS_Video->Release();
    pTMObj->lpDDS_Video = NULL;
  }

  /*================================================================*/
  /* Translate the the Mesa/OpenGL pixel channels to the D3D flags. */
  /*================================================================*/
  switch( dwRequestFlags )
  {
    case GL_ALPHA:
	 dwRequestFlags = DDPF_ALPHA; 
	 DPF(( DBG_TXT_WARN, "GL_ALPHA not supported!)" ));
	 return FALSE;
	 
    case GL_INTENSITY:
    case GL_LUMINANCE:
	 DPF(( DBG_TXT_WARN, "GL_INTENSITY/GL_LUMINANCE not supported!)" ));
	 dwRequestFlags = DDPF_LUMINANCE; 
	 return FALSE;
	 
    case GL_LUMINANCE_ALPHA:
	 DPF(( DBG_TXT_WARN, "GL_LUMINANCE_ALPHA not supported!)" ));
	 dwRequestFlags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS; 
	 return FALSE;
	 
    case GL_RGB:
	 DPF(( DBG_TXT_INFO, "Texture -> GL_RGB" ));
	 dwRequestFlags = DDPF_RGB; 
	 break;

    case GL_RGBA:
	 DPF(( DBG_TXT_INFO, "Texture -> GL_RGBA" ));
	 dwRequestFlags = DDPF_RGB | DDPF_ALPHAPIXELS; 
	 break;
  }

  /*==============================*/
  /* Populate the texture object. */
  /*==============================*/
  pTMObj->dwName      = dwName;
  pTMObj->lpD3DDevice = pHAL->lpD3DDevice;
  pTMObj->dwFlags     = dwRequestFlags;
  if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
  {
    DPF(( DBG_TXT_INFO, "Convert to Square..." ));
    pTMObj->dwSHeight  = dwHeight;
    pTMObj->dwSWidth   = dwWidth;

    /* Shrink non-square textures. */
    pTMObj->dwVHeight  = (dwHeight > dwWidth) ? dwWidth : dwHeight;
    pTMObj->dwVWidth	= (dwHeight > dwWidth) ? dwWidth : dwHeight;
  }
  else
  {
    pTMObj->dwSHeight  = dwHeight;
    pTMObj->dwSWidth   = dwWidth;
    pTMObj->dwVHeight  = dwHeight;
    pTMObj->dwVWidth   = dwWidth;
  }

  /*========================*/  
  /* Create SYSTEM surface. */
  /*========================*/

  /*  Request a surface in system memory. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
  ddsd2.dwSize          = sizeof( DDSURFACEDESC2 );
  ddsd2.dwWidth         = pTMObj->dwSWidth;
  ddsd2.dwHeight        = pTMObj->dwSHeight;
  ddsd2.dwFlags         = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
  ddsd2.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
  ddsd2.ddsCaps.dwCaps2 = 0L;
  memset( &ddsd2.ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT) );
  ddsd2.ddpfPixelFormat.dwSize  = sizeof( DDPIXELFORMAT );
  ddsd2.ddpfPixelFormat.dwFlags = dwRequestFlags;
  rc = pHAL->lpD3DDevice->EnumTextureFormats( EnumPFHook, &ddsd2.ddpfPixelFormat );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "EnumerTextureFormats (SYSTEM)->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Create the surface using the enumerated pixelformat. */
  rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pTMObj->lpDDS_System, NULL );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "CreateSurface (TEXTURE/SYSTEM)->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Solve the pixel mapping info using the surface pixelformat. */
  Solve8BitChannelPixelFormat( &ddsd2.ddpfPixelFormat, &pTMObj->pixel );

  /*===================================================================*/
  /* Fill the texture using the PixelInfo structure to do the mapping. */
  /*===================================================================*/
  UpdateTexture( pTMObj, FALSE, NULL, (UCHAR *)pPixels );

  /*=======================*/
  /* Create VIDEO surface. */
  /*=======================*/
  rc = LoadTextureInVideo( pHAL, pTMObj );
  if ( rc == FALSE )
    return FALSE;

  /* Make this the current texture. */
  rc = pHAL->lpD3DDevice->SetTexture( 0, pTMObj->lpD3DTexture2 );
  if ( FAILED(rc) )
  {
    DPF(( DBG_TXT_WARN, "Failed SetTexture() (%s)", ErrorStringD3D(rc) ));
    pHAL->lpD3DDevice->SetTexture( 0, NULL );
    return FALSE;
  }

  return TRUE;
}
/*===========================================================================*/
/*  This function will handle the creation and destruction of the texture    */
/* surfaces on the card.  Using the dw'V'Width/Height dimensions the call    */
/* try and create the texture on the card and keep using FreeTextureMemory   */
/* until the surace can be created.  Once the surface is created we get the  */
/* interface that we will use to make it the current texture.  I didn't put  */
/* the code to make the texture current in this function as BIND needs to    */
/* use the same code and this function doesn't always get called when we do a*/
/* bind.                                                                     */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static BOOL	LoadTextureInVideo( PMESAD3DHAL pHAL, PTM_OBJECT pTMObj )
{
  DDSURFACEDESC2	ddsd2;
  HRESULT       	rc;

  DPF(( DBG_FUNC, "LoadTextureInVideo();" ));

  /* Kill the interface if we have one no matter what. */
  if ( pTMObj->lpD3DTexture2 )
  {
    DPF(( DBG_TXT_INFO, "Release Texture2" ));
    pTMObj->lpD3DTexture2->Release();
    pTMObj->lpD3DTexture2 = NULL;
  }	

  /* Kill the Video surface. TODO: need some reuse system... */
  if ( pTMObj->lpDDS_Video )
  {
    DPF(( DBG_TXT_INFO, "Release texture (VID)" ));
    DX_RESTORE( pTMObj->lpDDS_Video );
    pTMObj->lpDDS_Video->Release();
    pTMObj->lpDDS_Video = NULL;
  }

  /*  Request a surface in Video memory. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
  ddsd2.dwSize          = sizeof( DDSURFACEDESC2 );
  ddsd2.dwWidth         = pTMObj->dwVWidth;
  ddsd2.dwHeight        = pTMObj->dwVHeight;
  ddsd2.dwFlags         = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
  ddsd2.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
  ddsd2.ddsCaps.dwCaps2 = 0L;
  memset( &ddsd2.ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT) );
  ddsd2.ddpfPixelFormat.dwSize  = sizeof( DDPIXELFORMAT );
  ddsd2.ddpfPixelFormat.dwFlags = pTMObj->dwFlags;
  rc = pHAL->lpD3DDevice->EnumTextureFormats( EnumPFHook, &ddsd2.ddpfPixelFormat );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "EnumerTextureFormats ->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Make sure we lock so we don't nuke this texture trying to free memory for it. */
  pTMObj->bLock = TRUE;

  /*  Start a loop that will free all textures until we have created the texture */
  /* surface or we can't free up more memory.                                    */
  do
  {
    /* Try to create the texture surface. */
    rc = pHAL->lpDD4->CreateSurface( &ddsd2, &pTMObj->lpDDS_Video, NULL );
    if ( !FAILED(rc) )
	 break;

    DPF(( DBG_TXT_INFO, "Free Texture Memory" ));

    /* DestroyTexture will return TRUE if a surface was freed. */
  } while( FreeTextureMemory(pHAL,NULL) );

  /* Make sure we unlock or we won't be able to nuke the TMO later. */
  pTMObj->bLock = FALSE;

  /* Did we create a valid texture surface? */
  if ( FAILED(rc) )
  {
    DPF(( DBG_TXT_WARN, "Failed to load texture" ));
    pHAL->lpD3DDevice->SetTexture( 0, NULL );
    return FALSE;
  }

  DX_RESTORE( pTMObj->lpDDS_System );
  DX_RESTORE( pTMObj->lpDDS_Video );

  DPF(( DBG_TXT_INFO, "Texture Blt SYSTEM -> VID" ));

  /* Now blt the texture in system memory to the card. */
  rc = pTMObj->lpDDS_Video->Blt( NULL, pTMObj->lpDDS_System, NULL, DDBLT_WAIT, NULL );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "Blt (TEXTURE) ->", ErrorStringD3D(rc) );
    return FALSE;
  }

  /* Get the Texture interface that is used to render with. */
  pTMObj->lpDDS_Video->QueryInterface( IID_IDirect3DTexture2, (void **)&pTMObj->lpD3DTexture2 ); 
  if ( pTMObj->lpD3DTexture2 == NULL )
  {
    DPF(( DBG_TXT_WARN, "Failed QueryTextureInterface" ));
    pHAL->lpD3DDevice->SetTexture( 0, NULL );
    return FALSE;
  }

  return TRUE;
}
/*===========================================================================*/
/*  If this function gets a texture object struc then we will try and free   */
/* it.  If we get a NULL then we will search from the bottom up and free one */
/* VMEM surface.  I can only free when the surface isn't locked and of course*/
/* there must be a VMEM surface.  We never free SMEM surfaces as that isn't  */
/* the point.                                                                */
/* TODO: should have a pointer to the bottom of the stack really.            */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static BOOL FreeTextureMemory( PMESAD3DHAL pHAL, PTM_OBJECT pTMObject )
{
  PTM_OBJECT	pCurrent;
  BOOL		bFreed = FALSE;

  DPF(( DBG_FUNC, "FreeTextureMemory();" ));
  DPF(( DBG_TXT_WARN, "FREE TEXTURE!" ));

  /* Just to be safe. */
  if ( !pHAL || !pHAL->pTMList )
  {
    DPF(( DBG_TXT_WARN, "FreeTextureMemory() -> NULL pHAL/pHAL->pTMList" ));
    return FALSE;
  }

  /* Free the last texture in the list. */
  if ( pTMObject == NULL )
  {
    DPF(( DBG_TXT_INFO, "Free Last texture in cache" ));

    /* Find the last texture object. */
    for( pCurrent = pHAL->pTMList; pCurrent->next; pCurrent = pCurrent->next );

    /* Now backup until we find a texture on the card. */
    while( pCurrent && (pCurrent->lpDDS_Video == NULL) && (pCurrent->bLock == FALSE) )
	 pCurrent = pCurrent->prev;

    /* Didn't find anything. */
    if ( pCurrent == NULL )
    {
	 DPF(( DBG_TXT_INFO, "No texture memory freed" ));
	 return FALSE;
    }
  }
  else
  {
    /* See if we can find this texture object. */
    for( pCurrent = pHAL->pTMList; pCurrent && (pCurrent != pTMObject); pCurrent = pCurrent->next );

    /* Didn't find anything. */
    if ( pCurrent == NULL )
    {
	 DPF(( DBG_TXT_INFO, "Requested texture to be freed NOT FOUND" ));
	 return FALSE;
    }
  }

  /* Can't free this baby. */
  if ( pCurrent->bLock == TRUE )
  {
    DPF(( DBG_TXT_WARN, "Requested texture LOCKED" ));
    return FALSE;
  }

  /* Free the texture memory. */
  if ( pCurrent->lpD3DTexture2 )
  {
    DPF(( DBG_TXT_INFO, "Release Texture2" ));
    pCurrent->lpD3DTexture2->Release();
    pCurrent->lpD3DTexture2 = NULL;
    bFreed = TRUE;
  }
  if ( pCurrent->lpDDS_Video )
  {
    DPF(( DBG_TXT_INFO, "Release texture (VID):" ));
    DPF(( DBG_TXT_INFO, "dwName: %d", pCurrent->dwName ));
    DPF(( DBG_TXT_INFO, "cx: %d, cy: %d", pCurrent->dwVWidth, pCurrent->dwVHeight ));
    pCurrent->lpDDS_Video->Release();
    pCurrent->lpDDS_Video = NULL;
    bFreed = TRUE;
  }
  
  return bFreed;
}
/*===========================================================================*/
/*  This function searches the linked list of texture objects in the supplied*/
/* D3Dwrapper structure.  If it finds a match it will free it and pull it out*/
/* of the linked list.  The function works on the bases of a matching pointer*/
/* to the object (not matching content).                                     */
/*  If the function gets passed a NULL then we want to free the last texture */
/* object in the list.  Used in a loop to destory all.                       */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static BOOL DestroyTextureObject( PMESAD3DHAL pHAL, PTM_OBJECT pTMObject )
{
  PTM_OBJECT	pCurrent;

  DPF(( DBG_FUNC, "DestoryTextureObject();" ));

  /* Just to be safe. */
  if ( !pHAL || !pHAL->pTMList )
  {
    DPF(( DBG_TXT_WARN, "DestroyTextureObject() -> NULL pHAL/pHAL->pTMList" ));
    return FALSE;
  }

  /* Free the last texture in the list. */
  if ( pTMObject == NULL )
  {
    /* Find the last texture object. */
    for( pCurrent = pHAL->pTMList; pCurrent->next; pCurrent = pCurrent->next );
  }
  else
  {
    /* See if we can find this texture object. */
    for( pCurrent = pHAL->pTMList; pCurrent && (pCurrent != pTMObject); pCurrent = pCurrent->next );

    /* Didn't find anything. */
    if ( pCurrent == NULL )
    {
	 DPF(( DBG_TXT_WARN, "No textures to be freed" ));
	 return FALSE;
    }
  }

  /* Can't free this baby. */
  if ( pCurrent->bLock == TRUE )
  {
    DPF(( DBG_TXT_WARN, "Requested texture to be freed LOCKED" ));
    return FALSE;
  }

  /* Free the texture memory. */
  if ( pCurrent->lpD3DTexture2 )
  { 
    DPF(( DBG_TXT_INFO, "Release Texture2" ));
    pCurrent->lpD3DTexture2->Release();
    pCurrent->lpD3DTexture2 = NULL;
  }
  if ( pCurrent->lpDDS_Video )
  {
    DPF(( DBG_TXT_INFO, "Release texture (VID):" ));
    pCurrent->lpDDS_Video->Release();
    pCurrent->lpDDS_Video = NULL;
  }
  if ( pCurrent->lpDDS_System )
  {
    DPF(( DBG_TXT_INFO, "Release texture (SYS):" ));
    pCurrent->lpDDS_System->Release();
    pCurrent->lpDDS_System = NULL;
  }

  /* Pull this texture out of the list. */
  if ( pCurrent == pHAL->pTMList )
    pHAL->pTMList = NULL;
  if ( pCurrent->prev )
    (pCurrent->prev)->next = pCurrent->next;
  if ( pCurrent->next )
    (pCurrent->next)->prev = pCurrent->prev;
  FREE( pCurrent );

  return TRUE;
}
/*===========================================================================*/
/*  This function is the callback function that gets called when we are doing*/
/* an enumeration of the texture formats supported by this device. The choice*/
/* is made by checking to see if we have a match with the supplied D3D pixel-*/
/* format.  So the enumeration has to pass a desired D3D PF as the user var. */
/*===========================================================================*/
/* RETURN: D3DENUMRET_OK, D3DENUMRET_CANCEL.                                 */
/*===========================================================================*/
static void UpdateTexture( PTM_OBJECT pTMObj, BOOL bVideo, RECT *pRect, UCHAR *pixels )
{
  LPDIRECTDRAWSURFACE4	lpDDS;
  DDSURFACEDESC2		ddsd2;
  DWORD          		srcPitch,
                         dwHeight, 
                         dwWidth,
                         dwCol,
                         dwColor;
  UCHAR          		*pSrc,
                         *pSrcRow,
                         *pDest,
                         *pDestRow;
  int				rc;

  // TODO:  Do I need to pass the h/w when its in the object!
  DPF(( DBG_FUNC, "UpdateTexture();" ));

  /* Get the surface pointer we are looking for. */
  lpDDS = (bVideo) ? pTMObj->lpDDS_Video : pTMObj->lpDDS_System;

  /*===================================================================*/
  /* Fill the texture using the PixelInfo structure to do the mapping. */
  /*===================================================================*/
	 
  /* Get the surface pointer. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);
  rc = lpDDS->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );
  if ( FAILED(rc) )
  {
    RIP( NULL, "Lock (TEXTURE/SYSTEM)->", ErrorStringD3D(rc) );
    return;
  }

  /* For now we are only updating the system surface so use its dimensions. */
  dwWidth  = (bVideo) ? pTMObj->dwVWidth : pTMObj->dwSWidth;
  dwHeight = (bVideo) ? pTMObj->dwVHeight : pTMObj->dwSHeight;

  /*  If we are updating the whole surface then the pDest/pSrc will */
  /* always be the same.                                            */
  if ( pRect == NULL )
  {
    pDest = (UCHAR *)ddsd2.lpSurface;
    pSrc  = pixels;
  }

  /* Fill the texture surface based on the pixelformat flags. */
  if ( pTMObj->dwFlags == (DDPF_RGB | DDPF_ALPHAPIXELS) )
  {
    srcPitch = dwWidth * 4;
    if ( pRect )
    {
	 pDest = ((UCHAR *)ddsd2.lpSurface) + (pRect->top * ddsd2.lPitch) + (pRect->left * pTMObj->pixel.cb);
	 pSrc  = pixels + (pRect->top * dwWidth * 4) + (pRect->left * 4);
	 dwHeight = (pRect->bottom - pRect->top);
	 dwWidth = (pRect->right - pRect->left);
    }

    for( pDestRow = pDest, pSrcRow = pSrc; dwHeight > 0; dwHeight--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
    {
	 for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < dwWidth; dwCol++ )
      {
	   dwColor =  ( ((DWORD)(*(pSrc  ) * pTMObj->pixel.rScale)) << pTMObj->pixel.rShift );
	   dwColor |= ( ((DWORD)(*(pSrc+1) * pTMObj->pixel.gScale)) << pTMObj->pixel.gShift );
	   dwColor |= ( ((DWORD)(*(pSrc+2) * pTMObj->pixel.bScale)) << pTMObj->pixel.bShift );
	   if ( pTMObj->pixel.aScale == -1.0 )
		dwColor |= ( (*(pSrc+3) & 0x80) ? (1 << pTMObj->pixel.aShift) : 0 );
	   else
		dwColor |= ( ((DWORD)(*(pSrc+3) * pTMObj->pixel.aScale)) << pTMObj->pixel.aShift );
	   memcpy( pDest, &dwColor, pTMObj->pixel.cb );
	   pDest += pTMObj->pixel.cb;
	   pSrc  += 4;
	 }
    }
  }
  else if ( pTMObj->dwFlags == DDPF_RGB )
  {
    srcPitch = dwWidth * 3;
    if ( pRect )
    {
	 pDest = ((UCHAR *)ddsd2.lpSurface) + (pRect->top * ddsd2.lPitch) + (pRect->left * pTMObj->pixel.cb);
	 pSrc  = pixels + (pRect->top * dwWidth * 3) + (pRect->left * 3);
	 dwHeight = (pRect->bottom - pRect->top);
	 dwWidth  = (pRect->right - pRect->left);
    }

    for( pDestRow = pDest, pSrcRow = pSrc; dwHeight > 0; dwHeight--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
    {
	 for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < dwWidth; dwCol++ )
      {
	   dwColor =  ( ((DWORD)(*(pSrc  ) * pTMObj->pixel.rScale)) << pTMObj->pixel.rShift );
	   dwColor |= ( ((DWORD)(*(pSrc+1) * pTMObj->pixel.gScale)) << pTMObj->pixel.gShift );
	   dwColor |= ( ((DWORD)(*(pSrc+2) * pTMObj->pixel.bScale)) << pTMObj->pixel.bShift );
	   memcpy( pDest, &dwColor, pTMObj->pixel.cb );
	   pDest += pTMObj->pixel.cb;
	   pSrc  += 3;
	 }
    }
  }
  else if ( pTMObj->dwFlags == (DDPF_LUMINANCE | DDPF_ALPHAPIXELS) )
  {
    srcPitch = dwWidth * 2;
    if ( pRect )
    {
	 pDest = ((UCHAR *)ddsd2.lpSurface) + (pRect->top * ddsd2.lPitch) + (pRect->left * pTMObj->pixel.cb);
	 pSrc  = pixels + (pRect->top * dwWidth * 2) + (pRect->left * 2);
	 dwHeight = (pRect->bottom - pRect->top);
	 dwWidth  = (pRect->right - pRect->left);
    }

    for( pDestRow = pDest, pSrcRow = pSrc; dwHeight > 0; dwHeight--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
    {
	 for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < dwWidth; dwCol++ )
      {
	   dwColor =  ( ((DWORD)(*(pSrc  ) * pTMObj->pixel.rScale)) << pTMObj->pixel.rShift );
	   if ( pTMObj->pixel.aScale == -1.0 )
		dwColor |= ( (*(pSrc+1) & 0x80) ? (1 << pTMObj->pixel.aShift) : 0 );
	   else
		dwColor |= ( ((DWORD)(*(pSrc+1) * pTMObj->pixel.aScale)) << pTMObj->pixel.aShift );
	   memcpy( pDest, &dwColor, pTMObj->pixel.cb );
	   pDest += pTMObj->pixel.cb;
	   pSrc  += 2;
	 }
    }
  }
  else if ( pTMObj->dwFlags == DDPF_LUMINANCE )
  {
    srcPitch = dwWidth;
    if ( pRect )
    {
	 pDest = ((UCHAR *)ddsd2.lpSurface) + (pRect->top * ddsd2.lPitch) + (pRect->left * pTMObj->pixel.cb);
	 pSrc  = pixels + (pRect->top * dwWidth) + (pRect->left);
	 dwHeight = (pRect->bottom - pRect->top);
	 dwWidth  = (pRect->right - pRect->left);
    }

    for( pDestRow = pDest, pSrcRow = pSrc; dwHeight > 0; dwHeight--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
    {
	 for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < dwWidth; dwCol++ )
      {
	   dwColor =  ( ((DWORD)(*pSrc * pTMObj->pixel.rScale)) << pTMObj->pixel.rShift );
	   memcpy( pDest, &dwColor, pTMObj->pixel.cb );
	   pDest += pTMObj->pixel.cb;
	   pSrc++;
	 }
    }
  }
  else if ( pTMObj->dwFlags == DDPF_ALPHAPIXELS )
  {
    srcPitch = dwWidth;
    if ( pRect )
    {
	 pDest = ((UCHAR *)ddsd2.lpSurface) + (pRect->top * ddsd2.lPitch) + (pRect->left * pTMObj->pixel.cb);
	 pSrc  = pixels + (pRect->top * dwWidth) + (pRect->left);
	 dwHeight = (pRect->bottom - pRect->top);
	 dwWidth  = (pRect->right - pRect->left);
    }

    for( pDestRow = pDest, pSrcRow = pSrc; dwHeight > 0; dwHeight--, pDestRow += ddsd2.lPitch, pSrcRow += srcPitch )
    {
	 for( dwCol = 0, pDest = pDestRow, pSrc = pSrcRow; dwCol < dwWidth; dwCol++ )
      {
	   if ( pTMObj->pixel.aScale == -1.0 )
		dwColor = ( (*pSrc & 0x80) ? (1 << pTMObj->pixel.aShift) : 0 );
	   else
		dwColor = ( ((DWORD)(*pSrc * pTMObj->pixel.aScale)) << pTMObj->pixel.aShift );
	   memcpy( pDest, &dwColor, pTMObj->pixel.cb );
	   pDest += pTMObj->pixel.cb;
	   pSrc++;
	 }
    }
  }

  /* Unlock the surface. */
  rc = lpDDS->Unlock( NULL );
  if ( FAILED(rc) )
  {
    RIP( NULL, "Unlock (TEXTURE/SYSTEM)->", ErrorStringD3D(rc) );
  }
}
/*===========================================================================*/
/*  This function is the callback function that gets called when we are doing*/
/* an enumeration of the texture formats supported by this device. The choice*/
/* is made by checking to see if we have a match with the supplied D3D pixel-*/
/* format.  So the enumeration has to pass a desired D3D PF as the user var. */
/*===========================================================================*/
/* RETURN: D3DENUMRET_OK, D3DENUMRET_CANCEL.                                 */
/*===========================================================================*/
HRESULT CALLBACK EnumPFHook( LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext )
{
  LPDDPIXELFORMAT   lpDDPixFmtRequest = (LPDDPIXELFORMAT)lpContext;
  PIXELINFO    	pixel;

  DPF(( DBG_FUNC, "EnumPFHook();" ));

  if ( lpDDPixFmt->dwFlags == lpDDPixFmtRequest->dwFlags )
  {
    /* Are we looking for an alpha channel? */
    if ( lpDDPixFmtRequest->dwFlags & DDPF_ALPHAPIXELS )
    {
	 /* Try for something that has more then 1bits of Alpha. */
	 Solve8BitChannelPixelFormat( lpDDPixFmt, &pixel );
	 if ( pixel.aScale == -1.0 )
	 {
	   /* Save this format no matter what as its a match of sorts. */
	   memcpy( lpDDPixFmtRequest, lpDDPixFmt, sizeof(DDPIXELFORMAT) );
	   return D3DENUMRET_OK;
	 }
    }

    /* Save this format as its a good match. */
    memcpy( lpDDPixFmtRequest, lpDDPixFmt, sizeof(DDPIXELFORMAT) );

    /* We are happy at this point so lets leave. */
    return D3DENUMRET_CANCEL;
  }
  
  return D3DENUMRET_OK;
}


