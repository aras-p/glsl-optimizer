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


#ifdef __cplusplus
extern "C" {
#endif


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
pipe_transfer_reference(struct pipe_transfer **ptr, struct pipe_transfer *trans)
{
   /* bump the refcount first */
   if (trans) {
      assert(trans->refcount);
      trans->refcount++;
   }

   if (*ptr) {
      struct pipe_screen *screen;
      assert((*ptr)->refcount);
      assert((*ptr)->texture);
      screen = (*ptr)->texture->screen;
      screen->tex_transfer_release( screen, ptr );
      assert(!*ptr);
   }

   *ptr = trans;
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
 * Convenience wrappers for screen buffer functions.
 */

static INLINE struct pipe_buffer *
pipe_buffer_create( struct pipe_screen *screen,
                    unsigned alignment, unsigned usage, unsigned size )
{
   return screen->buffer_create(screen, alignment, usage, size);
}

static INLINE struct pipe_buffer *
pipe_user_buffer_create( struct pipe_screen *screen, void *ptr, unsigned size )
{
   return screen->user_buffer_create(screen, ptr, size);
}

static INLINE void *
pipe_buffer_map(struct pipe_screen *screen,
                struct pipe_buffer *buf,
                unsigned usage)
{
   if(screen->buffer_map_range) {
      unsigned offset = 0;
      unsigned length = buf->size;
      return screen->buffer_map_range(screen, buf, offset, length, usage);
   }
   else
      return screen->buffer_map(screen, buf, usage);
}

static INLINE void
pipe_buffer_unmap(struct pipe_screen *screen,
                  struct pipe_buffer *buf)
{
   screen->buffer_unmap(screen, buf);
}

static INLINE void *
pipe_buffer_map_range(struct pipe_screen *screen,
                struct pipe_buffer *buf,
                unsigned offset,
                unsigned length,
                unsigned usage)
{
   if(screen->buffer_map_range)
      return screen->buffer_map_range(screen, buf, offset, length, usage);
   else {
      uint8_t *map;
      map = screen->buffer_map(screen, buf, usage);
      return map ? map + offset : NULL;
   }
}

static INLINE void
pipe_buffer_flush_mapped_range(struct pipe_screen *screen,
                               struct pipe_buffer *buf,
                               unsigned offset,
                               unsigned length)
{
   if(screen->buffer_flush_mapped_range)
      screen->buffer_flush_mapped_range(screen, buf, offset, length);
}

static INLINE void
pipe_buffer_write(struct pipe_screen *screen,
                  struct pipe_buffer *buf,
                  unsigned offset, unsigned size,
                  const void *data)
{
   uint8_t *map;
   
   assert(offset < buf->size);
   assert(offset + size <= buf->size);
   
   map = pipe_buffer_map_range(screen, buf, offset, size, PIPE_BUFFER_USAGE_CPU_WRITE);
   assert(map);
   if(map) {
      memcpy(map, data, size);
      pipe_buffer_flush_mapped_range(screen, buf, offset, size);
      pipe_buffer_unmap(screen, buf);
   }
}

static INLINE void
pipe_buffer_read(struct pipe_screen *screen,
                 struct pipe_buffer *buf,
                 unsigned offset, unsigned size,
                 void *data)
{
   uint8_t *map;
   
   assert(offset < buf->size);
   assert(offset + size <= buf->size);
   
   map = pipe_buffer_map_range(screen, buf, offset, size, PIPE_BUFFER_USAGE_CPU_READ);
   assert(map);
   if(map) {
      memcpy(data, map, size);
      pipe_buffer_unmap(screen, buf);
   }
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
         screen->buffer_destroy( screen, *ptr );
      }
   }

   *ptr = buf;
}


#ifdef __cplusplus
}
#endif

#endif /* P_INLINES_H */
