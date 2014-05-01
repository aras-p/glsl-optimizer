/**************************************************************************
 *
 * Copyright 2012 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * u_debug_flush.c Debug flush and map-related issues:
 * - Flush while synchronously mapped.
 * - Command stream reference while synchronously mapped.
 * - Synchronous map while referenced on command stream.
 * - Recursive maps.
 * - Unmap while not mapped.
 *
 * @author Thomas Hellstrom <thellstrom@vmware.com>
 */

#ifdef DEBUG
#include "pipe/p_compiler.h"
#include "util/u_debug_stack.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_debug_flush.h"
#include "util/u_hash_table.h"
#include "util/u_double_list.h"
#include "util/u_inlines.h"
#include "util/u_string.h"
#include "os/os_thread.h"
#include <stdio.h>

struct debug_flush_buf {
   /* Atomic */
   struct pipe_reference reference; /* Must be the first member. */
   pipe_mutex mutex;
   /* Immutable */
   boolean supports_unsync;
   unsigned bt_depth;
   /* Protected by mutex */
   boolean mapped;
   boolean mapped_sync;
   struct debug_stack_frame *map_frame;
};

struct debug_flush_item {
   struct debug_flush_buf *fbuf;
   unsigned bt_depth;
   struct debug_stack_frame *ref_frame;
};

struct debug_flush_ctx {
   /* Contexts are used by a single thread at a time */
   unsigned bt_depth;
   boolean catch_map_of_referenced;
   struct util_hash_table *ref_hash;
   struct list_head head;
};

pipe_static_mutex(list_mutex);
static struct list_head ctx_list = {&ctx_list, &ctx_list};

static struct debug_stack_frame *
debug_flush_capture_frame(int start, int depth)
{
   struct debug_stack_frame *frames;

   frames = CALLOC(depth, sizeof(*frames));
   if (!frames)
      return NULL;

   debug_backtrace_capture(frames, start, depth);
   return frames;
}

static int
debug_flush_pointer_compare(void *key1, void *key2)
{
   return (key1 == key2) ? 0 : 1;
}

static unsigned
debug_flush_pointer_hash(void *key)
{
   return (unsigned) (unsigned long) key;
}

struct debug_flush_buf *
debug_flush_buf_create(boolean supports_unsync, unsigned bt_depth)
{
   struct debug_flush_buf *fbuf = CALLOC_STRUCT(debug_flush_buf);

   if (!fbuf)
      goto out_no_buf;

   fbuf->supports_unsync = supports_unsync;
   fbuf->bt_depth = bt_depth;
   pipe_reference_init(&fbuf->reference, 1);
   pipe_mutex_init(fbuf->mutex);

   return fbuf;
out_no_buf:
   debug_printf("Debug flush buffer creation failed.\n");
   debug_printf("Debug flush checking for this buffer will be incomplete.\n");
   return NULL;
}

void
debug_flush_buf_reference(struct debug_flush_buf **dst,
                          struct debug_flush_buf *src)
{
   struct debug_flush_buf *fbuf = *dst;

   if (pipe_reference(&(*dst)->reference, &src->reference)) {
      if (fbuf->map_frame)
         FREE(fbuf->map_frame);

      FREE(fbuf);
   }

   *dst = src;
}

static void
debug_flush_item_destroy(struct debug_flush_item *item)
{
   debug_flush_buf_reference(&item->fbuf, NULL);

   if (item->ref_frame)
      FREE(item->ref_frame);

   FREE(item);
}

struct debug_flush_ctx *
debug_flush_ctx_create(boolean catch_reference_of_mapped, unsigned bt_depth)
{
   struct debug_flush_ctx *fctx = CALLOC_STRUCT(debug_flush_ctx);

   if (!fctx)
      goto out_no_ctx;

   fctx->ref_hash = util_hash_table_create(debug_flush_pointer_hash,
                                           debug_flush_pointer_compare);

   if (!fctx->ref_hash)
      goto out_no_ref_hash;

   fctx->bt_depth = bt_depth;
   pipe_mutex_lock(list_mutex);
   list_addtail(&fctx->head, &ctx_list);
   pipe_mutex_unlock(list_mutex);

   return fctx;

 out_no_ref_hash:
   FREE(fctx);
out_no_ctx:
   debug_printf("Debug flush context creation failed.\n");
   debug_printf("Debug flush checking for this context will be incomplete.\n");
   return NULL;
}

static void
debug_flush_alert(const char *s, const char *op,
                  unsigned start, unsigned depth,
                  boolean continued,
                  boolean capture,
                  const struct debug_stack_frame *frame)
{
   if (capture)
      frame = debug_flush_capture_frame(start, depth);

   if (s)
      debug_printf("%s ", s);
   if (frame) {
      debug_printf("%s backtrace follows:\n", op);
      debug_backtrace_dump(frame, depth);
   } else
      debug_printf("No %s backtrace was captured.\n", op);

   if (continued)
      debug_printf("**********************************\n");
   else
      debug_printf("*********END OF MESSAGE***********\n\n\n");

   if (capture)
      FREE((void *)frame);
}


