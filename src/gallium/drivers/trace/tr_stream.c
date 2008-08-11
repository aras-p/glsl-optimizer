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

#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX)
#include <stdio.h>
#else
#error Unsupported platform 
#endif

#include "pipe/p_util.h"

#include "tr_stream.h"


struct trace_stream 
{
#if defined(PIPE_OS_LINUX)
   FILE *file;
#endif
};


struct trace_stream *
trace_stream_create(const char *name, const char *ext)
{
   struct trace_stream *stream;
   static unsigned file_no = 0; 
   char filename[1024];
   
   stream = CALLOC_STRUCT(trace_stream);
   if(!stream)
      goto error1;
   
   snprintf(filename, sizeof(filename), "%s.%u.%s", name, file_no, ext);
   
#if defined(PIPE_OS_LINUX)
   stream->file = fopen(filename, "w");
   if(!stream->file)
      goto error2;
#endif
   
   return stream;
   
error2:
   FREE(stream);
error1:
   return NULL;
}


boolean
trace_stream_write(struct trace_stream *stream, const void *data, size_t size)
{
   if(!stream)
      return FALSE;
   
#if defined(PIPE_OS_LINUX)
   return fwrite(data, size, 1, stream->file) == size ? TRUE : FALSE;
#endif
}


void
trace_stream_close(struct trace_stream *stream) 
{
   if(!stream)
      return;
   
#if defined(PIPE_OS_LINUX)
   fclose(stream->file);
#endif

   FREE(stream);
}
