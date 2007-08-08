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

#include "mtypes.h"

#include "i915_context.h"
#include "i915_winsys.h"
#include "i915_blit.h"
#include "i915_reg.h"
#include "i915_batch.h"


void
i915_fill_blit(struct i915_context *i915,
	       GLuint cpp,
	       GLshort dst_pitch,
	       struct pipe_buffer_handle *dst_buffer,
	       GLuint dst_offset,
	       GLshort x, GLshort y, 
	       GLshort w, GLshort h, 
	       GLuint color)
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

   dst_pitch *= cpp;

   switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = dst_pitch | (0xF0 << 16) | (1 << 24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = dst_pitch | (0xF0 << 16) | (1 << 24) | (1 << 25);
      CMD = (XY_COLOR_BLT_CMD | XY_COLOR_BLT_WRITE_ALPHA |
             XY_COLOR_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

//   DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
//       __FUNCTION__, dst_buffer, dst_pitch, dst_offset, x, y, w, h);


   BEGIN_BATCH(6, 1);
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((y << 16) | x);
   OUT_BATCH(((y + h) << 16) | (x + w));
   OUT_RELOC( dst_buffer, I915_BUFFER_ACCESS_WRITE, dst_offset);
   OUT_BATCH(color);
   ADVANCE_BATCH();
}


void
i915_copy_blit( struct i915_context *i915,
                  GLuint cpp,
                  GLshort src_pitch,
                  struct pipe_buffer_handle *src_buffer,
                  GLuint src_offset,
                  GLshort dst_pitch,
                  struct pipe_buffer_handle *dst_buffer,
                  GLuint dst_offset,
                  GLshort src_x, GLshort src_y,
                  GLshort dst_x, GLshort dst_y, 
		  GLshort w, GLshort h )
{
   GLuint CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;


   printf("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (((GLint) dst_pitch) & 0xffff) | 
	 (0xCC << 16) | (1 << 24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 =
         (((GLint) dst_pitch) & 0xffff) | 
	 (0xCC << 16) | (1 << 24) | (1 << 25);
      CMD =
         (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
          XY_SRC_COPY_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (dst_y2 < dst_y || 
       dst_x2 < dst_x) {
      return;
   }

   /* Hardware can handle negative pitches but loses the ability to do
    * proper overlapping blits in that case.  We don't really have a
    * need for either at this stage.
    */
   assert (dst_pitch > 0 && src_pitch > 0);


   BEGIN_BATCH(8, 2);
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((dst_y << 16) | dst_x);
   OUT_BATCH((dst_y2 << 16) | dst_x2);
   OUT_RELOC(dst_buffer, I915_BUFFER_ACCESS_WRITE, dst_offset);
   OUT_BATCH((src_y << 16) | src_x);
   OUT_BATCH(((GLint) src_pitch & 0xffff));
   OUT_RELOC(src_buffer, I915_BUFFER_ACCESS_READ, src_offset);
   ADVANCE_BATCH();
}


