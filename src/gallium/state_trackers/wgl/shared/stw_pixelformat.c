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

#include "util/u_debug.h"
#include "stw_pixelformat.h"
#include "stw_public.h"
#include "stw_tls.h"

#define STW_MAX_PIXELFORMATS   16

static struct stw_pixelformat_info stw_pixelformats[STW_MAX_PIXELFORMATS];
static uint stw_pixelformat_count = 0;
static uint stw_pixelformat_extended_count = 0;


static void
stw_add_standard_pixelformats(
   struct stw_pixelformat_info **ppf,
   uint flags )
{
   struct stw_pixelformat_info *pf = *ppf;
   struct stw_pixelformat_color_info color24 = { 8, 0, 8, 8, 8, 16 };
   struct stw_pixelformat_alpha_info noalpha = { 0, 0 };
   struct stw_pixelformat_alpha_info alpha8 = { 8, 24 };
   struct stw_pixelformat_depth_info depth24s8 = { 24, 8 };
   struct stw_pixelformat_depth_info depth16 = { 16, 0 };

   pf->flags = STW_PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth24s8;
   pf++;

   pf->flags = STW_PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth24s8;
   pf++;

   pf->flags = STW_PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth16;
   pf++;

   pf->flags = STW_PF_FLAG_DOUBLEBUFFER | flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth16;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth24s8;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth24s8;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = noalpha;
   pf->depth = depth16;
   pf++;

   pf->flags = flags;
   pf->color = color24;
   pf->alpha = alpha8;
   pf->depth = depth16;
   pf++;

   *ppf = pf;
}

void
stw_pixelformat_init( void )
{
   struct stw_pixelformat_info *pf = stw_pixelformats;

   stw_add_standard_pixelformats( &pf, 0 );
   stw_pixelformat_count = pf - stw_pixelformats;

   stw_add_standard_pixelformats( &pf, STW_PF_FLAG_MULTISAMPLED );
   stw_pixelformat_extended_count = pf - stw_pixelformats;

   assert( stw_pixelformat_extended_count <= STW_MAX_PIXELFORMATS );
}

uint
stw_pixelformat_get_count( void )
{
   return stw_pixelformat_count;
}

uint
stw_pixelformat_get_extended_count( void )
{
   return stw_pixelformat_extended_count;
}

const struct stw_pixelformat_info *
stw_pixelformat_get_info( uint index )
{
   assert( index < stw_pixelformat_extended_count );

   return &stw_pixelformats[index];
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
   const struct stw_pixelformat_info *pf;

   (void) hdc;

   count = stw_pixelformat_get_extended_count();
   index = (uint) iPixelFormat - 1;

   if (ppfd == NULL)
      return count;
   if (index >= count || nBytes != sizeof( PIXELFORMATDESCRIPTOR ))
      return 0;

   pf = stw_pixelformat_get_info( index );

   ppfd->nSize = sizeof( PIXELFORMATDESCRIPTOR );
   ppfd->nVersion = 1;
   ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
   if (pf->flags & STW_PF_FLAG_DOUBLEBUFFER)
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

/* Only used by the wgl code, but have it here to avoid exporting the
 * pixelformat.h functionality.
 */
int stw_pixelformat_choose( HDC hdc,
                            CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   uint count;
   uint index;
   uint bestindex;
   uint bestdelta;

   (void) hdc;

   count = stw_pixelformat_get_count();
   bestindex = count;
   bestdelta = ~0U;

   for (index = 0; index < count; index++) {
      uint delta = 0;
      const struct stw_pixelformat_info *pf = stw_pixelformat_get_info( index );

      if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
          !!(ppfd->dwFlags & PFD_DOUBLEBUFFER) !=
          !!(pf->flags & STW_PF_FLAG_DOUBLEBUFFER))
         continue;

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


int
stw_pixelformat_get(
   HDC hdc )
{
   return stw_tls_get_data()->currentPixelFormat;
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
   count = stw_pixelformat_get_extended_count();
   if (index >= count)
      return FALSE;

   stw_tls_get_data()->currentPixelFormat = iPixelFormat;

   /* Some applications mistakenly use the undocumented wglSetPixelFormat 
    * function instead of SetPixelFormat, so we call SetPixelFormat here to 
    * avoid opengl32.dll's wglCreateContext to fail */
   if (GetPixelFormat(hdc) == 0) {
        SetPixelFormat(hdc, iPixelFormat, NULL);
   }
   
   return TRUE;
}



/* XXX: this needs to be turned into queries on pipe_screen or
 * stw_winsys.
 */
int
stw_query_sample_buffers( void )
{
   return 1;
}

int
stw_query_samples( void )
{
   return 4;
}

