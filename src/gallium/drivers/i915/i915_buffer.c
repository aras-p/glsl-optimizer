/**************************************************************************
 *
 * Copyright © 2009 Jakob Bornecrantz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "i915_screen.h"
#include "i915_buffer.h"

struct intel_buffer;

struct i915_buffer
{
   struct pipe_buffer base;

   struct intel_buffer *ibuf; /** hw buffer */

   void *data; /**< user and malloc data */
   boolean own; /**< we own the data incase of malloc */
};

static INLINE struct i915_buffer *
i915_buffer(struct pipe_buffer *buffer)
{
   return (struct i915_buffer *)buffer;
}

static struct pipe_buffer *
i915_buffer_create(struct pipe_screen *screen,
                   unsigned alignment,
                   unsigned usage,
                   unsigned size)
{
   struct i915_buffer *buf = CALLOC_STRUCT(i915_buffer);

   if (!buf)
      return NULL;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = alignment;
   buf->base.screen = screen;
   buf->base.usage = usage;
   buf->base.size = size;
   buf->data = MALLOC(size);
   buf->own = TRUE;

   if (!buf->data)
      goto err;

   return &buf->base;

err:
   FREE(buf);
   return NULL;
}

static struct pipe_buffer *
i915_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes)
{
   struct i915_buffer *buf = CALLOC_STRUCT(i915_buffer);

   if (!buf)
      return NULL;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = 0;
   buf->base.screen = screen;
   buf->base.usage = 0;
   buf->base.size = bytes;
   buf->data = ptr;
   buf->own = FALSE;

   return &buf->base;
}

static void *
i915_buffer_map(struct pipe_screen *screen,
                struct pipe_buffer *buffer,
                unsigned usage)
{
   struct i915_buffer *buf = i915_buffer(buffer);
   assert(!buf->ibuf);
   return buf->data;
}

static void
i915_buffer_unmap(struct pipe_screen *screen,
                  struct pipe_buffer *buffer)
{
   struct i915_buffer *buf = i915_buffer(buffer);
   assert(!buf->ibuf);
   (void) buf;
}

static void
i915_buffer_destroy(struct pipe_buffer *buffer)
{
   struct i915_buffer *buf = i915_buffer(buffer);
   assert(!buf->ibuf);

   if (buf->own)
      FREE(buf->data);
   FREE(buf);
}

void i915_init_screen_buffer_functions(struct i915_screen *screen)
{
   screen->base.buffer_create = i915_buffer_create;
   screen->base.user_buffer_create = i915_user_buffer_create;
   screen->base.buffer_map = i915_buffer_map;
   screen->base.buffer_map_range = NULL;
   screen->base.buffer_flush_mapped_range = NULL;
   screen->base.buffer_unmap = i915_buffer_unmap;
   screen->base.buffer_destroy = i915_buffer_destroy;
}
