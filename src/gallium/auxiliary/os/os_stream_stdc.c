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
 * Stream implementation based on the Standard C Library.
 */

#include "pipe/p_config.h"

#if defined(PIPE_OS_UNIX) || defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#include <stdlib.h>
#include <stdio.h>

#include "os_stream.h"


struct os_stream 
{
   FILE *file;
};


struct os_stream *
os_stream_create(const char *filename, size_t max_size)
{
   struct os_stream *stream;
   
   (void)max_size;
   
   stream = (struct os_stream *)calloc(1, sizeof(struct os_stream));
   if(!stream)
      goto no_stream;
   
   stream->file = fopen(filename, "w");
   if(!stream->file)
      goto no_file;
   
   return stream;
   
no_file:
   free(stream);
no_stream:
   return NULL;
}


boolean
os_stream_write(struct os_stream *stream, const void *data, size_t size)
{
   if(!stream)
      return FALSE;
   
   return fwrite(data, size, 1, stream->file) == size ? TRUE : FALSE;
}


void
os_stream_flush(struct os_stream *stream) 
{
   if(!stream)
      return;
   
   fflush(stream->file);
}


void
os_stream_close(struct os_stream *stream) 
{
   if(!stream)
      return;
   
   fclose(stream->file);

   free(stream);
}


#endif
