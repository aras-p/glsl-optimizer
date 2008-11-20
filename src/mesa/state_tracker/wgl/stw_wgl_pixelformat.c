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

#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "stw_pixelformat.h"
#include "stw_wgl_pixelformat.h"

static uint currentpixelformat = 0;

WINGDIAPI int APIENTRY
wglChoosePixelFormat(
   HDC hdc,
   CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   uint count;
   uint index;
   uint bestindex;
   uint bestdelta;

   (void) hdc;

   count = pixelformat_get_count();
   bestindex = count;
   bestdelta = 0xffffffff;

   if (ppfd->nSize != sizeof( PIXELFORMATDESCRIPTOR ) || ppfd->nVersion != 1)
      return 0;
   if (ppfd->iPixelType != PFD_TYPE_RGBA)
      return 0;
   if (!(ppfd->dwFlags & PFD_DRAW_TO_WINDOW))
      return 0;
   if (!(ppfd->dwFlags & PFD_SUPPORT_OPENGL))
      return 0;
   if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP)
      return 0;
   if (ppfd->dwFlags & PFD_SUPPORT_GDI)
      return 0;
   if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE) && (ppfd->dwFlags & PFD_STEREO))
      return 0;

   for (index = 0; index < count; index++) {
      uint delta = 0;
      const struct pixelformat_info *pf = pixelformat_get_info( index );

      if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE)) {
         if ((ppfd->dwFlags & PFD_DOUBLEBUFFER) && !(pf->flags & PF_FLAG_DOUBLEBUFFER))
            continue;
         if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER) && (pf->flags & PF_FLAG_DOUBLEBUFFER))
            continue;
      }

      if (ppfd->cColorBits != pf->color.redbits + pf->color.greenbits + pf->color.bluebits)
         delta += 8;

      if (ppfd->cDepthBits != pf->depth.depthbits)
         delta += 4;

      if (ppfd->cStencilBits != pf->depth.stencilbits)
         delta += 2;

      if (ppfd->cAlphaBits != pf->alpha.alphabits)
         delta++;

      if (delta < bestdelta) {
         bestindex = index;
         bestdelta = delta;
         if (bestdelta == 0)
            break;
      }
   }

   if (bestindex == count)
      return 0;
   return bestindex + 1;
}

WINGDIAPI int APIENTRY
wglDescribePixelFormat(
   HDC hdc,
   int iPixelFormat,
   UINT nBytes,
   LPPIXELFORMATDESCRIPTOR ppfd )
{
   uint count;
   uint index;
   const struct pixelformat_info *pf;

   (void) hdc;

   count = pixelformat_get_extended_count();
   index = (uint) iPixelFormat - 1;

   if (ppfd == NULL)
      return count;
   if (index >= count || nBytes != sizeof( PIXELFORMATDESCRIPTOR ))
      return 0;

   pf = pixelformat_get_info( index );

   ppfd->nSize = sizeof( PIXELFORMATDESCRIPTOR );
   ppfd->nVersion = 1;
   ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   if (pf->flags & PF_FLAG_DOUBLEBUFFER)
      ppfd->dwFlags |= PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
   ppfd->iPixelType = PFD_TYPE_RGBA;
   ppfd->cColorBits = pf->color.redbits + pf->color.greenbits + pf->color.bluebits;
   ppfd->cRedBits = pf->color.redbits;
   ppfd->cRedShift = pf->color.redshift;
   ppfd->cGreenBits = pf->color.greenbits;
   ppfd->cGreenShift = pf->color.greenshift;
   ppfd->cBlueBits = pf->color.bluebits;
   ppfd->cBlueShift = pf->color.blueshift;
   ppfd->cAlphaBits = pf->alpha.alphabits;
   ppfd->cAlphaShift = pf->alpha.alphashift;
   ppfd->cAccumBits = 0;
   ppfd->cAccumRedBits = 0;
   ppfd->cAccumGreenBits = 0;
   ppfd->cAccumBlueBits = 0;
   ppfd->cAccumAlphaBits = 0;
   ppfd->cDepthBits = pf->depth.depthbits;
   ppfd->cStencilBits = pf->depth.stencilbits;
   ppfd->cAuxBuffers = 0;
   ppfd->iLayerType = 0;
   ppfd->bReserved = 0;
   ppfd->dwLayerMask = 0;
   ppfd->dwVisibleMask = 0;
   ppfd->dwDamageMask = 0;

   return count;
}

WINGDIAPI int APIENTRY
wglGetPixelFormat(
   HDC hdc )
{
   (void) hdc;

   return currentpixelformat;
}

WINGDIAPI BOOL APIENTRY
wglSetPixelFormat(
   HDC hdc,
   int iPixelFormat,
   const PIXELFORMATDESCRIPTOR *ppfd )
{
   uint count;
   uint index;

   (void) hdc;

   count = pixelformat_get_extended_count();
   index = (uint) iPixelFormat - 1;

   if (index >= count || ppfd->nSize != sizeof( PIXELFORMATDESCRIPTOR ))
      return FALSE;

   currentpixelformat = index + 1;
   return TRUE;
}
