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

 /*
  * Authors:
  *   Zack Rusin <zack@tungstengraphics.com>
  */

#ifndef CSO_CACHE_H
#define CSO_CACHE_H

#include "state_tracker/st_context.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"


#include "main/hash.h"

struct cso_cache_item {
   unsigned key;

   unsigned    state_size;
   const void *state;

   struct cso_cache_item *next;
};

struct cso_cache {
   struct _mesa_HashTable *hash;
};

void cso_cache_destroy(struct cso_cache *sc);
struct cso_cache *cso_cache_create(void);

unsigned cso_construct_key(void *item, int item_size);

struct cso_cache_item *cso_insert_state(struct cso_cache *sc,
                                                   unsigned hash_key,
                                                   void *state, int state_size);
struct cso_cache_item *cso_find_state(struct cso_cache *sc,
                                                 unsigned hash_key,
                                                 void *state, int state_size);
struct cso_cache_item *cso_remove_state(struct cso_cache *sc,
                                                   unsigned hash_key,
                                                   void *state, int state_size);

struct pipe_blend_state *cso_cached_blend_state(
    struct st_context *pipe,
    const struct pipe_blend_state *state);

#endif
