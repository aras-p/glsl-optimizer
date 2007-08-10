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
#include "sp_flush.h"
#include "sp_context.h"
#include "sp_winsys.h"

/* There will be actual work to do here.  In future we may want a
 * fence-like interface instead of finish, and perhaps flush will take
 * flags to indicate what type of flush is required.
 */
void
softpipe_flush( struct pipe_context *pipe,
		unsigned flags )
{
   /* - flush the quad pipeline
    * - flush the texture cache
    * - flush the render cache
    */
}

void
softpipe_wait_idle(struct pipe_context *pipe)
{
   /* Nothing to do.   
    * XXX: What about swapbuffers.
    * XXX: Even more so - what about fake frontbuffer copies??
    */
   struct softpipe_context *softpipe = softpipe_context(pipe);
   softpipe->winsys->wait_idle( softpipe->winsys );
}


void 
softpipe_flush_frontbuffer( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->winsys->flush_frontbuffer( softpipe->winsys );
}
