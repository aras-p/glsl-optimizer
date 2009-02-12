/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_state.h"

#include "brw_wm.h"
#include "brw_vs.h"
#include "brw_clip.h"
#include "brw_sf.h"
#include "brw_gs.h"

#include "util/u_memory.h"



/***********************************************************************
 * Check cache for uploaded version of struct, else upload new one.
 * Fail when memory is exhausted.
 *
 * XXX: FIXME: Currently search is so slow it would be quicker to
 * regenerate the data every time...
 */

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

static struct brw_cache_item *search_cache( struct brw_cache *cache,
					     unsigned hash,
					     const void *key,
					     unsigned key_size)
{
   struct brw_cache_item *c;

   for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (c->hash == hash &&
	  c->key_size == key_size &&
	  memcmp(c->key, key, key_size) == 0)
	 return c;
   }

   return NULL;
}


static void rehash( struct brw_cache *cache )
{
   struct brw_cache_item **items;
   struct brw_cache_item *c, *next;
   unsigned size, i;

   size = cache->size * 3;
   items = (struct brw_cache_item**) MALLOC(size * sizeof(*items));
   memset(items, 0, size * sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 c->next = items[c->hash % size];
	 items[c->hash % size] = c;
      }

   FREE(cache->items);
   cache->items = items;
   cache->size = size;
}


boolean brw_search_cache( struct brw_cache *cache,
			    const void *key,
			    unsigned key_size,
			    void *aux_return,
			    unsigned *offset_return)
{
   struct brw_cache_item *item;
   unsigned addr = 0;
   unsigned hash = hash_key(key, key_size);

   item = search_cache(cache, hash, key, key_size);

   if (item) {
      if (aux_return)
	 *(void **)aux_return = (void *)((char *)item->key + item->key_size);

      *offset_return = addr = item->offset;
   }

   if (item == NULL || addr != cache->last_addr) {
      cache->brw->state.dirty.cache |= 1<<cache->id;
      cache->last_addr = addr;
   }

   return item != NULL;
}

unsigned brw_upload_cache( struct brw_cache *cache,
			 const void *key,
			 unsigned key_size,
			 const void *data,
			 unsigned data_size,
			 const void *aux,
			 void *aux_return )
{
   unsigned offset;
   struct brw_cache_item *item = CALLOC_STRUCT(brw_cache_item);
   unsigned hash = hash_key(key, key_size);
   void *tmp = MALLOC(key_size + cache->aux_size);

   if (!brw_pool_alloc(cache->pool, data_size, 1 << 6, &offset)) {
      /* Should not be possible:
       */
      debug_printf("brw_pool_alloc failed\n");
      exit(1);
   }

   memcpy(tmp, key, key_size);

   if (cache->aux_size)
      memcpy(tmp+key_size, aux, cache->aux_size);

   item->key = tmp;
   item->hash = hash;
   item->key_size = key_size;
   item->offset = offset;
   item->data_size = data_size;

   if (++cache->n_items > cache->size * 1.5)
      rehash(cache);

   hash %= cache->size;
   item->next = cache->items[hash];
   cache->items[hash] = item;

   if (aux_return) {
      assert(cache->aux_size);
      *(void **)aux_return = (void *)((char *)item->key + item->key_size);
   }

   if (BRW_DEBUG & DEBUG_STATE)
      debug_printf("upload %s: %d bytes to pool buffer %p offset %x\n",
             cache->name, 
	     data_size,
             (void*)cache->pool->buffer,
             offset);

   /* Copy data to the buffer:
    */
   cache->brw->winsys->buffer_subdata_typed(cache->brw->winsys,
					    cache->pool->buffer, 
					    offset, 
					    data_size, 
					    data,
					    cache->id);

   cache->brw->state.dirty.cache |= 1<<cache->id;
   cache->last_addr = offset;

   return offset;
}

/* This doesn't really work with aux data.  Use search/upload instead
 */
unsigned brw_cache_data_sz(struct brw_cache *cache,
			 const void *data,
			 unsigned data_size)
{
   unsigned addr;

   if (!brw_search_cache(cache, data, data_size, NULL, &addr)) {
      addr = brw_upload_cache(cache,
			      data, data_size,
			      data, data_size,
			      NULL, NULL);
   }

   return addr;
}

unsigned brw_cache_data(struct brw_cache *cache,
		      const void *data)
{
   return brw_cache_data_sz(cache, data, cache->key_size);
}

enum pool_type {
   DW_SURFACE_STATE,
   DW_GENERAL_STATE
};

static void brw_init_cache( struct brw_context *brw,
			    const char *name,
			    unsigned id,
			    unsigned key_size,
			    unsigned aux_size,
			    enum pool_type pool_type)
{
   struct brw_cache *cache = &brw->cache[id];
   cache->brw = brw;
   cache->id = id;
   cache->name = name;
   cache->items = NULL;

