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

#include "pipe/p_debug.h"
#include "stw_pixelformat.h"

#define MAX_PIXELFORMATS   16

static struct pixelformat_info pixelformats[MAX_PIXELFORMATS];
static uint pixelformat_count = 0;
static uint pixelformat_extended_count = 0;

static uint currentpixelformat = 0;


static void
add_standard_pixelformats(
   struct pixelformat_info **ppf,
   uint flags )
{
   struct pixelformat_info *pf = *ppf;
   struct pixelformat_color_info color24 = { 8, 0, 8, 8, 8, 16 };
   struct pixelformat_alpha_info alpha8 = { 8, 24 };
   struct pixelformat_alpha_info noalpha = { 0, 0 };
   struct pixelformat_depth_info depth24s8 = { 24, 8 };
   struct pixelformat_depth_info depth16 = { 16, 0 };

   pf->flags = PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth16;
   pf++;

   pf->flags = PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth24s8;
   pf++;

   pf->flags = PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth16;
   pf++;

   pf->flags = PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth24s8;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth16;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth24s8;
   pf++;

   *ppf = pf;
}

void
pixelformat_init( void )
{
   struct pixelformat_info *pf = pixelformats;

   add_standard_pixelformats( &pf, 0 );
   pixelformat_count = pf - pixelformats;

   add_standard_pixelformats( &pf, PF_FLAG_MULTISAMPLED );
   pixelformat_extended_count = pf - pixelformats;

   assert( pixelformat_extended_count <= MAX_PIXELFORMATS );
}

uint
pixelformat_get_count( void )
{
   return pixelformat_count;
}

uint
pixelformat_get_extended_count( void )
{
   return pixelformat_extended_count;
}

const struct pixelformat_info *
pixelformat_get_info( uint index )
{
   assert( index < pixelformat_extended_count );

   return &pixelformats[index];
}


int
stw_pixelformat_describe(
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


int
stw_pixelformat_get(
   HDC hdc )
{
   return currentpixelformat;
}


BOOL
stw_pixelformat_set(
   HDC hdc,
   int iPixelFormat )
{
   uint count;
   uint index;

   (void) hdc;

   index = (uint) iPixelFormat - 1;
   count = pixelformat_get_extended_count();
   if (index >= count)
      return FALSE;

   currentpixelformat = iPixelFormat;
   return TRUE;
}
