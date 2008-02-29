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
/* Macros.                                                                   */
/*===========================================================================*/
#define SRCBLEND_MAP(gl,d3d,fall)	if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwSrcBlendCaps & d3d ) \
                                   { \
					            sprintf( buffer, "SRC Blend: %s -> %s", # gl, # d3d ); \
                                     DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), buffer )); \
                                     pShared->dwSrcBlendCaps[index] = d3d; \
						     } \
							else \
                                   { \
					            sprintf( buffer, "SRC Blend: %s -> %s", # gl, # fall ); \
                                     DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), buffer )); \
                                     pShared->dwSrcBlendCaps[index] = fall; \
							}
#define DSTBLEND_MAP(gl,d3d,fall)	if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwDestBlendCaps & d3d ) \
                                   { \
					            sprintf( buffer, "DST Blend: %s -> %s", # gl, # d3d ); \
                                     DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), buffer )); \
                                     pShared->dwDestBlendCaps[index] = d3d; \
						     } \
							else \
                                   { \
					            sprintf( buffer, "DST Blend: %s -> %s", # gl, # fall ); \
                                     DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), buffer )); \
                                     pShared->dwDestBlendCaps[index] = fall; \
							}

/*===========================================================================*/
/*  I use this function to handle the fact that the D3D texture blending and */
/* OpenGL texture blending functions don't map one to one.  Also there is the*/
/* problem with cards not supporting all the D3D functions. So I use the CAPS*/
/* of the card to make a table of functions that will have defaults for the  */
/* unsupported functions.                                                    */
/*  So first I fill the table with the fallback function then I check to see */
/* if the card supports the requested function.  If it does I replace the    */
/* default thats already in the array.  Now order does matter as I used an   */
/* enum type in D3DShared.h so that the mapping would be a little easier.    */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void AlphaBlendTableHAL( PMESAD3DHAL pHAL )
{
  PMESAD3DSHARED	pShared = &pHAL->shared;
  int			index;
  char			buffer[128];

  DPF(( DBG_FUNC, "AlphaBlendTableHAL();" ));

  /* Make the fallback for the Source blend. */
  for( index = 0; index < 14; index++ )
  {
    switch( index )
    {
      case s_zero:
        SRCBLEND_MAP( GL_ZERO, D3DBLEND_ZERO, D3DBLEND_ONE );
	   break;
      case s_one:
        SRCBLEND_MAP( GL_ONE, D3DBLEND_ONE, D3DBLEND_ONE );
	   break;
      case s_dst_color:
        SRCBLEND_MAP( GL_DST_COLOR, D3DBLEND_DESTCOLOR, D3DBLEND_ONE );
	   break;
      case s_one_minus_dst_color:
        SRCBLEND_MAP( GL_ONE_MINUS_DST_COLOR, D3DBLEND_INVDESTCOLOR, D3DBLEND_ONE );
	   break;
      case s_src_alpha:
        SRCBLEND_MAP( GL_SRC_ALPHA, D3DBLEND_SRCALPHA, D3DBLEND_ONE );
	   break;
      case s_one_minus_src_alpha:
        SRCBLEND_MAP( GL_ONE_MINUS_SRC_ALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE );
	   break;
      case s_dst_alpha:
        SRCBLEND_MAP( GL_DST_ALPHA, D3DBLEND_DESTALPHA, D3DBLEND_ONE );
	   break;
      case s_one_minus_dst_alpha:
        SRCBLEND_MAP( GL_ONE_MINUS_DST_ALPHA, D3DBLEND_INVDESTALPHA, D3DBLEND_ONE );
	   break;
      case s_src_alpha_saturate:
        SRCBLEND_MAP( GL_SRC_ALPHA_SATURATE, D3DBLEND_SRCALPHASAT, D3DBLEND_ONE );
	   break;
      case s_constant_color:
        SRCBLEND_MAP( GL_CONSTANT_COLOR, D3DBLEND_SRCCOLOR, D3DBLEND_ONE );
	   break;
      case s_one_minus_constant_color:
        SRCBLEND_MAP( GL_ONE_MINUS_CONSTANT_COLOR, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE );
	   break;
      case s_constant_alpha:
        SRCBLEND_MAP( GL_CONSTANT_ALPHA, D3DBLEND_BOTHSRCALPHA, D3DBLEND_ONE );
	   break;
      case s_one_minus_constant_alpha:
        SRCBLEND_MAP( GL_ONE_MINUS_CONSTANT_ALPHA, D3DBLEND_BOTHINVSRCALPHA, D3DBLEND_ONE );
	   break;
    }
  }

  /* Make the fallback for the Destination blend. */
  for( index = 0; index < 14; index++ )
  {
    switch( index )
    {
      case d_zero:
        DSTBLEND_MAP( GL_ZERO, D3DBLEND_ZERO, D3DBLEND_ONE );
	   break;
      case d_one:
        DSTBLEND_MAP( GL_ONE, D3DBLEND_ONE, D3DBLEND_ONE );
	   break;
      case d_src_color:
        DSTBLEND_MAP( GL_SRC_COLOR, D3DBLEND_SRCCOLOR, D3DBLEND_ONE );
	   break;
      case d_one_minus_src_color:
        DSTBLEND_MAP( GL_ONE_MINUS_SRC_COLOR, D3DBLEND_INVSRCCOLOR, D3DBLEND_ONE );
	   break;
      case d_src_alpha:
        DSTBLEND_MAP( GL_SRC_ALPHA, D3DBLEND_SRCALPHA, D3DBLEND_ONE );
	   break;
      case d_one_minus_src_alpha:
        DSTBLEND_MAP( GL_ONE_MINUS_SRC_ALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE );
	   break;
      case d_dst_alpha:
        DSTBLEND_MAP( GL_DST_ALPHA, D3DBLEND_DESTALPHA, D3DBLEND_ONE );
	   break;
      case d_one_minus_dst_alpha:
        DSTBLEND_MAP( GL_ONE_MINUS_DST_ALPHA, D3DBLEND_INVDESTALPHA, D3DBLEND_ONE );
	   break;
      case d_constant_color:
        DSTBLEND_MAP( GL_CONSTANT_COLOR, D3DBLEND_DESTCOLOR, D3DBLEND_ONE );
	   break;
      case d_one_minus_constant_color:
        DSTBLEND_MAP( GL_ONE_MINUS_CONSTANT_COLOR, D3DBLEND_INVDESTCOLOR, D3DBLEND_ONE );
	   break;
      case d_constant_alpha:
        DSTBLEND_MAP( GL_CONSTANT_ALPHAR, D3DBLEND_BOTHSRCALPHA, D3DBLEND_ONE );
	   break;
      case d_one_minus_constant_alpha:
        DSTBLEND_MAP( GL_ONE_MINUS_CONSTANT_ALPHA, D3DBLEND_BOTHINVSRCALPHA, D3DBLEND_ONE );
	   break;
    }
  }

  /* Make the fallbacks for the texture functions. */
  for( index = 0; index < 4; index++ )
  {
    switch( index )
    {
      case d3dtblend_decal:
	   if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_DECAL )
	   {
		DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECAL -> D3DTBLEND_DECAL" )); 
		pShared->dwTexFunc[index] = D3DTBLEND_DECAL; 
	   }
        else
	   {
		if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_MODULATE )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECAL -> D3DTBLEND_MODULATE" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_MODULATE; 
		}
		else
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECAL -> D3DTBLEND_ADD" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_ADD; 
		}
	   }
	   break;
      case d3dtblend_decalalpha:
	   if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_DECALALPHA )
	   {
		DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECALALPHA -> D3DTBLEND_DECALALPHA" )); 
		pShared->dwTexFunc[index] = D3DTBLEND_DECALALPHA; 
	   }
        else
	   {
		if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_DECAL )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECALALPA -> D3DTBLEND_DECAL" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_DECAL; 
		}
		else
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_DECALALPHA -> D3DTBLEND_ADD" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_ADD; 
		}
	   }
	   break;
    case d3dtblend_modulate:
	   if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_MODULATE )
	   {
		DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATE -> D3DTBLEND_MODULATE" )); 
		pShared->dwTexFunc[index] = D3DTBLEND_MODULATE; 
	   }
        else
	   {
		if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_MODULATEALPHA )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATE -> D3DTBLEND_MODULATEALPHA" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_MODULATEALPHA; 
		}
		else	if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_DECAL )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATE -> D3DTBLEND_DECAL" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_DECAL; 
		}
		else
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATE -> D3DTBLEND_ADD" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_ADD; 
		}
	   }
	   break;
    case d3dtblend_modulatealpha:
	   if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_MODULATEALPHA )
	   {
		DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATEALPHA -> D3DTBLEND_MODULATEALPHA" )); 
		pShared->dwTexFunc[index] = D3DTBLEND_MODULATEALPHA; 
	   }
        else
	   {
		if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_MODULATE )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATEALPHA -> D3DTBLEND_MODULATE" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_MODULATE;
		}
		else	if ( pHAL->D3DHWDevDesc.dpcTriCaps.dwTextureBlendCaps & D3DTBLEND_DECAL )
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATEALPHA -> D3DTBLEND_DECALE" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_DECAL; 
		}
		else
		{
		  DPF(( (DBG_CNTX_INFO|DBG_TXT_INFO), "D3DTBLEND_MODULATEALPHA -> D3DTBLEND_ADD" )); 
		  pShared->dwTexFunc[index] = D3DTBLEND_ADD; 
		}
	   }
	   break;
    }
  }
}

