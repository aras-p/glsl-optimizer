/**
 * \file hash.c
 * Generic hash table. 
 *
 * Used for display lists, texture objects, vertex/fragment programs,
 * buffer objects, etc.  The hash functions are thread-safe.
 * 
 * \note key=0 is illegal.
 *
 * \author Brian Paul
 */

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "imports.h"
#include "hash.h"
#include "hash_table.h"

/**
 * Magic GLuint object name that gets stored outside of the struct hash_table.
 *
 * The hash table needs a particular pointer to be the marker for a key that
 * was deleted from the table, along with NULL for the "never allocated in the
 * table" marker.  Legacy GL allows any GLuint to be used as a GL object name,
 * and we use a 1:1 mapping from GLuints to key pointers, so we need to be
 * able to track a GLuint that happens to match the deleted key outside of
 * struct hash_table.  We tell the hash table to use "1" as the deleted key
 * value, so that we test the deleted-key-in-the-table path as best we can.
 */
#define DELETED_KEY_VALUE 1

/**
 * The hash table data structure.  
 */
struct _mesa_HashTable {
   struct hash_table *ht;
   GLuint MaxKey;                        /**< highest key inserted so far */
   mtx_t Mutex;                /**< mutual exclusion lock */
   mtx_t WalkMutex;            /**< for _mesa_HashWalk() */
   GLboolean InDeleteAll;                /**< Debug check */
   /** Value that would be in the table for DELETED_KEY_VALUE. */
   void *deleted_key_data;
};

/** @{
 * Mapping from our use of GLuint as both the key and the hash value to the
 * hash_table.h API
 *
 * There exist many integer hash functions, designed to avoid collisions when
 * the integers are spread across key space with some patterns.  In GL, the
 * pattern (in the case of glGen*()ed object IDs) is that the keys are unique
 * contiguous integers starting from 1.  Because of that, we just use the key
 * as the hash value, to minimize the cost of the hash function.  If objects
 * are never deleted, we will never see a collision in the table, because the
 * table resizes itself when it approaches full, and thus key % table_size ==
 * key.
 *
 * The case where we could have collisions for genned objects would be
 * something like: glGenBuffers(&a, 100); glDeleteBuffers(&a + 50, 50);
 * glGenBuffers(&b, 100), because objects 1-50 and 101-200 are allocated at
 * the end of that sequence, instead of 1-150.  So far it doesn't appear to be
 * a problem.
 */
static bool
uint_key_compare(const void *a, const void *b)
{
   return a == b;
}

static uint32_t
uint_hash(GLuint id)
{
   return id;
}

static void *
uint_key(GLuint id)
{
   return (void *)(uintptr_t) id;
}
/** @} */

/**
 * Create a new hash table.
 * 
 * \return pointer to a new, empty hash table.
 */
struct _mesa_HashTable *
_mesa_NewHashTable(void)
{
   struct _mesa_HashTable *table = CALLOC_STRUCT(_mesa_HashTable);

   if (table) {
      table->ht = _mesa_hash_table_create(NULL, uint_key_compare);
      _mesa_hash_table_set_deleted_key(table->ht, uint_key(DELETED_KEY_VALUE));
      mtx_init(&table->Mutex, mtx_plain);
      mtx_init(&table->WalkMutex, mtx_plain);
   }
   return table;
}



/**
 * Delete a hash table.
 * Frees each entry on the hash table and then the hash table structure itself.
 * Note that the caller should have already traversed the table and deleted
 * the objects in the table (i.e. We don't free the entries' data pointer).
 *
 * \param table the hash table to delete.
 */
void
_mesa_DeleteHashTable(struct _mesa_HashTable *table)
{
   assert(table);

   if (_mesa_hash_table_next_entry(table->ht, NULL) != NULL) {
      _mesa_problem(NULL, "In _mesa_DeleteHashTable, found non-freed data");
   }

   _mesa_hash_table_destroy(table->ht, NULL);

   mtx_destroy(&table->Mutex);
   mtx_destroy(&table->WalkMutex);
   free(table);
}



/**
 * Lookup an entry in the hash table, without locking.
 * \sa _mesa_HashLookup
 */
static inline void *
_mesa_HashLookup_unlocked(struct _mesa_HashTable *table, GLuint key)
{
   const struct hash_entry *entry;

   assert(table);
   assert(key);

   if (key == DELETED_KEY_VALUE)
      return table->deleted_key_data;

   entry = _mesa_hash_table_search(table->ht, uint_hash(key), uint_key(key));
   if (!entry)
      return NULL;

   return entry->data;
}


/**
 * Lookup an entry in the hash table.
 * 
 * \param table the hash table.
 * \param key the key.
 * 
 * \return pointer to user's data or NULL if key not in table
 */
