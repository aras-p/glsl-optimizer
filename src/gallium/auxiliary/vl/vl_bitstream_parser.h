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

#ifndef vl_bitstream_parser_h
#define vl_bitstream_parser_h

#include "pipe/p_compiler.h"

struct vl_bitstream_parser
{
   unsigned num_bitstreams;
   const unsigned **bitstreams;
   const unsigned *sizes;
   unsigned cur_bitstream;
   unsigned cursor;
};

bool vl_bitstream_parser_init(struct vl_bitstream_parser *parser,
                              unsigned num_bitstreams,
                              const void **bitstreams,
                              const unsigned *sizes);

void vl_bitstream_parser_cleanup(struct vl_bitstream_parser *parser);

unsigned
vl_bitstream_parser_get_bits(struct vl_bitstream_parser *parser,
                             unsigned how_many_bits);

unsigned
vl_bitstream_parser_show_bits(struct vl_bitstream_parser *parser,
                              unsigned how_many_bits);

void vl_bitstream_parser_forward(struct vl_bitstream_parser *parser,
                                 unsigned how_many_bits);

void vl_bitstream_parser_rewind(struct vl_bitstream_parser *parser,
                                unsigned how_many_bits);

#endif /* vl_bitstream_parser_h */
