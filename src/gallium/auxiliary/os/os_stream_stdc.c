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


struct os_stdc_stream
{
   struct os_stream base;

   FILE *file;
};


static INLINE struct os_stdc_stream *
os_stdc_stream(struct os_stream *stream)
{
   return (struct os_stdc_stream *)stream;
}


static void
os_stdc_stream_close(struct os_stream *_stream)
{
   struct os_stdc_stream *stream = os_stdc_stream(_stream);

   fclose(stream->file);

   free(stream);
}


static boolean
os_stdc_stream_write(struct os_stream *_stream, const void *data, size_t size)
{
   struct os_stdc_stream *stream = os_stdc_stream(_stream);

   return fwrite(data, size, 1, stream->file) == size ? TRUE : FALSE;
}


static void
os_stdc_stream_flush(struct os_stream *_stream)
{
   struct os_stdc_stream *stream = os_stdc_stream(_stream);

   fflush(stream->file);
}


struct os_stream *
os_file_stream_create(const char *filename)
{
   struct os_stdc_stream *stream;

   stream = (struct os_stdc_stream *)calloc(1, sizeof(*stream));
   if(!stream)
      goto no_stream;

   stream->base.close = &os_stdc_stream_close;
   stream->base.write = &os_stdc_stream_write;
   stream->base.flush = &os_stdc_stream_flush;

   stream->file = fopen(filename, "w");
   if(!stream->file)
      goto no_file;

   return &stream->base;

no_file:
   free(stream);
no_stream:
   return NULL;
}


#endif
