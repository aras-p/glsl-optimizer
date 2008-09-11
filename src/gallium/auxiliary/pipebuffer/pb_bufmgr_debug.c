/**************************************************************************
 *
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * \file
 * Debug buffer manager to detect buffer under- and overflows.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_winsys.h"
#include "pipe/p_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"
#include "util/u_time.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


#ifdef DEBUG


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct pb_debug_manager;


/**
 * Wrapper around a pipe buffer which adds delayed destruction.
 */
struct pb_debug_buffer
{
   struct pb_buffer base;
   
   struct pb_buffer *buffer;
   struct pb_debug_manager *mgr;
   
   size_t underflow_size;
   size_t overflow_size;
};


struct pb_debug_manager
{
   struct pb_manager base;

   struct pb_manager *provider;

   size_t band_size;
};


static INLINE struct pb_debug_buffer *
pb_debug_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct pb_debug_buffer *)buf;
}


static INLINE struct pb_debug_manager *
pb_debug_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_debug_manager *)mgr;
}


static const uint8_t random_pattern[32] = {
   0xaf, 0xcf, 0xa5, 0xa2, 0xc2, 0x63, 0x15, 0x1a, 
   0x7e, 0xe2, 0x7e, 0x84, 0x15, 0x49, 0xa2, 0x1e,
   0x49, 0x63, 0xf5, 0x52, 0x74, 0x66, 0x9e, 0xc4, 
   0x6d, 0xcf, 0x2c, 0x4a, 0x74, 0xe6, 0xfd, 0x94
};


static INLINE void 
fill_random_pattern(uint8_t *dst, size_t size)
{
   size_t i = 0;
   while(size--) {
      *dst++ = random_pattern[i++];
      i &= sizeof(random_pattern) - 1;
   }
}


static INLINE boolean 
check_random_pattern(const uint8_t *dst, size_t size, 
                     size_t *min_ofs, size_t *max_ofs) 
{
   boolean result = TRUE;
   size_t i;
   *min_ofs = size;
   *max_ofs = 0;
   for(i = 0; i < size; ++i) {
      if(*dst++ != random_pattern[i % sizeof(random_pattern)]) {
         *min_ofs = MIN2(*min_ofs, i);
         *max_ofs = MAX2(*max_ofs, i);
	 result = FALSE;
      }
   }
   return result;
}