void
debug_flush_map(struct debug_flush_buf *fbuf, unsigned flags)
{
   boolean mapped_sync = FALSE;

   if (!fbuf)
      return;

   pipe_mutex_lock(fbuf->mutex);
   if (fbuf->mapped) {
      debug_flush_alert("Recursive map detected.", "Map",
                        2, fbuf->bt_depth, TRUE, TRUE, NULL);
      debug_flush_alert(NULL, "Previous map", 0, fbuf->bt_depth, FALSE,
                        FALSE, fbuf->map_frame);
   } else if (!(flags & PIPE_TRANSFER_UNSYNCHRONIZED) ||
              !fbuf->supports_unsync) {
      fbuf->mapped_sync = mapped_sync = TRUE;
   }
   fbuf->map_frame = debug_flush_capture_frame(1, fbuf->bt_depth);
   fbuf->mapped = TRUE;
   pipe_mutex_unlock(fbuf->mutex);

   if (mapped_sync) {
      struct debug_flush_ctx *fctx;

      pipe_mutex_lock(list_mutex);
      LIST_FOR_EACH_ENTRY(fctx, &ctx_list, head) {
         struct debug_flush_item *item =
            util_hash_table_get(fctx->ref_hash, fbuf);

         if (item && fctx->catch_map_of_referenced) {
            debug_flush_alert("Already referenced map detected.",
                              "Map", 2, fbuf->bt_depth, TRUE, TRUE, NULL);
            debug_flush_alert(NULL, "Reference", 0, item->bt_depth,
                              FALSE, FALSE, item->ref_frame);
         }
      }
      pipe_mutex_unlock(list_mutex);
   }
}

void
debug_flush_unmap(struct debug_flush_buf *fbuf)
{
   if (!fbuf)
      return;

   pipe_mutex_lock(fbuf->mutex);
   if (!fbuf->mapped)
      debug_flush_alert("Unmap not previously mapped detected.", "Map",
                        2, fbuf->bt_depth, FALSE, TRUE, NULL);

   fbuf->mapped_sync = FALSE;
   fbuf->mapped = FALSE;
   if (fbuf->map_frame) {
      FREE(fbuf->map_frame);
      fbuf->map_frame = NULL;
   }
   pipe_mutex_unlock(fbuf->mutex);
}

void
debug_flush_cb_reference(struct debug_flush_ctx *fctx,
                         struct debug_flush_buf *fbuf)
{
   struct debug_flush_item *item;

   if (!fctx || !fbuf)
      return;

   item = util_hash_table_get(fctx->ref_hash, fbuf);

   pipe_mutex_lock(fbuf->mutex);
   if (fbuf->mapped_sync) {
      debug_flush_alert("Reference of mapped buffer detected.", "Reference",
                        2, fctx->bt_depth, TRUE, TRUE, NULL);
      debug_flush_alert(NULL, "Map", 0, fbuf->bt_depth, FALSE,
                        FALSE, fbuf->map_frame);
   }
   pipe_mutex_unlock(fbuf->mutex);

   if (!item) {
      item = CALLOC_STRUCT(debug_flush_item);
      if (item) {
         debug_flush_buf_reference(&item->fbuf, fbuf);
         item->bt_depth = fctx->bt_depth;
         item->ref_frame = debug_flush_capture_frame(2, item->bt_depth);
         if (util_hash_table_set(fctx->ref_hash, fbuf, item) != PIPE_OK) {
            debug_flush_item_destroy(item);
            goto out_no_item;
         }
         return;
      }
      goto out_no_item;
   }
   return;

out_no_item:
   debug_printf("Debug flush command buffer reference creation failed.\n");
   debug_printf("Debug flush checking will be incomplete "
                "for this command batch.\n");
}

static enum pipe_error
debug_flush_might_flush_cb(void *key, void *value, void *data)
{
   struct debug_flush_item *item =
      (struct debug_flush_item *) value;
   struct debug_flush_buf *fbuf = item->fbuf;
   const char *reason = (const char *) data;
   char message[80];

   util_snprintf(message, sizeof(message),
                 "%s referenced mapped buffer detected.", reason);

   pipe_mutex_lock(fbuf->mutex);
   if (fbuf->mapped_sync) {
      debug_flush_alert(message, reason, 3, item->bt_depth, TRUE, TRUE, NULL);
      debug_flush_alert(NULL, "Map", 0, fbuf->bt_depth, TRUE, FALSE,
                        fbuf->map_frame);
      debug_flush_alert(NULL, "First reference", 0, item->bt_depth, FALSE,
                        FALSE, item->ref_frame);
   }
   pipe_mutex_unlock(fbuf->mutex);

   return PIPE_OK;
}

void
debug_flush_might_flush(struct debug_flush_ctx *fctx)
{
   if (!fctx)
      return;

   util_hash_table_foreach(fctx->ref_hash,
                           debug_flush_might_flush_cb,
                           "Might flush");
}

static enum pipe_error
debug_flush_flush_cb(void *key, void *value, void *data)
{
   struct debug_flush_item *item =
      (struct debug_flush_item *) value;

   debug_flush_item_destroy(item);

   return PIPE_OK;
}


void
debug_flush_flush(struct debug_flush_ctx *fctx)
{
   if (!fctx)
      return;

   util_hash_table_foreach(fctx->ref_hash,
                           debug_flush_might_flush_cb,
                           "Flush");
   util_hash_table_foreach(fctx->ref_hash,
                           debug_flush_flush_cb,
                           NULL);
   util_hash_table_clear(fctx->ref_hash);
}

void
debug_flush_ctx_destroy(struct debug_flush_ctx *fctx)
{
   if (!fctx)
      return;

   list_del(&fctx->head);
   util_hash_table_foreach(fctx->ref_hash,
                           debug_flush_flush_cb,
                           NULL);
   util_hash_table_clear(fctx->ref_hash);
   util_hash_table_destroy(fctx->ref_hash);
   FREE(fctx);
}
#endif
