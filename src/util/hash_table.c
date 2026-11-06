/*
 * Copyright © 2009,2012 Intel Corporation
 * Copyright © 1988-2004 Keith Packard and Bart Massey.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors
 * or their institutions shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this
 * Software without prior written authorization from the
 * authors.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 */

/**
 * Implements an open-addressing, linear-reprobing hash table.
 *
 * For more information, see:
 *
 * http://cgit.freedesktop.org/~anholt/hash_table/tree/README
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash_table.h"
#include "ralloc.h"
#include "macros.h"
#include "u_memory.h"
#include "fast_urem_by_const.h"
#include "util/u_memory.h"

/* glsl-optimizer change: FNV-1a instead of xxHash (no xxhash.h). These are
 * in-memory hash tables only, any reasonable hash works. */
static inline uint32_t
hash_fnv1a(const void *data, size_t size, uint32_t seed)
{
   const uint8_t *p = (const uint8_t *)data;
   uint32_t h = 2166136261u ^ seed;
   for (size_t i = 0; i < size; i++) {
      h ^= p[i];
      h *= 16777619u;
   }
   return h;
}

static inline void *
uint_key(unsigned id)
{
   return (void *)(uintptr_t) id;
}

/**
 * From Knuth -- a good choice for hash/rehash values is p, p-2 where
 * p and p-2 are both prime.  These tables are sized to have an extra 10%
 * free to avoid exponential performance degradation as the hash table fills
 */
static const struct {
   uint32_t max_entries, size, rehash;
   uint64_t size_magic, rehash_magic;
} hash_sizes[] = {
#define ENTRY(max_entries, size, rehash) \
   { max_entries, size, rehash, \
      REMAINDER_MAGIC(size), REMAINDER_MAGIC(rehash) }

   /* Starting with only 2 entries at initialization causes a lot of table
    * reallocations and rehashing while growing the table.
    *
    * Below are results from counting reallocations when compiling
    * my GLSL shader-db on radeonsi+ACO.
    *
    * 2 entries is the baseline.
    * Starting with  4 entries reduces reallocations to 70%.
    * Starting with  8 entries reduces reallocations to 48%.
    * Starting with 16 entries reduces reallocations to 33%.
    * Starting with 32 entries reduces reallocations to 21%.
    * Starting with 64 entries reduces reallocations to 13%.
    */
#if 0 /* Start with 16 entries. */
   ENTRY(2,            5,            3            ),
   ENTRY(4,            7,            5            ),
   ENTRY(8,            13,           11           ),
#endif
   ENTRY(16,           19,           17           ),
   ENTRY(32,           43,           41           ),
   ENTRY(64,           73,           71           ),
   ENTRY(128,          151,          149          ),
   ENTRY(256,          283,          281          ),
   ENTRY(512,          571,          569          ),
   ENTRY(1024,         1153,         1151         ),
   ENTRY(2048,         2269,         2267         ),
   ENTRY(4096,         4519,         4517         ),
   ENTRY(8192,         9013,         9011         ),
   ENTRY(16384,        18043,        18041        ),
   ENTRY(32768,        36109,        36107        ),
   ENTRY(65536,        72091,        72089        ),
   ENTRY(131072,       144409,       144407       ),
   ENTRY(262144,       288361,       288359       ),
   ENTRY(524288,       576883,       576881       ),
   ENTRY(1048576,      1153459,      1153457      ),
   ENTRY(2097152,      2307163,      2307161      ),
   ENTRY(4194304,      4613893,      4613891      ),
   ENTRY(8388608,      9227641,      9227639      ),
   ENTRY(16777216,     18455029,     18455027     ),
   ENTRY(33554432,     36911011,     36911009     ),
   ENTRY(67108864,     73819861,     73819859     ),
   ENTRY(134217728,    147639589,    147639587    ),
   ENTRY(268435456,    295279081,    295279079    ),
   ENTRY(536870912,    590559793,    590559791    ),
   ENTRY(1073741824,   1181116273,   1181116271   ),
   ENTRY(2147483648ul, 2362232233ul, 2362232231ul )
};

