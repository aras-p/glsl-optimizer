/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#define _GDI32_

#include <windows.h>

#include "pipe/p_debug.h"

WINGDIAPI BOOL APIENTRY
wglUseFontBitmapsA(
   HDC hdc,
   DWORD first,
   DWORD count,
   DWORD listBase )
{
   (void) hdc;
   (void) first;
   (void) count;
   (void) listBase;

   assert( 0 );

   return FALSE;
}

WINGDIAPI BOOL APIENTRY
wglShareLists(
   HGLRC hglrc1,
   HGLRC hglrc2 )
{
   (void) hglrc1;
   (void) hglrc2;

   assert( 0 );

   return FALSE;
}

WINGDIAPI BOOL APIENTRY
wglUseFontBitmapsW(
   HDC hdc,
   DWORD first,
   DWORD count,
   DWORD listBase )
{
   (void) hdc;
   (void) first;
   (void) count;
   (void) listBase;

   assert( 0 );

   return FALSE;
}

WINGDIAPI BOOL APIENTRY
wglUseFontOutlinesA(
   HDC hdc,
   DWORD first,
   DWORD count,
   DWORD listBase,
   FLOAT deviation,
   FLOAT extrusion,
   int format,
   LPGLYPHMETRICSFLOAT lpgmf )
{
   (void) hdc;
   (void) first;
   (void) count;
   (void) listBase;
   (void) deviation;
   (void) extrusion;
   (void) format;
   (void) lpgmf;

   assert( 0 );

   return FALSE;
}

WINGDIAPI BOOL APIENTRY
wglUseFontOutlinesW(
   HDC hdc,
   DWORD first,
   DWORD count,
   DWORD listBase,
   FLOAT deviation,
   FLOAT extrusion,
   int format,
   LPGLYPHMETRICSFLOAT lpgmf )
{
   (void) hdc;
   (void) first;
   (void) count;
   (void) listBase;
   (void) deviation;
   (void) extrusion;
   (void) format;
   (void) lpgmf;

   assert( 0 );

   return FALSE;
}

WINGDIAPI BOOL APIENTRY
wglDescribeLayerPlane(
   HDC hdc,
   int iPixelFormat,
   int iLayerPlane,
   UINT nBytes,
   LPLAYERPLANEDESCRIPTOR plpd )
{
   (void) hdc;
   (void) iPixelFormat;
   (void) iLayerPlane;
   (void) nBytes;
   (void) plpd;

   assert( 0 );

   return FALSE;
}

WINGDIAPI int APIENTRY
wglSetLayerPaletteEntries(
   HDC hdc,
   int iLayerPlane,
   int iStart,
   int cEntries,
   CONST COLORREF *pcr )
{
   (void) hdc;
   (void) iLayerPlane;
   (void) iStart;
   (void) cEntries;
   (void) pcr;

   assert( 0 );

   return 0;
}

WINGDIAPI int APIENTRY
wglGetLayerPaletteEntries(
   HDC hdc,
   int iLayerPlane,
   int iStart,
   int cEntries,
   COLORREF *pcr )
{
   (void) hdc;
   (void) iLayerPlane;
   (void) iStart;
   (void) cEntries;
   (void) pcr;

   assert( 0 );

   return 0;
}

WINGDIAPI BOOL APIENTRY
wglRealizeLayerPalette(
   HDC hdc,
   int iLayerPlane,
   BOOL bRealize )
{
   (void) hdc;
   (void) iLayerPlane;
   (void) bRealize;

   assert( 0 );

   return FALSE;
}
