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
#include "sp_screen.h"


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

   if (--(*pt)->refcount <= 0) {
      struct softpipe_texture *spt = softpipe_texture(*pt);

      pipe_buffer_reference(screen->winsys, &spt->buffer, NULL);
      FREE(spt);
   }
   *pt = NULL;
}


static struct pipe_surface *
softpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_texture *pt,
                         unsigned face, unsigned level, unsigned zslice,
                         unsigned usage)
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
      ps->usage = usage;
      
      /* Because we are softpipe, anything that the state tracker
       * thought was going to be done with the GPU will actually get
       * done with the CPU.  Let's adjust the flags to take that into
       * account.
       */
      if (ps->usage & PIPE_BUFFER_USAGE_GPU_WRITE)
         ps->usage |= PIPE_BUFFER_USAGE_CPU_WRITE;

      if (ps->usage & PIPE_BUFFER_USAGE_GPU_READ)
         ps->usage |= PIPE_BUFFER_USAGE_CPU_READ;


      pipe_texture_reference(&ps->texture, pt); 
      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;

      if (pt->target == PIPE_TEXTURE_CUBE || pt->target == PIPE_TEXTURE_3D) {
	 ps->offset += ((pt->target == PIPE_TEXTURE_CUBE) ? face : zslice) *
		       (pt->compressed ? ps->height/4 : ps->height) *
		       ps->width * ps->cpp;
      }
      else {
	 assert(face == 0);
	 assert(zslice == 0);
      }

      if (usage & (PIPE_BUFFER_USAGE_CPU_WRITE |
                   PIPE_BUFFER_USAGE_GPU_WRITE)) {
         /* XXX if writing to the texture, invalidate the texcache entries!!! 
          *
          * Actually, no.  Flushing dependent contexts is still done
          * explicitly and separately.  Hardware drivers won't insert
          * FLUSH commands into a command stream at this point,
          * neither should softpipe try to flush caches.  
          *
          * Those contexts could be living in separate threads & doing
          * all sorts of unrelated stuff...  Context<->texture
          * dependency tracking needs to happen elsewhere.
          */
         /* assert(0); */
      }
   }
   return ps;
}


static void 
softpipe_tex_surface_release(struct pipe_screen *screen, 
                             struct pipe_surface **s)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For softpipe, nothing to do.
    */
   assert ((*s)->texture);
   pipe_texture_reference(&(*s)->texture, NULL); 

   screen->winsys->surface_release(screen->winsys, s);
}


static void *
softpipe_surface_map( struct pipe_screen *screen,
                      struct pipe_surface *surface,
                      unsigned flags )
{
   ubyte *map;

   if (flags & ~surface->usage) {
      assert(0);
      return NULL;
   }

   map = screen->winsys->buffer_map( screen->winsys, surface->buffer, flags );
   if (map == NULL)
      return NULL;

   /* May want to different things here depending on read/write nature
    * of the map:
    */
   if (surface->texture &&
       (flags & PIPE_BUFFER_USAGE_CPU_WRITE)) 
   {
      /* Do something to notify sharing contexts of a texture change.
       * In softpipe, that would mean flushing the texture cache.
       */
      softpipe_screen(screen)->timestamp++;
   }
   
   return map + surface->offset;
}


static void
softpipe_surface_unmap(struct pipe_screen *screen,
                       struct pipe_surface *surface)
{
   screen->winsys->buffer_unmap( screen->winsys, surface->buffer );
}


void
softpipe_init_texture_funcs(struct softpipe_context *sp)
{
}


void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = softpipe_texture_create;
   screen->texture_release = softpipe_texture_release;

   screen->get_tex_surface = softpipe_get_tex_surface;
   screen->tex_surface_release = softpipe_tex_surface_release;

   screen->surface_map = softpipe_surface_map;
   screen->surface_unmap = softpipe_surface_unmap;
}
