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

#include "util/u_memory.h"
#include "util/u_string.h"

#include "u_stream.h"


#define MAP_FILE_SIZE (4*1024*1024)


struct util_stream 
{
   char filename[MAX_PATH + 1];
   WCHAR wFileName[MAX_PATH + 1];
   boolean growable;
   size_t map_size;
   ULONG_PTR iFile;
   char *pMap;
   size_t written;
   unsigned suffix;
};


static INLINE boolean
util_stream_map(struct util_stream *stream)
{
   ULONG BytesInUnicodeString;
   static char filename[MAX_PATH + 1];
   unsigned filename_len;

   if(stream->growable)
      filename_len = util_snprintf(filename,
                                   sizeof(filename),
                                   "%s.%04x",
                                   stream->filename,
                                   stream->suffix++);
   else
      filename_len = util_snprintf(filename,
                                   sizeof(filename),
                                   "%s",
                                   stream->filename);

   EngMultiByteToUnicodeN(
         stream->wFileName,
         sizeof(stream->wFileName),
         &BytesInUnicodeString,
         filename,
         filename_len);
   
   stream->pMap = EngMapFile(stream->wFileName, stream->map_size, &stream->iFile);
   if(!stream->pMap)
      return FALSE;
   
   memset(stream->pMap, 0, stream->map_size);
   stream->written = 0;
   
   return TRUE;
}


static INLINE void
util_stream_unmap(struct util_stream *stream)
{
   EngUnmapFile(stream->iFile);
   if(stream->written < stream->map_size) {
      /* Truncate file size */
      stream->pMap = EngMapFile(stream->wFileName, stream->written, &stream->iFile);
      if(stream->pMap)
         EngUnmapFile(stream->iFile);
   }
   
   stream->pMap = NULL;
}


static INLINE void
util_stream_full_qualified_filename(char *dst, size_t size, const char *src)
{
   boolean need_drive, need_root;
   
   if((('A' <= src[0] && src[0] <= 'Z') || ('a' <= src[0] && src[0] <= 'z')) && src[1] == ':') {
      need_drive = FALSE;
      need_root = src[2] == '\\' ? FALSE : TRUE;
   }
   else {
      need_drive = TRUE;
      need_root = src[0] == '\\' ? FALSE : TRUE;
   }
   
   util_snprintf(dst, size, 
                 "\\??\\%s%s%s",
                 need_drive ? "C:" : "",
                 need_root ? "\\" : "",
                 src);
}


struct util_stream *
util_stream_create(const char *filename, size_t max_size)
{
   struct util_stream *stream;
   
   stream = CALLOC_STRUCT(util_stream);
   if(!stream)
      goto error1;
   
   util_stream_full_qualified_filename(stream->filename,
                                       sizeof(stream->filename),
                                       filename);
   
   if(max_size) {
      stream->growable = FALSE;
      stream->map_size = max_size;
   }
   else {
      stream->growable = TRUE;
      stream->map_size = MAP_FILE_SIZE;
   }
   
   if(!util_stream_map(stream))
      goto error2;
   
   return stream;
   
error2:
   FREE(stream);
error1:
   return NULL;
}


static INLINE void
util_stream_copy(struct util_stream *stream, const char *data, size_t size)
{
   assert(stream->written + size <= stream->map_size);
   memcpy(stream->pMap + stream->written, data, size);
   stream->written += size;
}


boolean
util_stream_write(struct util_stream *stream, const void *data, size_t size)
{
   if(!stream)
      return FALSE;
   
   if(!stream->pMap)
      return FALSE;
   
   while(stream->written + size > stream->map_size) {
      size_t step = stream->map_size - stream->written;
      util_stream_copy(stream, data, step);
      data = (const char *)data + step;
      size -= step;
      
      util_stream_unmap(stream);
      if(!stream->growable || !util_stream_map(stream))
         return FALSE;
   }

   util_stream_copy(stream, data, size);
   
   return TRUE;
}


void
util_stream_flush(struct util_stream *stream) 
{
   (void)stream;
}


void
util_stream_close(struct util_stream *stream) 
{
   if(!stream)
      return;
   
   util_stream_unmap(stream);

   FREE(stream);
}


#endif
