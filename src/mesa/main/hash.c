/* $Id: hash.c,v 1.3 1999/10/13 18:42:50 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include "GL/xf86glx.h"
#endif
#include "hash.h"
#include "macros.h"
#endif


/*
 * Generic hash table.  Only dependency is the GLuint datatype.
 *
 * This is used to implement display list and texture object lookup.
 * NOTE: key=0 is illegal.
 */


#define TABLE_SIZE 1024

struct HashEntry {
   GLuint Key;
   void *Data;
   struct HashEntry *Next;
};

struct HashTable {
   struct HashEntry *Table[TABLE_SIZE];
   GLuint MaxKey;
};



/*
 * Return pointer to a new, empty hash table.
 */
struct HashTable *NewHashTable(void)
{
   return CALLOC_STRUCT(HashTable);
}



/*
 * Delete a hash table.
 */
void DeleteHashTable(struct HashTable *table)
{
   GLuint i;
   assert(table);
   for (i=0;i<TABLE_SIZE;i++) {
      struct HashEntry *entry = table->Table[i];
      while (entry) {
	 struct HashEntry *next = entry->Next;
	 FREE(entry);
	 entry = next;
      }
   }
   FREE(table);
}



/*
 * Lookup an entry in the hash table.
 * Input:  table - the hash table
 *         key - the key
 * Return:  user data pointer or NULL if key not in table
 */
void *HashLookup(const struct HashTable *table, GLuint key)
{
   GLuint pos;
   const struct HashEntry *entry;

   assert(table);
   assert(key);

   pos = key & (TABLE_SIZE-1);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
	 return entry->Data;
      }
      entry = entry->Next;
   }
   return NULL;
}



/*
 * Insert into the hash table.  If an entry with this key already exists
 * we'll replace the existing entry.
 * Input:  table - the hash table
 *         key - the key (not zero)
 *         data - pointer to user data
 */
void HashInsert(struct HashTable *table, GLuint key, void *data)
{
   /* search for existing entry with this key */
   GLuint pos;
   struct HashEntry *entry;

   assert(table);
   assert(key);

   if (key > table->MaxKey)
      table->MaxKey = key;

   pos = key & (TABLE_SIZE-1);
   entry = table->Table[pos];
   while (entry) {
      if (entry->Key == key) {
         /* replace entry's data */
	 entry->Data = data;
	 return;
      }
      entry = entry->Next;
   }

   /* alloc and insert new table entry */
   entry = MALLOC_STRUCT(HashEntry);
   entry->Key = key;
   entry->Data = data;
   entry->Next = table->Table[pos];
   table->Table[pos] = entry;
}



/*
 * Remove an entry from the hash table.
 * Input:  table - the hash table
 *         key - key of entry to remove
 */
void HashRemove(struct HashTable *table, GLuint key)
{
   GLuint pos;
   struct HashEntry *entry, *prev;

   assert(table);
   assert(key);

   pos = key & (TABLE_SIZE-1);
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
         FREE(entry);
	 return;
      }
      prev = entry;
      entry = entry->Next;
   }
}



/*
 * Return the key of the "first" entry in the hash table.
 * By calling this function until zero is returned we can get
 * the keys of all entries in the table.
 */
GLuint HashFirstEntry(const struct HashTable *table)
{
   GLuint pos;
   assert(table);
   for (pos=0; pos < TABLE_SIZE; pos++) {
      if (table->Table[pos])
         return table->Table[pos]->Key;
   }
   return 0;
}



/*
 * Dump contents of hash table for debugging.
 */
void HashPrint(const struct HashTable *table)
{
   GLuint i;
   assert(table);
   for (i=0;i<TABLE_SIZE;i++) {
      const struct HashEntry *entry = table->Table[i];
      while (entry) {
	 printf("%u %p\n", entry->Key, entry->Data);
	 entry = entry->Next;
      }
   }
}



/*
 * Find a block of 'numKeys' adjacent unused hash keys.
 * Input:  table - the hash table
 *         numKeys - number of keys needed
 * Return:  startint key of free block or 0 if failure
 */
GLuint HashFindFreeKeyBlock(const struct HashTable *table, GLuint numKeys)
{
   GLuint maxKey = ~((GLuint) 0);
   if (maxKey - numKeys > table->MaxKey) {
      /* the quick solution */
      return table->MaxKey + 1;
   }
   else {
      /* the slow solution */
      GLuint freeCount = 0;
      GLuint freeStart = 0;
      GLuint key;
      for (key=0; key!=maxKey; key++) {
	 if (HashLookup(table, key)) {
	    /* darn, this key is already in use */
	    freeCount = 0;
	    freeStart = key+1;
	 }
	 else {
	    /* this key not in use, check if we've found enough */
	    freeCount++;
	    if (freeCount == numKeys) {
	       return freeStart;
	    }
	 }
      }
      /* cannot allocate a block of numKeys consecutive keys */
      return 0;
   }
}



#ifdef HASH_TEST_HARNESS
int main(int argc, char *argv[])
{
   int a, b, c;
   struct HashTable *t;

   printf("&a = %p\n", &a);
   printf("&b = %p\n", &b);

   t = NewHashTable();
   HashInsert(t, 501, &a);
   HashInsert(t, 10, &c);
   HashInsert(t, 0xfffffff8, &b);
   HashPrint(t);
   printf("Find 501: %p\n", HashLookup(t,501));
   printf("Find 1313: %p\n", HashLookup(t,1313));
   printf("Find block of 100: %d\n", HashFindFreeKeyBlock(t, 100));
   DeleteHashTable(t);

   return 0;
}
#endif
