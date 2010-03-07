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

#ifndef U_INLINES_H
#define U_INLINES_H

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_screen.h"
#include "util/u_debug.h"
#include "util/u_atomic.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Reference counting helper functions.
 */


static INLINE void
pipe_reference_init(struct pipe_reference *reference, unsigned count)
{
   p_atomic_set(&reference->count, count);
}

static INLINE boolean
pipe_is_referenced(struct pipe_reference *reference)
{
   return p_atomic_read(&reference->count) != 0;
}

/**
 * Update reference counting.
 * The old thing pointed to, if any, will be unreferenced.
 * Both 'ptr' and 'reference' may be NULL.
 * \return TRUE if the object's refcount hits zero and should be destroyed.
 */
static INLINE boolean
pipe_reference(struct pipe_reference *ptr, struct pipe_reference *reference)
{
   boolean destroy = FALSE;

   if(ptr != reference) {
      /* bump the reference.count first */
      if (reference) {
         assert(pipe_is_referenced(reference));
         p_atomic_inc(&reference->count);
      }

      if (ptr) {
         assert(pipe_is_referenced(ptr));
         if (p_atomic_dec_zero(&ptr->count)) {
            destroy = TRUE;
         }
      }
   }

   return destroy;
}

static INLINE void
pipe_buffer_reference(struct pipe_buffer **ptr, struct pipe_buffer *buf)
{
   struct pipe_buffer *old_buf;

   assert(ptr);
   old_buf = *ptr;

   if (pipe_reference(&(*ptr)->reference, &buf->reference))
      old_buf->screen->buffer_destroy(old_buf);
   *ptr = buf;
}

static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   struct pipe_surface *old_surf = *ptr;

   if (pipe_reference(&(*ptr)->reference, &surf->reference))
      old_surf->texture->screen->tex_surface_destroy(old_surf);
   *ptr = surf;
}

static INLINE void
pipe_texture_reference(struct pipe_texture **ptr, struct pipe_texture *tex)
{
   struct pipe_texture *old_tex = *ptr;

   if (pipe_reference(&(*ptr)->reference, &tex->reference))
      old_tex->screen->texture_destroy(old_tex);
   *ptr = tex;
}


/*
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
   assert(offset < buf->size);
   assert(offset + length <= buf->size);
   assert(length);
   if(screen->buffer_map_range)
      return screen->buffer_map_range(screen, buf, offset, length, usage);
   else
      return screen->buffer_map(screen, buf, usage);
}

static INLINE void
pipe_buffer_flush_mapped_range(struct pipe_screen *screen,
                               struct pipe_buffer *buf,
                               unsigned offset,
                               unsigned length)
{
   assert(offset < buf->size);
   assert(offset + length <= buf->size);
   assert(length);
   if(screen->buffer_flush_mapped_range)
      screen->buffer_flush_mapped_range(screen, buf, offset, length);
}

static INLINE void
pipe_buffer_write(struct pipe_screen *screen,
                  struct pipe_buffer *buf,
                  unsigned offset, unsigned size,
                  const void *data)
{
   void *map;
   
   assert(offset < buf->size);
   assert(offset + size <= buf->size);
   assert(size);

   map = pipe_buffer_map_range(screen, buf, offset, size, 
                               PIPE_BUFFER_USAGE_CPU_WRITE | 
                               PIPE_BUFFER_USAGE_FLUSH_EXPLICIT |
                               PIPE_BUFFER_USAGE_DISCARD);
   assert(map);
   if(map) {
      memcpy((uint8_t *)map + offset, data, size);
      pipe_buffer_flush_mapped_range(screen, buf, offset, size);
      pipe_buffer_unmap(screen, buf);
   }
}

/**
 * Special case for writing non-overlapping ranges.
 *
 * We can avoid GPU/CPU synchronization when writing range that has never
 * been written before.
 */
static INLINE void
pipe_buffer_write_nooverlap(struct pipe_screen *screen,
                            struct pipe_buffer *buf,
                            unsigned offset, unsigned size,
                            const void *data)
{
   void *map;

   assert(offset < buf->size);
   assert(offset + size <= buf->size);
   assert(size);

   map = pipe_buffer_map_range(screen, buf, offset, size,
                               PIPE_BUFFER_USAGE_CPU_WRITE |
                               PIPE_BUFFER_USAGE_FLUSH_EXPLICIT |
                               PIPE_BUFFER_USAGE_DISCARD |
                               PIPE_BUFFER_USAGE_UNSYNCHRONIZED);
   assert(map);
   if(map) {
      memcpy((uint8_t *)map + offset, data, size);
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
   void *map;
   
   assert(offset < buf->size);
   assert(offset + size <= buf->size);
   assert(size);

   map = pipe_buffer_map_range(screen, buf, offset, size, PIPE_BUFFER_USAGE_CPU_READ);
   assert(map);
   if(map) {
      memcpy(data, (const uint8_t *)map + offset, size);
      pipe_buffer_unmap(screen, buf);
   }
}

static INLINE void *
pipe_transfer_map( struct pipe_transfer *transf )
{
   struct pipe_screen *screen = transf->texture->screen;
   return screen->transfer_map(screen, transf);
}

static INLINE void
pipe_transfer_unmap( struct pipe_transfer *transf )
{
   struct pipe_screen *screen = transf->texture->screen;
   screen->transfer_unmap(screen, transf);
}

static INLINE void
pipe_transfer_destroy( struct pipe_transfer *transf )
{
   struct pipe_screen *screen = transf->texture->screen;
   screen->tex_transfer_destroy(transf);
}

static INLINE unsigned
pipe_transfer_buffer_flags( struct pipe_transfer *transf )
{
   switch (transf->usage & PIPE_TRANSFER_READ_WRITE) {
   case PIPE_TRANSFER_READ_WRITE:
      return PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE;
   case PIPE_TRANSFER_READ:
      return PIPE_BUFFER_USAGE_CPU_READ;
   case PIPE_TRANSFER_WRITE:
      return PIPE_BUFFER_USAGE_CPU_WRITE;
   default:
      debug_assert(0);
      return 0;
   }
}

#ifdef __cplusplus
}
#endif

#endif /* U_INLINES_H */