   cache->size = 7;
   cache->n_items = 0;
   cache->items = (struct brw_cache_item **)
      CALLOC(cache->size, sizeof(struct brw_cache_item));


   cache->key_size = key_size;
   cache->aux_size = aux_size;
   switch (pool_type) {
   case DW_GENERAL_STATE: cache->pool = &brw->pool[BRW_GS_POOL]; break;
   case DW_SURFACE_STATE: cache->pool = &brw->pool[BRW_SS_POOL]; break;
   default: assert(0); break;
   }
}

void brw_init_caches( struct brw_context *brw )
{

   brw_init_cache(brw,
		  "CC_VP",
		  BRW_CC_VP,
		  sizeof(struct brw_cc_viewport),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "CC_UNIT",
		  BRW_CC_UNIT,
		  sizeof(struct brw_cc_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "WM_PROG",
		  BRW_WM_PROG,
		  sizeof(struct brw_wm_prog_key),
		  sizeof(struct brw_wm_prog_data),
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SAMPLER_DEFAULT_COLOR",
		  BRW_SAMPLER_DEFAULT_COLOR,
		  sizeof(struct brw_sampler_default_color),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SAMPLER",
		  BRW_SAMPLER,
		  0,		/* variable key/data size */
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "WM_UNIT",
		  BRW_WM_UNIT,
		  sizeof(struct brw_wm_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SF_PROG",
		  BRW_SF_PROG,
		  sizeof(struct brw_sf_prog_key),
		  sizeof(struct brw_sf_prog_data),
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SF_VP",
		  BRW_SF_VP,
		  sizeof(struct brw_sf_viewport),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SF_UNIT",
		  BRW_SF_UNIT,
		  sizeof(struct brw_sf_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "VS_UNIT",
		  BRW_VS_UNIT,
		  sizeof(struct brw_vs_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "VS_PROG",
		  BRW_VS_PROG,
		  sizeof(struct brw_vs_prog_key),
		  sizeof(struct brw_vs_prog_data),
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "CLIP_UNIT",
		  BRW_CLIP_UNIT,
		  sizeof(struct brw_clip_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "CLIP_PROG",
		  BRW_CLIP_PROG,
		  sizeof(struct brw_clip_prog_key),
		  sizeof(struct brw_clip_prog_data),
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "GS_UNIT",
		  BRW_GS_UNIT,
		  sizeof(struct brw_gs_unit_state),
		  0,
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "GS_PROG",
		  BRW_GS_PROG,
		  sizeof(struct brw_gs_prog_key),
		  sizeof(struct brw_gs_prog_data),
		  DW_GENERAL_STATE);

   brw_init_cache(brw,
		  "SS_SURFACE",
		  BRW_SS_SURFACE,
		  sizeof(struct brw_surface_state),
		  0,
		  DW_SURFACE_STATE);

   brw_init_cache(brw,
		  "SS_SURF_BIND",
		  BRW_SS_SURF_BIND,
		  sizeof(struct brw_surface_binding_table),
		  0,
		  DW_SURFACE_STATE);
}


/* When we lose hardware context, need to invalidate the surface cache
 * as these structs must be explicitly re-uploaded.  They are subject
 * to fixup by the memory manager as they contain absolute agp
 * offsets, so we need to ensure there is a fresh version of the
 * struct available to receive the fixup.
 *
 * XXX: Need to ensure that there aren't two versions of a surface or
 * bufferobj with different backing data active in the same buffer at
 * once?  Otherwise the cache could confuse them.  Maybe better not to
 * cache at all?
 *
 * --> Isn't this the same as saying need to ensure batch is flushed
 *         before new data is uploaded to an existing buffer?  We
 *         already try to make sure of that.
 */
static void clear_cache( struct brw_cache *cache )
{
   struct brw_cache_item *c, *next;
   unsigned i;

   for (i = 0; i < cache->size; i++) {
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 free((void *)c->key);
	 free(c);
      }
      cache->items[i] = NULL;
   }

   cache->n_items = 0;
}

void brw_clear_all_caches( struct brw_context *brw )
{
   int i;

   if (BRW_DEBUG & DEBUG_STATE)
      debug_printf("%s\n", __FUNCTION__);

   for (i = 0; i < BRW_MAX_CACHE; i++)
      clear_cache(&brw->cache[i]);

   if (brw->curbe.last_buf) {
      FREE(brw->curbe.last_buf);
      brw->curbe.last_buf = NULL;
   }

   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;
}





void brw_destroy_caches( struct brw_context *brw )
{
   unsigned i;

   for (i = 0; i < BRW_MAX_CACHE; i++)
      clear_cache(&brw->cache[i]);
}
