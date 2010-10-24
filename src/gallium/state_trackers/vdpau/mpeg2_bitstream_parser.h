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

#ifndef MPEG2_BITSTREAM_PARSER_H
#define MPEG2_BITSTREAM_PARSER_H

#include <vdpau/vdpau.h>
#include <pipe/p_video_state.h>
#include "vdpau_private.h"

enum vdpMPEG2States
{
	MPEG2_SEEK_HEADER,
	MPEG2_HEADER_DONE,
	MPEG2_BITSTREAM_DONE,
	MPEG2_HEADER_START_CODE
};


struct vdpMPEG2BitstreamParser
{
	enum vdpMPEG2States state;
	uint32_t cur_bitstream;
	const uint8_t *ptr_bitstream_end;
	const uint8_t *ptr_bitstream;
	uint8_t code;
	
	/* The decoded bitstream goes here: */
	/* Sequence_header_info */
	uint32_t horizontal_size_value;
};

int
vlVdpMPEG2BitstreamToMacroblock(struct pipe_screen *screen,
                  VdpBitstreamBuffer const *bitstream_buffers,
				  uint32_t bitstream_buffer_count,
                  unsigned int *num_macroblocks,
                  struct pipe_mpeg12_macroblock **pipe_macroblocks);
				  

#endif // MPEG2_BITSTREAM_PARSER_H
