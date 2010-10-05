/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "mpeg2_bitstream_parser.h"

int
vlVdpMPEG2NextStartCode(struct vdpMPEG2BitstreamParser *parser)
{
	uint32_t integer = 0;
	uint32_t bytes_to_end;
	
	/* Move cursor to the start of a byte */
	while(parser->cursor % 8)
		parser->cursor++;
		
	bytes_to_end = parser->cur_bitstream_length - parser->cursor/8 - 1;
		
	/* Read byte after byte, until startcode is found */
	while(integer != 0x00000100)
	{
		if (bytes_to_end < 0)
		{
			parser->state = MPEG2_HEADER_DONE;
			return 1;
		}
		
		integer << 8;
		integer = integer & (unsigned char)(parser->ptr_bitstream + parser->cursor/8)[0];
	
		bytes_to_end--;
		parser->cursor += 8;
		
	}
	
	/* start_code found. rewind cursor a byte */
	parser->cursor -= 8;
	
	return 0;
}

int
vlVdpMPEG2BitstreamToMacroblock (
		  struct pipe_screen *screen,
		  VdpBitstreamBuffer const *bitstream_buffers,
		  uint32_t bitstream_buffer_count,
          unsigned int *num_macroblocks,
          struct pipe_mpeg12_macroblock **pipe_macroblocks)
{
	bool b_header_done = false;
	struct vdpMPEG2BitstreamParser parser;
	
	num_macroblocks[0] = 0;
	
	memset(&parser,0,sizeof(parser));
	parser.state = MPEG2_HEADER_START_CODE;
	parser.cur_bitstream_length = bitstream_buffers[0].bitstream_bytes;
	parser.ptr_bitstream = (unsigned char *)bitstream_buffers[0].bitstream;
	
	/* Main header parser loop */
	while(!b_header_done)
	{
		switch (parser.state)
		{
		case MPEG2_HEADER_START_CODE:
			if (vlVdpMPEG2NextStartCode(&parser))
				continue;
			
			/* Start_code found */
			switch ((parser.ptr_bitstream + parser.cursor/8)[0])
			{
				/* sequence_header_code */
				case 0xB3:
				debug_printf("[VDPAU][Bitstream parser] Sequence header code found at cursor pos: %d\n", parser.cursor);
				exit(1);
				break;
			}
		
		break;
		case MPEG2_HEADER_DONE:
			debug_printf("[VDPAU][Bitstream parser] Done parsing current header\n");
		break;
		
		}
		
		
	}
	

	return 0;
}

