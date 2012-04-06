#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "pipe/p_context.h"

struct nouveau_pushbuf;

struct nouveau_context {
   struct pipe_context pipe;
   struct nouveau_screen *screen;

   struct nouveau_pushbuf *pushbuf;

   boolean vbo_dirty;
   boolean cb_dirty;

   void (*copy_data)(struct nouveau_context *,
                     struct nouveau_bo *dst, unsigned, unsigned,
                     struct nouveau_bo *src, unsigned, unsigned, unsigned);
   void (*push_data)(struct nouveau_context *,
                     struct nouveau_bo *dst, unsigned, unsigned,
                     unsigned, const void *);
   /* base, size refer to the whole constant buffer */
   void (*push_cb)(struct nouveau_context *,
                   struct nouveau_bo *, unsigned domain,
                   unsigned base, unsigned size,
                   unsigned offset, unsigned words, const uint32_t *);
};

static INLINE struct nouveau_context *
nouveau_context(struct pipe_context *pipe)
{
   return (struct nouveau_context *)pipe;
}

void
nouveau_context_init_vdec(struct nouveau_context *);

#endif
