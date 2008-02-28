/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                	             */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include "D3DMesa.h"
/*===========================================================================*/
/*  This call will clear the render surface using the pixel info built from  */
/* the surface at creation time.  The call uses Lock/Unlock to access the    */
/* surface.  The call also special cases a full clear or a dirty rectangle.  */
/*  Finally the call returns the new clear mask that reflects that the color */
/* buffer was cleared.                                                       */
/*===========================================================================*/
/* RETURN: the original mask with the bits cleared that represents the buffer*/
/* or buffers we just cleared.                                               */
/*===========================================================================*/
GLbitfield ClearBuffers( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer,
                  *pScanLine;
   int            index,
	              index2;
   DWORD          dwColor;

   if ( mask & GL_COLOR_BUFFER_BIT )
   {
	/* Lock the surface to get the surface pointer. */
	pddsd2 = LockHAL( pContext->pShared, TRUE );
   
	/* Solve the color once only. */
	dwColor =  ( ((DWORD)((float)pContext->rClear * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	dwColor |= ( ((DWORD)((float)pContext->gClear * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	dwColor |= ( ((DWORD)((float)pContext->bClear * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );
   
	if ( all ) 
	{
	  for( index = 0, pScanLine = (UCHAR *)pddsd2->lpSurface; index < pContext->pShared->dwHeight; index++, pScanLine += pddsd2->lPitch )
	    for( pBuffer = pScanLine, index2 = 0; index2 < pContext->pShared->dwWidth; index2++, pBuffer += pContext->pShared->pixel.cb )
               memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	}
	else
	{
	  pScanLine = ((UCHAR *)pddsd2->lpSurface) +
          	    ( (FLIP( pContext->pShared->dwHeight, (y+height)) * pddsd2->lPitch) + (x * pContext->pShared->pixel.cb) );

	  for( index = 0; index < height; index++, pScanLine += pddsd2->lPitch )
	  {
	    for( index2 = 0, pBuffer = pScanLine; index2 < width; index2++, pBuffer += pContext->pShared->pixel.cb )
		 memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	  }
	}
   
	UnlockHAL( pContext->pShared, TRUE );
   }
   
   return (mask & ~GL_COLOR_BUFFER_BIT);
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGB( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgb[][3], const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          dwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   /* Find the start of the span. Invert y for Windows. */
   pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y) * pddsd2->lPitch) + (x*pContext->pShared->pixel.cb);

   if ( mask ) 
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	{
	  if ( mask[index] )
	  {
	    /* Pack the color components. */
	    dwColor =  ( ((DWORD)((float)rgb[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	    dwColor |= ( ((DWORD)((float)rgb[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	    dwColor |= ( ((DWORD)((float)rgb[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );
	    memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	  }
	}	
   }
   else
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	{
	  /* Pack the color components. */
	  dwColor =  ( ((DWORD)((float)rgb[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	  dwColor |= ( ((DWORD)((float)rgb[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	  dwColor |= ( ((DWORD)((float)rgb[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );
	  memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	}
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          dwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   /* Find the start of the span. Invert y for Windows. */
   pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y) * pddsd2->lPitch) + (x*pContext->pShared->pixel.cb);

   if ( mask ) 
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	{
	  if ( mask[index] )
	  {
         /* Pack the color components. */
	    dwColor =  ( ((DWORD)((float)rgba[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	    dwColor |= ( ((DWORD)((float)rgba[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	    dwColor |= ( ((DWORD)((float)rgba[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );
	    memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	  }
	}	
   }
   else
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	{
	  /* Pack the color components. */
	  dwColor =  ( ((DWORD)((float)rgba[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	  dwColor |= ( ((DWORD)((float)rgba[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	  dwColor |= ( ((DWORD)((float)rgba[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );
	  memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	}
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The color is solved once from the current    */
/* color components.  The alpha is ignored as Mesa is doing it in SW.        */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WSpanRGBAMono( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          dwColor;

   /* Lock the surface to get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   /* Solve the color once only. (no alpha) */
   dwColor =  ( ((DWORD)((float)pContext->rCurrent * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
   dwColor |= ( ((DWORD)((float)pContext->gCurrent * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
   dwColor |= ( ((DWORD)((float)pContext->bCurrent * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );

   /* Find the start of the span. Invert y for Windows. */
   pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y) * pddsd2->lPitch) + (x*pContext->pShared->pixel.cb);

   if ( mask ) 
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	  if ( mask[index] )
	    memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
   }
   else
   {
	for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
	  memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The alpha is ignored as Mesa does it in SW.  */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte rgba[][4], const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          dwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   if ( mask ) 
   {
	for( index = 0; index < n; index++ )
	{
	  if ( mask[index] )
	  {
	    /* Pack the color components. */
	    dwColor =  ( ((DWORD)((float)rgba[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	    dwColor |= ( ((DWORD)((float)rgba[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	    dwColor |= ( ((DWORD)((float)rgba[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );

	    /* Find the pixel. Invert y for Windows. */
	    pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb);
	    memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	  }
	}
   }
   else
   {
	for( index = 0; index < n; index++ )
	{
	  /* Pack the color components. */
	  dwColor =  ( ((DWORD)((float)rgba[index][RCOMP] * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
	  dwColor |= ( ((DWORD)((float)rgba[index][GCOMP] * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
	  dwColor |= ( ((DWORD)((float)rgba[index][BCOMP] * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );

	  /* Find the pixel. Invert y for Windows. */
	  pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb);
	  memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	}
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc (as all others) has been written for the general case. I use   */
/* the PIXELINFO structure to pack the pixel from RGB24 to whatever the Off- */
/* Screen render surface uses.  The color is solved once from the current    */
/* color components.  The alpha is ignored as Mesa is doing it in SW.        */
/*===========================================================================*/
/* RETURN:                                                                   */ 
/*===========================================================================*/
void WPixelsRGBAMono( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          dwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   /* Solve the color once only. I don't uses the alpha. */
   dwColor =  ( ((DWORD)((float)pContext->rCurrent * pContext->pShared->pixel.rScale)) << pContext->pShared->pixel.rShift );
   dwColor |= ( ((DWORD)((float)pContext->gCurrent * pContext->pShared->pixel.gScale)) << pContext->pShared->pixel.gShift );
   dwColor |= ( ((DWORD)((float)pContext->bCurrent * pContext->pShared->pixel.bScale)) << pContext->pShared->pixel.bShift );

   if ( mask ) 
   {
	/* We store the surface pointer as a UCHAR so that pixel.cb (count in btyles) will work for all. */
	for( index = 0; index < n; index++ )
	{
	  if ( mask[index] )
	  {
	    /* Find the pixel. Invert y for Windows. */
	    pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb);
	    memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	  }
	}
   }
   else
   {
	/* We store the surface pointer as a UCHAR so that pixel.cb (count in btyles) will work for all. */
	for( index = 0; index < n; index++ )
	{
	  /* Find the pixel. Invert y for Windows. */
	  pBuffer = (UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb);
	  memcpy( pBuffer, &dwColor, pContext->pShared->pixel.cb );
	}
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc isn't written for speed rather its to handle the general case. */
/* I grab each pixel from the surface and unpack the info using the PIXELINFO*/
/* structure that was generated from the OffScreen surface pixelformat.  The */
/* function will not fill in the alpha value as Mesa I have Mesa allocate its*/
/* own alpha channel when the context was created.  I did this as I didn't   */
/* feel that it was worth the effort to try and get HW to work (bus bound).  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void RSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   UCHAR          *pBuffer;
   int            index;
   DWORD          *pdwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   /* Find the start of the span. Invert y for Windows. */
   pBuffer = (UCHAR *)pddsd2->lpSurface + 
	                 (FLIP(pContext->pShared->dwHeight,y) * pddsd2->lPitch) + 
	                 (x*pContext->pShared->pixel.cb);

   /* We store the surface pointer as a UCHAR so that pixel.cb (count in btyles) will work for all. */
   for( index = 0; index < n; index++, pBuffer += pContext->pShared->pixel.cb )
   {
	pdwColor = (DWORD *)pBuffer;
	rgba[index][RCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwRMask) >> pContext->pShared->pixel.rShift) / pContext->pShared->pixel.rScale);
	rgba[index][GCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwGMask) >> pContext->pShared->pixel.gShift) / pContext->pShared->pixel.gScale);
	rgba[index][BCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwBMask) >> pContext->pShared->pixel.bShift) / pContext->pShared->pixel.bScale);
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
/*===========================================================================*/
/*  This proc isn't written for speed rather its to handle the general case. */
/* I grab each pixel from the surface and unpack the info using the PIXELINFO*/
/* structure that was generated from the OffScreen surface pixelformat.  The */
/* function will not fill in the alpha value as Mesa I have Mesa allocate its*/
/* own alpha channel when the context was created.  I did this as I didn't   */
/* feel that it was worth the effort to try and get HW to work (bus bound).  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
void RPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], GLubyte rgba[][4], const GLubyte mask[] )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DDSURFACEDESC2 *pddsd2;
   int            index;
   DWORD          *pdwColor;

   /* Get the surface pointer and the pitch. */
   pddsd2 = LockHAL( pContext->pShared, TRUE );

   if ( mask )
   {
	/* We store the surface pointer as a UCHAR so that pixel.cb (count in btyles) will work for all. */
	for( index = 0; index < n; index++ )
	{
	  if ( mask[index] )
	  {
	    /* Find the start of the pixel. Invert y for Windows. */
	    pdwColor = (DWORD *)((UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb));
	    rgba[index][RCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwRMask) >> pContext->pShared->pixel.rShift) / pContext->pShared->pixel.rScale);
	    rgba[index][GCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwGMask) >> pContext->pShared->pixel.gShift) / pContext->pShared->pixel.gScale);
	    rgba[index][BCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwBMask) >> pContext->pShared->pixel.bShift) / pContext->pShared->pixel.bScale);
	  }
	}
   }
   else
   {
	/* We store the surface pointer as a UCHAR so that pixel.cb (count in btyles) will work for all. */
	for( index = 0; index < n; index++ )
	{
	  /* Find the start of the pixel. Invert y for Windows. */
	  pdwColor = (DWORD *)((UCHAR *)pddsd2->lpSurface + (FLIP(pContext->pShared->dwHeight,y[index]) * pddsd2->lPitch) + (x[index]*pContext->pShared->pixel.cb));
	  rgba[index][RCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwRMask) >> pContext->pShared->pixel.rShift) / pContext->pShared->pixel.rScale);
	  rgba[index][GCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwGMask) >> pContext->pShared->pixel.gShift) / pContext->pShared->pixel.gScale);
	  rgba[index][BCOMP] = (GLubyte)((float)((*pdwColor & pContext->pShared->pixel.dwBMask) >> pContext->pShared->pixel.bShift) / pContext->pShared->pixel.bScale);
	}
   }

   /* Giver back. */
   UnlockHAL( pContext->pShared, TRUE );
}
