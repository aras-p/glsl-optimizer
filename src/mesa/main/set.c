/*
 * Copyright © 2009-2012 Intel Corporation
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

#include <stdlib.h>

#include "macros.h"
#include "set.h"
#include "util/ralloc.h"

/*
 * From Knuth -- a good choice for hash/rehash values is p, p-2 where
 * p and p-2 are both prime.  These tables are sized to have an extra 10%
 * free to avoid exponential performance degradation as the hash table fills
 */

uint32_t deleted_key_value;
const void *deleted_key = &deleted_key_value;

static const struct {
   uint32_t max_entries, size, rehash;
} hash_sizes[] = {
   { 2,            5,            3            },
   { 4,            7,            5            },
   { 8,            13,           11           },
   { 16,           19,           17           },
   { 32,           43,           41           },
   { 64,           73,           71           },
   { 128,          151,          149          },
   { 256,          283,          281          },
   { 512,          571,          569          },
   { 1024,         1153,         1151         },
   { 2048,         2269,         2267         },
   { 4096,         4519,         4517         },
   { 8192,         9013,         9011         },
   { 16384,        18043,        18041        },
   { 32768,        36109,        36107        },
   { 65536,        72091,        72089        },
   { 131072,       144409,       144407       },
   { 262144,       288361,       288359       },
   { 524288,       576883,       576881       },
   { 1048576,      1153459,      1153457      },
   { 2097152,      2307163,      2307161      },
   { 4194304,      4613893,      4613891      },
   { 8388608,      9227641,      9227639      },
   { 16777216,     18455029,     18455027     },
   { 33554432,     36911011,     36911009     },
   { 67108864,     73819861,     73819859     },
   { 134217728,    147639589,    147639587    },
   { 268435456,    295279081,    295279079    },
   { 536870912,    590559793,    590559791    },
   { 1073741824,   1181116273,   1181116271   },
   { 2147483648ul, 2362232233ul, 2362232231ul }
};

static int
entry_is_free(struct set_entry *entry)
{
   return entry->key == NULL;
}

static int
entry_is_deleted(struct set_entry *entry)
{
   return entry->key == deleted_key;
}

static int
entry_is_present(struct set_entry *entry)
{
   return entry->key != NULL && entry->key != deleted_key;
}

struct set *
_mesa_set_create(void *mem_ctx,
                 bool (*key_equals_function)(const void *a,
                                             const void *b))
{
   struct set *ht;

   ht = ralloc(mem_ctx, struct set);
   if (ht == NULL)
      return NULL;

   ht->size_index = 0;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->key_equals_function = key_equals_function;
   ht->table = rzalloc_array(ht, struct set_entry, ht->size);
   ht->entries = 0;
   ht->deleted_entries = 0;

   if (ht->table == NULL) {
      ralloc_free(ht);
      return NULL;
   }

   return ht;
}

/**
 * Frees the given set.
 *
 * If delete_function is passed, it gets called on each entry present before
 * freeing.
 */
void
_mesa_set_destroy(struct set *ht, void (*delete_function)(struct set_entry *entry))
{
   if (!ht)
      return;

   if (delete_function) {
      struct set_entry *entry;

      set_foreach (ht, entry) {
         delete_function(entry);
      }
   }
   ralloc_free(ht->table);
   ralloc_free(ht);
}

/**
 * Finds a set entry with the given key and hash of that key.
 *
 * Returns NULL if no entry is found.
 */
struct set_entry *
_mesa_set_search(const struct set *ht, uint32_t hash, const void *key)
{
   uint32_t hash_address;

   hash_address = hash % ht->size;
   do {
      uint32_t double_hash;

      struct set_entry *entry = ht->table + hash_address;

      if (entry_is_free(entry)) {
         return NULL;
      } else if (entry_is_present(entry) && entry->hash == hash) {
         if (ht->key_equals_function(key, entry->key)) {
            return entry;
         }
      }

      double_hash = 1 + hash % ht->rehash;

      hash_address = (hash_address + double_hash) % ht->size;
   } while (hash_address != hash % ht->size);

   return NULL;
}