void
_mesa_hash_table_init(struct hash_table *ht,
                      void *mem_ctx,
                      uint32_t (*key_hash_function)(const void *key),
                      bool (*key_equals_function)(const void *a,
                                                  const void *b))
{
   ht->mem_ctx = mem_ctx;
   ht->size_index = 0;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->size_magic = hash_sizes[ht->size_index].size_magic;
   ht->rehash_magic = hash_sizes[ht->size_index].rehash_magic;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->key_hash_function = key_hash_function;
   ht->key_equals_function = key_equals_function;
   ht->table_destructor = NULL;
   assert(ht->size == ARRAY_SIZE(ht->_initial_storage));
   ht->table = ht->_initial_storage;
   memset(ht->table, 0, sizeof(ht->_initial_storage));
   ht->entries = 0;
   ht->deleted_entries = 0;
}

static void
_mesa_hash_table_initial_storage_destructor(void *data)
{
   struct hash_table *ht = (struct hash_table*)data;

   if (!ht->table)
      return;

   if (ht->table_destructor)
      ht->table_destructor(ht);

   ht->table = NULL; /* avoid double-calling the table destructor */
}

static void
_mesa_hash_table_ralloc_table_destructor(void *data)
{
   void *parent = ralloc_parent(data);

   _mesa_hash_table_initial_storage_destructor(parent);
}

static void
_mesa_hash_table_set_destructor(struct hash_table *ht,
                                void(*table_destructor)(void *data))
{
   ht->table_destructor = table_destructor;
   ralloc_set_destructor(ht, _mesa_hash_table_initial_storage_destructor);
   if (ht->table != ht->_initial_storage)
      ralloc_set_destructor(ht->table, _mesa_hash_table_ralloc_table_destructor);
}

/* It's preferred to use _mesa_hash_table_init instead of this to skip ralloc. */
struct hash_table *
_mesa_hash_table_create(void *mem_ctx,
                        uint32_t (*key_hash_function)(const void *key),
                        bool (*key_equals_function)(const void *a,
                                                    const void *b))
{
   struct hash_table *ht;

   /* mem_ctx is used to allocate the hash table, but the hash table is used
    * to allocate all of the suballocations.
    */
   ht = ralloc(mem_ctx, struct hash_table);
   if (ht == NULL)
      return NULL;

   _mesa_hash_table_init(ht, ht, key_hash_function, key_equals_function);
   return ht;
}

static uint32_t
key_u32_hash(const void *key)
{
   uint32_t u = (uint32_t)(uintptr_t)key;
   return _mesa_hash_uint(&u);
}

static bool
key_u32_equals(const void *a, const void *b)
{
   return (uint32_t)(uintptr_t)a == (uint32_t)(uintptr_t)b;
}

struct hash_table *
_mesa_hash_table_create_u32_keys(void *mem_ctx)
{
   return _mesa_hash_table_create(mem_ctx, key_u32_hash, key_u32_equals);
}

void
_mesa_hash_table_init_u32_keys(struct hash_table *ht, void *mem_ctx)
{
   _mesa_hash_table_init(ht, mem_ctx, key_u32_hash, key_u32_equals);
}

/* Copy the hash table from src to dst. */
bool
_mesa_hash_table_copy(struct hash_table *dst, struct hash_table *src,
                      void *dst_mem_ctx)
{
   /* Copy the whole structure except the initial storage. */
   memcpy(dst, src, offsetof(struct hash_table, _initial_storage));
   dst->mem_ctx = dst_mem_ctx;

   if (src->table != src->_initial_storage) {
      dst->table = ralloc_array(dst_mem_ctx, struct hash_entry, dst->size);
      if (dst->table == NULL)
         return false;

      memcpy(dst->table, src->table, dst->size * sizeof(struct hash_entry));
   } else {
      dst->table = dst->_initial_storage;
      memcpy(dst->table, src->_initial_storage, sizeof(src->_initial_storage));
   }

   return true;
}

