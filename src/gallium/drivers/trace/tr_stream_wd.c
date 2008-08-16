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

#include "tr_stream.h"


#define MAP_FILE_SIZE (4*1024*1024)


struct trace_stream 
{
   WCHAR wFileName[MAX_PATH + 1];
   ULONG_PTR iFile;
   char *pMap;
   size_t written;
};


struct trace_stream *
trace_stream_create(const char *filename)
{
   struct trace_stream *stream;
   ULONG BytesInUnicodeString;
   
   stream = CALLOC_STRUCT(trace_stream);
   if(!stream)
      goto error1;
   
   EngMultiByteToUnicodeN(
         stream->wFileName,
         sizeof(stream->wFileName),
         &BytesInUnicodeString,
         (char *)filename,
         strlen(filename));
   
   stream->pMap = EngMapFile(stream->wFileName, MAP_FILE_SIZE, &stream->iFile);
   if(!stream->pMap)
      goto error2;
   
   return stream;
   
error2:
   FREE(stream);
error1:
   return NULL;
}


boolean
trace_stream_write(struct trace_stream *stream, const void *data, size_t size)
{
   boolean ret;
   
   if(!stream)
      return FALSE;
   
   if(stream->written + size > MAP_FILE_SIZE) {
      ret = FALSE;
      size = MAP_FILE_SIZE - stream->written;
   }
   else
      ret = TRUE;
   
   memcpy(stream->pMap + stream->written, data, size);
   stream->written += size;
   
   return ret;
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
   
   EngUnmapFile(stream->iFile);
   if(stream->written < MAP_FILE_SIZE) {
      /* Truncate file size */
      stream->pMap = EngMapFile(stream->wFileName, stream->written, &stream->iFile);
      if(stream->pMap)
         EngUnmapFile(stream->iFile);
   }

   FREE(stream);
}


#endif
