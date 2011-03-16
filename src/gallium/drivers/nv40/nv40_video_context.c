/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include "nv40_video_context.h"
#include "util/u_video.h"
#include <vl/vl_mpeg12_context.h>

struct pipe_video_context *
nv40_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, void *priv)
{
   struct pipe_context *pipe;

   assert(screen);

   pipe = screen->context_create(screen, priv);
   if (!pipe)
      return NULL;

   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12:
         return vl_create_mpeg12_context(pipe, profile,
                                         chroma_format,
                                         width, height,
                                         true,
                                         PIPE_FORMAT_XYUV);
      default:
         return NULL;
   }
}