/* It's preferred to use _mesa_hash_table_copy instead of this to skip ralloc. */
struct hash_table *
_mesa_hash_table_clone(struct hash_table *src, void *dst_mem_ctx)
{
   struct hash_table *ht;

   ht = ralloc(dst_mem_ctx, struct hash_table);
   if (ht == NULL)
      return NULL;

   if (!_mesa_hash_table_copy(ht, src, dst_mem_ctx)) {
      ralloc_free(ht);
      return NULL;
   }

   return ht;
}

/**
 * Deallocates the internal table. This is optional and doesn't need to be
 * called when:
 * - you don't need to call delete_function
 * - the initial ralloc context is non-NULL, meaning it gets freed
 *   automatically when the ralloc parent is freed.
 */
void
_mesa_hash_table_fini(struct hash_table *ht,
                      void (*delete_function)(struct hash_entry *entry))
{
   if (delete_function) {
      hash_table_foreach(ht, entry) {
         delete_function(entry);
      }
   }
   if (ht->table != ht->_initial_storage)
      ralloc_free(ht->table);

   ht->table = NULL;
}

/**
 * Frees the given hash table.
 *
 * If delete_function is passed, it gets called on each entry present before
 * freeing.
 */
void
_mesa_hash_table_destroy(struct hash_table *ht,
                         void (*delete_function)(struct hash_entry *entry))
{
   if (!ht)
      return;

   _mesa_hash_table_fini(ht, delete_function);
   ralloc_free(ht);
}

static void
hash_table_clear_fast(struct hash_table *ht)
{
   memset(ht->table, 0, sizeof(struct hash_entry) * hash_sizes[ht->size_index].size);
   ht->entries = ht->deleted_entries = 0;
}

/**
 * Deletes all entries of the given hash table without deleting the table
 * itself or changing its structure.
 *
 * If delete_function is passed, it gets called on each entry present.
 */
void
_mesa_hash_table_clear(struct hash_table *ht,
                       void (*delete_function)(struct hash_entry *entry))
{
   if (!ht)
      return;

   struct hash_entry *entry;

   if (delete_function) {
      for (entry = ht->table; entry != ht->table + ht->size; entry++) {
         if (entry->present)
            delete_function(entry);

         entry->present = false;
         entry->deleted = false;
      }
      ht->entries = 0;
      ht->deleted_entries = 0;
   } else
      hash_table_clear_fast(ht);
}

static struct hash_entry *
hash_table_search(const struct hash_table *ht, uint32_t hash, const void *key)
{
   uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
                                               ht->rehash_magic);
   uint32_t hash_address = start_hash_address;

   do {
      struct hash_entry *entry = ht->table + hash_address;

      if (!entry->present && !entry->deleted) {
         return NULL;
      } else if (entry->present && entry->hash == hash &&
                 ht->key_equals_function(key, entry->key)) {
         return entry;
      }

      hash_address += double_hash;
      if (hash_address >= size)
         hash_address -= size;
   } while (hash_address != start_hash_address);

   return NULL;
}

/**
 * Finds a hash table entry with the given key and hash of that key.
 *
 * Returns NULL if no entry is found.  Note that the data pointer may be
 * modified by the user.
 */
struct hash_entry *
_mesa_hash_table_search(const struct hash_table *ht, const void *key)
{
   assert(ht->key_hash_function);
   return hash_table_search(ht, ht->key_hash_function(key), key);
}

struct hash_entry *
_mesa_hash_table_search_pre_hashed(struct hash_table *ht, uint32_t hash,
                                  const void *key)
{
   assert(ht->key_hash_function == NULL || hash == ht->key_hash_function(key));
   return hash_table_search(ht, hash, key);
}

static struct hash_entry *
hash_table_insert(struct hash_table *ht, uint32_t hash,
                  const void *key, void *data);

static void
hash_table_insert_rehash(struct hash_table *ht, uint32_t hash,
                         const void *key, void *data)
{
   uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
                                               ht->rehash_magic);
   uint32_t hash_address = start_hash_address;
   do {
      struct hash_entry *entry = ht->table + hash_address;

      if (likely(!entry->present)) {
         entry->hash = hash;
         entry->key = key;
         entry->data = data;
         entry->present = true;
         return;
      }

      hash_address += double_hash;
      if (hash_address >= size)
         hash_address -= size;
   } while (true);
}

