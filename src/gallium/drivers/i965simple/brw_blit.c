/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include <errno.h>

#include "brw_batch.h"
#include "brw_blit.h"
#include "brw_context.h"
#include "brw_reg.h"

#include "pipe/p_context.h"
#include "pipe/p_winsys.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

void brw_fill_blit(struct brw_context *brw,
                   unsigned cpp,
                   short dst_pitch,
                   struct pipe_buffer *dst_buffer,
                   unsigned dst_offset,
                   boolean dst_tiled,
                   short x, short y,
                   short w, short h,
                   unsigned color)
{
   unsigned BR13, CMD;
   BATCH_LOCALS;

   dst_pitch *= cpp;

   switch(cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (0xF0 << 16) | (1<<24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = (0xF0 << 16) | (1<<24) | (1<<25);
      CMD = XY_COLOR_BLT_CMD | XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
      break;
   default:
      return;
   }

   if (dst_tiled) {
      CMD |= XY_DST_TILED;
      dst_pitch /= 4;
   }

   BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH( CMD );
   OUT_BATCH( dst_pitch | BR13 );
   OUT_BATCH( (y << 16) | x );
   OUT_BATCH( ((y+h) << 16) | (x+w) );
   OUT_RELOC( dst_buffer, BRW_BUFFER_ACCESS_WRITE, dst_offset );
   OUT_BATCH( color );
   ADVANCE_BATCH();
}

static unsigned translate_raster_op(unsigned logicop)
{
   switch(logicop) {
   case PIPE_LOGICOP_CLEAR: return 0x00;
   case PIPE_LOGICOP_AND: return 0x88;
   case PIPE_LOGICOP_AND_REVERSE: return 0x44;
   case PIPE_LOGICOP_COPY: return 0xCC;
   case PIPE_LOGICOP_AND_INVERTED: return 0x22;
   case PIPE_LOGICOP_NOOP: return 0xAA;
   case PIPE_LOGICOP_XOR: return 0x66;
   case PIPE_LOGICOP_OR: return 0xEE;
   case PIPE_LOGICOP_NOR: return 0x11;
   case PIPE_LOGICOP_EQUIV: return 0x99;
   case PIPE_LOGICOP_INVERT: return 0x55;
   case PIPE_LOGICOP_OR_REVERSE: return 0xDD;
   case PIPE_LOGICOP_COPY_INVERTED: return 0x33;
   case PIPE_LOGICOP_OR_INVERTED: return 0xBB;
   case PIPE_LOGICOP_NAND: return 0x77;
   case PIPE_LOGICOP_SET: return 0xFF;
   default: return 0;
   }
}


/* Copy BitBlt
 */
void brw_copy_blit(struct brw_context *brw,
                   unsigned do_flip,
                   unsigned cpp,
                   short src_pitch,
                   struct pipe_buffer *src_buffer,
                   unsigned  src_offset,
                   boolean src_tiled,
                   short dst_pitch,
                   struct pipe_buffer *dst_buffer,
                   unsigned  dst_offset,
                   boolean dst_tiled,
                   short src_x, short src_y,
                   short dst_x, short dst_y,
                   short w, short h,
                   unsigned logic_op)
{
   unsigned CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;


   DBG("%s src:buf(%d)/%d %d,%d dst:buf(%d)/%d %d,%d sz:%dx%d op:%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_x, src_y,
       dst_buffer, dst_pitch, dst_x, dst_y,
       w,h,logic_op);

   assert( logic_op - PIPE_LOGICOP_CLEAR >= 0 );
   assert( logic_op - PIPE_LOGICOP_CLEAR < 0x10 );

   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch(cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (translate_raster_op(logic_op) << 16) | (1<<24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 = (translate_raster_op(logic_op) << 16) | (1<<24) |
	  (1<<25);
      CMD = XY_SRC_COPY_BLT_CMD | XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
      break;
   default:
      return;
   }

   if (src_tiled) {
      CMD |= XY_SRC_TILED;
      src_pitch /= 4;
   }

   if (dst_tiled) {
      CMD |= XY_DST_TILED;
      dst_pitch /= 4;
   }

   if (dst_y2 < dst_y ||
       dst_x2 < dst_x) {
      return;
   }

   dst_pitch &= 0xffff;
   src_pitch &= 0xffff;

   /* Initial y values don't seem to work with negative pitches.  If
    * we adjust the offsets manually (below), it seems to work fine.
    *
    * On the other hand, if we always adjust, the hardware doesn't
    * know which blit directions to use, so overlapping copypixels get
    * the wrong result.
    */
   if (dst_pitch > 0 && src_pitch > 0) {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( dst_pitch | BR13 );
      OUT_BATCH( (dst_y << 16) | dst_x );
      OUT_BATCH( (dst_y2 << 16) | dst_x2 );
      OUT_RELOC( dst_buffer, BRW_BUFFER_ACCESS_WRITE,
		 dst_offset );
      OUT_BATCH( (src_y << 16) | src_x );
      OUT_BATCH( src_pitch );
      OUT_RELOC( src_buffer, BRW_BUFFER_ACCESS_READ,
		 src_offset );
      ADVANCE_BATCH();
   }
   else {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( (dst_pitch & 0xffff) | BR13 );
      OUT_BATCH( (0 << 16) | dst_x );
      OUT_BATCH( (h << 16) | dst_x2 );
      OUT_RELOC( dst_buffer, BRW_BUFFER_ACCESS_WRITE,
		 dst_offset + dst_y * dst_pitch );
      OUT_BATCH( (src_pitch & 0xffff) );
      OUT_RELOC( src_buffer, BRW_BUFFER_ACCESS_READ,
		 src_offset + src_y * src_pitch );
      ADVANCE_BATCH();
   }
}