static void
pb_debug_buffer_fill(struct pb_debug_buffer *buf)
{
   uint8_t *map;
   
   map = pb_map(buf->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
   assert(map);
   if(map) {
      fill_random_pattern(map, buf->underflow_size);
      fill_random_pattern(map + buf->underflow_size + buf->base.base.size, 
                          buf->overflow_size);
      pb_unmap(buf->buffer);
   }
}


/**
 * Check for under/over flows.
 * 
 * Should be called with the buffer unmaped.
 */
static void
pb_debug_buffer_check(struct pb_debug_buffer *buf)
{
   uint8_t *map;
   
   map = pb_map(buf->buffer, PIPE_BUFFER_USAGE_CPU_READ);
   assert(map);
   if(map) {
      boolean underflow, overflow;
      size_t min_ofs, max_ofs;
      
      underflow = !check_random_pattern(map, buf->underflow_size, 
                                        &min_ofs, &max_ofs);
      if(underflow) {
         debug_printf("buffer underflow (offset -%u%s to -%u bytes) detected\n",
                      buf->underflow_size - min_ofs,
                      min_ofs == 0 ? "+" : "",
                      buf->underflow_size - max_ofs);
      }
      
      overflow = !check_random_pattern(map + buf->underflow_size + buf->base.base.size, 
                                       buf->overflow_size, 
                                       &min_ofs, &max_ofs);
      if(overflow) {
         debug_printf("buffer overflow (size %u plus offset %u to %u%s bytes) detected\n",
                      buf->base.base.size,
                      min_ofs,
                      max_ofs,
                      max_ofs == buf->overflow_size - 1 ? "+" : "");
      }
      
      debug_assert(!underflow && !overflow);

      /* re-fill if not aborted */
      if(underflow)
         fill_random_pattern(map, buf->underflow_size);
      if(overflow)
         fill_random_pattern(map + buf->underflow_size + buf->base.base.size, 
                             buf->overflow_size);

      pb_unmap(buf->buffer);
   }
}


static void
pb_debug_buffer_destroy(struct pb_buffer *_buf)
{
   struct pb_debug_buffer *buf = pb_debug_buffer(_buf);  
   
   assert(!buf->base.base.refcount);
   
   pb_debug_buffer_check(buf);

   pb_reference(&buf->buffer, NULL);
   FREE(buf);
}


static void *
pb_debug_buffer_map(struct pb_buffer *_buf, 
                    unsigned flags)
{
   struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
   void *map;
   
   pb_debug_buffer_check(buf);

   map = pb_map(buf->buffer, flags);
   if(!map)
      return NULL;
   
   return (uint8_t *)map + buf->underflow_size;
}


static void
pb_debug_buffer_unmap(struct pb_buffer *_buf)
{
   struct pb_debug_buffer *buf = pb_debug_buffer(_buf);   
   pb_unmap(buf->buffer);
   
   pb_debug_buffer_check(buf);
}


static void
pb_debug_buffer_get_base_buffer(struct pb_buffer *_buf,
                                struct pb_buffer **base_buf,
                                unsigned *offset)
{
   struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
   pb_get_base_buffer(buf->buffer, base_buf, offset);
   *offset += buf->underflow_size;
}


const struct pb_vtbl 
pb_debug_buffer_vtbl = {
      pb_debug_buffer_destroy,
      pb_debug_buffer_map,
      pb_debug_buffer_unmap,
      pb_debug_buffer_get_base_buffer
};


static struct pb_buffer *
pb_debug_manager_create_buffer(struct pb_manager *_mgr, 
                               size_t size,
                               const struct pb_desc *desc)
{
   struct pb_debug_manager *mgr = pb_debug_manager(_mgr);
   struct pb_debug_buffer *buf;
   struct pb_desc real_desc;
   size_t real_size;
   
   buf = CALLOC_STRUCT(pb_debug_buffer);
   if(!buf)
      return NULL;
   
   real_size = size + 2*mgr->band_size;
   real_desc = *desc;
   real_desc.usage |= PIPE_BUFFER_USAGE_CPU_WRITE;
   real_desc.usage |= PIPE_BUFFER_USAGE_CPU_READ;

   buf->buffer = mgr->provider->create_buffer(mgr->provider, 
                                              real_size, 
                                              &real_desc);
   if(!buf->buffer) {
      FREE(buf);
      return NULL;
   }
   
   assert(buf->buffer->base.refcount >= 1);
   assert(pb_check_alignment(real_desc.alignment, buf->buffer->base.alignment));
   assert(pb_check_usage(real_desc.usage, buf->buffer->base.usage));
   assert(buf->buffer->base.size >= real_size);
   
   buf->base.base.refcount = 1;
   buf->base.base.alignment = desc->alignment;
   buf->base.base.usage = desc->usage;
   buf->base.base.size = size;
   
   buf->base.vtbl = &pb_debug_buffer_vtbl;
   buf->mgr = mgr;

   buf->underflow_size = mgr->band_size;
   buf->overflow_size = buf->buffer->base.size - buf->underflow_size - size;
   
   pb_debug_buffer_fill(buf);
   
   return &buf->base;
}


static void
pb_debug_manager_destroy(struct pb_manager *_mgr)
{
   struct pb_debug_manager *mgr = pb_debug_manager(_mgr);
   mgr->provider->destroy(mgr->provider);
   FREE(mgr);
}


struct pb_manager *
pb_debug_manager_create(struct pb_manager *provider, size_t band_size) 
{
   struct pb_debug_manager *mgr;

   if(!provider)
      return NULL;
   
   mgr = CALLOC_STRUCT(pb_debug_manager);
   if (!mgr)
      return NULL;

   mgr->base.destroy = pb_debug_manager_destroy;
   mgr->base.create_buffer = pb_debug_manager_create_buffer;
   mgr->provider = provider;
   mgr->band_size = band_size;
      
   return &mgr->base;
}


#else /* !DEBUG */


struct pb_manager *
pb_debug_manager_create(struct pb_manager *provider, size_t band_size) 
{
   return provider;
}


#endif /* !DEBUG */
