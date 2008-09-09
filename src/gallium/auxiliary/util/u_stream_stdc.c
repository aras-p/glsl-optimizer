/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * @file
 * Stream implementation based on the Standard C Library.
 */

#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX) || defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#include <stdio.h>

#include "util/u_memory.h"

#include "u_stream.h"


struct util_stream 
{
   FILE *file;
};


struct util_stream *
util_stream_create(const char *filename, size_t max_size)
{
   struct util_stream *stream;
   
   (void)max_size;
   
   stream = CALLOC_STRUCT(util_stream);
   if(!stream)
      goto error1;
   
   stream->file = fopen(filename, "w");
   if(!stream->file)
      goto error2;
   
   return stream;
   
error2:
   FREE(stream);
error1:
   return NULL;
}


boolean
util_stream_write(struct util_stream *stream, const void *data, size_t size)
{
   if(!stream)
      return FALSE;
   
   return fwrite(data, size, 1, stream->file) == size ? TRUE : FALSE;
}


void
util_stream_flush(struct util_stream *stream) 
{
   if(!stream)
      return;
   
   fflush(stream->file);
}


void
util_stream_close(struct util_stream *stream) 
{
   if(!stream)
      return;
   
   fclose(stream->file);

   FREE(stream);
}


#endif
