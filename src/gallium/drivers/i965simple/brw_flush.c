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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_defines.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_batch.h"


static void brw_flush( struct pipe_context *pipe,
                       unsigned flags,
                       struct pipe_fence_handle **fence )
{
   struct brw_context *brw = brw_context(pipe);

   /* Do we need to emit an MI_FLUSH command to flush the hardware
    * caches?
    */
   if (flags & (PIPE_FLUSH_RENDER_CACHE | PIPE_FLUSH_TEXTURE_CACHE)) {
      struct brw_mi_flush flush;

      memset(&flush, 0, sizeof(flush));      
      flush.opcode = CMD_MI_FLUSH;

      if (!(flags & PIPE_FLUSH_RENDER_CACHE))
	 flush.flags |= BRW_INHIBIT_FLUSH_RENDER_CACHE;

      if (flags & PIPE_FLUSH_TEXTURE_CACHE)
	 flush.flags |= BRW_FLUSH_READ_CACHE;

      BRW_BATCH_STRUCT(brw, &flush);
   }

   /* If there are no flags, just flush pending commands to hardware:
    */
   FLUSH_BATCH( fence );
}



void brw_init_flush_functions( struct brw_context *brw )
{
   brw->pipe.flush = brw_flush;
}
