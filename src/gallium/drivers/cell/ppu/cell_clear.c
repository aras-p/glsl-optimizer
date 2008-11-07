/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * Authors
 *  Brian Paul
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "cell/common.h"
#include "cell_clear.h"
#include "cell_context.h"
#include "cell_batch.h"
#include "cell_flush.h"
#include "cell_spu.h"
#include "cell_state.h"


/**
 * Convert packed pixel from one format to another.
 */
static unsigned
convert_color(enum pipe_format srcFormat, unsigned srcColor,
              enum pipe_format dstFormat)
{
   ubyte r, g, b, a;
   unsigned dstColor;

   util_unpack_color_ub(srcFormat, &srcColor, &r, &g, &b, &a);
   util_pack_color_ub(r, g, b, a, dstFormat, &dstColor);

   return dstColor;
}



/**
 * Called via pipe->clear()
 */
void
cell_clear_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                   unsigned clearValue)
{
   struct pipe_screen *screen = pipe->screen;
   struct cell_context *cell = cell_context(pipe);
   uint surfIndex;

   if (cell->dirty)
      cell_update_derived(cell);


   if (!cell->cbuf_map[0])
      cell->cbuf_map[0] = screen->surface_map(screen, ps,
                                              PIPE_BUFFER_USAGE_GPU_WRITE);

   if (ps == cell->framebuffer.zsbuf) {
      /* clear z/stencil buffer */
      surfIndex = 1;
   }
   else {
      /* clear color buffer */
      surfIndex = 0;

      if (ps->format != PIPE_FORMAT_A8R8G8B8_UNORM) {
         clearValue = convert_color(PIPE_FORMAT_A8R8G8B8_UNORM, clearValue,
                                    ps->format);
      }
   }


   /* Build a CLEAR command and place it in the current batch buffer */
   {
      struct cell_command_clear_surface *clr
         = (struct cell_command_clear_surface *)
         cell_batch_alloc(cell, sizeof(*clr));
      clr->opcode = CELL_CMD_CLEAR_SURFACE;
      clr->surface = surfIndex;
      clr->value = clearValue;
   }

   /* Technically, the surface's contents are now known and cleared,
    * so we could set the status to PIPE_SURFACE_STATUS_CLEAR.  But
    * it turns out it's quite painful to recognize when any particular
    * surface goes from PIPE_SURFACE_STATUS_CLEAR to 
    * PIPE_SURFACE_STATUS_DEFINED (i.e. with known contents), because
    * the drawing commands could be operating on numerous draw buffers,
    * which we'd have to iterate through to set all their stati...
    * For now, we cheat a bit and set the surface's status to DEFINED
    * right here.  Later we should revisit this and set the status to
    * CLEAR here, and find a better place to set the status to DEFINED.
    */
   ps->status = PIPE_SURFACE_STATUS_DEFINED;
}