static void
_mesa_hash_table_rehash(struct hash_table *ht, unsigned new_size_index)
{
   struct hash_table old_ht;
   struct hash_entry *table;

   if (ht->size_index == new_size_index && ht->deleted_entries == ht->max_entries) {
      hash_table_clear_fast(ht);
      assert(!ht->entries);
      return;
   }

   if (new_size_index >= ARRAY_SIZE(hash_sizes))
      return;

   table = rzalloc_array(ht->mem_ctx, struct hash_entry,
                         hash_sizes[new_size_index].size);
   if (table == NULL)
      return;

   if (ht->table_destructor)
      ralloc_set_destructor(table, _mesa_hash_table_ralloc_table_destructor);

   if (ht->table == ht->_initial_storage) {
      /* Copy the whole structure including the initial storage. */
      old_ht = *ht;
      old_ht.table = old_ht._initial_storage;
   } else {
      /* Copy everything except the initial storage. */
      memcpy(&old_ht, ht, offsetof(struct hash_table, _initial_storage));
   }

   ht->table = table;
   ht->size_index = new_size_index;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->size_magic = hash_sizes[ht->size_index].size_magic;
   ht->rehash_magic = hash_sizes[ht->size_index].rehash_magic;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->entries = 0;
   ht->deleted_entries = 0;

   hash_table_foreach(&old_ht, entry) {
      hash_table_insert_rehash(ht, entry->hash, entry->key, entry->data);
   }

   ht->entries = old_ht.entries;

   if (old_ht.table != old_ht._initial_storage) {
      if (ht->table_destructor)
         ralloc_set_destructor(old_ht.table, NULL);
      ralloc_free(old_ht.table);
   }
}

static struct hash_entry *
hash_table_get_entry(struct hash_table *ht, uint32_t hash, const void *key)
{
   struct hash_entry *available_entry = NULL;

   if (ht->entries >= ht->max_entries) {
      _mesa_hash_table_rehash(ht, ht->size_index + 1);
   } else if (ht->deleted_entries + ht->entries >= ht->max_entries) {
      _mesa_hash_table_rehash(ht, ht->size_index);
   }

   uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
                                               ht->rehash_magic);
   uint32_t hash_address = start_hash_address;
   do {
      struct hash_entry *entry = ht->table + hash_address;

      if (!entry->present) {
         /* Stash the first available entry we find */
         if (available_entry == NULL)
            available_entry = entry;
         if (!entry->deleted)
            break;
      }

      /* Implement replacement when another insert happens
       * with a matching key.  This is a relatively common
       * feature of hash tables, with the alternative
       * generally being "insert the new value as well, and
       * return it first when the key is searched for".
       *
       * Note that the hash table doesn't have a delete
       * callback.  If freeing of old data pointers is
       * required to avoid memory leaks, perform a search
       * before inserting.
       */
      if (entry->present && entry->hash == hash &&
          ht->key_equals_function(key, entry->key))
         return entry;

      hash_address += double_hash;
      if (hash_address >= size)
         hash_address -= size;
   } while (hash_address != start_hash_address);

   if (available_entry) {
      if (available_entry->deleted)
         ht->deleted_entries--;
      available_entry->hash = hash;
      ht->entries++;
      return available_entry;
   }

   /* We could hit here if a required resize failed. An unchecked-malloc
    * application could ignore this result.
    */
   return NULL;
}

static struct hash_entry *
hash_table_insert(struct hash_table *ht, uint32_t hash,
                  const void *key, void *data)
{
   struct hash_entry *entry = hash_table_get_entry(ht, hash, key);

   if (entry) {
      entry->key = key;
      entry->data = data;
      entry->present = true;
      entry->deleted = false;
   }

   return entry;
}

/**
 * Inserts the key with the given hash into the table.
 *
 * Note that insertion may rearrange the table on a resize or rehash,
 * so previously found hash_entries are no longer valid after this function.
 */
