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

/* Authors:  Zack Rusin <zack@tungstengraphics.com>
 */

#include "cso_cache.h"

#if 1
static unsigned hash_key( const void *key, unsigned key_size )
{
   unsigned *ikey = (unsigned *)key;
   unsigned hash = 0, i;

   assert(key_size % 4 == 0);

   /* I'm sure this can be improved on:
    */
   for (i = 0; i < key_size/4; i++)
      hash ^= ikey[i];

   return hash;
}
#else
static unsigned hash_key(const unsigned char *p, int n)
{
   unsigned h = 0;
   unsigned g;

   while (n--) {
      h = (h << 4) + *p++;
      if ((g = (h & 0xf0000000)) != 0)
         h ^= g >> 23;
      h &= ~g;
   }
   return h;
}
#endif

unsigned cso_construct_key(void *item, int item_size)
{
   return hash_key((const unsigned char*)(item), item_size);
}

struct cso_cache_item *
cso_insert_state(struct cso_cache *sc,
                 unsigned hash_key,
                 void *state, int state_size)
{
   struct cso_cache_item *found_state =
      _mesa_HashLookup(sc->hash, hash_key);
   struct cso_cache_item *item =
      malloc(sizeof(struct cso_cache_item));
   _mesa_printf("inserting state ========= key = %d\n", hash_key);
   item->key        = hash_key;
   item->state_size = state_size;
   item->state      = state;
   item->next       = 0;

   if (found_state) {
      while (found_state->next)
         found_state = found_state->next;
      found_state->next = item;
   } else
      _mesa_HashInsert(sc->hash, hash_key, item);
   return item;
}

struct cso_cache_item *
cso_find_state(struct cso_cache *sc,
               unsigned hash_key,
               void *state, int state_size)
{
   struct cso_cache_item *found_state =
      _mesa_HashLookup(sc->hash, hash_key);

   while (found_state &&
          (found_state->state_size != state_size ||
           memcmp(found_state->state, state, state_size))) {
      found_state = found_state->next;
   }

   _mesa_printf("finding state  ========== %d (%p)\n", hash_key, found_state);
   return found_state;
}

struct cso_cache_item *
cso_remove_state(struct cso_cache *sc,
                 unsigned hash_key,
                 void *state, int state_size)
{
   struct cso_cache_item *found_state =
      _mesa_HashLookup(sc->hash, hash_key);
   struct cso_cache_item *prev = 0;

   while (found_state &&
          (found_state->state_size != state_size ||
           memcmp(found_state->state, state, state_size))) {
      prev = found_state;
      found_state = found_state->next;
   }
   if (found_state) {
      if (prev)
         prev->next = found_state->next;
      else {
         if (found_state->next)
            _mesa_HashInsert(sc->hash, hash_key, found_state->next);
         else
            _mesa_HashRemove(sc->hash, hash_key);
      }
   }
   return found_state;
}

struct cso_cache *cso_cache_create(void)
{
   struct cso_cache *sc = malloc(sizeof(struct cso_cache));

   sc->hash = _mesa_NewHashTable();

   return sc;
}

void cso_cache_destroy(struct cso_cache *sc)
{
   assert(sc);
   assert(sc->hash);
   _mesa_DeleteHashTable(sc->hash);
   free(sc);
}

/* This function will either find the state of the given template
 * in the cache or it will create a new state state from the given
 * template, will insert it in the cache and return it.
 */
struct pipe_blend_state * cso_cached_blend_state(
   struct st_context *st,
   const struct pipe_blend_state *blend)
{
   unsigned hash_key = cso_construct_key((void*)blend, sizeof(struct pipe_blend_state));
   struct cso_cache_item *cache_item = cso_find_state(st->cache,
                                                      hash_key,
                                                      (void*)blend,
                                                      sizeof(struct pipe_blend_state));
   if (!cache_item) {
      const struct pipe_blend_state *created_state = st->pipe->create_blend_state(
         st->pipe, blend);
      cache_item = cso_insert_state(st->cache, hash_key,
                                    (void*)created_state, sizeof(struct pipe_blend_state));
   }
   return (struct pipe_blend_state*)cache_item->state;
}
