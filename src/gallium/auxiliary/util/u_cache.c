/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
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
 * Simple cache implementation.
 *
 * We simply have fixed size array, and destroy previous values on collision. 
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "pipe/p_compiler.h"
#include "util/u_debug.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_cache.h"


struct util_cache_entry
{
   void *key;
   void *value;
   
#ifdef DEBUG
   unsigned count;
#endif
};


struct util_cache
{
   /** Hash function */
   uint32_t (*hash)(const void *key);
   
   /** Compare two keys */
   int (*compare)(const void *key1, const void *key2);

   /** Destroy a (key, value) pair */
   void (*destroy)(void *key, void *value);

   uint32_t size;
   
   struct util_cache_entry *entries;
   
#ifdef DEBUG
   unsigned count;
#endif
};


struct util_cache *
util_cache_create(uint32_t (*hash)(const void *key),
                  int (*compare)(const void *key1, const void *key2),
                  void (*destroy)(void *key, void *value),
                  uint32_t size)
{
   struct util_cache *cache;
   
   cache = CALLOC_STRUCT(util_cache);
   if(!cache)
      return NULL;
   
   cache->hash = hash;
   cache->compare = compare;
   cache->destroy = destroy;
   cache->size = size;
   
   cache->entries = CALLOC(size, sizeof(struct util_cache_entry));
   if(!cache->entries) {
      FREE(cache);
      return NULL;
   }
   
   return cache;
}


static INLINE struct util_cache_entry *
util_cache_entry_get(struct util_cache *cache,
                     const void *key)
{
   uint32_t hash;
   
   hash = cache->hash(key);

   return &cache->entries[hash % cache->size];
}

static INLINE void
util_cache_entry_destroy(struct util_cache *cache,
                         struct util_cache_entry *entry)
{
   void *key = entry->key;
   void *value = entry->value;
   
   entry->key = NULL;
   entry->value = NULL;
   
   if(key || value)
      if(cache->destroy)
         cache->destroy(key, value);
}


void
util_cache_set(struct util_cache *cache,
               void *key,
               void *value)
{
   struct util_cache_entry *entry;

   assert(cache);
   if (!cache)
      return;

   entry = util_cache_entry_get(cache, key);
   util_cache_entry_destroy(cache, entry);
   
#ifdef DEBUG
   ++entry->count;
   ++cache->count;
#endif
   
   entry->key = key;
   entry->value = value;
}


void *
util_cache_get(struct util_cache *cache, 
               const void *key)
{
   struct util_cache_entry *entry;

   assert(cache);
   if (!cache)
      return NULL;

   entry = util_cache_entry_get(cache, key);
   if(!entry->key && !entry->value)
      return NULL;
   
   if(cache->compare(key, entry->key) != 0)
      return NULL;
   
   return entry->value;
}


void 
util_cache_clear(struct util_cache *cache)
{
   uint32_t i;

   assert(cache);
   if (!cache)
      return;

   for(i = 0; i < cache->size; ++i)
      util_cache_entry_destroy(cache, &cache->entries[i]);
}


void
util_cache_destroy(struct util_cache *cache)
{
   assert(cache);
   if (!cache)
      return;

#ifdef DEBUG
   if(cache->count >= 20*cache->size) {
      /* Normal approximation of the Poisson distribution */
      double mean = (double)cache->count/(double)cache->size;
      double stddev = sqrt(mean);
      unsigned i;
      for(i = 0; i < cache->size; ++i) {
         double z = fabs(cache->count - mean)/stddev;
         /* This assert should not fail 99.9999998027% of the times, unless 
          * the hash function is a poor one */
         assert(z <= 6.0);
      }
   }
#endif
      
   util_cache_clear(cache);
   
   FREE(cache->entries);
   FREE(cache);
}