struct hash_entry *
_mesa_hash_table_insert(struct hash_table *ht, const void *key, void *data)
{
   assert(ht->key_hash_function);
   return hash_table_insert(ht, ht->key_hash_function(key), key, data);
}

struct hash_entry *
_mesa_hash_table_insert_pre_hashed(struct hash_table *ht, uint32_t hash,
                                   const void *key, void *data)
{
   assert(ht->key_hash_function == NULL || hash == ht->key_hash_function(key));
   return hash_table_insert(ht, hash, key, data);
}

/**
 * This function deletes the given hash table entry.
 *
 * Note that deletion doesn't otherwise modify the table, so an iteration over
 * the table deleting entries is safe.
 */
void
_mesa_hash_table_remove(struct hash_table *ht,
                        struct hash_entry *entry)
{
   if (!entry)
      return;

   entry->present = false;
   entry->deleted = true;
   ht->entries--;
   ht->deleted_entries++;
}

/**
 * Removes the entry with the corresponding key, if exists.
 */
void _mesa_hash_table_remove_key(struct hash_table *ht,
                                 const void *key)
{
   _mesa_hash_table_remove(ht, _mesa_hash_table_search(ht, key));
}

/**
 * This function is an iterator over the hash_table when no deleted entries are present.
 *
 * Pass in NULL for the first entry, as in the start of a for loop.
 */
struct hash_entry *
_mesa_hash_table_next_entry_unsafe(const struct hash_table *ht, struct hash_entry *entry)
{
   assert(!ht->deleted_entries);
   if (!ht->entries)
      return NULL;
   if (entry == NULL)
      entry = ht->table;
   else
      entry = entry + 1;
   if (entry != ht->table + ht->size)
      return entry->present ? entry : _mesa_hash_table_next_entry_unsafe(ht, entry);

   return NULL;
}

/**
 * This function is an iterator over the hash table.
 *
 * Pass in NULL for the first entry, as in the start of a for loop.  Note that
 * an iteration over the table is O(table_size) not O(entries).
 */
struct hash_entry *
_mesa_hash_table_next_entry(struct hash_table *ht,
                            struct hash_entry *entry)
{
   if (entry == NULL)
      entry = ht->table;
   else
      entry = entry + 1;

   for (; entry != ht->table + ht->size; entry++) {
      if (entry->present)
         return entry;
   }

   return NULL;
}

/**
 * Returns a random entry from the hash table.
 *
 * This may be useful in implementing random replacement (as opposed
 * to just removing everything) in caches based on this hash table
 * implementation.  @predicate may be used to filter entries, or may
 * be set to NULL for no filtering.
 */
struct hash_entry *
_mesa_hash_table_random_entry(struct hash_table *ht,
                              bool (*predicate)(struct hash_entry *entry))
{
   struct hash_entry *entry;
   uint32_t i = rand() % ht->size;

   if (ht->entries == 0)
      return NULL;

   for (entry = ht->table + i; entry != ht->table + ht->size; entry++) {
      if (entry->present && (!predicate || predicate(entry))) {
         return entry;
      }
   }

   for (entry = ht->table; entry != ht->table + i; entry++) {
      if (entry->present && (!predicate || predicate(entry))) {
         return entry;
      }
   }

   return NULL;
}


uint32_t
_mesa_hash_data(const void *data, size_t size)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(data, size, 0);
}

uint32_t
_mesa_hash_data_with_seed(const void *data, size_t size, uint32_t seed)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(data, size, seed);
}

uint32_t
_mesa_hash_int(const void *key)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(key, sizeof(int), 0);
}

uint32_t
_mesa_hash_uint(const void *key)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(key, sizeof(unsigned), 0);
}

uint32_t
_mesa_hash_u32(const void *key)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(key, 4, 0);
}

uint32_t
_mesa_hash_u64(const void *key)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(key, 8, 0);
}

/** FNV-1a string hash implementation */
uint32_t
_mesa_hash_string(const void *_key)
{
   return _mesa_hash_string_with_length(_key, strlen((const char *)_key));
}

