/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef P_INLINES_H
#define P_INLINES_H

#include "p_context.h"
//#include "p_util.h"

/**
 * Set 'ptr' to point to 'region' and update reference counting.
 * The old thing pointed to, if any, will be unreferenced first.
 * 'region' may be NULL.
 */
static INLINE void
pipe_region_reference(struct pipe_region **ptr, struct pipe_region *region)
{
   assert(ptr);
   if (*ptr) {
      /* unreference the old thing */
      struct pipe_region *oldReg = *ptr;
      assert(oldReg->refcount > 0);
      oldReg->refcount--;
      if (oldReg->refcount == 0) {
         /* free the old region */
         assert(oldReg->map_refcount == 0);
         /* XXX dereference the region->buffer */
         FREE( oldReg );
      }
      *ptr = NULL;
   }
   if (region) {
      /* reference the new thing */
      region->refcount++;
      *ptr = region;
   }
}

/**
 * \sa pipe_region_reference
 */
static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   assert(ptr);
   if (*ptr) {
      /* unreference the old thing */
      struct pipe_surface *oldSurf = *ptr;
      assert(oldSurf->refcount > 0);
      oldSurf->refcount--;
      if (oldSurf->refcount == 0) {
         /* free the old region */
         pipe_region_reference(&oldSurf->region, NULL);
         FREE( oldSurf );
      }
      *ptr = NULL;
   }
   if (surf) {
      /* reference the new thing */
      surf->refcount++;
      *ptr = surf;
   }
}

#endif /* P_INLINES_H */
