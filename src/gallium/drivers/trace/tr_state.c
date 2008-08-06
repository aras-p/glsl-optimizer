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


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#include "tr_dump.h"
#include "tr_state.h"


void trace_dump_block(struct trace_stream *stream, 
                      const struct pipe_format_block *block)
{
   trace_dump_struct_begin(stream, "pipe_format_block");
   trace_dump_member(stream, uint, block, size);
   trace_dump_member(stream, uint, block, width);
   trace_dump_member(stream, uint, block, height);
   trace_dump_struct_end(stream);
}


void trace_dump_template(struct trace_stream *stream, 
                         const struct pipe_texture *templat)
{
   trace_dump_struct_begin(stream, "pipe_texture");
   
   trace_dump_member(stream, int, templat, target);
   trace_dump_member(stream, int, templat, format);
   
   trace_dump_member_begin(stream, "width");
   trace_dump_array(stream, uint, templat->width, 1);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "height");
   trace_dump_array(stream, uint, templat->height, 1);
   trace_dump_member_end(stream);

   trace_dump_member_begin(stream, "block");
   trace_dump_block(stream, &templat->block);
   trace_dump_member_end(stream);
   
   trace_dump_member(stream, uint, templat, last_level);
   trace_dump_member(stream, uint, templat, tex_usage);
   
   trace_dump_struct_end(stream);
}

