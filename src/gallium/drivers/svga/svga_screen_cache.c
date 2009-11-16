/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_memory.h"

#include "svga_debug.h"
#include "svga_winsys.h"
#include "svga_screen.h"
#include "svga_screen_cache.h"


#define SVGA_SURFACE_CACHE_ENABLED 1


/** 
 * Compute the bucket for this key. 
 * 
 * We simply compute log2(width) for now, but
 */
static INLINE unsigned
svga_screen_cache_bucket(const struct svga_host_surface_cache_key *key)
{
   unsigned bucket = 0;
   unsigned size = key->size.width;
   
   while ((size >>= 1))
      ++bucket;
   
   if(key->flags & SVGA3D_SURFACE_HINT_INDEXBUFFER)
      bucket += 32;
   
   assert(bucket < SVGA_HOST_SURFACE_CACHE_BUCKETS);
   
   return bucket;
}


static INLINE struct svga_winsys_surface *
svga_screen_cache_lookup(struct svga_screen *svgascreen,
                         const struct svga_host_surface_cache_key *key)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct svga_winsys_surface *handle = NULL;
   struct list_head *curr, *next;
   unsigned bucket;
   unsigned tries = 0;

   bucket = svga_screen_cache_bucket(key);

   pipe_mutex_lock(cache->mutex);

   curr = cache->bucket[bucket].next;
   next = curr->next;
   while(curr != &cache->bucket[bucket]) {
      ++tries;
      
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry, curr, bucket_head);

      assert(entry->handle);
      
      if(memcmp(&entry->key, key, sizeof *key) == 0 &&
         sws->fence_signalled( sws, entry->fence, 0 ) == 0) {
         assert(sws->surface_is_flushed(sws, entry->handle));
         
         handle = entry->handle; // Reference is transfered here.
         entry->handle = NULL;
         
         LIST_DEL(&entry->bucket_head);

         LIST_DEL(&entry->head);
         
         LIST_ADD(&entry->head, &cache->empty);

         break;
      }

      curr = next; 
      next = curr->next;
   }

   pipe_mutex_unlock(cache->mutex);
   
#if 0
   _debug_printf("%s: cache %s after %u tries\n", __FUNCTION__, handle ? "hit" : "miss", tries);
#else
   (void)tries;
#endif
   
   return handle;
}


/*
 * Transfers a handle reference.
 */
                           
static INLINE void
svga_screen_cache_add(struct svga_screen *svgascreen,
                      const struct svga_host_surface_cache_key *key, 
                      struct svga_winsys_surface **p_handle)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry = NULL;
   struct svga_winsys_surface *handle = *p_handle;
   

   assert(handle);
   if(!handle)
      return;
   
   *p_handle = NULL;
   pipe_mutex_lock(cache->mutex);
   
   if(!LIST_IS_EMPTY(&cache->empty)) {
        /* use the first empty entry */
        entry = LIST_ENTRY(struct svga_host_surface_cache_entry, cache->empty.next, head);
        
        LIST_DEL(&entry->head);
     }
   else if(!LIST_IS_EMPTY(&cache->unused)) {
      /* free the last used buffer and reuse its entry */
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry, cache->unused.prev, head);
      SVGA_DBG(DEBUG_DMA, "unref sid %p\n", entry->handle);
      sws->surface_reference(sws, &entry->handle, NULL);

      LIST_DEL(&entry->bucket_head);

      LIST_DEL(&entry->head);
   }

   if(entry) {
      entry->handle = handle;
      memcpy(&entry->key, key, sizeof entry->key);
   
      LIST_ADD(&entry->head, &cache->validated);
   }
   else {
      /* Couldn't cache the buffer -- this really shouldn't happen */
      SVGA_DBG(DEBUG_DMA, "unref sid %p\n", handle);
      sws->surface_reference(sws, &handle, NULL);
   }
   
   pipe_mutex_unlock(cache->mutex);
}


