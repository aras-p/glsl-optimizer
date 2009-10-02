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

#include "vl_bitstream_parser.h"
#include <assert.h>
#include <limits.h>
#include <util/u_memory.h>

static unsigned
grab_bits(unsigned cursor, unsigned how_many_bits, unsigned bitstream_elt)
{
   unsigned excess_bits = sizeof(unsigned) * CHAR_BIT - how_many_bits - cursor;
	
   assert(cursor < sizeof(unsigned) * CHAR_BIT);
   assert(how_many_bits > 0 && how_many_bits <= sizeof(unsigned) * CHAR_BIT);
   assert(cursor + how_many_bits <= sizeof(unsigned) * CHAR_BIT);

   return (bitstream_elt << excess_bits) >> (excess_bits + cursor);
}

static unsigned
show_bits(unsigned cursor, unsigned how_many_bits, const unsigned *bitstream)
{	
   unsigned cur_int = cursor / (sizeof(unsigned) * CHAR_BIT);
   unsigned cur_bit = cursor % (sizeof(unsigned) * CHAR_BIT);
	
   assert(bitstream);
	
   if (cur_bit + how_many_bits > sizeof(unsigned) * CHAR_BIT) {
      unsigned lower = grab_bits(cur_bit, sizeof(unsigned) * CHAR_BIT - cur_bit,
                                 bitstream[cur_int]);
      unsigned upper = grab_bits(0, cur_bit + how_many_bits - sizeof(unsigned) * CHAR_BIT,
                                 bitstream[cur_int + 1]);
      return lower | upper << (sizeof(unsigned) * CHAR_BIT - cur_bit);
   }
   else
      return grab_bits(cur_bit, how_many_bits, bitstream[cur_int]);
}

bool vl_bitstream_parser_init(struct vl_bitstream_parser *parser,
                              unsigned num_bitstreams,
                              const void **bitstreams,
                              const unsigned *sizes)
{
   assert(parser);
   assert(num_bitstreams);
   assert(bitstreams);
   assert(sizes);

   parser->num_bitstreams = num_bitstreams;
   parser->bitstreams = (const unsigned**)bitstreams;
   parser->sizes = sizes;
   parser->cur_bitstream = 0;
   parser->cursor = 0;

   return true;
}

void vl_bitstream_parser_cleanup(struct vl_bitstream_parser *parser)
{
   assert(parser);
}

unsigned
vl_bitstream_parser_get_bits(struct vl_bitstream_parser *parser,
                             unsigned how_many_bits)
{
   unsigned bits;

   assert(parser);

   bits = vl_bitstream_parser_show_bits(parser, how_many_bits);

   vl_bitstream_parser_forward(parser, how_many_bits);

   return bits;
}

unsigned
vl_bitstream_parser_show_bits(struct vl_bitstream_parser *parser,
                              unsigned how_many_bits)
{	
   unsigned bits = 0;
   unsigned shift = 0;
   unsigned cursor;
   unsigned cur_bitstream;

   assert(parser);

   cursor = parser->cursor;
   cur_bitstream = parser->cur_bitstream;

   while (1) {
      unsigned bits_left = parser->sizes[cur_bitstream] * CHAR_BIT - cursor;
      unsigned bits_to_show = how_many_bits > bits_left ? bits_left : how_many_bits;

      bits |= show_bits(cursor, bits_to_show,
                        parser->bitstreams[cur_bitstream]) << shift;
		
      if (how_many_bits > bits_to_show) {
         how_many_bits -= bits_to_show;
         cursor = 0;
         ++cur_bitstream;
         shift += bits_to_show;
      }
      else
         break;
   }

   return bits;
}

void vl_bitstream_parser_forward(struct vl_bitstream_parser *parser,
                                 unsigned how_many_bits)
{
   assert(parser);
   assert(how_many_bits);

   parser->cursor += how_many_bits;

   while (parser->cursor > parser->sizes[parser->cur_bitstream] * CHAR_BIT) {
      parser->cursor -= parser->sizes[parser->cur_bitstream++] * CHAR_BIT;
      assert(parser->cur_bitstream < parser->num_bitstreams);
   }
}

void vl_bitstream_parser_rewind(struct vl_bitstream_parser *parser,
                                unsigned how_many_bits)
{
   signed c;
	
   assert(parser);
   assert(how_many_bits);
	
   c = parser->cursor - how_many_bits;

   while (c < 0) {
      c += parser->sizes[parser->cur_bitstream--] * CHAR_BIT;
      assert(parser->cur_bitstream < parser->num_bitstreams);
   }

   parser->cursor = (unsigned)c;
}
