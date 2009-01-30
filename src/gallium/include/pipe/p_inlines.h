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
   struct pipe_screen *screen;
   assert(surf->texture);
   screen = surf->texture->screen;
   return screen->surface_map( screen, surf, flags );
}

static INLINE void
pipe_surface_unmap( struct pipe_surface *surf )
{
   struct pipe_screen *screen;
   assert(surf->texture);
   screen = surf->texture->screen;
   screen->surface_unmap( screen, surf );
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
   if (surf) {
      assert(surf->refcount);
      surf->refcount++;
   }

   if (*ptr) {
      struct pipe_screen *screen;
      assert((*ptr)->refcount);
      assert((*ptr)->texture);
      screen = (*ptr)->texture->screen;
      screen->tex_surface_release( screen, ptr );
      assert(!*ptr);
   }

   *ptr = surf;
}


/**
 * \sa pipe_surface_reference
 */
static INLINE void
pipe_texture_reference(struct pipe_texture **ptr,
		       struct pipe_texture *pt)
{
   assert(ptr);

   if (pt) { 
      assert(pt->refcount);
      pt->refcount++;
   }

   if (*ptr) {
      struct pipe_screen *screen = (*ptr)->screen;
      assert(screen);
      assert((*ptr)->refcount);
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
   assert((*ptr)->refcount);
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
   if (screen->buffer_create)
      return screen->buffer_create(screen, alignment, usage, size);
   else
      return screen->winsys->_buffer_create(screen->winsys, alignment, usage, size);
}

static INLINE struct pipe_buffer *
pipe_user_buffer_create( struct pipe_screen *screen, void *ptr, unsigned size )
{
   if (screen->user_buffer_create)
      return screen->user_buffer_create(screen, ptr, size);
   else
      return screen->winsys->_user_buffer_create(screen->winsys, ptr, size);
}

static INLINE void *
pipe_buffer_map(struct pipe_screen *screen,
                struct pipe_buffer *buf,
                unsigned usage)
{
   if (screen->buffer_map)
      return screen->buffer_map(screen, buf, usage);
   else
      return screen->winsys->_buffer_map(screen->winsys, buf, usage);
}

static INLINE void
pipe_buffer_unmap(struct pipe_screen *screen,
                  struct pipe_buffer *buf)
{
   if (screen->buffer_unmap)
      screen->buffer_unmap(screen, buf);
   else
      screen->winsys->_buffer_unmap(screen->winsys, buf);
}

/* XXX: thread safety issues!
 */
static INLINE void
pipe_buffer_reference(struct pipe_screen *screen,
		      struct pipe_buffer **ptr,
		      struct pipe_buffer *buf)
{
   if (buf) {
      assert(buf->refcount);
      buf->refcount++;
   }

   if (*ptr) {
      assert((*ptr)->refcount);
      if(--(*ptr)->refcount == 0) {
         if (screen->buffer_destroy)
            screen->buffer_destroy( screen, *ptr );
         else
            screen->winsys->_buffer_destroy( screen->winsys, *ptr );
      }
   }

   *ptr = buf;
}


#ifdef __cplusplus
}
#endif

#endif /* P_INLINES_H */
