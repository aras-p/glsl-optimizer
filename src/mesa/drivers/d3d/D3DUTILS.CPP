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
/* Local only functions.                                                     */
/*===========================================================================*/
static int  CountTrailingZeros( DWORD dwMask );
/*===========================================================================*/
/*  This function is used to get the pointer to the surface and the pitch for*/
/* the scanline rendering functions.                                         */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" DDSURFACEDESC2 *LockHAL( PMESAD3DSHARED pShared, BOOL bBack )
{
  PMESAD3DHAL			pHAL = (PMESAD3DHAL)pShared;
  static DDSURFACEDESC2	ddsd2;
  HRESULT               	rc;      

  DPF(( DBG_FUNC, "LockHAL();" ));

  /* Set the request structure up first. */
  memset( &ddsd2, 0, sizeof(DDSURFACEDESC2) );
  ddsd2.dwSize = sizeof(DDSURFACEDESC2);

  /* Make sure we have enough info. */
  if ( pHAL )
  {
    rc = pHAL->lpDDSRender->Lock( NULL, &ddsd2, DDLOCK_WAIT, NULL );
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "Lock (RENDER) ->", ErrorStringD3D(rc) );
    }
  }

  return &ddsd2;
}
/*===========================================================================*/
/*  This is just a simple wrapper.  I probably don't need to do any error    */
/* checking as the Lock must have worked inorder to get here...              */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void UnlockHAL( PMESAD3DSHARED pShared, BOOL bBack )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;
  HRESULT		rc;

  DPF(( DBG_FUNC, "UnlockHAL();" ));

  /* Make sure we have enough info. */
  if ( pHAL )
  {
    rc = pHAL->lpDDSRender->Unlock( NULL );
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "Unlock (RENDER) ->", ErrorStringD3D(rc) );
    }
  }
}
/*===========================================================================*/
/*  This function will track the main/Primary window that will be used as the*/
/* target for the Blt in SwapBuffers.  As a side effect the call will check  */
/* to see if the primary surface is the same size and position as the screen.*/
/* If they are the same size we will call it fullscreen...                   */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void UpdateScreenPosHAL( PMESAD3DSHARED pShared )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;
  POINT		pt;
  DWORD		dwWidth, dwHeight;

  DPF(( DBG_FUNC, "UpdateScreenPosHAL();" ));

  /* Make sure we have enough info. */
  if ( pHAL != NULL )
  {
    /* Update the windows screen position. */
    GetClientRect( pShared->hwnd, &pShared->rectW );
    pt.x = pt.y = 0;
    ClientToScreen( pShared->hwnd, &pt );
    OffsetRect( &pShared->rectW, pt.x, pt.y);

    /* Compare the primary to the screen. */
    dwWidth = GetSystemMetrics( SM_CXSCREEN );
    dwHeight =  GetSystemMetrics( SM_CYSCREEN );
    if ( (pShared->rectW.left > 0) ||  (pShared->rectW.top > 0)  ||
	    (pShared->rectW.right > dwWidth) || (pShared->rectW.bottom > dwHeight) )
	 pShared->bWindow = TRUE;
    else
	 pShared->bWindow = FALSE;
  }
}
/*===========================================================================*/
/*  This function will fill in the pixel info structure defined in D3Dshared.*/
/* Basicly it will take a DirectDraw pixelformat structure and make scaling  */
/* values that will convert from 8bit channels to whatever the supplied ddpf */
/* uses.  Also we will generate shift values that will be used to get move   */
/* each component of the pixel into place.                                   */
/*  I have now added a special case for a 1bit alpha channel.  If I find a 1b*/
/* alpha then I will set the scale to -1.0 which should be unique.  Later I  */
/* can check the alpha scale value too see if its -1.0 and thus handle it.  I*/
/* was finding that the case was not working tom my advantage so this is my  */
/* HACK for the day.  As a TODO I should work on this...                     */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void	Solve8BitChannelPixelFormat( DDPIXELFORMAT *pddpf, PPIXELINFO pPixel )
{
  DPF(( DBG_FUNC, "Solve8BitChannelPixelFromat();" ));

  memset( pPixel, 0, sizeof(PPIXELINFO) );

  /* Check too see if the color space is valid in the PF. */
  if ( pddpf->dwFlags & DDPF_RGB )
  {
    /* Solve the red stuff. */
    pPixel->dwRMask = pddpf->dwRBitMask;
    pPixel->rShift = CountTrailingZeros( pPixel->dwRMask );
    pPixel->rScale = (float)0.00392156 * (float)(pPixel->dwRMask >> pPixel->rShift);

    /* Solve the green thingy's. */
    pPixel->dwGMask = pddpf->dwGBitMask;
    pPixel->gShift = CountTrailingZeros( pPixel->dwGMask );
    pPixel->gScale = (float)0.00392156 * (float)(pPixel->dwGMask >> pPixel->gShift);

    /* Solve the blues. */
    pPixel->dwBMask = pddpf->dwBBitMask;
    pPixel->bShift = CountTrailingZeros( pddpf->dwBBitMask );
    pPixel->bScale = (float)0.00392156 * (float)(pddpf->dwBBitMask >> pPixel->bShift);
  }

  /* Do the alpha channel if there is one. */
  if ( pddpf->dwFlags & DDPF_ALPHAPIXELS )
  {
    pPixel->dwAMask = pddpf->dwRGBAlphaBitMask;
    pPixel->aShift = CountTrailingZeros( pPixel->dwAMask );

    /* Special case a 1bit alpha. */
    if ( (pPixel->dwAMask >> pPixel->aShift) == 1 )
	 pPixel->aScale = -1.0;
    else
	 pPixel->aScale = (float)0.00392156 * (float)(pPixel->dwAMask >> pPixel->aShift);
  }

  /* Get the size of the pixel in bytes. Should work as dwRGBBitCount is in a union. */
  pPixel->cb = pddpf->dwRGBBitCount / 8;
}
/*===========================================================================*/
/*  See RETURN :)                                                            */
/*===========================================================================*/
/* RETURN: number of contiguous zeros starting from the right.               */
/*===========================================================================*/
static int  CountTrailingZeros( DWORD dwMask )
{
  DWORD Mask;

  if ( dwMask == 0 ) 
    return 32;

  /* Can't take credit for this one! */
  Mask = dwMask & -(int)dwMask;
  return ((Mask & 0xFFFF0000)!=0) << 4
         | ((Mask & 0xFF00FF00)!=0) << 3
         | ((Mask & 0xF0F0F0F0)!=0) << 2
         | ((Mask & 0xCCCCCCCC)!=0) << 1
         | ((Mask & 0xAAAAAAAA)!=0);
}
/*===========================================================================*/
/*  This function will convert the DDraw error code to its macro string.  The*/
/* returned pointer is static so you need not worry about memory managemnet  */
/* but the error message gets written over from call to call...              */
/*===========================================================================*/
/* RETURN: pointer to the single static buffer that hold the error message.  */
/*===========================================================================*/
char *ErrorStringD3D( HRESULT hr )
{
  static   char  errorString[128];

  switch( hr )
    {
    case DDERR_ALREADYINITIALIZED:
	 strcpy( errorString, "DDERR_ALREADYINITIALIZED" );
	 break;
	 
    case DDERR_CANNOTATTACHSURFACE:
	 strcpy( errorString, "DDERR_CANNOTATTACHSURFACE" );
	 break;
	 
    case DDERR_CANNOTDETACHSURFACE:
	 strcpy( errorString, "DDERR_CANNOTDETACHSURFACE" );
	 break;
	 
    case DDERR_CURRENTLYNOTAVAIL:
	 strcpy( errorString, "DDERR_CURRENTLYNOTAVAIL" );
	 break;
	 
    case DDERR_EXCEPTION:
	 strcpy( errorString, "DDERR_EXCEPTION" );
	 break;
	 
    case DDERR_GENERIC:
	 strcpy( errorString, "DDERR_GENERIC" );
	 break;
	 
    case DDERR_HEIGHTALIGN:
	 strcpy( errorString, "DDERR_HEIGHTALIGN" );
	 break;
	 
    case DDERR_INCOMPATIBLEPRIMARY:
	 strcpy( errorString, "DDERR_INCOMPATIBLEPRIMARY" );
	 break;
	 
    case DDERR_INVALIDCAPS:
	 strcpy( errorString, "DDERR_INVALIDCAPS" );
	 break;
	 
    case DDERR_INVALIDCLIPLIST:
	 strcpy( errorString, "DDERR_INVALIDCLIPLIST" );
	 break;
	 
    case DDERR_INVALIDMODE:
	 strcpy( errorString, "DDERR_INVALIDMODE" );
	 break;
	 
    case DDERR_INVALIDOBJECT:
	 strcpy( errorString, "DDERR_INVALIDOBJECT" );
	 break;
	 
    case DDERR_INVALIDPARAMS:
	 strcpy( errorString, "DDERR_INVALIDPARAMS" );
	 break;
	 
    case DDERR_INVALIDPIXELFORMAT:
	 strcpy( errorString, "DDERR_INVALIDPIXELFORMAT" );
	 break;
	 
    case DDERR_INVALIDRECT:
	 strcpy( errorString, "DDERR_INVALIDRECT" );
	 break;
	 
    case DDERR_LOCKEDSURFACES:
	 strcpy( errorString, "DDERR_LOCKEDSURFACES" );
	 break;
	 
    case DDERR_NO3D:
	 strcpy( errorString, "DDERR_NO3D" );
	 break;
	 
    case DDERR_NOALPHAHW:
	 strcpy( errorString, "DDERR_NOALPHAHW" );
	 break;
	 
    case DDERR_NOCLIPLIST:
	 strcpy( errorString, "DDERR_NOCLIPLIST" );
	 break;
	 
    case DDERR_NOCOLORCONVHW:
	 strcpy( errorString, "DDERR_NOCOLORCONVHW" );
	 break;
	 
    case DDERR_NOCOOPERATIVELEVELSET:
	 strcpy( errorString, "DDERR_NOCOOPERATIVELEVELSET" );
	 break;
	 
    case DDERR_NOCOLORKEY:
	 strcpy( errorString, "DDERR_NOCOLORKEY" );
	 break;
	 
    case DDERR_NOCOLORKEYHW:
	 strcpy( errorString, "DDERR_NOCOLORKEYHW" );
	 break;
	 
    case DDERR_NODIRECTDRAWSUPPORT:
	 strcpy( errorString, "DDERR_NODIRECTDRAWSUPPORT" );
	 break;
	 
    case DDERR_NOEXCLUSIVEMODE:
	 strcpy( errorString, "DDERR_NOEXCLUSIVEMODE" );
	 break;
	 
    case DDERR_NOFLIPHW:
	 strcpy( errorString, "DDERR_NOFLIPHW" );
	 break;
	 
    case DDERR_NOGDI:
	 strcpy( errorString, "DDERR_NOGDI" );
	 break;
	 
    case DDERR_NOMIRRORHW:
	 strcpy( errorString, "DDERR_NOMIRRORHW" );
	 break;
	 
    case DDERR_NOTFOUND:
	 strcpy( errorString, "DDERR_NOTFOUND" );
	 break;
	 
    case DDERR_NOOVERLAYHW:
	 strcpy( errorString, "DDERR_NOOVERLAYHW" );
	 break;
	 
    case DDERR_OVERLAPPINGRECTS:
	 strcpy( errorString, "DDERR_OVERLAPPINGRECTS" );
	 break;
	 
    case DDERR_NORASTEROPHW:
	 strcpy( errorString, "DDERR_NORASTEROPHW" );
	 break;
	 
    case DDERR_NOROTATIONHW:
	 strcpy( errorString, "DDERR_NOROTATIONHW" );
	 break;
	 
    case DDERR_NOSTRETCHHW:
	 strcpy( errorString, "DDERR_NOSTRETCHHW" );
	 break;
	 
    case DDERR_NOT4BITCOLOR:
	 strcpy( errorString, "DDERR_NOT4BITCOLOR" );
	 break;
	 
    case DDERR_NOT4BITCOLORINDEX:
	 strcpy( errorString, "DDERR_NOT4BITCOLORINDEX" );
	 break;
	 
    case DDERR_NOT8BITCOLOR:
	 strcpy( errorString, "DDERR_NOT8BITCOLOR" );
	 break;
	 
    case DDERR_NOTEXTUREHW:
	 strcpy( errorString, "DDERR_NOTEXTUREHW" );
	 break;
	 
    case DDERR_NOVSYNCHW:
	 strcpy( errorString, "DDERR_NOVSYNCHW" );
	 break;
	 
    case DDERR_NOZBUFFERHW:
	 strcpy( errorString, "DDERR_NOZBUFFERHW" );
	 break;

    case DDERR_NOZOVERLAYHW:
	 strcpy( errorString, "DDERR_NOZOVERLAYHW" );
	 break;
	 
    case DDERR_OUTOFCAPS:
	 strcpy( errorString, "DDERR_OUTOFCAPS" );
	 break;
	 
    case DDERR_OUTOFMEMORY:
	 strcpy( errorString, "DDERR_OUTOFMEMORY" );
	 break;
	 
    case DDERR_OUTOFVIDEOMEMORY:
	 strcpy( errorString, "DDERR_OUTOFVIDEOMEMORY" );
	 break;
	 
    case DDERR_OVERLAYCANTCLIP:
	 strcpy( errorString, "DDERR_OVERLAYCANTCLIP" );
	 break;
	 
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
	 strcpy( errorString, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" );
	 break;
	 
    case DDERR_PALETTEBUSY:
	 strcpy( errorString, "DDERR_PALETTEBUSY" );
	 break;
	 
    case DDERR_COLORKEYNOTSET:
	 strcpy( errorString, "DDERR_COLORKEYNOTSET" );
	 break;
	 
    case DDERR_SURFACEALREADYATTACHED:
	 strcpy( errorString, "DDERR_SURFACEALREADYATTACHED" );
	 break;
	 
    case DDERR_SURFACEALREADYDEPENDENT:
	 strcpy( errorString, "DDERR_SURFACEALREADYDEPENDENT" );
	 break;
	 
    case DDERR_SURFACEBUSY:
	 strcpy( errorString, "DDERR_SURFACEBUSY" );
	 break;
	 
    case DDERR_CANTLOCKSURFACE:
	 strcpy( errorString, "DDERR_CANTLOCKSURFACE" );
	 break;

    case DDERR_SURFACEISOBSCURED:
	 strcpy( errorString, "DDERR_SURFACEISOBSCURED" );
	 break;

    case DDERR_SURFACELOST:
	 strcpy( errorString, "DDERR_SURFACELOST" );
	 break;
	 
    case DDERR_SURFACENOTATTACHED:
	 strcpy( errorString, "DDERR_SURFACENOTATTACHED" );
	 break;
	 
    case DDERR_TOOBIGHEIGHT:
	 strcpy( errorString, "DDERR_TOOBIGHEIGHT" );
	 break;
	 
    case DDERR_TOOBIGSIZE:
	 strcpy( errorString, "DDERR_TOOBIGSIZE" );
	 break;
	 
    case DDERR_TOOBIGWIDTH:
	 strcpy( errorString, "DDERR_TOOBIGWIDTH" );
	 break;
	 
    case DDERR_UNSUPPORTED:
	 strcpy( errorString, "DDERR_UNSUPPORTED" );
	 break;
	 
    case DDERR_UNSUPPORTEDFORMAT:
	 strcpy( errorString, "DDERR_UNSUPPORTEDFORMAT" );
	 break;
	 
    case DDERR_UNSUPPORTEDMASK:
	 strcpy( errorString, "DDERR_UNSUPPORTEDMASK" );
	 break;
	 
    case DDERR_INVALIDSTREAM:
	 strcpy( errorString, "DDERR_INVALIDSTREAM" );
	 break;
	 
    case DDERR_VERTICALBLANKINPROGRESS:
	 strcpy( errorString, "DDERR_VERTICALBLANKINPROGRESS" );
	 break;
	 
    case DDERR_WASSTILLDRAWING:
	 strcpy( errorString, "DDERR_WASSTILLDRAWING" );
	 break;
	 
    case DDERR_XALIGN:
	 strcpy( errorString, "DDERR_XALIGN" );
	 break;
	 
    case DDERR_INVALIDDIRECTDRAWGUID:
	 strcpy( errorString, "DDERR_INVALIDDIRECTDRAWGUID" );
	 break;
	 
    case DDERR_DIRECTDRAWALREADYCREATED:
	 strcpy( errorString, "DDERR_DIRECTDRAWALREADYCREATED" );
	 break;
	 
    case DDERR_NODIRECTDRAWHW:
	 strcpy( errorString, "DDERR_NODIRECTDRAWHW" );
	 break;
	 
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
	 strcpy( errorString, "DDERR_PRIMARYSURFACEALREADYEXISTS" );
	 break;
	 
    case DDERR_NOEMULATION:
	 strcpy( errorString, "DDERR_NOEMULATION" );
	 break;
	 
    case DDERR_REGIONTOOSMALL:
	 strcpy( errorString, "DDERR_REGIONTOOSMALL" );
	 break;
	 
    case DDERR_CLIPPERISUSINGHWND:
	 strcpy( errorString, "DDERR_CLIPPERISUSINGHWND" );
	 break;
	 
    case DDERR_NOCLIPPERATTACHED:
	 strcpy( errorString, "DDERR_NOCLIPPERATTACHED" );
	 break;
	 
    case DDERR_NOHWND:
	 strcpy( errorString, "DDERR_NOHWND" );
	 break;
	 
    case DDERR_HWNDSUBCLASSED:
	 strcpy( errorString, "DDERR_HWNDSUBCLASSED" );
	 break;
	 
    case DDERR_HWNDALREADYSET:
	 strcpy( errorString, "DDERR_HWNDALREADYSET" );
	 break;
	 
    case DDERR_NOPALETTEATTACHED:
	 strcpy( errorString, "DDERR_NOPALETTEATTACHED" );
	 break;
	 
    case DDERR_NOPALETTEHW:
	 strcpy( errorString, "DDERR_NOPALETTEHW" );
	 break;
	 
    case DDERR_BLTFASTCANTCLIP:
         strcpy( errorString, "DDERR_BLTFASTCANTCLIP" );
         break;

      case DDERR_NOBLTHW:
         strcpy( errorString, "DDERR_NOBLTHW" );
         break;

      case DDERR_NODDROPSHW:
         strcpy( errorString, "DDERR_NODDROPSHW" );
         break;

      case DDERR_OVERLAYNOTVISIBLE:
         strcpy( errorString, "DDERR_OVERLAYNOTVISIBLE" );
         break;

      case DDERR_NOOVERLAYDEST:
         strcpy( errorString, "DDERR_NOOVERLAYDEST" );
         break;

      case DDERR_INVALIDPOSITION:
         strcpy( errorString, "DDERR_INVALIDPOSITION" );
         break;

      case DDERR_NOTAOVERLAYSURFACE:
         strcpy( errorString, "DDERR_NOTAOVERLAYSURFACE" );
         break;

      case DDERR_EXCLUSIVEMODEALREADYSET:
         strcpy( errorString, "DDERR_EXCLUSIVEMODEALREADYSET" );
         break;

      case DDERR_NOTFLIPPABLE:
         strcpy( errorString, "DDERR_NOTFLIPPABLE" );
         break;

      case DDERR_CANTDUPLICATE:
         strcpy( errorString, "DDERR_CANTDUPLICATE" );
         break;

      case DDERR_NOTLOCKED:
         strcpy( errorString, "DDERR_NOTLOCKED" );
         break;

      case DDERR_CANTCREATEDC:
         strcpy( errorString, "DDERR_CANTCREATEDC" );
         break;

      case DDERR_NODC:
         strcpy( errorString, "DDERR_NODC" );
         break;

      case DDERR_WRONGMODE:
         strcpy( errorString, "DDERR_WRONGMODE" );
         break;

      case DDERR_IMPLICITLYCREATED:
         strcpy( errorString, "DDERR_IMPLICITLYCREATED" );
         break;

      case DDERR_NOTPALETTIZED:
         strcpy( errorString, "DDERR_NOTPALETTIZED" );
         break;

      case DDERR_UNSUPPORTEDMODE:
         strcpy( errorString, "DDERR_UNSUPPORTEDMODE" );
         break;

      case DDERR_NOMIPMAPHW:
         strcpy( errorString, "DDERR_NOMIPMAPHW" );
         break;

      case DDERR_INVALIDSURFACETYPE:
         strcpy( errorString, "DDERR_INVALIDSURFACETYPE" );
         break;

      case DDERR_NOOPTIMIZEHW:
         strcpy( errorString, "DDERR_NOOPTIMIZEHW" );
         break;

      case DDERR_NOTLOADED:
         strcpy( errorString, "DDERR_NOTLOADED" );
         break;

      case DDERR_NOFOCUSWINDOW:
         strcpy( errorString, "DDERR_NOFOCUSWINDOW" );
         break;

      case DDERR_DCALREADYCREATED:
         strcpy( errorString, "DDERR_DCALREADYCREATED" );
         break;

      case DDERR_NONONLOCALVIDMEM:
         strcpy( errorString, "DDERR_NONONLOCALVIDMEM" );
         break;

      case DDERR_CANTPAGELOCK:
         strcpy( errorString, "DDERR_CANTPAGELOCK" );
         break;

      case DDERR_CANTPAGEUNLOCK:
         strcpy( errorString, "DDERR_CANTPAGEUNLOCK" );
         break;

      case DDERR_NOTPAGELOCKED:
         strcpy( errorString, "DDERR_NOTPAGELOCKED" );
         break;

      case DDERR_MOREDATA:
         strcpy( errorString, "DDERR_MOREDATA" );
         break;

      case DDERR_EXPIRED:
         strcpy( errorString, "DDERR_EXPIRED" );
         break;

      case DDERR_VIDEONOTACTIVE:
         strcpy( errorString, "DDERR_VIDEONOTACTIVE" );
         break;

      case DDERR_DEVICEDOESNTOWNSURFACE:
         strcpy( errorString, "DDERR_DEVICEDOESNTOWNSURFACE" );
         break;

      case DDERR_NOTINITIALIZED:
         strcpy( errorString, "DDERR_NOTINITIALIZED" );
         break;

      default:
         strcpy( errorString, "<unknown error code>" );
         break;
   }

   return &errorString[0];
} 