static void
set_rehash(struct set *ht, int new_size_index)
{
   struct set old_ht;
   struct set_entry *table, *entry;

   if (new_size_index >= ARRAY_SIZE(hash_sizes))
      return;

   table = rzalloc_array(ht, struct set_entry,
                         hash_sizes[new_size_index].size);
   if (table == NULL)
      return;

   old_ht = *ht;

   ht->table = table;
   ht->size_index = new_size_index;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->entries = 0;
   ht->deleted_entries = 0;

   for (entry = old_ht.table;
        entry != old_ht.table + old_ht.size;
        entry++) {
      if (entry_is_present(entry)) {
         _mesa_set_add(ht, entry->hash, entry->key);
      }
   }

   ralloc_free(old_ht.table);
}

/**
 * Inserts the key with the given hash into the table.
 *
 * Note that insertion may rearrange the table on a resize or rehash,
 * so previously found hash_entries are no longer valid after this function.
 */
struct set_entry *
_mesa_set_add(struct set *ht, uint32_t hash, const void *key)
{
   uint32_t hash_address;

   if (ht->entries >= ht->max_entries) {
      set_rehash(ht, ht->size_index + 1);
   } else if (ht->deleted_entries + ht->entries >= ht->max_entries) {
      set_rehash(ht, ht->size_index);
   }

   hash_address = hash % ht->size;
   do {
      struct set_entry *entry = ht->table + hash_address;
      uint32_t double_hash;

      if (!entry_is_present(entry)) {
         if (entry_is_deleted(entry))
            ht->deleted_entries--;
         entry->hash = hash;
         entry->key = key;
         ht->entries++;
         return entry;
      }

      /* Implement replacement when another insert happens
       * with a matching key.  This is a relatively common
       * feature of hash tables, with the alternative
       * generally being "insert the new value as well, and
       * return it first when the key is searched for".
       *
       * Note that the hash table doesn't have a delete callback.
       * If freeing of old keys is required to avoid memory leaks,
       * perform a search before inserting.
       */
      if (entry->hash == hash &&
          ht->key_equals_function(key, entry->key)) {
         entry->key = key;
         return entry;
      }

      double_hash = 1 + hash % ht->rehash;

      hash_address = (hash_address + double_hash) % ht->size;
   } while (hash_address != hash % ht->size);

   /* We could hit here if a required resize failed. An unchecked-malloc
    * application could ignore this result.
    */
   return NULL;
}

/**
 * This function deletes the given hash table entry.
 *
 * Note that deletion doesn't otherwise modify the table, so an iteration over
 * the table deleting entries is safe.
 */
void
_mesa_set_remove(struct set *ht, struct set_entry *entry)
{
   if (!entry)
      return;

   entry->key = deleted_key;
   ht->entries--;
   ht->deleted_entries++;
}

/**
 * This function is an iterator over the hash table.
 *
 * Pass in NULL for the first entry, as in the start of a for loop.  Note that
 * an iteration over the table is O(table_size) not O(entries).
 */
struct set_entry *
_mesa_set_next_entry(const struct set *ht, struct set_entry *entry)
{
   if (entry == NULL)
      entry = ht->table;
   else
      entry = entry + 1;

   for (; entry != ht->table + ht->size; entry++) {
      if (entry_is_present(entry)) {
         return entry;
      }
   }

   return NULL;
}

struct set_entry *
_mesa_set_random_entry(struct set *ht,
                       int (*predicate)(struct set_entry *entry))
{
   struct set_entry *entry;
   uint32_t i = rand() % ht->size;

   if (ht->entries == 0)
      return NULL;

   for (entry = ht->table + i; entry != ht->table + ht->size; entry++) {
      if (entry_is_present(entry) &&
          (!predicate || predicate(entry))) {
         return entry;
      }
   }

   for (entry = ht->table; entry != ht->table + i; entry++) {
      if (entry_is_present(entry) &&
          (!predicate || predicate(entry))) {
         return entry;
      }
   }

   return NULL;
}