void *
_mesa_HashLookup(struct _mesa_HashTable *table, GLuint key)
{
   void *res;
   assert(table);
   mtx_lock(&table->Mutex);
   res = _mesa_HashLookup_unlocked(table, key);
   mtx_unlock(&table->Mutex);
   return res;
}


/**
 * Lookup an entry in the hash table without locking the mutex.
 *
 * The hash table mutex must be locked manually by calling
 * _mesa_HashLockMutex() before calling this function.
 *
 * \param table the hash table.
 * \param key the key.
 *
 * \return pointer to user's data or NULL if key not in table
 */
void *
_mesa_HashLookupLocked(struct _mesa_HashTable *table, GLuint key)
{
   return _mesa_HashLookup_unlocked(table, key);
}


/**
 * Lock the hash table mutex.
 *
 * This function should be used when multiple objects need
 * to be looked up in the hash table, to avoid having to lock
 * and unlock the mutex each time.
 *
 * \param table the hash table.
 */
void
_mesa_HashLockMutex(struct _mesa_HashTable *table)
{
   assert(table);
   mtx_lock(&table->Mutex);
}


/**
 * Unlock the hash table mutex.
 *
 * \param table the hash table.
 */
void
_mesa_HashUnlockMutex(struct _mesa_HashTable *table)
{
   assert(table);
   mtx_unlock(&table->Mutex);
}


static inline void
_mesa_HashInsert_unlocked(struct _mesa_HashTable *table, GLuint key, void *data)
{
   uint32_t hash = uint_hash(key);
   struct hash_entry *entry;

   assert(table);
   assert(key);

   if (key > table->MaxKey)
      table->MaxKey = key;

   if (key == DELETED_KEY_VALUE) {
      table->deleted_key_data = data;
   } else {
      entry = _mesa_hash_table_search(table->ht, hash, uint_key(key));
      if (entry) {
         entry->data = data;
      } else {
         _mesa_hash_table_insert(table->ht, hash, uint_key(key), data);
      }
   }
}


/**
 * Insert a key/pointer pair into the hash table without locking the mutex.
 * If an entry with this key already exists we'll replace the existing entry.
 *
 * The hash table mutex must be locked manually by calling
 * _mesa_HashLockMutex() before calling this function.
 *
 * \param table the hash table.
 * \param key the key (not zero).
 * \param data pointer to user data.
 */
void
_mesa_HashInsertLocked(struct _mesa_HashTable *table, GLuint key, void *data)
{
   _mesa_HashInsert_unlocked(table, key, data);
}


/**
 * Insert a key/pointer pair into the hash table.
 * If an entry with this key already exists we'll replace the existing entry.
 *
 * \param table the hash table.
 * \param key the key (not zero).
 * \param data pointer to user data.
 */
void
_mesa_HashInsert(struct _mesa_HashTable *table, GLuint key, void *data)
{
   assert(table);
   mtx_lock(&table->Mutex);
   _mesa_HashInsert_unlocked(table, key, data);
   mtx_unlock(&table->Mutex);
}


/**
 * Remove an entry from the hash table.
 * 
 * \param table the hash table.
 * \param key key of entry to remove.
 *
 * While holding the hash table's lock, searches the entry with the matching
 * key and unlinks it.
 */
void
_mesa_HashRemove(struct _mesa_HashTable *table, GLuint key)
{
   struct hash_entry *entry;

   assert(table);
   assert(key);

   /* have to check this outside of mutex lock */
   if (table->InDeleteAll) {
      _mesa_problem(NULL, "_mesa_HashRemove illegally called from "
                    "_mesa_HashDeleteAll callback function");
      return;
   }

   mtx_lock(&table->Mutex);
   if (key == DELETED_KEY_VALUE) {
      table->deleted_key_data = NULL;
   } else {
      entry = _mesa_hash_table_search(table->ht, uint_hash(key), uint_key(key));
      _mesa_hash_table_remove(table->ht, entry);
   }
   mtx_unlock(&table->Mutex);
}



/**
 * Delete all entries in a hash table, but don't delete the table itself.
 * Invoke the given callback function for each table entry.
 *
 * \param table  the hash table to delete
 * \param callback  the callback function
 * \param userData  arbitrary pointer to pass along to the callback
 *                  (this is typically a struct gl_context pointer)
 */
void
_mesa_HashDeleteAll(struct _mesa_HashTable *table,
                    void (*callback)(GLuint key, void *data, void *userData),
                    void *userData)
{
   struct hash_entry *entry;

   ASSERT(table);
   ASSERT(callback);
   mtx_lock(&table->Mutex);
   table->InDeleteAll = GL_TRUE;
   hash_table_foreach(table->ht, entry) {
      callback((uintptr_t)entry->key, entry->data, userData);
      _mesa_hash_table_remove(table->ht, entry);
   }
   if (table->deleted_key_data) {
      callback(DELETED_KEY_VALUE, table->deleted_key_data, userData);
      table->deleted_key_data = NULL;
   }
   table->InDeleteAll = GL_FALSE;
   mtx_unlock(&table->Mutex);
}


