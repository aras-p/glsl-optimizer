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

#include "stw_device.h"
#include "stw_pixelformat.h"
#include "stw_public.h"
#include "stw_tls.h"


static void
stw_pixelformat_add(
   struct stw_device *stw_dev,
   const struct stw_pixelformat_color_info *color,
   const struct stw_pixelformat_depth_info *depth,
   boolean doublebuffer,
   boolean multisampled,
   boolean extended )
{
   struct stw_pixelformat_info *pfi;
   
   assert(stw_dev->pixelformat_extended_count < STW_MAX_PIXELFORMATS);
   if(stw_dev->pixelformat_extended_count >= STW_MAX_PIXELFORMATS)
      return;
   
   pfi = &stw_dev->pixelformats[stw_dev->pixelformat_extended_count];
   
   memset(pfi, 0, sizeof *pfi);
   
   pfi->pfd.nSize = sizeof pfi->pfd;
   pfi->pfd.nVersion = 1;

   pfi->pfd.dwFlags = PFD_SUPPORT_OPENGL;
   
   /* TODO: also support non-native pixel formats */
   pfi->pfd.dwFlags |= PFD_DRAW_TO_WINDOW ;
   
   if (doublebuffer)
      pfi->pfd.dwFlags |= PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
   
   pfi->pfd.iPixelType = PFD_TYPE_RGBA;

   pfi->pfd.cColorBits = color->redbits + color->greenbits + color->bluebits;
   pfi->pfd.cRedBits = color->redbits;
   pfi->pfd.cRedShift = color->redshift;
   pfi->pfd.cGreenBits = color->greenbits;
   pfi->pfd.cGreenShift = color->greenshift;
   pfi->pfd.cBlueBits = color->bluebits;
   pfi->pfd.cBlueShift = color->blueshift;
   pfi->pfd.cAlphaBits = color->alphabits;
   pfi->pfd.cAlphaShift = color->alphashift;
   pfi->pfd.cAccumBits = 0;
   pfi->pfd.cAccumRedBits = 0;
   pfi->pfd.cAccumGreenBits = 0;
   pfi->pfd.cAccumBlueBits = 0;
   pfi->pfd.cAccumAlphaBits = 0;
   pfi->pfd.cDepthBits = depth->depthbits;
   pfi->pfd.cStencilBits = depth->stencilbits;
   pfi->pfd.cAuxBuffers = 0;
   pfi->pfd.iLayerType = 0;
   pfi->pfd.bReserved = 0;
   pfi->pfd.dwLayerMask = 0;
   pfi->pfd.dwVisibleMask = 0;
   pfi->pfd.dwDamageMask = 0;

   if(multisampled) {
      /* FIXME: re-enable when we can query this */
#if 0
      pfi->numSampleBuffers = 1;
      pfi->numSamples = 4;
#else
      return;
#endif
   }
   
   ++stw_dev->pixelformat_extended_count;
   
   if(!extended) {
      ++stw_dev->pixelformat_count;
      assert(stw_dev->pixelformat_count == stw_dev->pixelformat_extended_count);
   }
}

static void
stw_add_standard_pixelformats(
   struct stw_device *stw_dev,
   boolean doublebuffer,
   boolean multisampled,
   boolean extended )
{
   const struct stw_pixelformat_color_info color24 = { 8, 0, 8, 8, 8, 16, 0, 0 };
   const struct stw_pixelformat_color_info color24a8 = { 8, 0, 8, 8, 8, 16, 8, 24 };
   const struct stw_pixelformat_depth_info depth24s8 = { 24, 8 };
   const struct stw_pixelformat_depth_info depth16 = { 16, 0 };

   stw_pixelformat_add( stw_dev, &color24, &depth24s8, doublebuffer, multisampled, extended );

   stw_pixelformat_add( stw_dev, &color24a8, &depth24s8, doublebuffer, multisampled, extended );

   stw_pixelformat_add( stw_dev, &color24, &depth16, doublebuffer, multisampled, extended );

   stw_pixelformat_add( stw_dev, &color24a8, &depth16, doublebuffer, multisampled, extended );
}

void
stw_pixelformat_init( void )
{
   stw_add_standard_pixelformats( stw_dev, FALSE, FALSE, FALSE );
   stw_add_standard_pixelformats( stw_dev, TRUE,  FALSE, FALSE );
   stw_add_standard_pixelformats( stw_dev, FALSE, TRUE, TRUE  );
   stw_add_standard_pixelformats( stw_dev, TRUE,  TRUE, TRUE  );

   assert( stw_dev->pixelformat_count <= STW_MAX_PIXELFORMATS );
   assert( stw_dev->pixelformat_extended_count <= STW_MAX_PIXELFORMATS );
}

uint
stw_pixelformat_get_count( void )
{
   return stw_dev->pixelformat_count;
}

uint
stw_pixelformat_get_extended_count( void )
{
   return stw_dev->pixelformat_extended_count;
}

const struct stw_pixelformat_info *
stw_pixelformat_get_info( uint index )
{
   assert( index < stw_dev->pixelformat_extended_count );

   return &stw_dev->pixelformats[index];
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
   const struct stw_pixelformat_info *pfi;

   (void) hdc;

   count = stw_pixelformat_get_extended_count();
   index = (uint) iPixelFormat - 1;

   if (ppfd == NULL)
      return count;
   if (index >= count || nBytes != sizeof( PIXELFORMATDESCRIPTOR ))
      return 0;

   pfi = stw_pixelformat_get_info( index );
   
   memcpy(ppfd, &pfi->pfd, sizeof( PIXELFORMATDESCRIPTOR ));

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
      const struct stw_pixelformat_info *pfi = stw_pixelformat_get_info( index );

      if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
          !!(ppfd->dwFlags & PFD_DOUBLEBUFFER) !=
          !!(pfi->pfd.dwFlags & PFD_DOUBLEBUFFER))
         continue;

      /* FIXME: Take in account individual channel bits */
      if (ppfd->cColorBits != pfi->pfd.cColorBits)
         delta += 8;

      if (ppfd->cDepthBits != pfi->pfd.cDepthBits)
         delta += 4;

      if (ppfd->cStencilBits != pfi->pfd.cStencilBits)
         delta += 2;

      if (ppfd->cAlphaBits != pfi->pfd.cAlphaBits)
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