uint32_t
_mesa_hash_string_with_length(const void *_key, unsigned length)
{
   /* glsl-optimizer change: FNV-1a instead of xxHash */
   return hash_fnv1a(_key, length, 0);
}

uint32_t
_mesa_hash_pointer(const void *pointer)
{
   uintptr_t num = (uintptr_t) pointer;
   return (uint32_t) ((num >> 2) ^ (num >> 6) ^ (num >> 10) ^ (num >> 14));
}

bool
_mesa_key_int_equal(const void *a, const void *b)
{
   return *((const int *)a) == *((const int *)b);
}

bool
_mesa_key_uint_equal(const void *a, const void *b)
{

   return *((const unsigned *)a) == *((const unsigned *)b);
}

bool
_mesa_key_u32_equal(const void *a, const void *b)
{
   return *((const uint32_t *)a) == *((const uint32_t *)b);
}

bool
_mesa_key_u64_equal(const void *a, const void *b)
{
   return *((const uint64_t *)a) == *((const uint64_t *)b);
}

/**
 * String compare function for use as the comparison callback in
 * _mesa_hash_table_create() and _mesa_hash_table_init().
 */
bool
_mesa_key_string_equal(const void *a, const void *b)
{
   return strcmp(a, b) == 0;
}

bool
_mesa_key_pointer_equal(const void *a, const void *b)
{
   return a == b;
}

/**
 * It's preferred to use _mesa_pointer_hash_table_init instead of this to skip ralloc.
 * Helper to create a hash table with pointer keys.
 */
struct hash_table *
_mesa_pointer_hash_table_create(void *mem_ctx)
{
   return _mesa_hash_table_create(mem_ctx, _mesa_hash_pointer,
                                  _mesa_key_pointer_equal);
}

void
_mesa_pointer_hash_table_init(struct hash_table *ht, void *mem_ctx)
{
   _mesa_hash_table_init(ht, mem_ctx, _mesa_hash_pointer,
                         _mesa_key_pointer_equal);
}

/* It's preferred to use _mesa_string_hash_table_init instead of this to skip ralloc. */
struct hash_table *
_mesa_string_hash_table_create(void *mem_ctx)
{
   return _mesa_hash_table_create(mem_ctx, _mesa_hash_string,
                                  _mesa_key_string_equal);
}

void
_mesa_string_hash_table_init(struct hash_table *ht, void *mem_ctx)
{
   _mesa_hash_table_init(ht, mem_ctx, _mesa_hash_string,
                         _mesa_key_string_equal);
}

bool
_mesa_hash_table_reserve(struct hash_table *ht, unsigned size)
{
   if (size < ht->max_entries)
      return true;
   for (unsigned i = ht->size_index + 1; i < ARRAY_SIZE(hash_sizes); i++) {
      if (hash_sizes[i].max_entries >= size) {
         _mesa_hash_table_rehash(ht, i);
         break;
      }
   }
   return ht->max_entries >= size;
}

/**
 * Hash table wrapper which supports 64-bit keys.
 *
 * TODO: unify all hash table implementations.
 */

struct hash_key_u64 {
   uint64_t value;
};

static uint32_t
key_u64_hash(const void *key)
{
   return _mesa_hash_data(key, sizeof(struct hash_key_u64));
}

static bool
key_u64_equals(const void *a, const void *b)
{
   const struct hash_key_u64 *aa = a;
   const struct hash_key_u64 *bb = b;

   return aa->value == bb->value;
}

static void _mesa_hash_table_u64_delete_keys(void *data)
{
   struct hash_table_u64 *ht = (struct hash_table_u64*)(data);

   _mesa_hash_table_u64_clear(ht);
}

