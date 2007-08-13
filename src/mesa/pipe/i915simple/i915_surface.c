/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_context.h"
#include "i915_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
//#include "main/imports.h"


struct i915_surface
{
   struct pipe_surface surface;
   /* anything else? */
};


/**
 * Note: this is exactly like a8r8g8b8_get_tile() in sp_surface.c
 * Share it someday.
 */
static void
i915_get_tile(struct pipe_surface *ps,
              unsigned x, unsigned y, unsigned w, unsigned h, float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8_R8_G8_B8);

#if 0
   assert(x + w <= ps->width);
   assert(y + h <= ps->height);
#else
   /* temp clipping hack */
   if (x + w > ps->width)
      w = ps->width - x;
   if (y + h > ps->height)
      h = ps->height -y;
#endif
   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++) {
         const unsigned pixel = src[j];
         pRow[0] = UBYTE_TO_FLOAT((pixel >> 16) & 0xff);
         pRow[1] = UBYTE_TO_FLOAT((pixel >>  8) & 0xff);
         pRow[2] = UBYTE_TO_FLOAT((pixel >>  0) & 0xff);
         pRow[3] = UBYTE_TO_FLOAT((pixel >> 24) & 0xff);
         pRow += 4;
      }
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


static void
i915_put_tile(struct pipe_surface *ps,
              unsigned x, unsigned y, unsigned w, unsigned h, const float *p)
{
   /* any need to put tiles into i915 surfaces? */
   assert(0);
}



static struct pipe_surface *
i915_surface_alloc(struct pipe_context *pipe, unsigned format)
{
   struct i915_surface *surf = CALLOC_STRUCT(i915_surface);

   if (!surf)
      return NULL;

   surf->surface.format = format;
   surf->surface.refcount = 1;

   surf->surface.get_tile = i915_get_tile;
   surf->surface.put_tile = i915_put_tile;

   return &surf->surface;
}


void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->pipe.surface_alloc = i915_surface_alloc;
}
