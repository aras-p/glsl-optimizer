/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "ilo_context.h"
#include "ilo_video.h"

/*
 * Nothing here.  We could make use of the video codec engine someday.
 */

static struct pipe_video_codec *
ilo_create_video_decoder(struct pipe_context *pipe,
                         const struct pipe_video_codec *templ)
{
   return vl_create_decoder(pipe, templ);
}

static struct pipe_video_buffer *
ilo_create_video_buffer(struct pipe_context *pipe,
                        const struct pipe_video_buffer *templ)
{
   return vl_video_buffer_create(pipe, templ);
}

/**
 * Initialize video-related functions.
 */
void
ilo_init_video_functions(struct ilo_context *ilo)
{
   ilo->base.create_video_codec = ilo_create_video_decoder;
   ilo->base.create_video_buffer = ilo_create_video_buffer;
}
