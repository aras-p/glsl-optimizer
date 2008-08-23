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
 * Stream implementation for the Windows Display driver.
 */

#include "pipe/p_config.h"

#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

#include <windows.h>
#include <winddi.h>

#include "pipe/p_util.h"
#include "util/u_string.h"

#include "tr_stream.h"


#define MAP_FILE_SIZE (4*1024*1024)


struct trace_stream 
{
   char filename[MAX_PATH + 1];
   WCHAR wFileName[MAX_PATH + 1];
   ULONG_PTR iFile;
   char *pMap;
   size_t written;
   unsigned suffix;
};


static INLINE boolean
trace_stream_map(struct trace_stream *stream)
{
   ULONG BytesInUnicodeString;
   static char filename[MAX_PATH + 1];
   unsigned filename_len;

   filename_len = util_snprintf(filename,
                                sizeof(filename),
                                "\\??\\%s.%04x",
                                stream->filename,
                                stream->suffix++);

   EngMultiByteToUnicodeN(
         stream->wFileName,
         sizeof(stream->wFileName),
         &BytesInUnicodeString,
         filename,
         filename_len);
   
   stream->pMap = EngMapFile(stream->wFileName, MAP_FILE_SIZE, &stream->iFile);
   if(!stream->pMap)
      return FALSE;
   
   memset(stream->pMap, 0, MAP_FILE_SIZE);
   stream->written = 0;
   
   return TRUE;
}


static INLINE void
trace_stream_unmap(struct trace_stream *stream)
{
   EngUnmapFile(stream->iFile);
   if(stream->written < MAP_FILE_SIZE) {
      /* Truncate file size */
      stream->pMap = EngMapFile(stream->wFileName, stream->written, &stream->iFile);
      if(stream->pMap)
         EngUnmapFile(stream->iFile);
   }
   
   stream->pMap = NULL;
}


struct trace_stream *
trace_stream_create(const char *filename)
{
   struct trace_stream *stream;
   
   stream = CALLOC_STRUCT(trace_stream);
   if(!stream)
      goto error1;
   
   strncpy(stream->filename, filename, sizeof(stream->filename));
   
   if(!trace_stream_map(stream))
      goto error2;
   
   return stream;
   
error2:
   FREE(stream);
error1:
   return NULL;
}


static INLINE void
trace_stream_copy(struct trace_stream *stream, const char *data, size_t size)
{
   assert(stream->written + size <= MAP_FILE_SIZE);
   memcpy(stream->pMap + stream->written, data, size);
   stream->written += size;
}


boolean
trace_stream_write(struct trace_stream *stream, const void *data, size_t size)
{
   if(!stream)
      return FALSE;
   
   if(!stream->pMap)
      return FALSE;
   
   while(stream->written + size > MAP_FILE_SIZE) {
      size_t step = MAP_FILE_SIZE - stream->written;
      trace_stream_copy(stream, data, step);
      data = (const char *)data + step;
      size -= step;
      
      trace_stream_unmap(stream);
      if(!trace_stream_map(stream))
         return FALSE;
   }

   trace_stream_copy(stream, data, size);
   
   return TRUE;
}


void
trace_stream_flush(struct trace_stream *stream) 
{
   (void)stream;
}


void
trace_stream_close(struct trace_stream *stream) 
{
   if(!stream)
      return;
   
   trace_stream_unmap(stream);

   FREE(stream);
}


#endif
