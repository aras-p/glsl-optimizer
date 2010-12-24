/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Helper utility for uploading user buffers & other data, and
 * coalescing small buffers into larger ones.
 */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "u_upload_mgr.h"


struct u_upload_mgr {
   struct pipe_context *pipe;

   unsigned default_size;
   unsigned alignment;
   unsigned usage;

   /* The active buffer:
    */
   struct pipe_resource *buffer;
   struct pipe_transfer *transfer;
   uint8_t *map;
   unsigned size;
   unsigned offset;
};


struct u_upload_mgr *u_upload_create( struct pipe_context *pipe,
                                      unsigned default_size,
                                      unsigned alignment,
                                      unsigned usage )
{
   struct u_upload_mgr *upload = CALLOC_STRUCT( u_upload_mgr );
   if (!upload)
      return NULL;

   upload->pipe = pipe;
   upload->default_size = default_size;
   upload->alignment = alignment;
   upload->usage = usage;
   upload->buffer = NULL;

   return upload;
}

/* Release old buffer.
 * 
 * This must usually be called prior to firing the command stream
 * which references the upload buffer, as many memory managers will
 * cause subsequent maps of a fired buffer to wait.
 *
 * Can improve this with a change to pipe_buffer_write to use the
 * DONT_WAIT bit, but for now, it's easiest just to grab a new buffer.
 */
void u_upload_flush( struct u_upload_mgr *upload )
{
   if (upload->transfer) {
      pipe_transfer_unmap(upload->pipe, upload->transfer);
      upload->transfer = NULL;
   }
   pipe_resource_reference( &upload->buffer, NULL );
   upload->size = 0;
}


void u_upload_destroy( struct u_upload_mgr *upload )
{
   u_upload_flush( upload );
   FREE( upload );
}


static enum pipe_error 
u_upload_alloc_buffer( struct u_upload_mgr *upload,
                       unsigned min_size )
{
   unsigned size;

   /* Release old buffer, if present:
    */
   u_upload_flush( upload );

   /* Allocate a new one: 
    */
   size = align(MAX2(upload->default_size, min_size), 4096);

   upload->buffer = pipe_buffer_create( upload->pipe->screen,
                                        upload->usage,
                                        size );
   if (upload->buffer == NULL) 
      goto fail;

   upload->map = pipe_buffer_map(upload->pipe, upload->buffer,
                                 PIPE_TRANSFER_WRITE, &upload->transfer);
   
   upload->size = size;

   upload->offset = 0;
   return 0;

fail:
   if (upload->buffer)
      pipe_resource_reference( &upload->buffer, NULL );

   return PIPE_ERROR_OUT_OF_MEMORY;
}


enum pipe_error u_upload_data( struct u_upload_mgr *upload,
                               unsigned size,
                               const void *data,
                               unsigned *out_offset,
                               struct pipe_resource **outbuf )
{
   unsigned alloc_size = align( size, upload->alignment );
   enum pipe_error ret = PIPE_OK;

   if (upload->offset + alloc_size > upload->size) {
      ret = u_upload_alloc_buffer( upload, alloc_size );
      if (ret)
         return ret;
   }

   assert(upload->offset < upload->buffer->width0);
   assert(upload->offset + size <= upload->buffer->width0);
   assert(size);

   memcpy(upload->map + upload->offset, data, size);

   /* Emit the return values:
    */
   pipe_resource_reference( outbuf, upload->buffer );
   *out_offset = upload->offset;
   upload->offset += alloc_size;
   return PIPE_OK;
}


/* As above, but upload the full contents of a buffer.  Useful for
 * uploading user buffers, avoids generating an explosion of GPU
 * buffers if you have an app that does lots of small vertex buffer
 * renders or DrawElements calls.
 */
enum pipe_error u_upload_buffer( struct u_upload_mgr *upload,
                                 unsigned offset,
                                 unsigned size,
                                 struct pipe_resource *inbuf,
                                 unsigned *out_offset,
                                 struct pipe_resource **outbuf )
{
   enum pipe_error ret = PIPE_OK;
   struct pipe_transfer *transfer = NULL;
   const char *map = NULL;

   map = (const char *)pipe_buffer_map(upload->pipe,
				       inbuf,
				       PIPE_TRANSFER_READ,
				       &transfer);

   if (map == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto done;
   }

   if (0)
      debug_printf("upload ptr %p ofs %d sz %d\n", map, offset, size);

   ret = u_upload_data( upload, 
                        size,
                        map + offset,
                        out_offset,
                        outbuf );
   if (ret)
      goto done;

done:
   if (map)
      pipe_buffer_unmap( upload->pipe, transfer );

   return ret;
}
