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

#include "util/u_debug.h"

#include "util/u_memory.h"

#include "cso_cache.h"
#include "cso_hash.h"


struct cso_cache {
   struct cso_hash *blend_hash;
   struct cso_hash *depth_stencil_hash;
   struct cso_hash *fs_hash;
   struct cso_hash *vs_hash;
   struct cso_hash *rasterizer_hash;
   struct cso_hash *sampler_hash;
   int    max_size;

   cso_sanitize_callback sanitize_cb;
   void                 *sanitize_data;
};

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
      break;
   case CSO_SAMPLER:
      hash = sc->sampler_hash;
      break;
   case CSO_DEPTH_STENCIL_ALPHA:
      hash = sc->depth_stencil_hash;
      break;
   case CSO_RASTERIZER:
      hash = sc->rasterizer_hash;
      break;
   case CSO_FRAGMENT_SHADER:
      hash = sc->fs_hash;
      break;
   case CSO_VERTEX_SHADER:
      hash = sc->vs_hash;
      break;
   }

   return hash;
}

static void delete_blend_state(void *state, void *data)
{
   struct cso_blend *cso = (struct cso_blend *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}

static void delete_depth_stencil_state(void *state, void *data)
{
   struct cso_depth_stencil_alpha *cso = (struct cso_depth_stencil_alpha *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}

static void delete_sampler_state(void *state, void *data)
{
   struct cso_sampler *cso = (struct cso_sampler *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}

static void delete_rasterizer_state(void *state, void *data)
{
   struct cso_rasterizer *cso = (struct cso_rasterizer *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}

static void delete_fs_state(void *state, void *data)
{
   struct cso_fragment_shader *cso = (struct cso_fragment_shader *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}

static void delete_vs_state(void *state, void *data)
{
   struct cso_vertex_shader *cso = (struct cso_vertex_shader *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
}


static INLINE void delete_cso(void *state, enum cso_cache_type type)
{
   switch (type) {
   case CSO_BLEND:
      delete_blend_state(state, 0);
      break;
   case CSO_SAMPLER:
      delete_sampler_state(state, 0);
      break;
   case CSO_DEPTH_STENCIL_ALPHA:
      delete_depth_stencil_state(state, 0);
      break;
   case CSO_RASTERIZER:
      delete_rasterizer_state(state, 0);
      break;
   case CSO_FRAGMENT_SHADER:
      delete_fs_state(state, 0);
      break;
   case CSO_VERTEX_SHADER:
      delete_vs_state(state, 0);
      break;
   default:
      assert(0);
      FREE(state);
   }
}


static INLINE void sanitize_hash(struct cso_cache *sc,
                                 struct cso_hash *hash,
                                 enum cso_cache_type type,
                                 int max_size)
{
   if (sc->sanitize_cb)
      sc->sanitize_cb(hash, type, max_size, sc->sanitize_data);
}


static INLINE void sanitize_cb(struct cso_hash *hash, enum cso_cache_type type,
                               int max_size, void *user_data)
{
   /* if we're approach the maximum size, remove fourth of the entries
    * otherwise every subsequent call will go through the same */
   int hash_size = cso_hash_size(hash);
   int max_entries = (max_size > hash_size) ? max_size : hash_size;
   int to_remove =  (max_size < max_entries) * max_entries/4;
   if (hash_size > max_size)
      to_remove += hash_size - max_size;
   while (to_remove) {
      /*remove elements until we're good */
      /*fixme: currently we pick the nodes to remove at random*/
      struct cso_hash_iter iter = cso_hash_first_node(hash);
      void  *cso = cso_hash_take(hash, cso_hash_iter_key(iter));
      delete_cso(cso, type);
      --to_remove;
   }
}

struct cso_hash_iter
cso_insert_state(struct cso_cache *sc,
                 unsigned hash_key, enum cso_cache_type type,
                 void *state)
{
   struct cso_hash *hash = _cso_hash_for_type(sc, type);
   sanitize_hash(sc, hash, type, sc->max_size);

   return cso_hash_insert(hash, hash_key, state);
}

struct cso_hash_iter
cso_find_state(struct cso_cache *sc,
               unsigned hash_key, enum cso_cache_type type)
{
   struct cso_hash *hash = _cso_hash_for_type(sc, type);

   return cso_hash_find(hash, hash_key);
}


void *cso_hash_find_data_from_template( struct cso_hash *hash,
				        unsigned hash_key, 
				        void *templ,
				        int size )
{
   struct cso_hash_iter iter = cso_hash_find(hash, hash_key);
   while (!cso_hash_iter_is_null(iter)) {
      void *iter_data = cso_hash_iter_data(iter);
      if (!memcmp(iter_data, templ, size)) {
	 /* We found a match
	  */
         return iter_data;
      }
      iter = cso_hash_iter_next(iter);
   }
   return NULL;
}


struct cso_hash_iter cso_find_state_template(struct cso_cache *sc,
                                             unsigned hash_key, enum cso_cache_type type,
                                             void *templ, unsigned size)
{
   struct cso_hash_iter iter = cso_find_state(sc, hash_key, type);
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
   struct cso_cache *sc = MALLOC_STRUCT(cso_cache);
   if (sc == NULL)
      return NULL;

   sc->max_size           = 4096;
   sc->blend_hash         = cso_hash_create();
   sc->sampler_hash       = cso_hash_create();
   sc->depth_stencil_hash = cso_hash_create();
   sc->rasterizer_hash    = cso_hash_create();
   sc->fs_hash            = cso_hash_create();
   sc->vs_hash            = cso_hash_create();
   sc->sanitize_cb        = sanitize_cb;
   sc->sanitize_data      = 0;

   return sc;
}

void cso_for_each_state(struct cso_cache *sc, enum cso_cache_type type,
                        cso_state_callback func, void *user_data)
{
   struct cso_hash *hash = 0;
   struct cso_hash_iter iter;

   switch (type) {
   case CSO_BLEND:
      hash = sc->blend_hash;
      break;
   case CSO_SAMPLER:
      hash = sc->sampler_hash;
      break;
   case CSO_DEPTH_STENCIL_ALPHA:
      hash = sc->depth_stencil_hash;
      break;
   case CSO_RASTERIZER:
      hash = sc->rasterizer_hash;
      break;
   case CSO_FRAGMENT_SHADER:
      hash = sc->fs_hash;
      break;
   case CSO_VERTEX_SHADER:
      hash = sc->vs_hash;
      break;
   }

   iter = cso_hash_first_node(hash);
   while (!cso_hash_iter_is_null(iter)) {
      void *state = cso_hash_iter_data(iter);
      iter = cso_hash_iter_next(iter);
      if (state) {
         func(state, user_data);
      }
   }
}

void cso_cache_delete(struct cso_cache *sc)
{
   assert(sc);

   if (!sc)
      return;

   /* delete driver data */
   cso_for_each_state(sc, CSO_BLEND, delete_blend_state, 0);
   cso_for_each_state(sc, CSO_DEPTH_STENCIL_ALPHA, delete_depth_stencil_state, 0);
   cso_for_each_state(sc, CSO_FRAGMENT_SHADER, delete_fs_state, 0);
   cso_for_each_state(sc, CSO_VERTEX_SHADER, delete_vs_state, 0);
   cso_for_each_state(sc, CSO_RASTERIZER, delete_rasterizer_state, 0);
   cso_for_each_state(sc, CSO_SAMPLER, delete_sampler_state, 0);

   cso_hash_delete(sc->blend_hash);
   cso_hash_delete(sc->sampler_hash);
   cso_hash_delete(sc->depth_stencil_hash);
   cso_hash_delete(sc->rasterizer_hash);
   cso_hash_delete(sc->fs_hash);
   cso_hash_delete(sc->vs_hash);
   FREE(sc);
}

void cso_set_maximum_cache_size(struct cso_cache *sc, int number)
{
   sc->max_size = number;

   sanitize_hash(sc, sc->blend_hash, CSO_BLEND, sc->max_size);
   sanitize_hash(sc, sc->depth_stencil_hash, CSO_DEPTH_STENCIL_ALPHA,
                 sc->max_size);
   sanitize_hash(sc, sc->fs_hash, CSO_FRAGMENT_SHADER, sc->max_size);
   sanitize_hash(sc, sc->vs_hash, CSO_VERTEX_SHADER, sc->max_size);
   sanitize_hash(sc, sc->rasterizer_hash, CSO_RASTERIZER, sc->max_size);
   sanitize_hash(sc, sc->sampler_hash, CSO_SAMPLER, sc->max_size);
}

int cso_maximum_cache_size(const struct cso_cache *sc)
{
   return sc->max_size;
}

void cso_cache_set_sanitize_callback(struct cso_cache *sc,
                                     cso_sanitize_callback cb,
                                     void *user_data)
{
   sc->sanitize_cb   = cb;
   sc->sanitize_data = user_data;
}

