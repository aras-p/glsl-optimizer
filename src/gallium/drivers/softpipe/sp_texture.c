/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tile_cache.h"


/* Simple, maximally packed layout.
 */

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}


static void
softpipe_texture_layout(struct softpipe_texture * spt)
{
   struct pipe_texture *pt = &spt->base;
   unsigned level;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned depth = pt->depth[0];

   spt->buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      pt->width[level] = width;
      pt->height[level] = height;
      pt->depth[level] = depth;

      spt->level_offset[level] = spt->buffer_size;

      spt->buffer_size += ((pt->compressed) ? MAX2(1, height/4) : height) *
			  ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
			  width * pt->cpp;

      width  = minify(width);
      height = minify(height);
      depth = minify(depth);
   }
}


static struct pipe_texture *
softpipe_texture_create(struct pipe_screen *screen,
                        const struct pipe_texture *templat)
{
   struct pipe_winsys *ws = screen->winsys;
   struct softpipe_texture *spt = CALLOC_STRUCT(softpipe_texture);
   if (!spt)
      return NULL;

   spt->base = *templat;
   spt->base.refcount = 1;
   spt->base.screen = screen;

   softpipe_texture_layout(spt);

   spt->buffer = ws->buffer_create(ws, 32,
                                   PIPE_BUFFER_USAGE_PIXEL,
                                   spt->buffer_size);
   if (!spt->buffer) {
      FREE(spt);
      return NULL;
   }

   assert(spt->base.refcount == 1);

   return &spt->base;
}


static void
softpipe_texture_release(struct pipe_screen *screen,
                         struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      struct softpipe_texture *spt = softpipe_texture(*pt);

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) spt);
      */

      pipe_buffer_reference(screen->winsys, &spt->buffer, NULL);

      FREE(spt);
   }
   *pt = NULL;
}


static struct pipe_surface *
softpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_texture *pt,
                         unsigned face, unsigned level, unsigned zslice)
{
   struct pipe_winsys *ws = screen->winsys;
   struct softpipe_texture *spt = softpipe_texture(pt);
   struct pipe_surface *ps;

   assert(level <= pt->last_level);

   ps = ws->surface_alloc(ws);
   if (ps) {
      assert(ps->refcount);
      assert(ps->winsys);
      pipe_buffer_reference(ws, &ps->buffer, spt->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = ps->width;
      ps->offset = spt->level_offset[level];

      if (pt->target == PIPE_TEXTURE_CUBE || pt->target == PIPE_TEXTURE_3D) {
	 ps->offset += ((pt->target == PIPE_TEXTURE_CUBE) ? face : zslice) *
		       (pt->compressed ? ps->height/4 : ps->height) *
		       ps->width * ps->cpp;
      }
      else {
	 assert(face == 0);
	 assert(zslice == 0);
      }
   }
   return ps;
}


static void
softpipe_texture_update(struct pipe_context *pipe,
                        struct pipe_texture *texture,
                        uint face, uint levelsMask)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint unit;
   for (unit = 0; unit < softpipe->num_textures; unit++) {
      if (softpipe->texture[unit] == texture) {
         sp_flush_tile_cache(softpipe, softpipe->tex_cache[unit]);
      }
   }
}


void
softpipe_init_texture_funcs( struct softpipe_context *softpipe )
{
   softpipe->pipe.texture_update = softpipe_texture_update;
}


void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = softpipe_texture_create;
   screen->texture_release = softpipe_texture_release;
   screen->get_tex_surface = softpipe_get_tex_surface;
}
