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
#include "p_defines.h"
#include "p_screen.h"
#include "p_winsys.h"


#ifdef __cplusplus
extern "C" {
#endif


/* XXX: these are a kludge.  will fix when all surfaces are views into
 * textures, and free-floating winsys surfaces go away.
 */
static INLINE void *
pipe_surface_map( struct pipe_surface *surf, unsigned flags )
{
   if (surf->texture) {
      struct pipe_screen *screen = surf->texture->screen;
      return surf->texture->screen->surface_map( screen, surf, flags );
   }
   else {
      struct pipe_winsys *winsys = surf->winsys;
      char *map = (char *)winsys->buffer_map( winsys, surf->buffer, flags );
      if (map == NULL)
         return NULL;
      return (void *)(map + surf->offset);
   }
}

static INLINE void
pipe_surface_unmap( struct pipe_surface *surf )
{
   if (surf->texture) {
      struct pipe_screen *screen = surf->texture->screen;
      surf->texture->screen->surface_unmap( screen, surf );
   }
   else {
      struct pipe_winsys *winsys = surf->winsys;
      winsys->buffer_unmap( winsys, surf->buffer );
   }
}



/**
 * Set 'ptr' to point to 'surf' and update reference counting.
 * The old thing pointed to, if any, will be unreferenced first.
 * 'surf' may be NULL.
 */
static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   /* bump the refcount first */
   if (surf) 
      surf->refcount++;

   if (*ptr) {

      /* There are currently two sorts of surfaces... This needs to be
       * fixed so that all surfaces are views into a texture.
       */
      if ((*ptr)->texture) {
         struct pipe_screen *screen = (*ptr)->texture->screen;
         screen->tex_surface_release( screen, ptr );
      }
      else {
         struct pipe_winsys *winsys = (*ptr)->winsys;
         winsys->surface_release(winsys, ptr);
      }

      assert(!*ptr);
   }

   *ptr = surf;
}


/* XXX: thread safety issues!
 */
static INLINE void
winsys_buffer_reference(struct pipe_winsys *winsys,
		      struct pipe_buffer **ptr,
		      struct pipe_buffer *buf)
{
   if (buf) 
      buf->refcount++;

   if (*ptr && --(*ptr)->refcount == 0)
      winsys->buffer_destroy( winsys, *ptr );

   *ptr = buf;
}



/**
 * \sa pipe_surface_reference
 */
static INLINE void
pipe_texture_reference(struct pipe_texture **ptr,
		       struct pipe_texture *pt)
{
   assert(ptr);

   if (pt) 
      pt->refcount++;

   if (*ptr) {
      struct pipe_screen *screen = (*ptr)->screen;
      assert(screen);
      screen->texture_release(screen, ptr);

      assert(!*ptr);
   }

   *ptr = pt;
}


static INLINE void
pipe_texture_release(struct pipe_texture **ptr)
{
   struct pipe_screen *screen;
   assert(ptr);
   screen = (*ptr)->screen;
   screen->texture_release(screen, ptr);
   *ptr = NULL;
}


/**
 * Convenience wrappers for winsys buffer functions.
 */

static INLINE struct pipe_buffer *
pipe_buffer_create( struct pipe_screen *screen,
                    unsigned alignment, unsigned usage, unsigned size )
{
   return screen->winsys->buffer_create(screen->winsys, alignment, usage, size);
}

static INLINE struct pipe_buffer *
pipe_user_buffer_create( struct pipe_screen *screen, void *ptr, unsigned size )
{
   return screen->winsys->user_buffer_create(screen->winsys, ptr, size);
}

static INLINE void
pipe_buffer_destroy( struct pipe_screen *screen, struct pipe_buffer *buf )
{
   screen->winsys->buffer_destroy(screen->winsys, buf);
}

static INLINE void *
pipe_buffer_map(struct pipe_screen *screen,
                struct pipe_buffer *buf,
                unsigned usage)
{
   return screen->winsys->buffer_map(screen->winsys, buf, usage);
}

static INLINE void
pipe_buffer_unmap(struct pipe_screen *screen,
                  struct pipe_buffer *buf)
{
   screen->winsys->buffer_unmap(screen->winsys, buf);
}

/* XXX when we're using this everywhere, get rid of
 * winsys_buffer_reference() above.
 */
static INLINE void
pipe_buffer_reference(struct pipe_screen *screen,
		      struct pipe_buffer **ptr,
		      struct pipe_buffer *buf)
{
   winsys_buffer_reference(screen->winsys, ptr, buf);
}


#ifdef __cplusplus
}
#endif

#endif /* P_INLINES_H */
