/**
 * \file eglhash.h
 * Generic hash table. 
 */


#ifndef EGLHASH_INCLUDED
#define EGLHASH_INCLUDED


/* XXX move this? */
typedef unsigned int EGLuint;


typedef struct _egl_hashtable _EGLHashtable;


extern _EGLHashtable *_eglNewHashTable(void);

extern void _eglDeleteHashTable(_EGLHashtable *table);

extern void *_eglHashLookup(const _EGLHashtable *table, EGLuint key);

extern void _eglHashInsert(_EGLHashtable *table, EGLuint key, void *data);

extern void _eglHashRemove(_EGLHashtable *table, EGLuint key);

extern EGLuint _eglHashFirstEntry(_EGLHashtable *table);

extern EGLuint _eglHashNextEntry(const _EGLHashtable *table, EGLuint key);

extern void _eglHashPrint(const _EGLHashtable *table);

extern EGLuint _eglHashGenKey(_EGLHashtable *table);

extern void _egltest_hash_functions(void);


#endif /* EGLHASH_INCLUDED */
