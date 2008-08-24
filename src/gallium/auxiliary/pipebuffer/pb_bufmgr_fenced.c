/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/**
 * \file
 * A buffer manager that wraps buffers in fenced buffers.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.dot.com>
 */


#include "pipe/p_debug.h"
#include "util/u_memory.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"
#include "pb_bufmgr.h"


struct fenced_pb_manager
{
   struct pb_manager base;

   struct pb_manager *provider;
   
   struct fenced_buffer_list *fenced_list;
};


static INLINE struct fenced_pb_manager *
fenced_pb_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct fenced_pb_manager *)mgr;
}


static struct pb_buffer *
fenced_bufmgr_create_buffer(struct pb_manager *mgr, 
                            size_t size,
                            const struct pb_desc *desc)
{
   struct fenced_pb_manager *fenced_mgr = fenced_pb_manager(mgr);
   struct pb_buffer *buf;
   struct pb_buffer *fenced_buf;

   /* check for free buffers before allocating new ones */
   fenced_buffer_list_check_free(fenced_mgr->fenced_list, 0);
   
   buf = fenced_mgr->provider->create_buffer(fenced_mgr->provider, size, desc);
   if(!buf) {
      /* try harder to get a buffer */
      fenced_buffer_list_check_free(fenced_mgr->fenced_list, 1);
      
      buf = fenced_mgr->provider->create_buffer(fenced_mgr->provider, size, desc);
      if(!buf) {
         /* give up */
         return NULL;
      }
   }
   
   fenced_buf = fenced_buffer_create(fenced_mgr->fenced_list, buf);
   if(!fenced_buf) {
      assert(buf->base.refcount == 1);
      pb_destroy(buf);
   }
   
   return fenced_buf;
}


static void
fenced_bufmgr_destroy(struct pb_manager *mgr)
{
   struct fenced_pb_manager *fenced_mgr = fenced_pb_manager(mgr);

   fenced_buffer_list_destroy(fenced_mgr->fenced_list);

   if(fenced_mgr->provider)
      fenced_mgr->provider->destroy(fenced_mgr->provider);
   
   FREE(fenced_mgr);
}


struct pb_manager *
fenced_bufmgr_create(struct pb_manager *provider, 
                     struct pipe_winsys *winsys) 
{
   struct fenced_pb_manager *fenced_mgr;

   if(!provider)
      return NULL;
   
   fenced_mgr = CALLOC_STRUCT(fenced_pb_manager);
   if (!fenced_mgr)
      return NULL;

   fenced_mgr->base.destroy = fenced_bufmgr_destroy;
   fenced_mgr->base.create_buffer = fenced_bufmgr_create_buffer;

   fenced_mgr->provider = provider;
   fenced_mgr->fenced_list = fenced_buffer_list_create(winsys);
   if(!fenced_mgr->fenced_list) {
      FREE(fenced_mgr);
      return NULL;
   }
      
   return &fenced_mgr->base;
}
