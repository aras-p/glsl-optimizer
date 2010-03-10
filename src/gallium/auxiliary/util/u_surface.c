/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Surface utility functions.
 *  
 * @author Brian Paul
 */


#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_memory.h"
#include "util/u_surface.h"


/**
 * Helper to quickly create an RGBA rendering surface of a certain size.
 * \param textureOut  returns the new texture
 * \param surfaceOut  returns the new surface
 * \return TRUE for success, FALSE if failure
 */
boolean
util_create_rgba_surface(struct pipe_screen *screen,
                         uint width, uint height,
                         struct pipe_texture **textureOut,
                         struct pipe_surface **surfaceOut)
{
   static const enum pipe_format rgbaFormats[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_NONE
   };
   const uint target = PIPE_TEXTURE_2D;
   const uint usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
   enum pipe_format format = PIPE_FORMAT_NONE;
   struct pipe_texture templ;
   uint i;

   /* Choose surface format */
   for (i = 0; rgbaFormats[i]; i++) {
      if (screen->is_format_supported(screen, rgbaFormats[i],
                                      target, usage, 0)) {
         format = rgbaFormats[i];
         break;
      }
   }
   if (format == PIPE_FORMAT_NONE)
      return FALSE;  /* unable to get an rgba format!?! */

   /* create texture */
   memset(&templ, 0, sizeof(templ));
   templ.target = target;
   templ.format = format;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.tex_usage = usage;

   *textureOut = screen->texture_create(screen, &templ);
   if (!*textureOut)
      return FALSE;

   /* create surface / view into texture */
   *surfaceOut = screen->get_tex_surface(screen, *textureOut, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE);
   if (!*surfaceOut) {
      pipe_texture_reference(textureOut, NULL);
      return FALSE;
   }

   return TRUE;
}


/**
 * Release the surface and texture from util_create_rgba_surface().
 */
void
util_destroy_rgba_surface(struct pipe_texture *texture,
                          struct pipe_surface *surface)
{
   pipe_surface_reference(&surface, NULL);
   pipe_texture_reference(&texture, NULL);
}



/**
 * Compare pipe_framebuffer_state objects.
 * \return TRUE if same, FALSE if different
 */
boolean
util_framebuffer_state_equal(const struct pipe_framebuffer_state *dst,
                             const struct pipe_framebuffer_state *src)
{
   unsigned i;

   if (dst->width != src->width ||
       dst->height != src->height)
      return FALSE;

   for (i = 0; i < Elements(src->cbufs); i++) {
      if (dst->cbufs[i] != src->cbufs[i]) {
         return FALSE;
      }
   }

   if (dst->nr_cbufs != src->nr_cbufs) {
      return FALSE;
   }

   if (dst->zsbuf != src->zsbuf) {
      return FALSE;
   }

   return TRUE;
}


/**
 * Copy framebuffer state from src to dst, updating refcounts.
 */
void
util_copy_framebuffer_state(struct pipe_framebuffer_state *dst,
                            const struct pipe_framebuffer_state *src)
{
   unsigned i;

   dst->width = src->width;
   dst->height = src->height;

   for (i = 0; i < Elements(src->cbufs); i++) {
      pipe_surface_reference(&dst->cbufs[i], src->cbufs[i]);
   }

   dst->nr_cbufs = src->nr_cbufs;

   pipe_surface_reference(&dst->zsbuf, src->zsbuf);
}


void
util_unreference_framebuffer_state(struct pipe_framebuffer_state *fb)
{
   unsigned i;

   for (i = 0; i < fb->nr_cbufs; i++) {
      pipe_surface_reference(&fb->cbufs[i], NULL);
   }

   pipe_surface_reference(&fb->zsbuf, NULL);

   fb->width = fb->height = 0;
   fb->nr_cbufs = 0;
}
