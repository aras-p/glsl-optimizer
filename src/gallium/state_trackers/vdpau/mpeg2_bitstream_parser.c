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
#include <stdio.h>
#include <stdlib.h>
#include "mpeg2_bitstream_parser.h"

int
vlVdpMPEG2NextStartCode(struct vdpMPEG2BitstreamParser *parser)
{
	uint32_t integer = 0xffffff00;
	uint8_t * ptr_read = parser->ptr_bitstream;
	int8_t * bytes_to_end;
		
	bytes_to_end = parser->ptr_bitstream_end - parser->ptr_bitstream;
		
	/* Read byte after byte, until startcode is found */
	while(integer != 0x00000100)
	{
		if (bytes_to_end <= 0)
		{
			parser->state = MPEG2_BITSTREAM_DONE;
			parser->code = 0;
			return 0;
		}
		integer = ( integer | *ptr_read++ ) << 8;
		bytes_to_end--;	
	}
	parser->ptr_bitstream = ptr_read;
	parser->code = parser->ptr_bitstream;
	/* start_code found. rewind cursor a byte */
	//parser->cursor -= 8;
	
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
	
	#if(1)
	FILE *fp;
   
      if ((fp = fopen("binout", "w"))==NULL) {
        printf("Cannot open file.\n");
        exit(1);
      }
	fwrite(bitstream_buffers[0].bitstream, 1, bitstream_buffers[0].bitstream_bytes, fp);
	fclose(fp);
	
	#endif
	
	
	debug_printf("[VDPAU] Starting decoding MPEG2 stream\n");
	
	num_macroblocks[0] = 0;
	
	memset(&parser,0,sizeof(parser));
	parser.state = MPEG2_HEADER_START_CODE;
	parser.ptr_bitstream = (unsigned char *)bitstream_buffers[0].bitstream;
	parser.ptr_bitstream_end = parser.ptr_bitstream + bitstream_buffers[0].bitstream_bytes;
	
	/* Main header parser loop */
	while(!b_header_done)
	{
		switch (parser.state)
		{
		case MPEG2_SEEK_HEADER:
			if (vlVdpMPEG2NextStartCode(&parser))
				exit(1);
			break;
			/* Start_code found */
			switch (parser.code)
			{
				/* sequence_header_code */
				case 0xB3:
				debug_printf("[VDPAU][Bitstream parser] Sequence header code found\n");
				
				/* We dont need to read this, because we already have this information */
				break;
				case 0xB5:
				debug_printf("[VDPAU][Bitstream parser] Extension start code found\n");
				//exit(1);
				break;
				
				case 0xB8:
				debug_printf("[VDPAU][Bitstream parser] Extension start code found\n");
				//exit(1);
				break;
				
			}
		
		break;
		case MPEG2_BITSTREAM_DONE:
			if (parser.cur_bitstream < bitstream_buffer_count - 1)
			{
				debug_printf("[VDPAU][Bitstream parser] Done parsing current bitstream. Moving to the next\n");
				parser.cur_bitstream++;
				parser.ptr_bitstream = (unsigned char *)bitstream_buffers[parser.cur_bitstream].bitstream;
				parser.ptr_bitstream_end = parser.ptr_bitstream + bitstream_buffers[parser.cur_bitstream].bitstream_bytes; 
				parser.state = MPEG2_HEADER_START_CODE;
			}
			else
			{
				debug_printf("[VDPAU][Bitstream parser] Done with frame\n");
				exit(0);
				// return 0;
			}
		break;
		
		}
		
		
	}
	

	return 0;
}

