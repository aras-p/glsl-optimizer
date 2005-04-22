/**
 * \file hash.c
 * Generic hash table. 
 *
 * This code taken from Mesa and adapted.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "eglhash.h"


#define TABLE_SIZE 1023  /**< Size of lookup table/array */

#define HASH_FUNC(K)  ((K) % TABLE_SIZE)


/*
 * Unfinished mutex stuff
 */

typedef int _EGLMutex;

static void
_eglInitMutex(_EGLMutex m)
{
}

static void
_eglDestroyMutex(_EGLMutex m)
{
}

static void
_eglLockMutex(_EGLMutex m)
{
}

static void
_eglUnlockMutex(_EGLMutex m)
{
}



typedef struct _egl_hashentry _EGLHashentry;

struct _egl_hashentry
{
   EGLuint Key;            /**< the entry's key */
   void *Data;             /**< the entry's data */
   _EGLHashentry *Next;    /**< pointer to next entry */
};


struct _egl_hashtable
{
   _EGLHashentry *Table[TABLE_SIZE];  /**< the lookup table */
   EGLuint MaxKey;                    /**< highest key inserted so far */
   _EGLMutex Mutex;                   /**< mutual exclusion lock */
};


/**
 * Create a new hash table.
 * 
 * \return pointer to a new, empty hash table.
 */
_EGLHashtable *
_eglNewHashTable(void)
{
   _EGLHashtable *table = (_EGLHashtable *) calloc(1, sizeof(_EGLHashtable));
   if (table) {
      _eglInitMutex(table->Mutex);
      table->MaxKey = 1;
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
_eglDeleteHashTable(_EGLHashtable *table)
{
   EGLuint i;
   assert(table);
   for (i = 0; i < TABLE_SIZE; i++) {
      _EGLHashentry *entry = table->Table[i];
      while (entry) {
	 _EGLHashentry *next = entry->Next;
	 free(entry);
	 entry = next;
      }
   }
   _eglDestroyMutex(table->Mutex);
   free(table);
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
_eglHashLookup(const _EGLHashtable *table, EGLuint key)
{
   EGLuint pos;
   const _EGLHashentry *entry;

   assert(table);

   if (!key)
      return NULL;

   pos = HASH_FUNC(key);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
	 return entry->Data;
      }
      entry = entry->Next;
   }
   return NULL;
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
_eglHashInsert(_EGLHashtable *table, EGLuint key, void *data)
{
   /* search for existing entry with this key */
   EGLuint pos;
   _EGLHashentry *entry;

   assert(table);
   assert(key);

   _eglLockMutex(table->Mutex);

   if (key > table->MaxKey)
      table->MaxKey = key;

   pos = HASH_FUNC(key);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         /* replace entry's data */
	 entry->Data = data;
         _eglUnlockMutex(table->Mutex);
	 return;
      }
      entry = entry->Next;
   }

   /* alloc and insert new table entry */
   entry = (_EGLHashentry *) malloc(sizeof(_EGLHashentry));
   entry->Key = key;
   entry->Data = data;
   entry->Next = table->Table[pos];
   table->Table[pos] = entry;

   _eglUnlockMutex(table->Mutex);
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
_eglHashRemove(_EGLHashtable *table, EGLuint key)
{
   EGLuint pos;
   _EGLHashentry *entry, *prev;

   assert(table);
   assert(key);

   _eglLockMutex(table->Mutex);

   pos = HASH_FUNC(key);
   prev = NULL;
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         /* found it! */
         if (prev) {
            prev->Next = entry->Next;
         }
         else {
            table->Table[pos] = entry->Next;
         }
         free(entry);
         _eglUnlockMutex(table->Mutex);
	 return;
      }
      prev = entry;
      entry = entry->Next;
   }

   _eglUnlockMutex(table->Mutex);
}



/**
 * Get the key of the "first" entry in the hash table.
 * 
 * This is used in the course of deleting all display lists when
 * a context is destroyed.
 * 
 * \param table the hash table
 * 
 * \return key for the "first" entry in the hash table.
 *
 * While holding the lock, walks through all table positions until finding
 * the first entry of the first non-empty one.
 */
EGLuint
_eglHashFirstEntry(_EGLHashtable *table)
{
   EGLuint pos;
   assert(table);
   _eglLockMutex(table->Mutex);
   for (pos = 0; pos < TABLE_SIZE; pos++) {
      if (table->Table[pos]) {
         _eglUnlockMutex(table->Mutex);
         return table->Table[pos]->Key;
      }
   }
   _eglUnlockMutex(table->Mutex);
   return 0;
}


/**
 * Given a hash table key, return the next key.  This is used to walk
 * over all entries in the table.  Note that the keys returned during
 * walking won't be in any particular order.
 * \return next hash key or 0 if end of table.
 */
EGLuint
_eglHashNextEntry(const _EGLHashtable *table, EGLuint key)
{
   const _EGLHashentry *entry;
   EGLuint pos;

   assert(table);
   assert(key);

   /* Find the entry with given key */
   pos = HASH_FUNC(key);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         break;
      }
      entry = entry->Next;
   }

   if (!entry) {
      /* the key was not found, we can't find next entry */
      return 0;
   }

   if (entry->Next) {
      /* return next in linked list */
      return entry->Next->Key;
   }
   else {
      /* look for next non-empty table slot */
      pos++;
      while (pos < TABLE_SIZE) {
         if (table->Table[pos]) {
            return table->Table[pos]->Key;
         }
         pos++;
      }
      return 0;
   }
}


/**
 * Dump contents of hash table for debugging.
 * 
 * \param table the hash table.
 */
void
_eglHashPrint(const _EGLHashtable *table)
{
   EGLuint i;
   assert(table);
   for (i = 0; i < TABLE_SIZE; i++) {
      const _EGLHashentry *entry = table->Table[i];
      while (entry) {
	 printf("%u %p\n", entry->Key, entry->Data);
	 entry = entry->Next;
      }
   }
}



/**
 * Return a new, unused hash key.
 */
EGLuint
_eglHashGenKey(_EGLHashtable *table)
{
   EGLuint k;

   _eglLockMutex(table->Mutex);
   k = table->MaxKey;
   table->MaxKey++;
   _eglUnlockMutex(table->Mutex);
   return k;
}

