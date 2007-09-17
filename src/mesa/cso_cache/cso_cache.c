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
#include "cso_hash.h"

#if 1
static unsigned hash_key(const void *key, unsigned key_size)
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
   return hash_key((item), item_size);
}

static struct cso_hash *_cso_hash_for_type(struct cso_cache *sc, enum cso_cache_type type)
{
   struct cso_hash *hash = 0;

   switch(type) {
   case CSO_BLEND:
      hash = sc->blend_hash;
   case CSO_SAMPLER:
      hash = sc->sampler_hash;
   }

   return hash;
}

static int _cso_size_for_type(enum cso_cache_type type)
{
   switch(type) {
   case CSO_BLEND:
      return sizeof(struct pipe_blend_state);
   case CSO_SAMPLER:
      return sizeof(struct pipe_sampler_state);
   }
   return 0;
}

struct cso_hash_iter
cso_insert_state(struct cso_cache *sc,
                 unsigned hash_key, enum cso_cache_type type,
                 void *state)
{
   struct cso_hash *hash = _cso_hash_for_type(sc, type);
   return cso_hash_insert(hash, hash_key, state);
}

struct cso_hash_iter
cso_find_state(struct cso_cache *sc,
               unsigned hash_key, enum cso_cache_type type)
{
   struct cso_hash *hash = _cso_hash_for_type(sc, type);

   return cso_hash_find(hash, hash_key);
}

struct cso_hash_iter cso_find_state_template(struct cso_cache *sc,
                                             unsigned hash_key, enum cso_cache_type type,
                                             void *templ)
{
   struct cso_hash_iter iter = cso_find_state(sc, hash_key, type);
   int size = _cso_size_for_type(type);
   while (!cso_hash_iter_is_null(iter)) {
      void *iter_data = cso_hash_iter_data(iter);
      if (!memcmp(iter_data, templ, size))
         return iter;
      iter = cso_hash_iter_next(iter);
   }
   return iter;
}

void * cso_take_state(struct cso_cache *sc,
                      unsigned hash_key, enum cso_cache_type type)
{
   struct cso_hash *hash = _cso_hash_for_type(sc, type);
   return cso_hash_take(hash, hash_key);
}

struct cso_cache *cso_cache_create(void)
{
   struct cso_cache *sc = malloc(sizeof(struct cso_cache));

   sc->blend_hash = cso_hash_create();
   sc->sampler_hash = cso_hash_create();

   return sc;
}

void cso_cache_delete(struct cso_cache *sc)
{
   assert(sc);
   cso_hash_delete(sc->blend_hash);
   cso_hash_delete(sc->sampler_hash);
   free(sc);
}