struct hash_table_u64 *
_mesa_hash_table_u64_create(void *mem_ctx)
{
   struct hash_table_u64 *ht;

   ht = rzalloc(mem_ctx, struct hash_table_u64);
   if (!ht)
      return NULL;

   if (sizeof(void *) == 8) {
      _mesa_hash_table_init(&ht->table, ht, _mesa_hash_pointer,
                            _mesa_key_pointer_equal);
   } else {
      _mesa_hash_table_init(&ht->table, ht, key_u64_hash, key_u64_equals);
      /* hash_table takes care of placing the correct ralloc callback(s) and calling this
       * callback early enough to deallocate the hash_key_u64's before hash_table.table */
      _mesa_hash_table_set_destructor(&ht->table, _mesa_hash_table_u64_delete_keys);
   }

   return ht;
}

static void
_mesa_hash_table_u64_delete_key(struct hash_entry *entry)
{
   if (sizeof(void *) == 8)
      return;

   struct hash_key_u64 *_key = (struct hash_key_u64 *)entry->key;

   FREE(_key);
}

void
_mesa_hash_table_u64_clear(struct hash_table_u64 *ht)
{
   if (!ht)
      return;

   _mesa_hash_table_clear(&ht->table, _mesa_hash_table_u64_delete_key);
}

void
_mesa_hash_table_u64_destroy(struct hash_table_u64 *ht)
{
   ralloc_free(ht);
}

void
_mesa_hash_table_u64_insert(struct hash_table_u64 *ht, uint64_t key,
                            void *data)
{
   if (sizeof(void *) == 8) {
      _mesa_hash_table_insert(&ht->table, (void *)(uintptr_t)key, data);
   } else {
      struct hash_key_u64 *_key = CALLOC_STRUCT(hash_key_u64);

      if (!_key)
         return;
      _key->value = key;

      struct hash_entry *entry =
         hash_table_get_entry(&ht->table, key_u64_hash(_key), _key);

      if (!entry) {
         FREE(_key);
         return;
      }

      entry->data = data;
      if (!entry->present) {
         entry->key = _key;
         entry->present = true;
         entry->deleted = false;
      } else {
         FREE(_key);
      }
   }
}

static struct hash_entry *
hash_table_u64_search(struct hash_table_u64 *ht, uint64_t key)
{
   if (sizeof(void *) == 8) {
      return _mesa_hash_table_search(&ht->table, (void *)(uintptr_t)key);
   } else {
      struct hash_key_u64 _key = { .value = key };
      return _mesa_hash_table_search(&ht->table, &_key);
   }
}

void *
_mesa_hash_table_u64_search(struct hash_table_u64 *ht, uint64_t key)
{
   struct hash_entry *entry = hash_table_u64_search(ht, key);
   if (!entry)
      return NULL;

   return entry->data;
}

void
_mesa_hash_table_u64_remove(struct hash_table_u64 *ht, uint64_t key)
{
   struct hash_entry *entry = hash_table_u64_search(ht, key);
   if (!entry)
      return;

   if (sizeof(void *) == 8) {
      _mesa_hash_table_remove(&ht->table, entry);
   } else {
      struct hash_key *_key = (struct hash_key *)entry->key;

      _mesa_hash_table_remove(&ht->table, entry);
      FREE(_key);
   }
}


/*
 * Iterates in order ("freed key", "deleted key", regular entries...)
 */
struct hash_entry_u64
_mesa_hash_table_u64_next_entry(struct hash_table_u64 *ht,
                                struct hash_entry_u64 *ent)
{
   struct hash_entry *next =
      _mesa_hash_table_next_entry(&ht->table, ent ? ent->_entry : NULL);

   if (!next)
      return (struct hash_entry_u64){.data = NULL};

   uint64_t key;
   if (sizeof(void *) == 8) {
      key = (uintptr_t)next->key;
   } else {
      const struct hash_key_u64 *_key = next->key;
      key = _key->value;
   }

   return (struct hash_entry_u64){
      .key = key,
      .data = next->data,
      ._entry = next,
   };
}

/* Updates the data of a u64 hash_table entry inside a
 * hash_table_u64_foreach() loop
 */
void
_mesa_hash_table_u64_replace(struct hash_table_u64 *ht,
                             const struct hash_entry_u64 *ent,
                             void *new_data)
{
   if (ent->_entry)
      ent->_entry->data = new_data;
}
