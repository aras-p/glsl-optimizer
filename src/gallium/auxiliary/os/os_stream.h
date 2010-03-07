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
 * Cross-platform sequential access stream abstraction.
 */

#ifndef _OS_STREAM_H_
#define _OS_STREAM_H_


#include "pipe/p_compiler.h"


/**
 * OS stream (FILE, socket, etc) abstraction.
 */
struct os_stream
{
   void
   (*close)(struct os_stream *stream);

   boolean
   (*write)(struct os_stream *stream, const void *data, size_t size);

   void
   (*flush)(struct os_stream *stream);
};


static INLINE void
os_stream_close(struct os_stream *stream)
{
   if (!stream)
      return;

   stream->close(stream);
}


static INLINE boolean
os_stream_write(struct os_stream *stream, const void *data, size_t size)
{
   if (!stream)
      return FALSE;
   return stream->write(stream, data, size);
}


static INLINE boolean
os_stream_write_str(struct os_stream *stream, const char *str)
{
   size_t size;
   if (!stream)
      return FALSE;
   for(size = 0; str[size]; ++size)
      ;
   return stream->write(stream, str, size);
}


static INLINE void
os_stream_flush(struct os_stream *stream)
{
   stream->flush(stream);
}


struct os_stream *
os_file_stream_create(const char *filename);


struct os_stream *
os_null_stream_create(void);


extern struct os_stream *
os_log_stream;


struct os_stream *
os_str_stream_create(size_t initial_size);


const char *
os_str_stream_get(struct os_stream *stream);

char *
os_str_stream_get_and_close(struct os_stream *stream);


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
#define os_file_stream_create(_filename) os_null_stream_create()
#endif


#endif /* _OS_STREAM_H_ */
