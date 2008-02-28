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
/*  This function clears the context bound to the supplied shared context.   */
/* The function takes the D3D flags D3DCLEAR_TARGET, D3DCLEAR_STENCIL and    */
/* D3DCLEAR_ZBUFFER.  Set bAll to TRUE for a full clear else supply the coord*/
/* of the rect to be cleared relative to the window.  The color is always a  */
/* 32bit value (RGBA).  Fill in the z-value and stencil if needed.           */
/*                                                                           */
/*  TODO: this can be redone to be called by Mesa directly.                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void ClearHAL( PMESAD3DSHARED pShared, DWORD dwFlags, BOOL bAll, int x, int y, int cx, int cy, DWORD dwColor, float zv, DWORD dwStencil )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;
  D3DRECT		d3dRect;

#ifdef D3D_DEBUG
  HRESULT		rc;

  DPF(( DBG_FUNC, "CleaHAL();" ));

  /* Make sure we have enough info. */
  if ( (pHAL == NULL) || (pHAL->lpViewport == NULL) )
    return;
#endif

  if ( bAll )
  {
    /* I assume my viewport is valid. */
    d3dRect.lX1 = pShared->rectV.left;
    d3dRect.lY1 = pShared->rectV.top;
    d3dRect.lX2 = pShared->rectV.right;
    d3dRect.lY2 = pShared->rectV.bottom;
  }
  else
  {
    d3dRect.lX1 = pShared->rectV.left + x;
    d3dRect.lY1 = pShared->rectV.top  + y;
    d3dRect.lX2 = d3dRect.lX1 + cx;
    d3dRect.lY2 = d3dRect.lY1 + cy;
  }

#ifdef D3D_DEBUG
  rc = pHAL->lpViewport->Clear2( 1, &d3dRect, dwFlags, dwColor, zv, dwStencil );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "Clear2 ->", ErrorStringD3D(rc) );
  }
#else
  pHAL->lpViewport->Clear2( 1, &d3dRect, dwFlags, dwColor, zv, dwStencil );
#endif
}
/*===========================================================================*/
/*  Well this is the guts of it all.  Here we rasterize the primitives that  */
/* are in their final form.  OpenGL has done all the lighting, transfomations*/
/* and clipping at this point.                                               */
/*                                                                           */
/* TODO:  I'm not sure if I want to bother to check for errors on this call. */
/*       The overhead kills me...                                            */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void DrawPrimitiveHAL( PMESAD3DSHARED pShared, D3DPRIMITIVETYPE dptPrimitiveType, D3DTLVERTEX *pVertices, DWORD dwCount )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;

#ifdef D3D_DEBUG
  HRESULT		rc;      

  DPF(( DBG_FUNC, "DrawPrimitveHAL();" ));

  /* Make sure we have enough info. */
  if ( (pHAL == NULL) || (pHAL->lpD3DDevice == NULL) )
    return;

  DPF(( DBG_PRIM_INFO, "DP( %d )", dwCount ));

  rc = pHAL->lpD3DDevice->DrawPrimitive( dptPrimitiveType,
								 D3DFVF_TLVERTEX,
								 (LPVOID)pVertices,
								 dwCount, 
								 (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "DrawPrimitive ->", ErrorStringD3D(rc) );
  }
#else
  pHAL->lpD3DDevice->DrawPrimitive( dptPrimitiveType,
							 D3DFVF_TLVERTEX,
							 (LPVOID)pVertices,
							 dwCount, 
							 (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
#endif
}
/*===========================================================================*/
/*  This call will handle the swapping of the buffers.  Now I didn't bother  */
/* to support single buffered so this will be used for glFlush() as its all  */
/* the same.  So first we do an EndScene as we are always considered to be in*/
/* a BeginScene because when we leave we do a BeginScene.  Now note that when*/
/* the context is created in the first place we do a BeginScene also just to */
/* get things going.  The call will use either Flip/blt based on the type of */
/* surface was created for rendering.                                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void SwapBuffersHAL( PMESAD3DSHARED pShared )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;

#ifdef D3D_DEBUG
  HRESULT		rc;      

  DPF(( DBG_FUNC, "SwapBuffersHAL();" ));
  DPF(( DBG_ALL_PROFILE, "=================SWAP===================" ));

  /* Make sure we have enough info. */
  if ( (pHAL == NULL) || (pHAL->lpD3DDevice == NULL) )
    return;

  /* Make sure we have enough info. */
  if ( pHAL->lpDDSPrimary != NULL )
  {
    rc = pHAL->lpD3DDevice->EndScene();   
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "EndScene ->", ErrorStringD3D(rc) );
    }
 
    if ( pShared->bFlipable )
    {
	 DPF(( DBG_CNTX_PROFILE, "Swap->FLIP" ));
	 rc = pHAL->lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
    }
    else
    {
	 DPF(( DBG_CNTX_PROFILE, "Swap->Blt" ));
	 rc = pHAL->lpDDSPrimary->Blt( &pShared->rectW, pHAL->lpDDSRender, NULL, DDBLT_WAIT, NULL );
    }
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "Blt (RENDER/PRIMARY) ->", ErrorStringD3D(rc) );
    }

    rc = pHAL->lpD3DDevice->BeginScene(); 
    if ( FAILED(rc) )
    {
	 RIP( pHAL, "BeginScene ->", ErrorStringD3D(rc) );
    }
  }
#else
  pHAL->lpD3DDevice->EndScene();   

  if ( pShared->bFlipable )
    pHAL->lpDDSPrimary->Flip( NULL, DDFLIP_WAIT );
  else
    pHAL->lpDDSPrimary->Blt( &pShared->rectW, pHAL->lpDDSRender, NULL, DDBLT_WAIT, NULL );

  pHAL->lpD3DDevice->BeginScene(); 

#endif
}
/*===========================================================================*/
/*  This function is a very thin wrapper for the D3D call 'SetRenderState'.  */
/* Using this function requires all the types to be defined by including the */
/* D3D header file.                                                          */
/*                                                                           */
/*  TODO:  would be much better to get ride of all these calls per VBRender. */
/*        I feel I should get this call into SetRenderStates() the RenderVB. */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void SetStateHAL( PMESAD3DSHARED pShared, DWORD dwType, DWORD dwState )
{
  PMESAD3DHAL	pHAL = (PMESAD3DHAL)pShared;

#ifdef D3D_DEBUG   
  HRESULT		rc;

  DPF(( DBG_FUNC, "SetStateHAL();" ));

  /* Make sure we have enough info. */
  if ( (pHAL == NULL) || (pHAL->lpD3DDevice == NULL) )
    return;

  rc = pHAL->lpD3DDevice->SetRenderState( (D3DRENDERSTATETYPE)dwType, dwState );
  if ( FAILED(rc) )
  {
    RIP( pHAL, "SetRenderState ->", ErrorStringD3D(rc) );
  }

#else
  pHAL->lpD3DDevice->SetRenderState( (D3DRENDERSTATETYPE)dwType, dwState );
#endif
}








