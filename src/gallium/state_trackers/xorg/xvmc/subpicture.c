/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include <assert.h>
#include <X11/Xlibint.h>
#include <X11/extensions/XvMClib.h>

Status XvMCCreateSubpicture(Display *dpy, XvMCContext *context, XvMCSubpicture *subpicture,
                            unsigned short width, unsigned short height, int xvimage_id)
{
   assert(dpy);

   if (!context)
      return XvMCBadContext;

   assert(subpicture);

   /*if (width > || height > )
      return BadValue;*/

   /*if (xvimage_id != )
      return BadMatch;*/

   subpicture->subpicture_id = XAllocID(dpy);
   subpicture->context_id = context->context_id;
   subpicture->xvimage_id = xvimage_id;
   subpicture->width = width;
   subpicture->height = height;
   subpicture->num_palette_entries = 0;
   subpicture->entry_bytes = 0;
   subpicture->component_order[0] = 0;
   subpicture->component_order[1] = 0;
   subpicture->component_order[2] = 0;
   subpicture->component_order[3] = 0;
   /* TODO: subpicture->privData = ;*/

   SyncHandle();

   return Success;
}

Status XvMCClearSubpicture(Display *dpy, XvMCSubpicture *subpicture, short x, short y,
                           unsigned short width, unsigned short height, unsigned int color)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   /* TODO: Assert clear rect is within bounds? Or clip? */

   return Success;
}

Status XvMCCompositeSubpicture(Display *dpy, XvMCSubpicture *subpicture, XvImage *image,
                               short srcx, short srcy, unsigned short width, unsigned short height,
                               short dstx, short dsty)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(image);

   if (subpicture->xvimage_id != image->id)
      return BadMatch;

   /* TODO: Assert rects are within bounds? Or clip? */

   return Success;
}

Status XvMCDestroySubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return BadImplementation;
}

Status XvMCSetSubpicturePalette(Display *dpy, XvMCSubpicture *subpicture, unsigned char *palette)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(palette);

   /* We don't support paletted subpictures */
   return BadMatch;
}

Status XvMCBlendSubpicture(Display *dpy, XvMCSurface *target_surface, XvMCSubpicture *subpicture,
                           short subx, short suby, unsigned short subw, unsigned short subh,
                           short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   assert(dpy);

   if (!target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (target_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Assert rects are within bounds? Or clip? */
   return Success;
}

Status XvMCBlendSubpicture2(Display *dpy, XvMCSurface *source_surface, XvMCSurface *target_surface,
                            XvMCSubpicture *subpicture, short subx, short suby, unsigned short subw, unsigned short subh,
                            short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   assert(dpy);

   if (!source_surface || !target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Assert rects are within bounds? Or clip? */
   return Success;
}

Status XvMCSyncSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

Status XvMCFlushSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

Status XvMCGetSubpictureStatus(Display *dpy, XvMCSubpicture *subpicture, int *status)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(status);

   /* TODO */
   *status = 0;

   return Success;
}
