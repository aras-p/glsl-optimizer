/**************************************************************************
 *
 * Copyright 2008-2010 VMware, Inc.
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

/**
 * @file
 * Malloc string stream implementation.
 */

#include "pipe/p_config.h"

#include "os_memory.h"
#include "os_stream.h"


struct os_str_stream
{
   struct os_stream base;

   char *str;

   size_t size;
   size_t written;
};


static INLINE struct os_str_stream *
os_str_stream(struct os_stream *stream)
{
   return (struct os_str_stream *)stream;
}


static void
os_str_stream_close(struct os_stream *_stream)
{
   struct os_str_stream *stream = os_str_stream(_stream);

   os_free(stream->str);

   os_free(stream);
}


static boolean
os_str_stream_write(struct os_stream *_stream, const void *data, size_t size)
{
   struct os_str_stream *stream = os_str_stream(_stream);
   size_t minimum_size;
   boolean ret = TRUE;

   minimum_size = stream->written + size + 1;
   if (stream->size < minimum_size) {
      size_t new_size = stream->size;
      char * new_str;

      do {
         new_size *= 2;
      } while (new_size < minimum_size);

      new_str = os_realloc(stream->str, stream->size, new_size);
      if (new_str) {
         stream->str = new_str;
         stream->size = new_size;
      }
      else {
         size = stream->size - stream->written - 1;
         ret = FALSE;
      }
   }

   memcpy(stream->str + stream->written, data, size);
   stream->written += size;

   return ret;
}


static void
os_str_stream_flush(struct os_stream *stream)
{
   (void)stream;
}


struct os_stream *
os_str_stream_create(size_t size)
{
   struct os_str_stream *stream;

   stream = (struct os_str_stream *)os_calloc(1, sizeof(*stream));
   if(!stream)
      goto no_stream;

   stream->base.close = &os_str_stream_close;
   stream->base.write = &os_str_stream_write;
   stream->base.flush = &os_str_stream_flush;

   stream->str = os_malloc(size);
   if(!stream->str)
      goto no_str;

   stream->size = size;

   return &stream->base;

no_str:
   os_free(stream);
no_stream:
   return NULL;
}


const char *
os_str_stream_get(struct os_stream *_stream)
{
   struct os_str_stream *stream = os_str_stream(_stream);

   if (!stream)
      return NULL;

   stream->str[stream->written] = 0;
   return stream->str;
}


char *
os_str_stream_get_and_close(struct os_stream *_stream)
{
   struct os_str_stream *stream = os_str_stream(_stream);
   char *str;

   if (!stream)
      return NULL;

   str = stream->str;

   str[stream->written] = 0;

   os_free(stream);

   return str;
}