/**
 * Clone all entries in a hash table, into a new table.
 *
 * \param table  the hash table to clone
 */
struct _mesa_HashTable *
_mesa_HashClone(const struct _mesa_HashTable *table)
{
   /* cast-away const */
   struct _mesa_HashTable *table2 = (struct _mesa_HashTable *) table;
   struct hash_entry *entry;
   struct _mesa_HashTable *clonetable;

   ASSERT(table);
   mtx_lock(&table2->Mutex);

   clonetable = _mesa_NewHashTable();
   assert(clonetable);
   hash_table_foreach(table->ht, entry) {
      _mesa_HashInsert(clonetable, (GLint)(uintptr_t)entry->key, entry->data);
   }

   mtx_unlock(&table2->Mutex);

   return clonetable;
}


/**
 * Walk over all entries in a hash table, calling callback function for each.
 * Note: we use a separate mutex in this function to avoid a recursive
 * locking deadlock (in case the callback calls _mesa_HashRemove()) and to
 * prevent multiple threads/contexts from getting tangled up.
 * A lock-less version of this function could be used when the table will
 * not be modified.
 * \param table  the hash table to walk
 * \param callback  the callback function
 * \param userData  arbitrary pointer to pass along to the callback
 *                  (this is typically a struct gl_context pointer)
 */
void
_mesa_HashWalk(const struct _mesa_HashTable *table,
               void (*callback)(GLuint key, void *data, void *userData),
               void *userData)
{
   /* cast-away const */
   struct _mesa_HashTable *table2 = (struct _mesa_HashTable *) table;
   struct hash_entry *entry;

   ASSERT(table);
   ASSERT(callback);
   mtx_lock(&table2->WalkMutex);
   hash_table_foreach(table->ht, entry) {
      callback((uintptr_t)entry->key, entry->data, userData);
   }
   if (table->deleted_key_data)
      callback(DELETED_KEY_VALUE, table->deleted_key_data, userData);
   mtx_unlock(&table2->WalkMutex);
}

static void
debug_print_entry(GLuint key, void *data, void *userData)
{
   _mesa_debug(NULL, "%u %p\n", key, data);
}

/**
 * Dump contents of hash table for debugging.
 * 
 * \param table the hash table.
 */
void
_mesa_HashPrint(const struct _mesa_HashTable *table)
{
   if (table->deleted_key_data)
      debug_print_entry(DELETED_KEY_VALUE, table->deleted_key_data, NULL);
   _mesa_HashWalk(table, debug_print_entry, NULL);
}


/**
 * Find a block of adjacent unused hash keys.
 * 
 * \param table the hash table.
 * \param numKeys number of keys needed.
 * 
 * \return Starting key of free block or 0 if failure.
 *
 * If there are enough free keys between the maximum key existing in the table
 * (_mesa_HashTable::MaxKey) and the maximum key possible, then simply return
 * the adjacent key. Otherwise do a full search for a free key block in the
 * allowable key range.
 */
GLuint
_mesa_HashFindFreeKeyBlock(struct _mesa_HashTable *table, GLuint numKeys)
{
   const GLuint maxKey = ~((GLuint) 0) - 1;
   mtx_lock(&table->Mutex);
   if (maxKey - numKeys > table->MaxKey) {
      /* the quick solution */
      mtx_unlock(&table->Mutex);
      return table->MaxKey + 1;
   }
   else {
      /* the slow solution */
      GLuint freeCount = 0;
      GLuint freeStart = 1;
      GLuint key;
      for (key = 1; key != maxKey; key++) {
	 if (_mesa_HashLookup_unlocked(table, key)) {
	    /* darn, this key is already in use */
	    freeCount = 0;
	    freeStart = key+1;
	 }
	 else {
	    /* this key not in use, check if we've found enough */
	    freeCount++;
	    if (freeCount == numKeys) {
               mtx_unlock(&table->Mutex);
	       return freeStart;
	    }
	 }
      }
      /* cannot allocate a block of numKeys consecutive keys */
      mtx_unlock(&table->Mutex);
      return 0;
   }
}


/**
 * Return the number of entries in the hash table.
 */
GLuint
_mesa_HashNumEntries(const struct _mesa_HashTable *table)
{
   struct hash_entry *entry;
   GLuint count = 0;

   if (table->deleted_key_data)
      count++;

   hash_table_foreach(table->ht, entry)
      count++;

   return count;
}
