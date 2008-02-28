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
#include "Debug.h"
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
DWORD	g_DBGMask = DBG_ALL_ERROR;
/*===========================================================================*/
/*  This is your basic DPF function with printf like support.  The function  */
/* also works with a global debug mask variable.  I have written support that*/
/* allows for the user's enviroment variable space to be read and set the    */
/* masks.  This is done when the dll starts and is only in the debug version.*/
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void _cdecl DebugPrint( int mask, char *pszFormat, ... )
{
  char	buffer[512];
  va_list	args;

  /* A mask of 0 will always pass. Easy to remeber. */
  if ( (mask == 0) || (mask & g_DBGMask) ) 
  {
    va_start( args, pszFormat );

    if ( mask & DBG_ALL_ERROR ) 
	 OutputDebugString( "MesaD3D: (ERROR)" ); 
    else
	 OutputDebugString( "MesaD3D: " ); 

    vsprintf( buffer, pszFormat, args );
    strcat( buffer, "\n" );
    OutputDebugString( buffer );

    va_end( args );
  }
}
/*===========================================================================*/
/*  This call reads the users enviroment variables and sets any debug mask   */
/* that they have set to TRUE.  Now the value must be "TRUE".                */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void	ReadDBGEnv( void )
{
  g_DBGMask = DBG_ALL_ERROR;

#define IS_VAR_SET(v)	if ( getenv( # v ) && !strcmp(getenv( # v ),"TRUE") ) g_DBGMask |= v;
  
  IS_VAR_SET( DBG_FUNC );
  IS_VAR_SET( DBG_STATES );

  IS_VAR_SET( DBG_CNTX_INFO );
  IS_VAR_SET( DBG_CNTX_WARN );
  IS_VAR_SET( DBG_CNTX_PROFILE );
  IS_VAR_SET( DBG_CNTX_ERROR );
  IS_VAR_SET( DBG_CNTX_ALL );

  IS_VAR_SET( DBG_PRIM_INFO );
  IS_VAR_SET( DBG_PRIM_WARN );
  IS_VAR_SET( DBG_PRIM_PROFILE );
  IS_VAR_SET( DBG_PRIM_ERROR );
  IS_VAR_SET( DBG_PRIM_ALL );

  IS_VAR_SET( DBG_TXT_INFO );
  IS_VAR_SET( DBG_TXT_WARN );
  IS_VAR_SET( DBG_TXT_PROFILE );
  IS_VAR_SET( DBG_TXT_ERROR );
  IS_VAR_SET( DBG_TXT_ALL );

  IS_VAR_SET( DBG_ALL_INFO );
  IS_VAR_SET( DBG_ALL_WARN );
  IS_VAR_SET( DBG_ALL_PROFILE );
  IS_VAR_SET( DBG_ALL_ERROR );
  IS_VAR_SET( DBG_ALL );

#undef IS_VAR_SET
}
/*===========================================================================*/
/*  This function will take a pointer to a DDSURFACEDESC2 structure & display*/
/* the parsed information using a DPF call.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void	DebugPixelFormat( char *pszSurfaceName, DDPIXELFORMAT *pddpf )
{
  char	buffer[256];

  /* Parse the flag type and write the string equivalent. */
  if ( pddpf->dwFlags & DDPF_ALPHA )
	strcat( buffer, "DDPF_ALPHA " );
  if ( pddpf->dwFlags & DDPF_ALPHAPIXELS )
	strcat( buffer, "DDPF_ALPHAPIXELS " );
  if ( pddpf->dwFlags & DDPF_ALPHAPREMULT )
	strcat( buffer, "DDPF_ALPHAPREMULT " );
  if ( pddpf->dwFlags & DDPF_BUMPLUMINANCE )
	strcat( buffer, "DDPF_BUMPLUMINANCE " );
  if ( pddpf->dwFlags & DDPF_BUMPDUDV )
	strcat( buffer, "DDPF_BUMPDUDV " );
  if ( pddpf->dwFlags & DDPF_COMPRESSED )
	strcat( buffer, "DDPF_COMPRESSED " );
  if ( pddpf->dwFlags & DDPF_FOURCC )
	strcat( buffer, "DDPF_FOURCC " );
  if ( pddpf->dwFlags & DDPF_LUMINANCE )
	strcat( buffer, "DDPF_LUMINANCE " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED1 )
	strcat( buffer, "DDPF_PALETTEINDEXED1 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED2 )
	strcat( buffer, "DDPF_PALETTEINDEXED2 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED4 )
	strcat( buffer, "DDPF_PALETTEINDEXED4 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXED8 )
	strcat( buffer, "DDPF_PALETTEINDEXED8 " );
  if ( pddpf->dwFlags & DDPF_PALETTEINDEXEDTO8 )
	strcat( buffer, "DDPF_PALETTEINDEXEDTO8 " );
  if ( pddpf->dwFlags & DDPF_RGB )
	strcat( buffer, "DDPF_RGB  " );
  if ( pddpf->dwFlags & DDPF_RGBTOYUV )
	strcat( buffer, "DDPF_RGBTOYUV  " );
  if ( pddpf->dwFlags & DDPF_STENCILBUFFER )
	strcat( buffer, "DDPF_STENCILBUFFER  " );
  if ( pddpf->dwFlags & DDPF_YUV )
	strcat( buffer, "DDPF_YUV  " );
  if ( pddpf->dwFlags & DDPF_ZBUFFER )
	strcat( buffer, "DDPF_ZBUFFER  " );
  if ( pddpf->dwFlags & DDPF_ZPIXELS )
	strcat( buffer, "DDPF_ZPIXELS  " );

  DPF(( (DBG_TXT_INFO|DBG_CNTX_INFO),"%s", buffer ));
}