/**
 * Called during the screen flush to move all buffers not in a validate list
 * into the unused list.
 */
void
svga_screen_cache_flush(struct svga_screen *svgascreen,
                        struct pipe_fence_handle *fence)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct list_head *curr, *next;
   unsigned bucket;

   pipe_mutex_lock(cache->mutex);

   curr = cache->validated.next;
   next = curr->next;
   while(curr != &cache->validated) {
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry, curr, head);

      assert(entry->handle);

      if(sws->surface_is_flushed(sws, entry->handle)) {
         LIST_DEL(&entry->head);
         
         svgascreen->sws->fence_reference(svgascreen->sws, &entry->fence, fence);

         LIST_ADD(&entry->head, &cache->unused);

         bucket = svga_screen_cache_bucket(&entry->key);
         LIST_ADD(&entry->bucket_head, &cache->bucket[bucket]);
      }

      curr = next; 
      next = curr->next;
   }

   pipe_mutex_unlock(cache->mutex);
}


void
svga_screen_cache_cleanup(struct svga_screen *svgascreen)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   unsigned i;
   
   for(i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i) {
      if(cache->entries[i].handle) {
	 SVGA_DBG(DEBUG_DMA, "unref sid %p\n", cache->entries[i].handle);
	 sws->surface_reference(sws, &cache->entries[i].handle, NULL);
      }

      if(cache->entries[i].fence)
         svgascreen->sws->fence_reference(svgascreen->sws, &cache->entries[i].fence, NULL);
   }
   
   pipe_mutex_destroy(cache->mutex);
}


enum pipe_error
svga_screen_cache_init(struct svga_screen *svgascreen)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   unsigned i;

   pipe_mutex_init(cache->mutex);
   
   for(i = 0; i < SVGA_HOST_SURFACE_CACHE_BUCKETS; ++i)
      LIST_INITHEAD(&cache->bucket[i]);

   LIST_INITHEAD(&cache->unused);
   
   LIST_INITHEAD(&cache->validated);
   
   LIST_INITHEAD(&cache->empty);
   for(i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i)
      LIST_ADDTAIL(&cache->entries[i].head, &cache->empty);

   return PIPE_OK;
}

                           
struct svga_winsys_surface *
svga_screen_surface_create(struct svga_screen *svgascreen,
                           struct svga_host_surface_cache_key *key)
{
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_winsys_surface *handle = NULL;

   if (SVGA_SURFACE_CACHE_ENABLED && key->format == SVGA3D_BUFFER) {
      /* round the buffer size up to the nearest power of two to increase the
       * probability of cache hits */
      uint32_t size = 1;
      while(size < key->size.width)
         size <<= 1;
      key->size.width = size;
      
      handle = svga_screen_cache_lookup(svgascreen, key);
      if (handle)
         SVGA_DBG(DEBUG_DMA, "  reuse sid %p sz %d\n", handle, size);
   }

   if (!handle) {
      handle = sws->surface_create(sws,
                                   key->flags,
                                   key->format,
                                   key->size, 
                                   key->numFaces, 
                                   key->numMipLevels);
      if (handle)
         SVGA_DBG(DEBUG_DMA, "create sid %p sz %d\n", handle, key->size);
   }

   return handle;
}


void
svga_screen_surface_destroy(struct svga_screen *svgascreen,
                            const struct svga_host_surface_cache_key *key,
                            struct svga_winsys_surface **p_handle)
{
   struct svga_winsys_screen *sws = svgascreen->sws;
   
   if(SVGA_SURFACE_CACHE_ENABLED && key->format == SVGA3D_BUFFER) {
      svga_screen_cache_add(svgascreen, key, p_handle);
   }
   else {
      SVGA_DBG(DEBUG_DMA, "unref sid %p\n", *p_handle);
      sws->surface_reference(sws, p_handle, NULL);
   }
}
