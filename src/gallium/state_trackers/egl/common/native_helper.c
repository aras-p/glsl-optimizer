/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "native_helper.h"

struct resource_surface {
   struct pipe_screen *screen;
   enum pipe_format format;
   uint bind;

   struct pipe_resource *resources[NUM_NATIVE_ATTACHMENTS];
   uint resource_mask;
   uint width, height;
};

struct resource_surface *
resource_surface_create(struct pipe_screen *screen,
                        enum pipe_format format, uint bind)
{
   struct resource_surface *rsurf = CALLOC_STRUCT(resource_surface);

   if (rsurf) {
      rsurf->screen = screen;
      rsurf->format = format;
      rsurf->bind = bind;
   }

   return rsurf;
}

static void
resource_surface_free_resources(struct resource_surface *rsurf)
{
   if (rsurf->resource_mask) {
      int i;

      for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
         if (rsurf->resources[i])
            pipe_resource_reference(&rsurf->resources[i], NULL);
      }
      rsurf->resource_mask = 0x0;
   }
}

void
resource_surface_destroy(struct resource_surface *rsurf)
{
   resource_surface_free_resources(rsurf);
   FREE(rsurf);
}

boolean
resource_surface_set_size(struct resource_surface *rsurf,
                          uint width, uint height)
{
   boolean changed = FALSE;

   if (rsurf->width != width || rsurf->height != height) {
      resource_surface_free_resources(rsurf);
      rsurf->width = width;
      rsurf->height = height;
      changed = TRUE;
   }

   return changed;
}

void
resource_surface_get_size(struct resource_surface *rsurf,
                          uint *width, uint *height)
{
   if (width)
      *width = rsurf->width;
   if (height)
      *height = rsurf->height;
}

boolean
resource_surface_add_resources(struct resource_surface *rsurf,
                               uint resource_mask)
{
   struct pipe_resource templ;
   int i;

   resource_mask &= ~rsurf->resource_mask;
   if (!resource_mask)
      return TRUE;

   if (!rsurf->width || !rsurf->height)
      return FALSE;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = rsurf->format;
   templ.bind = rsurf->bind;
   templ.width0 = rsurf->width;
   templ.height0 = rsurf->height;
   templ.depth0 = 1;
   templ.array_size = 1;

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      if (resource_mask & (1 <<i)) {
         assert(!rsurf->resources[i]);

         rsurf->resources[i] =
            rsurf->screen->resource_create(rsurf->screen, &templ);
         if (rsurf->resources[i])
            rsurf->resource_mask |= 1 << i;
      }
   }

   return ((rsurf->resource_mask & resource_mask) == resource_mask);
}


void
resource_surface_get_resources(struct resource_surface *rsurf,
                               struct pipe_resource **resources,
                               uint resource_mask)
{
   int i;

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      if (resource_mask & (1 << i)) {
         resources[i] = NULL;
         pipe_resource_reference(&resources[i], rsurf->resources[i]);
      }
   }
}

struct pipe_resource *
resource_surface_get_single_resource(struct resource_surface *rsurf,
                                     enum native_attachment which)
{
   struct pipe_resource *pres = NULL;
   pipe_resource_reference(&pres, rsurf->resources[which]);
   return pres;
}

static INLINE void
pointer_swap(const void **p1, const void **p2)
{
   const void *tmp = *p1;
   *p1 = *p2;
   *p2 = tmp;
}

void
resource_surface_swap_buffers(struct resource_surface *rsurf,
                              enum native_attachment buf1,
                              enum native_attachment buf2,
                              boolean only_if_exist)
{
   const uint buf1_bit = 1 << buf1;
   const uint buf2_bit = 1 << buf2;
   uint mask;

   if (only_if_exist && !(rsurf->resources[buf1] && rsurf->resources[buf2]))
      return;

   pointer_swap((const void **) &rsurf->resources[buf1],
                (const void **) &rsurf->resources[buf2]);

   /* swap mask bits */
   mask = rsurf->resource_mask & ~(buf1_bit | buf2_bit);
   if (rsurf->resource_mask & buf1_bit)
      mask |= buf2_bit;
   if (rsurf->resource_mask & buf2_bit)
      mask |= buf1_bit;

   rsurf->resource_mask = mask;
}

boolean
resource_surface_present(struct resource_surface *rsurf,
                         enum native_attachment which,
                         void *winsys_drawable_handle)
{
   struct pipe_resource *pres = rsurf->resources[which];

   if (!pres)
      return TRUE;

   rsurf->screen->flush_frontbuffer(rsurf->screen,
         pres, 0, 0, winsys_drawable_handle);

   return TRUE;
}
